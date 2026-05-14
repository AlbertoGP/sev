# src/display/ Subsystem

## Purpose

UI component layer built on Clay. Each frame, declares the full layout hierarchy using `CLAY()` macros. Owns the pane tree, tab list, visual line cache, theming/color resolution, cursor rendering, icons, status bar, and tooltip system. Reads buffer state from `src/text/` but never mutates it.

## Layout Hierarchy

`create_app_layout(state)` in `layout.c` — called once per frame from `iterate.c`, returns `Clay_RenderCommandArray`:

1. `GlobalHeader` — app icon strip
2. `PaneContent` (recursive pane tree) or `WelcomePane` when root is NULL
3. `StatusBar` — global bottom bar (mode pill + echo area + position info)
4. `WhichKey` — floating overlay (when active)
5. `MinibufPalette` — floating command palette (when minibuf active)
6. Tooltip floating elements (rendered within their call sites, not here)

## Files

- **layout.c / layout.h** — Top-level frame entry point. Calls `tab_cb_reset()`, `buf_render_reset()`, `cursor_cb_reset()` before `Clay_BeginLayout()`. Assembles the hierarchy above.

- **pane.c / pane.h** — Pane tree: binary tree of `PANE_V_SPLIT`, `PANE_H_SPLIT`, and `PANE_DISPLAY` leaves. `PANE_DISPLAY` leaves own a doubly-linked tab list (`DisplayContent.list`, `DisplayContent.active_tab`). `DisplayContent` stores per-frame geometry (origin, size, gutter width, line height, font) for mouse hit-testing via `pane_at_coords()`. `PaneContent()` recurses the tree: for splits it subdivides space, for display leaves it renders `TabBar` then `BufferPane`. `pane_push_jump()` pushes the current position onto the active tab's jump list.

- **vline.c / vline.h** — Visual line cache. Maps logical lines → wrapped visual lines. Owned per-`Tab` (not per-pane). Full rebuild on cache key change (width, font_id, font_size); incremental when only text changes (uses `Line.version`). Word-boundary wrapping with 4-char tab stops. Doubles on growth, shrinks at <25%.

- **tab.c / tab.h** — Tabs are thin nodes in a doubly-linked list **owned by each `PANE_DISPLAY`**; there is no global tab list. Each `Tab` holds a `Buffer*`, `VLineCache`, `JumpList`, and last-rendered geometry. `TabBar()` is a Clay component rendered per display-pane. `tab_cb_reset()` must be called once per frame before layout. `display_tab_new/close/next/prev` operate on a given display pane's list. `tab_new_with_buffer` creates a root pane if none exists. `tab_register_string(s)` registers a heap-allocated string for deferred free by `tab_free_strings()`.

- **buf_render.c / buf_render.h** — Buffer content rendering. `BufferContentRender()` is the public entry point called from `pane.c`; internally delegates to focused static helpers via `BufRenderCtx`: geometry setup, per-visual-line rows, gutter cells (line numbers), cursor overlay, selection highlight, syntax-highlighted text runs, and scrollbar. `buf_render_reset()` must be called once per frame before layout (resets the selection rect pool).

- **theme.c / theme.h** — Two-level color resolution: role symbol → palette key → hex string → `Clay_Color`. `ui_resolve_color(state, role)` is the main lookup used everywhere. Also owns `CursorType` enum (SOLID, HOLLOW, THIN, UNDER) and cursor color/type queries.

