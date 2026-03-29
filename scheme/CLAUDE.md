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
(editor minibuffer)    ← imports primitives, command, mode
(editor which-key)     ← imports primitives, command
  ↑
(editor evil)          ← imports primitives, command, mode
(editor theme)         ← imports primitives only

init.scm               ← imports all modules; sets global keybindings, mouse/welcome handlers, runs startup
```

## Module Responsibilities

### (editor command)
Command infrastructure. `defcommand`, `defvar`, `defun` macros for self-documenting definitions. `call-interactively` resolves interactive specs via CPS traversal and invokes commands (cached by C at init). `set-key!` wraps `%set-key!` and records reverse binding for `where-is`. `bind-prefix!` registers prefix keymaps and tracks edges for path resolution. Spec handlers registered with `register-spec-handler!` handle async argument collection (e.g., `minibuffer-read`). Four registries: doc, interactive, keymap-edges, keymap-commands.

### (editor mode)
Major/minor mode wrappers over C mode primitives. `define-major-mode`, `define-minor-mode`, buffer-local variables (`set-local!`/`get-local`), `register-mode-icon` / `register-mode-icon/full`. Pre-defines `fundamental-mode` and `display-line-numbers-mode`.

### (editor icon)
Icon registration (`register-icon` wrapping `%register-icon!`), tab close/icon SVGs, default scale variables.

### (editor built-in)
~40 `defcommand` declarations documenting C primitives (no body). Compound Scheme commands built from primitives (e.g., `open-line-below`, `join-line`, `save-buffer`, `save-buffer-as`, `switch-to-buffer`, `describe-function`, `describe-key`).

### (editor minibuffer)
Minibuffer keymap and mode (`minibuffer-mode`). Submit/cancel bindings, cursor style registration. `minibuffer-read` activates the minibuffer with a prompt and Scheme callback. `execute-extended-command` (M-x). Registers `minibuffer-read` as the `'minibuffer-read` interactive spec handler so `(interactive (minibuffer-read "Prompt: "))` works in `defcommand`.

### (editor which-key)
`which-key-toggle` command wrapping `%which-key-toggle` C primitive.

### (editor evil)
Vim-like modal editing. Implementation is split across `scheme/editor/evil/` and included in order by `evil.sld`. All files share one scope so earlier definitions are visible to later ones.

- **`core.scm`** — Keymap declarations and all static key bindings (normal, insert, select, pending, command maps). Mode registration and icons for the 6 evil minor modes. UTF-8 helpers (`utf8-byte-len-at`, `count-chars-in-range`). Mode transition commands (`evil-normal`, `evil-insert`, `evil-replace`, `evil-select*`, `evil-command`, `evil-mode`, `evil-state`). State machine records (`<range>`, `<repeat-info>`), state variables, motion/operator/text-object hash-table registries, character classification (`evil-word-char?` etc.), and the three core dispatch functions (`evil-execute-motion`, `evil-enter-operator`, `evil-execute-text-object`).

- **`undo.scm`** — `evil-undo`, `evil-redo`, `evil-line-restore`. Dot-repeat infrastructure: `evil-replay-setup`, `evil-insert-text`, `evil-repeat`. Count helpers: `evil-digit-argument`, `evil-zero`.

- **`motion.scm`** — Motion registrations for h/j/k/l/0/$/^/w/b/e/W/B/E/goto-line/current-line (the lambdas passed to `register-motion!`). Command wrappers bound to keys, plus mark commands (`evil-set-mark`, `evil-goto-mark-*`), jump list commands, and f/F/t/T character-seek commands.

- **`operator.scm`** — Register helpers (`strip-trailing-newline`, `string-split/join-newlines`, `evil-paste-block`, `evil-register-write!`). Operator registrations (delete/change/yank). Operator command wrappers (`evil-op-*`, `evil-D/C/S`). Count-aware `evil-x`/`evil-X`. Character replace (`evil-char-replace-setup`, `evil-char-replace`, `evil-visual-char-replace`). Compound entry commands (`open-line-below/above`, `insert-at-start`, `append-char/line`, `substitute-char`). Yank/paste commands (`evil-paste-after`, `evil-paste-before`, `evil-insert-paste-clipboard`) and their key bindings.

- **`text-object.scm`** — Pair/quote/tag search helpers (`find-enclosing-pair`, `find-quote-pair`, `find-enclosing-tag`). Text object registrations for word/WORD/sentence/paragraph/bracket/paren/brace/angle/dquote/squote/backtick/tag. `evil-in-text-object` and `evil-around-text-object` commands and their dynamic `i`/`a` key bindings.

- **`visual.scm`** — Visual selection helpers (`evil-visual-range`, `line-content-end`, `evil-rect-bounds`, `evil-rect-apply`). Visual operator commands (delete/change/yank/paste, rect insert/append). Visual mode key bindings for operators and motions.

- **`macro.scm`** — `evil-recording-mode` keymap and minor mode. Macro commands (`evil-start-macro`, `evil-stop-macro`, `evil-play-macro`, `evil-play-last-macro`). Dynamic `q`/`@` key bindings. `(evil-mode)` and `(evil-normal)` activation calls.

### (editor theme)
Theme definitions. `define-theme` registers palette (~30 colors) + roles (~30 semantic mappings). `activate-theme` clears tables, sets palette/roles, updates icon colors. Three Catppuccin variants: mocha, macchiato (default), latte.

