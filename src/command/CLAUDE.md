# src/command/ Subsystem

## Purpose

Input handling and command execution. Converts SDL key events into `KeyEvent` structs, dispatches them through a keymap lookup chain, and executes matched commands via Scheme's `call-interactively`. Also owns the mode registry and Scheme interpreter initialization.

## Files

- **keyevent.h** — `KeyEvent` type: `KEYEVENT_CHAR` (unicode codepoint) or `KEYEVENT_SPECIAL` (arrows, F-keys, etc.), plus modifier flags (`MOD_CTRL`, `MOD_META`, `MOD_SHIFT`, `MOD_SUPER`).
- **keyboard.c / keyboard.h** — SDL-to-KeyEvent translation. `handle_text_input()` decodes UTF-8 into char events; `handle_key_down()` normalizes SDL modifiers and maps special keys. Both call `key_dispatch()`. Called from `SDL_AppEvent()` in `event.c`.
- **keymap.c / keymap.h** — Keymap data structure and dispatch engine. A `Keymap` is a dynamic array of `(KeyEvent → Binding)` entries with a parent pointer for single-inheritance. A `Binding` is either a Scheme symbol (`BINDING_COMMAND`) or a nested keymap (`BINDING_KEYMAP`, for prefix keys). `key_dispatch()` is the main entry point. Also provides `parse_key_sequence()` for Emacs-style key notation (`"C-x"`, `"M-f"`, `"SPC"`).
- **minibuf.c / minibuf.h** — Minibuffer lifecycle. `minibuf_init()` creates the persistent `*minibuffer*` buffer, enables `minibuffer-mode`, and disables `evil-normal-mode` to isolate it from editor keymaps. Activate stores a prompt string and a GC-preserved Scheme `on_submit` callback, redirecting dispatch to the minibuf. Submit reads buffer text, restores the previous buffer, and calls the callback. Cancel restores without calling `on_submit`.
- **message.c / message.h** — Echo area. Static 2048-byte buffer shared with the UI renderer via `Clay_String`, and bool lock controlling when message can be updated.
- **mode.c / mode.h** — Mode registry. Modes have a name, type (major/minor), keymap, and `allows_input` flag. Two global registries (linked lists) for major and minor modes. Modes are created and registered globally, then enabled per-buffer. Buffer-level mode list management lives in `src/text/buffer.c`.
- **scheme.c / scheme.h** — Scheme interpreter init. `scheme_init()` creates the Chibi Scheme context, registers ~60 foreign C functions as the `(editor primitives)` synthetic module, adds `scheme/` to the module search path, interns theme role symbols, and loads `init.scm`. The `(import ...)` forms in `init.scm` trigger Chibi's module system to load `editor/*.sld` libraries automatically. Caches `call-interactively` after load. The `scm_*` binding implementations live in their respective subsystem files, not here.
- **scheme_bindings.h** — Forward declarations of all `scm_*` binding functions across subsystems.
- **scheme_internal.h** — `SCM_CMD()` macro for simple bindings; provides global `G` pointer.

## Key Dispatch Pipeline

```
SDL_AppEvent (event.c)
  → handle_text_input / handle_key_down (keyboard.c)
    → key_dispatch (keymap.c)
      → if minibuf.active: set current buffer = minibuf.buf, reset prefix state
        → key_dispatch_inner (minibuffer-mode keymap, isolated from editor keymaps)
        → restore previous buffer (unless submit/cancel already did so)
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
- **Module dependencies are resolved by Chibi**: `init.scm` imports all `(editor ...)` modules; `.sld` files declare their own imports, so load order is automatic
- **Minibuffer is fully isolated**: `minibuffer-mode` keymap has no parent; `evil-normal-mode` is disabled on the buffer; `key_dispatch` bypasses the normal lookup chain entirely when active
- **`minibuf_init()` must run after modes are registered**: it calls `mode_lookup("minibuffer-mode")`, which is defined in Scheme; call after `scheme_init()`

## Boundaries

- **keyboard.c** translates SDL → KeyEvent; never touches text or keymaps
- **keymap.c** dispatches keys and manages bindings; never reads/writes buffer text
- **mode.c** manages the mode registry; buffer-level mode lists live in `src/text/buffer.c`
- **scheme.c** initializes and registers; `scm_*` implementations live in their subsystem files
- **All command execution goes through Scheme** — even C commands are invoked via `call-interactively`

## Common Modification Workflows

### Adding a new C command exposed to Scheme

1. Write `scm_*` function in the appropriate subsystem file (e.g., `src/text/buffer.c`)
2. Add forward declaration in `scheme_bindings.h`
3. Register in `scheme_init()` via `SDEF("scheme-name", arity, scm_function)`
4. Declare in `scheme/built-in.scm` with `(defcommand name "docstring")`
5. Bind to key with `(set-key! keymap "key" 'name)`

### Adding a new special key

1. Add variant to `KeySpecial` in `keyevent.h`
2. Map SDL keycode in `sdl_to_keyspecial()` in `keyboard.c`
3. Add string name in `parse_key_sequence()` in `keymap.c`

### Adding a new mode

- Usually from Scheme: `(define-minor-mode 'name keymap)` in `scheme/mode.scm`
- From C: `mode_create()` → `mode_register()`

### Changing keymap lookup priority

- Edit `keymap_lookup_chain()` in `keymap.c`
