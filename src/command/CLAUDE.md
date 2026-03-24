# src/command/ Subsystem

## Purpose

Input handling and command execution. Converts SDL key events into `KeyEvent` structs, dispatches them through a keymap lookup chain, and executes matched commands via Scheme's `call-interactively`. Also owns the mode registry, macro recording engine, and Scheme interpreter initialization.

## Files

- **keyevent.h** — `KeyEvent` type: `KEYEVENT_CHAR` (unicode codepoint) or `KEYEVENT_SPECIAL` (arrows, F-keys, etc.), plus modifier flags (`MOD_CTRL`, `MOD_META`, `MOD_SHIFT`, `MOD_SUPER`).
- **keyboard.c / keyboard.h** — SDL-to-KeyEvent translation. `handle_text_input()` decodes UTF-8 into char events; `handle_key_down()` normalizes SDL modifiers and maps special keys. Both call `key_dispatch()`. Called from `SDL_AppEvent()` in `event.c`.
- **keymap.c / keymap.h** — Keymap data structure and dispatch engine. A `Keymap` is a dynamic array of `(KeyEvent → Binding)` entries with a parent pointer for single-inheritance. A `Binding` is either a Scheme symbol (`BINDING_COMMAND`) or a nested keymap (`BINDING_KEYMAP`, for prefix keys). `key_dispatch()` is the main entry point. Also provides `parse_key_sequence()` for Emacs-style key notation (`"C-x"`, `"M-f"`, `"SPC"`). Which-key intercept: when `state->which_key.enabled`, `key_dispatch` feeds keys through the intercept keymap and shows the popup on prefix entry.
- **minibuf.c / minibuf.h** — Minibuffer lifecycle. `minibuf_init()` creates the persistent `*minibuffer*` buffer, enables `minibuffer-mode`, and disables `evil-normal-mode` to isolate it from editor keymaps. Supports a push/pop frame stack (depth 8 in `AppState.minibuf.stack`) so nested prompts work. Activate stores prompt and GC-preserved Scheme `on_submit`/`on_cancel` callbacks, redirecting dispatch to the minibuf. Submit reads buffer text, restores previous buffer/focus, calls callback. Cancel restores without calling `on_submit`.
- **message.c / message.h** — Echo area. Static 2048-byte buffer shared with the UI renderer via `Clay_String`, and bool lock controlling when message can be updated. `message-echo` sets without lock; `message-unlock` releases the lock.
- **mode.c / mode.h** — Mode registry. Modes have a name, type (major/minor), keymap, and `allows_input` flag. Two global registries (linked lists) for major and minor modes. Modes are created and registered globally, then enabled per-buffer. Buffer-level mode list management lives in `src/text/buffer.c`.
- **macro.c** (no header) — Macro recording and playback. `scm_macro_start` / `scm_macro_stop` manage `state->macro_recording` and the `KeyEvent` buffer (`macro_buf`). `scm_macro_play` deserializes a register's byte string back to `KeyEvent`s and replays via `key_dispatch`. Serialization handles UTF-8 chars and special keys; modifier combos outside C-a…C-z are silently skipped. `scm_macro_is_recording` exposes the flag to Scheme as `%macro-recording?`. No header — Scheme binding declarations live in `scheme_bindings.h`.
- **scheme.c / scheme.h** — Scheme interpreter init. `scheme_init()` creates the Chibi Scheme context, registers ~70 foreign C functions as the `(editor primitives)` synthetic module, adds `scheme/` to the module search path, interns theme role symbols, and loads `init.scm`. Caches `call-interactively` after load. The `scm_*` binding implementations live in their respective subsystem files.
- **scheme_bindings.h** — Forward declarations of all `scm_*` binding functions across subsystems.
- **scheme_internal.h** — `SCM_CMD()` macro for simple bindings; provides global `G` pointer (`AppState *`).

## Key Dispatch Pipeline

```
SDL_AppEvent (event.c)
  → handle_text_input / handle_key_down (keyboard.c)
    → key_dispatch (keymap.c)
      → record to macro_buf if macro_recording (skip first key via macro_skip_next)
      → if which_key.enabled: feed through intercept keymap, update popup state
      → if focus == SPLASH: dispatch through splash_map only
      → if minibuf.active: set current buffer = minibuf.buf, reset prefix state
        → key_dispatch_inner (minibuffer-mode keymap, isolated from editor keymaps)
        → restore previous buffer/focus (unless submit/cancel already did so)
      → else: key_dispatch_inner
        → keymap_lookup_chain: minor modes → buffer-local → major → global
          → each level: keymap_lookup traverses parent chain
        → BINDING_KEYMAP: enter prefix (current_map = nested keymap)
        → BINDING_COMMAND: execute_command → sexp_apply(call-interactively, symbol)
        → no match + char + allows_input: self-insert
        → reset_key_state (current_map = global_map)
```

## Key Invariants

- **Bindings store Scheme symbols, not function pointers** — resolved at execution time via `call-interactively`; commands can be redefined at runtime
- **`call-interactively` must exist before `scheme_init()` returns** — cached and GC-preserved
- **Prefix key state**: `current_map != global_map` means mid-prefix; only that map is searched (no chain); ESC or invalid key resets
- **Lookup chain priority**: minor modes (most-recently-enabled first) > buffer-local > major mode > global — first match wins
- **Parent chain**: single-inheritance; `keymap_lookup()` walks `km → parent → ...` until found or NULL
- **`allows_input` gates self-insert**: only for unbound plain chars (no modifiers) at top level (not mid-prefix)
- **Modes are global, enabled per-buffer**: one `Mode` object shared across buffers; `ModeList` per buffer
- **`minibuf_init()` must run after modes are registered**: calls `mode_lookup("minibuffer-mode")`, defined in Scheme; call after `scheme_init()`
- **`macro_recording` is the authoritative recording flag**: `evil-recording-mode` minor mode is toggled off during insert/replace so `q` can be typed freely — always read `state->macro_recording` in C, not the minor mode
- **Macro playback re-enters `key_dispatch`**: replayed events flow through the full dispatch pipeline including any active mode keymaps

## Boundaries

- **keyboard.c** translates SDL → KeyEvent; never touches text or keymaps
- **keymap.c** dispatches keys and manages bindings; never reads/writes buffer text
- **mode.c** manages the mode registry; buffer-level mode lists live in `src/text/buffer.c`
- **macro.c** owns the recording buffer and playback; serialization is self-contained
- **scheme.c** initializes and registers; `scm_*` implementations live in their subsystem files
- **All command execution goes through Scheme** — even C commands are invoked via `call-interactively`

## Common Modification Workflows

### Adding a new C command exposed to Scheme

1. Write `scm_*` function in the appropriate subsystem file (e.g., `src/text/buffer.c`)
2. Add forward declaration in `scheme_bindings.h`
3. Register in `scheme_init()` via `SDEF("scheme-name", arity, scm_function)`
4. Declare in `scheme/editor/built-in.scm` with `(defcommand name "docstring")`
5. Bind to key with `(set-key! keymap "key" 'name)`

### Adding a new special key

1. Add variant to `KeySpecial` in `keyevent.h`
2. Map SDL keycode in `sdl_to_keyspecial()` in `keyboard.c`
3. Add string name in `parse_key_sequence()` in `keymap.c`

### Adding a new mode

- Usually from Scheme: `(define-minor-mode 'name keymap)` in `scheme/editor/mode.scm`
- From C: `mode_create()` → `mode_register()`

### Changing keymap lookup priority

- Edit `keymap_lookup_chain()` in `keymap.c`
