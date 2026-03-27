# src/display/ Subsystem

## Purpose

UI component layer built on Clay. Each frame, declares the full layout hierarchy using `CLAY()` macros. Owns the pane tree, tab list, visual line cache, theming/color resolution, cursor rendering, icons, status bar, and bottom strip (minibuffer or echo area). Reads buffer state from `src/text/` but never mutates it.

## Files

- **layout.c / layout.h** — Top-level layout. `create_app_layout(state)` returns `Clay_RenderCommandArray`. Hierarchy: `GlobalHeader` (app icon strip) → pane tree via `PaneContent` (or `WelcomePane` when root is NULL) → global `StatusBar` → `MinibufArea`/`MessageArea` → `WhichKey` overlay. Called once per frame from `iterate.c`.
- **pane.c / pane.h** — Pane tree: binary tree of `PANE_V_SPLIT`, `PANE_H_SPLIT`, and `PANE_DISPLAY` leaves. `PANE_DISPLAY` leaves own a doubly-linked tab list (`DisplayContent.list`, `DisplayContent.active_tab`). `DisplayContent` also stores per-frame geometry (origin, size, gutter width, line height, font) for mouse hit-testing via `pane_at_coords()`. `PaneContent()` recurses the tree: for splits it subdivides space, for display leaves it renders `TabBar` then `BufferPane`. `pane_free_strings()` **must** be called after each frame. `pane_push_jump()` pushes the current position onto the active tab's jump list.
- **vline.c / vline.h** — Visual line cache. Maps logical lines → wrapped visual lines. Owned per-`Tab` (not per-pane). Full rebuild on cache key change (width, font_id, font_size); incremental when only text changes (uses `Line.version`). Word-boundary wrapping with 4-char tab stops. Doubles on growth, shrinks at <25%.
- **tab.c / tab.h** — Tabs are thin nodes in a doubly-linked list **owned by each `PANE_DISPLAY`**; there is no global tab list. Each `Tab` holds a `Buffer*`, `VLineCache`, `JumpList`, and last-rendered geometry. `TabBar()` is a Clay component rendered per display-pane (not globally). `tab_cb_reset()` must be called once per frame before layout. `display_tab_new/close/next/prev` operate on a given display pane's list. `tab_new_with_buffer` creates a root pane if none exists. `tab_register_string(s)` registers a heap-allocated string for deferred free by `tab_free_strings()`.
- **buf_render.c / buf_render.h** — Buffer content rendering component. `BufferContentRender()` is the public entry point called from `pane.c`; internally it delegates to a set of focused static helpers via a `BufRenderCtx` context struct: `BufRender_SetupGeometry` (font/gutter/scroll/geometry), `BufRender_VLine` (per visual-line row), `BufRender_GutterCell` (line number), `BufRender_CursorCell` (cursor overlay), `BufRender_SelectionCell` (selection highlight), `BufRender_TextCell` (syntax-highlighted text), `BufRender_Scrollbar` (track and thumb). `buf_render_reset()` must be called once per frame before layout (resets the selection rect pool).
- **theme.c / theme.h** — Two-level color resolution: role symbol → palette key → hex string → `Clay_Color`. `ui_resolve_color(state, role)` is the main lookup, used everywhere. Also provides `CursorType` enum (SOLID, HOLLOW, THIN, UNDER) and cursor color/type queries.
- **cursor.c / cursor.h** — Cursor rendering as a Clay floating element (overlay, doesn't affect text layout).
- **status.c / status.h** — Single **global** status bar rendered once at the bottom of the layout (not per-pane). Shows a mode icon + mode name pill. When `state->macro_recording` is true, appends a macro indicator: colored dot + "REC" text. `bar_free_strings()` **must** be called after each frame.
- **message.c / message.h** — Bottom strip components. `MessageArea` renders the echo area string. `MinibufArea` renders the minibuffer when active: concatenates the prompt with buffer text, measures cursor x-offset via `TTF_GetStringSize`, and overlays a `Cursor` element. Temporarily swaps `buffer_get_current()` to the minibuf so cursor helpers read the right buffer.
- **welcome.c / welcome.h** — `WelcomePane()` rendered in place of the pane tree when root is NULL. Controlled by `welcome-map` registered from Scheme via `%set-welcome-keymap!`.
- **which_key.c / which_key.h** — `WhichKey()` popup overlay showing available key bindings for the current prefix. Rendered as a floating element when `state->which_key.active && state->which_key.enabled`.
- **icon.c / icon.h** — SVG icon registry (64 slots). Textures rasterized lazily on first access or scale change. Colors reapplied after theme change.
- **mode_icon.c / mode_icon.h** — Maps mode names → icon + 4 theme roles (bg, label, cursor color, cursor type). Searched by current buffer's minor modes.
- **scale.c / scale.h** — Global (`state->ui.scale_factor`, ×1.1 per step) and buffer-local scale. Effective font size = `base × global_scale × buffer_scale`.

## Key Invariants

- **String lifetime**: `pane_free_strings()` and `bar_free_strings()` must be called after `SDL_Clay_RenderClayCommands()` every frame — Clay doesn't own string data
- **`tab_cb_reset()` and `buf_render_reset()` must be called** once per frame before `create_app_layout()` — reset Clay hover-callback slots and the selection rect pool respectively
- **Active pane sync**: `sync_active_buffer()` must be called after any pane or tab switch
- **Cursor and selection are floating elements**: overlay text, don't affect layout flow
- **VLineCache lives per-Tab**: cache keys are (tab_width, font_id, font_size) — any change triggers full rebuild
- **Tabs are owned by `PANE_DISPLAY` nodes**: there is no global tab list; each display pane has its own circular doubly-linked list
- **Pane tree is always a binary tree** of splits terminating in `PANE_DISPLAY` leaves; root NULL means welcome
- **Macro recording indicator**: driven by `state->macro_recording` (C flag), not `evil-recording-mode` minor mode — the minor mode is disabled during insert/replace while recording continues

## BufferPane Rendering Details

Implemented in `buf_render.c`. `BufferContentRender()` delegates to focused helpers via `BufRenderCtx`:

- **Selection**: `BufRender_SelectionCell` handles SELECT_REGULAR (char range per visual line), SELECT_LINE (whole logical lines), SELECT_RECTANGLE/RECTANGLE_RAGGED (column block)
- **Line number gutter**: `BufRender_GutterCell` — type 1 = absolute, type 2 = relative, type 3 = visual row; wrapped continuations show no number
- **Cursor**: `BufRender_CursorCell` — resolves correct font variant from hl_spans, then calls `Cursor()` floating element
- **Text**: `BufRender_TextCell` — iterates hl_spans, emits fixed-width Clay segments via `EMIT_RUN` macro

## Common Modification Workflows

### Adding a new UI component
1. Create `component.c` / `component.h` with a function using `CLAY()` macros
2. Call from layout hierarchy (usually `layout.c` or `pane.c`)
3. If it allocates strings, add a `_free_strings()` and call from `iterate.c` after rendering

### Adding a new cursor style
1. Add variant to `CursorType` in `theme.h`
2. Handle in `Cursor()` in `cursor.c`
3. Register from Scheme via `register-mode-icon`

### Changing the status bar content
1. Edit `StatusBar()` in `status.c`
2. Track new dynamic strings in `bar_strings[]` for cleanup

## Relationship to Other Subsystems

- **src/clay/** — this layer calls `CLAY()` macros; clay computes layout and renders. See `src/clay/CLAUDE.md`.
- **src/text/** — `BufferPane` reads buffer state (text, point, lines, marks, select mode) but never mutates it
- **scheme/** — themes, icons, modes, and scale commands registered from Scheme; display reads the resulting C state
