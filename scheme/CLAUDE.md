# scheme/ Subsystem

## Purpose

Scheme scripting layer that defines all user-facing commands, keybindings, modes, and themes. C provides ~60 primitive operations; Scheme composes them into the full editing experience. The evil (vim) modal editing system lives entirely here.

## Module System

All Scheme code (except `init.scm`) is encapsulated in R7RS `define-library` forms under the `(editor ...)` namespace. Each module has a `.sld` file (library definition with exports) and a `.scm` file (implementation) in `scheme/editor/`.

C primitives are exposed as the `(editor primitives)` module, registered synthetically in `scheme.c` via `add-module!`. Libraries import from `(editor primitives)` and from each other.

### Load mechanism

`scheme.c` adds `scheme/` to the module search path, registers `(editor primitives)`, then loads `init.scm`. The `(import ...)` forms in `init.scm` trigger Chibi's module system to load `.sld` files and resolve the dependency graph automatically.

### Module dependency graph

```
(editor primitives)    ← synthetic module wrapping C FFI bindings
  ↑
(editor command)       ← imports primitives
  ↑
(editor mode)          ← imports primitives, command
(editor icon)          ← imports primitives, command
(editor built-in)      ← imports primitives, command
  ↑
(editor evil)          ← imports primitives, command, mode
(editor theme)         ← imports primitives only

init.scm               ← imports all modules; sets global keybindings, runs startup
```

## Module Responsibilities

### (editor command)
Command infrastructure. `defcommand`, `defvar`, `defun` macros for self-documenting definitions. `call-interactively` resolves interactive specs and invokes commands (cached by C at init). `set-key!` wraps C's `%set-key!` and records reverse binding. Three registries: doc, interactive, and key-bindings tables.

### (editor mode)
Major/minor mode wrappers over C mode primitives. `define-major-mode`, `define-minor-mode`, buffer-local variables (`set-local!`/`get-local`), mode icon registration. Pre-defines `fundamental-mode` and `display-line-numbers-mode`.

### (editor icon)
Icon registration (`register-icon` wrapping `%register-icon!`), tab close/icon SVGs, default scale variables.

### (editor built-in)
~40 `defcommand` declarations documenting C primitives (no body). ~8 compound Scheme commands built from primitives (e.g., `open-line-below`, `join-line`).

### (editor evil) (largest module)
Vim-like modal editing. 7 keymaps (normal, insert, replace, select, command, pending; all inherit from `global-keymap`). State machine (`evil-sm-state`: `'normal` or `'operator-pending`; accumulates count). Motion and operator registries. Visual mode with char/line/rectangle submodes. Dot repeat. Word classification for `w` vs `W`.

### (editor theme)
Theme definitions. `define-theme` registers palette (~30 colors) + roles (~30 semantic mappings). `activate-theme` clears tables, sets palette/roles, updates icon colors. Three Catppuccin variants: mocha, macchiato (default), latte.

### init.scm (not a library)
Top-level script loaded by `scheme.c`. Imports all `(editor ...)` modules into the main environment. Sets global keybindings (ESC, C-q, C-e, tab nav, scaling, pane splits/navigation). Runs startup commands (`reset-global-scale`, `message-clear`).

## Key Invariants

- All commands reachable from keybindings are Scheme symbols looked up via `call-interactively`
- `call-interactively` is cached as a C `sexp` at init; must be defined before `scheme_init()` returns
- Keymap parent chains enable inheritance: mode keymaps inherit `global-keymap` so ESC/C-q always work
- Lookup order: minor mode keymaps -> major mode keymap -> global keymap (each traverses parent chain)
- Only one major mode per buffer; multiple minor modes allowed
- Minor modes with `allows_input: true` (insert, replace) capture unbound chars as `self-insert`
- Evil state is pure Scheme; C only provides motion/edit primitives
- Theme activation is immediate (sets palette + roles in C, triggers redraw)
- No persistence layer; all state is in-memory

## C Foreign Function API (defined in `src/command/scheme.c`)

Exposed as `(editor primitives)`. Convention: `%`-prefixed names are raw C primitives; unprefixed wrappers in Scheme add documentation/side-effects.

- **Text**: `insert-char`, `delete-char`, `move-point`, `char-at-point`, `char-at`, `delete-range`, `buffer-length`, `skip-whitespace`
- **Cursor**: `point-get`, `point-set!`, `line-start`, `line-end`, `set-column`, `last-key-char`
- **Line queries**: `%goto-line`, `%line-count`, `%position-line`, `%line-start-position`, `%line-end-position`
- **Keymap**: `make-keymap`, `%set-key!`, `%set-keymap-parent!`
- **Mode**: `%register-mode`, `%set-major-mode`, `%enable-minor-mode`, `%disable-minor-mode`, `%set-mode-allows-input!`
- **Marks**: `%mark-set-to-point!`, `%mark-position`, `%point-to-mark!`, `%select-mode-set!`, `%select-mode-get`, `exchange-point-and-mark`
- **Pane/Tab**: `split-vertical`, `split-horizontal`, `pane-navigate-*`, `pane-close`, `tab-next`, `tab-prev`
- **Theme**: `%set-palette!`, `%set-role!`, `%clear-palette!`, `%clear-roles!`, `%register-icon!`, `%update-icon-colors!`, `%register-mode-icon!`
- **System**: `quit`, `eval-buffer`, `message`, `message-clear`, `prefix-arg`, `ignore`

## Common Modification Workflows

### Adding a new command
1. If it needs a C primitive: add foreign function in `scheme.c`, add to `(editor primitives)` export list
2. Declare with `(defcommand name "docstring" body)` in the appropriate module
3. Add `(interactive ...)` spec if it takes arguments from user context
4. Export from the module's `.sld` file
5. Bind with `(set-key! keymap "key" 'name)` in the appropriate file

### Adding a new evil motion
1. Define motion proc in `evil.scm`: `(lambda (count) ...)`; must move point, return `(cons start end)` range
2. Register: `(register-motion! 'name proc)`
3. Bind in `normal-map`, `pending-map`, and `select-map`
4. Export from `editor/evil.sld`

### Adding a new evil operator
1. Define operator proc in `evil.scm`: `(lambda (start end) ...)`; operates on range
2. Register: `(register-operator! 'name proc)`
3. Bind in `normal-map` (enters pending), `select-map` (applies to selection)
4. Export from `editor/evil.sld`

### Adding a new mode
1. `(define-minor-mode 'name keymap [allows-input])` or `(define-major-mode 'name keymap)` in `mode.scm`
2. Optionally register icon: `(register-mode-icon 'name "file.svg" 'cursor-type)`
3. Create activation/deactivation commands
4. Export from `editor/mode.sld`

### Adding a new theme
1. `(define-theme 'name palette roles)` in `theme.scm`
2. Palette: alist of `(key . "#RRGGBB")` with ~30 color slots
3. Roles: alist of `(role . palette-key)` with ~30 semantic mappings
4. Activate with `(activate-theme 'name)`

### Adding a new module
1. Create `scheme/editor/foo.sld` with `(define-library (editor foo) ...)` and exports
2. Create `scheme/editor/foo.scm` with implementation
3. Add `(import (editor foo))` to `init.scm`
