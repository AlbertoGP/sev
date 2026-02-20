# src/display/ Subsystem

## Purpose

UI component layer built on Clay. Each frame, declares the full layout hierarchy using `CLAY()` macros. Owns the pane tree, tab list, visual line cache, theming/color resolution, cursor rendering, icons, status bar, and bottom strip (minibuffer or echo area). Reads buffer state from `src/text/` but never mutates it.

## Files

- **layout.c / layout.h** — Top-level layout. `create_app_layout(state)` returns `Clay_RenderCommandArray`. Hierarchy: TabBar → TabContent (panes) → bottom strip (`MinibufArea` when `minibuf.active`, `MessageArea` otherwise). Called once per frame from `iterate.c`.
- **pane.c / pane.h** — Pane tree: binary tree of splits (`V_SPLIT`/`H_SPLIT`) and `CONTENT` leaves. Each content leaf holds a `Buffer*` and an embedded `VLineCache`. `BufferPane()` is the complex core (~300 lines): rebuilds vline cache, renders gutter, cursor, selection highlights, and text. `pane_free_strings()` **must** be called after each frame.
- **vline.c / vline.h** — Visual line cache. Maps logical lines → wrapped visual lines for a pane. Full rebuild on cache key change (width, font_id, font_size); incremental when only text changes (uses `Line.version`). Word-boundary wrapping with 4-char tab stops. Doubles on growth, shrinks at <25%.
- **tab.c / tab.h** — Doubly-linked tab list. Each tab owns a pane tree root. Circular navigation. Close button uses `Clay_OnHover()`.
- **theme.c / theme.h** — Two-level color resolution: role symbol → palette key → hex string → `Clay_Color`. `ui_resolve_color(state, role)` is the main lookup, used everywhere. Also provides `CursorType` enum (SOLID, HOLLOW, THIN, UNDER) and cursor color/type queries.
- **cursor.c / cursor.h** — Cursor rendering as a Clay floating element (overlay, doesn't affect text layout).
- **status.c / status.h** — Per-pane status bar: mode icon + buffer name + line:col. `bar_free_strings()` **must** be called after each frame.
- **message.c / message.h** — Bottom strip components. `MessageArea` renders the echo area string. `MinibufArea` renders the minibuffer when active: concatenates the prompt with buffer text, measures cursor x-offset via `TTF_GetStringSize`, and overlays a `Cursor` element. Temporarily swaps `buffer_get_current()` to the minibuf so cursor helpers read the right buffer.
- **icon.c / icon.h** — SVG icon registry (64 slots). Textures rasterized lazily on first access or scale change. Colors reapplied after theme change.
- **mode_icon.c / mode_icon.h** — Maps mode names → icon + 4 theme roles (bg, label, cursor color, cursor type). Searched by current buffer's minor modes.
- **scale.c / scale.h** — Global (`state->ui.scale_factor`, ×1.1 per step) and buffer-local scale. Effective font size = `base × global_scale × buffer_scale`.

## Key Invariants

- **String lifetime**: `pane_free_strings()` and `bar_free_strings()` must be called after `SDL_Clay_RenderClayCommands()` every frame — Clay doesn't own string data
- **Active pane sync**: `sync_active_buffer()` must be called after any pane/tab switch
- **Cursor and selection are floating elements**: overlay text, don't affect layout flow
- **VLineCache keys**: (pane_width, font_id, font_size) — any change triggers full rebuild
- **One active content pane per tab**; pane tree is always a binary tree

## BufferPane Rendering Details

- **Selection**: SELECT_REGULAR (char range per visual line), SELECT_LINE (whole logical lines), SELECT_RECTANGLE (column block)
- **Line number gutter**: type 1 = absolute, type 2 = relative, type 3 = visual row; wrapped continuations show no number

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
