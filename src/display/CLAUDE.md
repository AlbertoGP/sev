# src/display/ Subsystem

## Purpose

UI components and rendering logic built on top of Clay. Declares the full layout hierarchy each frame using `CLAY()` macros, manages visual line caching for text wrapping, and handles pane/tab structure, theming, cursors, selections, icons, and the status bar.

## Files

### Layout & Composition

- **layout.c / layout.h** — `create_app_layout(state)`: top-level Clay layout, returns `Clay_RenderCommandArray`. Hierarchy: TabBar → TabContent (panes) → MessageArea. Called once per frame from `iterate.c`.

### Pane System

- **pane.c / pane.h** — Binary tree of splits and content leaves. Core types:
  - `Pane` — `PANE_CONTENT` | `PANE_V_SPLIT` | `PANE_H_SPLIT`; has parent pointer
  - `Content` — holds `Buffer*`, active flag, dimensions, embedded `VLineCache`
  - `PaneContent()` — recursive Clay component, dispatches to `BufferPane()` at leaves
  - `BufferPane()` (~300 lines) — the complex core: vline rebuild, gutter, cursor, selection, text rendering
  - Navigation: `pane_navigate(dir)` walks parent chain to find adjacent leaf
  - Split ratio: `pane_v/h_split_increase/decrease()` adjusts ±5%, clamped [0.2, 0.8]
  - `pane_free_strings()` — **must** be called after each frame to free allocated text pointers

### Visual Line Cache

- **vline.c / vline.h** — Maps logical lines → visual (wrapped) lines for a pane
  - `VLineCache` — array of `VisualLine` + `LogicalLineIndex` + scroll state + cache keys
  - `vline_rebuild()` — full rebuild on cache key change (width, font_id, font_size); incremental on text edits (checks `Line.version`)
  - `vline_scroll_to_cursor()` — adjusts `top_vline` to keep cursor visible
  - `vline_for_byte_pos()` — binary search for visual line containing a byte position
  - Word-boundary wrapping with tab-stop alignment (4-char default)
  - Capacity doubles on growth, shrinks if <25% utilized

### Tab System

- **tab.c / tab.h** — Doubly-linked list of tabs, each owning a `Pane*` tree root
  - `TabBar()` / `TabContent()` — Clay components for tab selector and current content
  - `tab_next()` / `tab_prev()` — circular navigation, calls `sync_active_buffer()`
  - `tab_close_current()` — destroys tab; quits app if last tab
  - Close button uses `Clay_OnHover()` for pointer interaction

### Theme & Colors

- **theme.c / theme.h** — Two-level color resolution:
  - Role symbol → palette key (via `UIState.role_table`) → hex string (via `UIState.palette_table`) → `Clay_Color`
  - `ui_resolve_color(state, role)` — main lookup function, used everywhere
  - `ui_get_cursor_color()` / `get_cursor_type()` — cursor styling from current mode icon
  - `CursorType` enum: SOLID, HOLLOW, THIN, UNDER

### Cursor

- **cursor.c / cursor.h** — `Cursor()` Clay floating element
  - Renders as overlay (doesn't participate in text layout flow)
  - Style varies by `CursorType`: filled block, outline, thin bar, underline

### Status Bar

- **status.c / status.h** — `StatusBar()` per-pane component
  - Shows mode icon + buffer name + line:col position
  - `bar_free_strings()` — **must** be called after each frame

### Message Area

- **message.c / message.h** — `MessageArea()` echo area at bottom of layout
  - Displays messages set via `message_send()` from Scheme/C

### Icons

- **icon.c / icon.h** — SVG icon registry (64 slots, static array)
  - `icon_register()` — record icon entry (texture created lazily)
  - `icon_get()` — returns `SDL_Texture*`; rasterizes SVG on first access or scale change
  - `icons_stash_renderer()` — stores renderer ref for lazy texture generation
  - `icons_update_colors()` — reapplies color modulation after theme change

- **mode_icon.c / mode_icon.h** — Mode-specific icons (32 slots)
  - `mode_icon_register()` — maps mode name → icon + 4 theme roles (bg, label, cursor color, cursor type)
  - `mode_icon_for_current_buffer()` — searches buffer's minor modes for a matching icon entry

### Scaling

- **scale.c / scale.h** — Global and buffer-local scale commands
  - Global: `state->ui.scale_factor` (multiplier, ×1.1 per step)
  - Buffer-local: `buffer_get_scale(buf)`
  - Effective font size: `base_size × scale_factor × buffer_scale`

## Key Invariants

- **String lifetime**: `pane_free_strings()` and `bar_free_strings()` must be called after `SDL_Clay_RenderClayCommands()` every frame — Clay doesn't own string data
- **Active pane sync**: call `sync_active_buffer()` after any pane/tab switch to keep global current buffer in sync
- **Cursor and selection are floating elements**: they overlay text, don't affect layout flow
- **VLineCache invalidation**: cache keys are (pane_width, font_id, font_size); any change triggers full rebuild
- **One active content pane per tab**: `pane_set_active()` deactivates the previous one
- **Pane tree is a binary tree**: splits always produce exactly two children

## Selection Rendering (in BufferPane)

Three modes, each with distinct highlight logic:
- **SELECT_REGULAR** — character range `[sel_min, sel_max)`, clamped to each visual line
- **SELECT_LINE** — entire logical lines within range
- **SELECT_RECTANGLE** — column-based block; iterates visible lines, clamps to column bounds

## Line Number Gutter (in BufferPane)

Three display modes (controlled by buffer-local `display-line-numbers-type`):
- Type 1: absolute line numbers
- Type 2: relative (distance from cursor line; current line shows absolute)
- Type 3: visual (screen row number)

Wrapped continuation lines show no number.

## Common Modification Workflows

### Adding a new UI component
1. Create `component.c` / `component.h` with a function that uses `CLAY()` macros
2. Call it from the appropriate place in the layout hierarchy (usually `layout.c` or `pane.c`)
3. If it allocates strings, add a `_free_strings()` function and call it from `iterate.c` after rendering

### Adding a new cursor style
1. Add variant to `CursorType` enum in `theme.h`
2. Handle new case in `Cursor()` in `cursor.c`
3. Register from Scheme via `register-mode-icon` with the new cursor type

### Adding a new pane split type or navigation behavior
1. Modify `pane.c` — follow the `V_SPLIT` / `H_SPLIT` pattern
2. Add recursive handling in `PaneContent()`

### Changing the status bar content
1. Edit `StatusBar()` in `status.c`
2. If adding dynamic strings, track them in `bar_strings[]` for cleanup

## Relationship to Other Subsystems

- **src/clay/** — this layer calls `CLAY()` macros; clay computes layout and renders. See `src/clay/CLAUDE.md`.
- **src/text/** — `BufferPane` reads buffer state (text, point, lines, marks, select mode) but never mutates it
- **scheme/** — themes, icons, modes, and scale commands are registered from Scheme; display reads the resulting C state