### init.scm (not a library)
Top-level script loaded by `scheme.c`. Imports all `(editor ...)` modules. Sets global keybindings and prefix maps (buffer, help, line-numbers, split, tab). Binds M-x to `execute-extended-command`. Installs mouse click handler (exits visual mode, moves point) and drag handler (enters visual-char mode, tracks cursor). Registers `welcome-map` (minimal keymap for when no panes are open) via `%set-welcome-keymap!`. Runs startup: `reset-global-scale`, `message-clear`.

## Key Invariants

- All commands reachable from keybindings are Scheme symbols looked up via `call-interactively`
- `call-interactively` is cached as a C `sexp` at init; must be defined before `scheme_init()` returns
- Keymap parent chains enable inheritance: mode keymaps inherit `global-keymap` so ESC/C-q always work
- Lookup order: minor mode keymaps → major mode keymap → global keymap (each traverses parent chain)
- Only one major mode per buffer; multiple minor modes allowed
- Minor modes with `allows_input: true` (insert, replace, minibuffer) capture unbound chars as `self-insert`
- Evil state is pure Scheme; C only provides motion/edit primitives
- `evil-recording-mode` is disabled while in insert/replace (so `q` self-inserts); `state->macro_recording` is the authoritative C flag for whether recording is active
- `macro-char` is a buffer-local string (1 char) set by `evil-start-macro`; read from C via `vartable_get`
- Theme activation is immediate (sets palette + roles in C, triggers redraw)
- No persistence layer; all state is in-memory

## C Foreign Function API (defined in `src/command/scheme.c`)

Exposed as `(editor primitives)`. Convention: `%`-prefixed names are raw C primitives; unprefixed wrappers in Scheme add documentation/side-effects.

- **Text**: `insert-char`, `delete-char`, `move-point`, `char-at-point`, `char-at`, `delete-range`, `buffer-length`, `skip-whitespace`
- **Cursor**: `point-get`, `point-set!`, `line-start`, `line-end`, `set-column`, `last-key-char`
- **Line queries**: `%goto-line`, `%line-count`, `%position-line`, `%line-start-position`, `%line-end-position`
- **Keymap**: `make-keymap`, `%set-key!`, `%set-keymap-parent!`, `%set-keymap-name!`, `%bind-prefix!`, `%read-key-binding`, `%set-key-unbound-cb!`
- **Mode**: `%register-mode`, `%set-major-mode`, `%enable-minor-mode`, `%disable-minor-mode`, `%set-mode-allows-input!`, `%buffer-has-minor-mode?`, `%buffer-minor-modes`
- **Marks**: `%mark-set-to-point!`, `%mark-position`, `%point-to-mark!`, `%select-mode-set!`, `%select-mode-get`, `exchange-point-and-mark`
- **Macros**: `%macro-start!`, `%macro-stop!`, `%macro-play`, `%macro-recording?`
- **Jump list**: `%jump-push!`, `%jump-backward!`, `%jump-forward!`
- **Mouse**: `%set-mouse-click-handler!`, `%set-mouse-drag-handler!`
- **Minibuffer**: `%minibuffer-activate`, `%minibuffer-submit`, `%minibuffer-cancel`
- **Pane/Tab**: `split-vertical`, `split-horizontal`, `pane-navigate-*`, `tab-close`, `tab-next`, `tab-prev`, `tab-new`, `%set-welcome-keymap!`, `no-panes?`
- **Theme**: `%set-palette!`, `%set-role!`, `%clear-palette!`, `%clear-roles!`, `%register-icon!`, `%update-icon-colors!`, `%register-mode-icon!`
- **Which-key**: `%which-key-toggle`
- **System**: `quit`, `eval-buffer`, `message`, `message-clear`, `message-echo`, `message-unlock`, `prefix-arg`, `ignore`

## Common Modification Workflows

### Adding a new command
1. If it needs a C primitive: add foreign function in appropriate subsystem file, declare in `scheme_bindings.h`, register in `scheme_init()` via `SDEF()`, add to `(editor primitives)` export list in `scheme.c`
2. Declare with `(defcommand (name) "docstring" body)` in the appropriate module
3. Add `(interactive (minibuffer-read "Prompt: "))` spec if it takes user input
4. Export from the module's `.sld` file
5. Bind with `(set-key! keymap "key" 'name)`

### Adding a new evil motion
1. Register in `evil/motion.scm`: `(register-motion! 'name (lambda (count) ...))`; must move point
2. Add a `defcommand` wrapper in the same file calling `(evil-execute-motion 'name)`
3. Bind the wrapper in `normal-map`, `pending-map`, and `select-map` (bindings live in `evil/core.scm` or inline in `motion.scm`)
4. Export from `editor/evil.sld`

### Adding a new evil operator
1. Register in `evil/operator.scm`: `(register-operator! 'name (lambda (range) ...))`; operates on range
2. Add a `defcommand` wrapper calling `(evil-enter-operator 'name)`
3. Bind in `normal-map` (enters pending); visual binding goes in `evil/visual.scm`
4. Export from `editor/evil.sld`

### Adding a new mode
1. `(define-minor-mode 'name keymap [allows-input])` or `(define-major-mode 'name keymap)` in `mode.scm`
2. Optionally register icon: `(register-mode-icon/full 'name "file.svg" 'bg-role 'label-role 'cursor-role 'cursor-type)`
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
