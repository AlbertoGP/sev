# scheme/ Subsystem

## Purpose

Scheme scripting layer that defines all user-facing commands, keybindings, modes, and themes. C provides ~60 primitive operations; Scheme composes them into the full editing experience. The evil (vim) modal editing system lives entirely here.

## Load Order (strict, set in `src/command/scheme.c`)

1. **command.scm** - Command infrastructure, documentation, interactive specs
2. **mode.scm** - Major/minor mode wrappers over C mode primitives
3. **icon.scm** - Icon registration and default scale variables
4. **built-in.scm** - Declares all C primitives as documented Scheme commands
5. **evil.scm** - Vim-like modal editing (operators, motions, visual, dot-repeat)
6. **theme.scm** - Theme definitions (palette + semantic roles), activates default theme
7. **init.scm** - Global keybindings and startup initialization

## File Roles

### command.scm
- `defcommand`, `defun`, `defvar` macros for self-documenting definitions
- `call-interactively` resolves interactive specs and invokes commands (cached by C at init)
- `set-key!` wraps C's `%set-key!` and records reverse binding in `*key-bindings-table*`
- Three registries: `*doc-table*`, `*interactive-table*`, `*key-bindings-table*`

### mode.scm
- `define-major-mode`, `define-minor-mode` wrap C's `%register-mode`
- `set-local!` / `get-local` for buffer-local variables
- `register-mode-icon` / `register-mode-icon/full` for mode icon theming
- Pre-defines `fundamental-mode` and `display-line-numbers-mode`

### icon.scm
- `register-icon` wraps `%register-icon!`
- Registers tab close/icon SVGs
- Sets `default-scale` and `default-buffer-scale`

### built-in.scm
- ~40 `defcommand` calls with no body (documents C primitives)
- ~8 compound Scheme commands built from primitives (e.g., `open-line-below`, `append-char`)

### evil.scm (923 lines, largest file)
- **7 keymaps**: normal, insert, replace, select, command, pending; all inherit from `global-keymap`
- **State machine**: `evil-sm-state` is `'normal` or `'operator-pending`; accumulates count via `evil-count`
- **Registries**: `*motion-table*` (symbol -> motion proc), `*operator-table*` (symbol -> operator proc)
- **Motions**: h/l/j/k, w/W/b/B/e/E, 0/$/^, gg/G, marks (`'a` / `` `a ``)
- **Operators**: `op-delete`, `op-change` (delete + enter insert)
- **Visual mode**: char (1), line (2), rectangle (3) submodes; operators work on selection
- **Dot repeat**: records last operator+motion+counts; `.` replays them
- **Word classification**: word-char `[a-zA-Z0-9_]`, punctuation, whitespace; `w` vs `W` distinction

### theme.scm
- `define-theme` registers palette (30 colors) + roles (30+ semantic mappings)
- `activate-theme` clears tables, sets palette/roles, updates icon colors
- Three Catppuccin variants: mocha (dark), macchiato (dark, default), latte (light)

### init.scm
- Global keybindings: ESC, C-q, C-e, tab nav, scaling, pane splits/navigation
- Calls `reset-global-scale` and `message-clear` at startup

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

- **Text**: `insert-char`, `delete-char`, `move-point`, `char-at-point`, `char-at`, `delete-range`, `buffer-length`, `skip-whitespace`
- **Cursor**: `point-get`, `point-set!`, `line-start`, `line-end`, `set-column`, `last-key-char`
- **Line queries**: `%goto-line`, `%line-count`, `%position-line`, `%line-start-position`, `%line-end-position`
- **Keymap**: `make-keymap`, `%set-key!`, `%set-keymap-parent!`
- **Mode**: `%register-mode`, `%set-major-mode`, `%enable-minor-mode`, `%disable-minor-mode`, `%set-mode-allows-input!`
- **Marks**: `%mark-set-to-point!`, `%mark-position`, `%point-to-mark!`, `%select-mode-set!`, `%select-mode-get`, `exchange-point-and-mark`
- **Pane/Tab**: `split-vertical`, `split-horizontal`, `pane-navigate-*`, `pane-close`, `tab-next`, `tab-prev`
- **Theme**: `%set-palette!`, `%set-role!`, `%clear-palette!`, `%clear-roles!`, `%register-icon!`, `%update-icon-colors!`, `%register-mode-icon!`
- **System**: `quit`, `eval-buffer`, `message`, `message-clear`, `prefix-arg`, `ignore`

Convention: `%`-prefixed names are raw C primitives; unprefixed wrappers in Scheme add documentation/side-effects.

## Common Modification Workflows

### Adding a new command
1. If it needs a C primitive: add foreign function in `scheme.c`, register it in `scheme_init()`
2. Declare with `(defcommand name "docstring" body)` in `built-in.scm` (C-only) or relevant file
3. Add `(interactive ...)` spec if it takes arguments from user context
4. Bind with `(set-key! keymap "key" 'name)` in the appropriate file

### Adding a new evil motion
1. Define motion proc: `(lambda (count) ...)`; must move point, return `(cons start end)` range
2. Register: `(register-motion! 'name proc)`
3. Bind in `normal-map`, `pending-map`, and `select-map`

### Adding a new evil operator
1. Define operator proc: `(lambda (start end) ...)`; operates on range
2. Register: `(register-operator! 'name proc)`
3. Bind in `normal-map` (enters pending), `select-map` (applies to selection)

### Adding a new mode
1. `(define-minor-mode 'name keymap [allows-input])` or `(define-major-mode 'name keymap)`
2. Optionally register icon: `(register-mode-icon 'name "file.svg" 'cursor-type)`
3. Create activation/deactivation commands

### Adding a new theme
1. `(define-theme 'name palette roles)` in `theme.scm`
2. Palette: alist of `(key . "#RRGGBB")` with ~30 color slots
3. Roles: alist of `(role . palette-key)` with ~30 semantic mappings
4. Activate with `(activate-theme 'name)`