- **cursor.c / cursor.h** — Cursor rendered as a Clay floating element (overlay, doesn't affect text layout). `cursor_cb_reset()` must be called once per frame.

- **status.c / status.h** — Single **global** status bar at the bottom of the layout. Left side: mode icon + mode name pill, optional macro indicator (colored dot + "REC"). Center: echo area (`MessageArea`). Right side: major-mode indicator, selection count, cursor position. Also owns the `Divider` helper used internally. `bar_free_strings()` **must** be called after each frame.

- **message.c / message.h** — `MessageArea` renders the current echo string inline inside the status bar. It no longer owns a separate bottom strip — the old separate `MinibufArea`/`MessageArea` bottom strip is gone; echo area is now embedded in `StatusBar`.

- **minibuf.c / minibuf.h** — `MinibufPalette()` renders a **floating command palette** centered near the top of the window when `state->minibuf.active`. Styled as a rounded, bordered card (Zed-style), rendered at `zIndex 150` above all content. Contains the prompt as placeholder text when input is empty, the user's typed text otherwise, and a cursor overlay at `zIndex 151`. After layout, writes back the computed palette geometry (`palette_x/y/w/h`) into `state->minibuf` for use by input hit-testing.

- **welcome.c / welcome.h** — `WelcomePane()` rendered in place of the pane tree when root is NULL. Shows an app icon + tagline banner, a "GET STARTED" section with clickable suggestion rows (icon + label + keybinding), and a "RECENT PROJECTS" section sorted by `last_opened`. Click handlers are deferred: `welcome_flush_pending()` (called from `iterate.c` after Clay rendering) executes the queued command or `chdir` + project open. Keybindings displayed in the suggestion rows are resolved live via `keymap_where_is_first`.

- **which_key.c / which_key.h** — `WhichKey()` popup overlay showing available key bindings for the current prefix. Floating element, only rendered when `state->which_key.active && state->which_key.enabled`.

- **tooltip.c / tooltip.h** — Hover tooltip system. `Tooltip(state, is_hovered, unique_id, render_fn, user_data)` attaches a tooltip to any call site: tracks which element is hovered, starts a 500 ms SDL timer on hover-start, and shows a floating Clay card at the cursor position on `TOOLTIP_SHOW_EVENT`. Hides immediately on hover-end. `TextTooltip` is a convenience wrapper for plain-text tooltips. `tooltip_handle_show()` must be called from `event.c` when `SDL_EVENT_USER` code == `TOOLTIP_SHOW_EVENT`.

- **icon.c / icon.h** — SVG icon registry (64 slots). Textures rasterized lazily on first access or scale change. Colors reapplied after theme change.

- **mode_icon.c / mode_icon.h** — Maps mode names → icon + 4 theme roles (bg, label, cursor color, cursor type). Searched by current buffer's minor modes.

- **scale.c / scale.h** — Global (`state->ui.scale_factor`, ×1.1 per step) and buffer-local scale. Effective font size = `base × global_scale × buffer_scale`.

## Key Invariants

- **String lifetime**: `bar_free_strings()` and `tab_free_strings()` must be called after `SDL_Clay_RenderClayCommands()` every frame — Clay doesn't own string data
- **`tab_cb_reset()`, `buf_render_reset()`, `cursor_cb_reset()` must be called** once per frame before `create_app_layout()`
- **`welcome_flush_pending()` must be called** after rendering each frame — defers Scheme calls and `chdir` out of the Clay layout pass
- **Active pane sync**: `sync_active_buffer()` must be called after any pane or tab switch
- **Cursor and selection are floating elements**: overlay text, don't affect layout flow
- **VLineCache lives per-Tab**: cache keys are (tab_width, font_id, font_size) — any change triggers full rebuild
- **Tabs are owned by `PANE_DISPLAY` nodes**: no global tab list; each display pane has its own circular doubly-linked list
- **Pane tree is always a binary tree** of splits terminating in `PANE_DISPLAY` leaves; root NULL means welcome screen
- **Macro recording indicator**: driven by `state->macro_recording` (C flag), not `vim-recording-mode` minor mode
- **Tooltip timer is one-shot**: fires `SDL_EVENT_USER` with code `TOOLTIP_SHOW_EVENT`; `event.c` must dispatch this to `tooltip_handle_show()`

## BufferPane Rendering Details

`BufferContentRender()` in `buf_render.c` delegates to focused helpers via `BufRenderCtx`:

- **Selection**: handles `SELECT_REGULAR` (char range per visual line), `SELECT_LINE` (whole logical lines), `SELECT_RECTANGLE`/`RECTANGLE_RAGGED` (column block)
- **Line number gutter**: type 1 = absolute, type 2 = relative, type 3 = visual row; wrapped continuations show no number
- **Cursor**: resolves correct font variant from hl_spans, then calls `Cursor()` floating element
- **Text**: iterates hl_spans, emits fixed-width Clay segments via `EMIT_RUN` macro

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

- **src/clay/** — this layer calls `CLAY()` macros; clay computes layout and renders. See `src/clay/AGENTS.md`.
- **src/text/** — `BufferContentRender` reads buffer state (text, point, lines, marks, select mode) but never mutates it
- **scheme/** — themes, icons, modes, and scale commands registered from Scheme; display reads the resulting C state
- **src/state_io.h** — `welcome.c` calls `state_io_update_recent_project()` when opening a project from the welcome screen
