# src/command/ Subsystem

## Purpose

Input handling and command execution layer. Converts SDL key events into `KeyEvent` structs, dispatches them through a keymap lookup chain (minor modes → buffer-local → major mode → global), and executes the matched command via Scheme's `call-interactively`. Also owns the mode registry and Scheme interpreter initialization.

## Files

### keyevent.h — Key Event Types
- `KeyEvent`: `{ type, mods, union { codepoint, keycode } }`
- `KeyEventType`: `KEYEVENT_CHAR` (unicode) or `KEYEVENT_SPECIAL` (arrows, F-keys, etc.)
- `KeyMod` flags: `MOD_CTRL`, `MOD_META`, `MOD_SHIFT`, `MOD_SUPER`
- `KeySpecial` enum: ESC, RETURN, TAB, BACKSPACE, DELETE, arrows, F1–F24, etc.
- `keyevent_equal(k1, k2)` — deep comparison for key matching

### keyboard.c / keyboard.h — SDL-to-KeyEvent Translation
- `handle_text_input(state, event)` — decodes UTF-8 → codepoints, creates `KEYEVENT_CHAR` with `MOD_NONE`, calls `key_dispatch()`
- `handle_key_down(state, event)` — normalizes SDL modifiers → `KeyMod`, blocks key repeat for text keys, creates `KEYEVENT_SPECIAL` or modified `KEYEVENT_CHAR`, calls `key_dispatch()`
- Called from `SDL_AppEvent()` in `src/event.c`

### keymap.c / keymap.h — Keymap System and Dispatch
- `Keymap`: dynamic array of `KeymapEntry { key, binding }` + parent pointer for inheritance
- `Binding`: either `BINDING_COMMAND` (stores Scheme symbol) or `BINDING_KEYMAP` (nested keymap for prefix keys)
- `keymap_create()` — allocate empty keymap
- `keymap_lookup(km, ev)` — search keymap + parent chain for matching key
- `keymap_lookup_chain(state, ev)` — full lookup: minor modes (head-first) → buffer-local → major mode → global
- `key_dispatch(state, ev)` — main dispatch entry point:
  - Mid-prefix: lookup only in `current_map` (no chain)
  - Otherwise: `keymap_lookup_chain()`
  - No match + char + no mods + input-allowing mode → `self-insert`
  - Match + `BINDING_KEYMAP` → enter prefix (set `current_map`)
  - Match + `BINDING_COMMAND` → `execute_command()` → `sexp_apply(call_interactively, [symbol])`
- `parse_key_sequence(s, out)` — Emacs-style notation parser (`"C-x"`, `"M-f"`, `"SPC"`, `"RET"`)
- `keymap_bind_sequence(km, seq, n, binding)` — bind multi-key sequence, creating intermediate keymaps as needed
- `keymap_add()` — overwrites existing binding for same key, or appends; grows by doubling (initial cap 8)

### mode.c / mode.h — Mode System
- `Mode`: `{ name, type, keymap, allows_input, next }`
- `ModeType`: `MODE_MAJOR` or `MODE_MINOR`
- Two global registries: `g_major_modes`, `g_minor_modes` (linked lists)
- `mode_create(name, type, keymap)` → allocate mode (not yet registered)
- `mode_register(mode)` → prepend to registry; rejects duplicate names
- `mode_lookup(name, type)` / `mode_lookup_any(name)` → linear search
- `mode_get_fundamental()` → get or create `fundamental-mode`
- `allows_input` flag: when true, unbound chars trigger `self-insert` (used by insert/replace modes)
- Buffer mode management lives in `src/text/buffer.c` (`buffer_enable/disable_minor_mode`, etc.)
- Scheme bindings: `%register-mode`, `%set-major-mode`, `%enable-minor-mode`, `%disable-minor-mode`, `%buffer-major-mode`, `%buffer-minor-modes`, `%buffer-has-minor-mode?`, `%set-mode-allows-input!`

### scheme.c / scheme.h — Scheme Interpreter Init
- `scheme_init(state)`:
  1. Init Chibi Scheme context (512MB heap limit)
  2. Load R7RS standard env + `(scheme base)`
  3. Bind `global-keymap` as cpointer in Scheme env
  4. Register ~60 foreign C functions via `sexp_define_foreign()`
  5. Load scripts in order: command.scm → mode.scm → icon.scm → built-in.scm → evil.scm → theme.scm → init.scm
  6. Intern and preserve role symbols for fast theme lookups
  7. Cache `call-interactively` Scheme procedure (preserved from GC)
- `scheme_eval_file(state, path)` — load and eval a .scm file; prints exceptions to stderr
- Script load paths: `scheme/` (desktop) or `/resources/` (WASM)
- Global `AppState *G` set at init for use by all `scm_*` binding functions

### scheme_bindings.h — Forward Declarations
- Forward-declares all `scm_*` C functions registered as Scheme foreign functions
- Grouped by subsystem: text ops, movement, queries, pane/tab, modes, marks, theme, icons, scale

### scheme_internal.h — Binding Helpers
- `SCM_CMD(cname, action)` macro: wraps a C action as a Scheme binding that sets `needs_redraw` and returns `SEXP_VOID`
- Provides access to global `G` pointer

## Key Dispatch Pipeline

```
SDL_AppEvent (event.c)
  → handle_text_input / handle_key_down (keyboard.c)
    → key_dispatch (keymap.c)
      → keymap_lookup_chain: minor modes → buffer-local → major → global
        → each level: keymap_lookup traverses parent chain
      → BINDING_KEYMAP: enter prefix (current_map = nested keymap)
      → BINDING_COMMAND: execute_command → sexp_apply(call-interactively, symbol)
      → no match + char + allows_input: self-insert
      → reset_key_state (current_map = global_map)
```

## Key Invariants

- **Bindings store Scheme symbols, not function pointers** — `call-interactively` resolves them at execution time; commands can be redefined at runtime
- **`call-interactively` must be defined before `scheme_init()` returns** — it's cached and preserved from GC
- **Prefix key state**: `current_map != global_map` means mid-prefix; only that map is searched (no chain), ESC or invalid key resets
- **Lookup chain priority**: minor modes (most-recently-enabled first) > buffer-local > major mode > global — first match wins
- **Parent chain is single-inheritance**: `keymap_lookup()` walks `km → km->parent → ...` until found or NULL
- **`allows_input` gates self-insert**: only checked for unbound plain chars (no modifiers) at top level (not mid-prefix)
- **Modes are registered globally, enabled per-buffer**: one `Mode` object shared across buffers; `ModeList` in each buffer tracks which are active
- **Script load order matters**: command.scm first (defines `call-interactively`), then mode/icon/built-in, then evil/theme/init

## Boundaries

- **keyboard.c** translates SDL → `KeyEvent`, never touches text or keymaps directly
- **keymap.c** dispatches keys and manages bindings; never reads/writes buffer text
- **mode.c** manages the mode registry; buffer-level mode lists live in `src/text/buffer.c`
- **scheme.c** initializes interpreter and registers bindings; actual `scm_*` implementations live in their respective subsystem files (`src/text/buffer.c`, `src/display/pane.c`, etc.)
- **All command execution goes through Scheme** — even C-implemented commands are called via `call-interactively` using their symbol name

## Common Modification Workflows

### Adding a new C command exposed to Scheme
1. Write `scm_*` function in the appropriate subsystem file (e.g., `src/text/buffer.c`)
2. Add forward declaration in `scheme_bindings.h`
3. Register in `scheme_init()` via `SDEF("scheme-name", arity, scm_function)`
4. Declare in `scheme/built-in.scm` with `(defcommand name "docstring")`
5. Bind to key in the appropriate .scm file with `(set-key! keymap "key" 'name)`

### Adding a new special key
1. Add variant to `KeySpecial` enum in `keyevent.h`
2. Add SDL keycode → `KeySpecial` mapping in `sdl_to_keyspecial()` in `keyboard.c`
3. Add string name in `parse_key_sequence()` in `keymap.c` (for key notation support)

### Adding a new mode from C
1. Call `mode_create(name, type, keymap)` → `mode_register(mode)`
2. Usually done from Scheme instead: `(define-minor-mode 'name keymap)` in `scheme/mode.scm`

### Changing keymap lookup priority
- Edit `keymap_lookup_chain()` in `keymap.c` — the for-loop over minor modes, then local/major/global checks

### Adding a prefix key binding
- `(set-key! keymap "C-x C-f" 'find-file)` — `keymap_bind_sequence()` auto-creates intermediate keymaps for `C-x`
