# Contributing to `sev`

Thanks for your interest in contributing. `sev` is early-stage, so there's a lot of ground to cover and contributions of all sizes are welcome.

## Where to Start

The best place to orient yourself is [`README.md`](README.md), which describes the overall architecture. If you want to get a better understanding of a specific area, the project breaks down roughly into:

- [`src/`](src/) — AppState type and SDL3 callbacks for main app event loop
- [`src/command/`](src/command/) — input, keymaps, command dispatch, Scheme bridge
- [`src/text/`](src/text/) — buffers, marks, lines, undo
- [`src/display/`](src/display/) — layout, panes, tabs, UI components
- [`scheme/`](scheme/) — Scheme scripting layer, module system, C FFI

**Good first areas:**

- **New theme** — `scheme/editor/theme/` has Catppuccin and Solarized as examples; a theme is a palette alist + role mapping, no C required
- **New command** — `scheme/editor/built-in.scm` has ~40 examples; most commands are 3–10 lines of Scheme
- **Documentation** — many commands have minimal docstrings; improving them helps the command palette and `describe-function`
- **Bug reports** — open an issue with steps to reproduce and your platform

## Development Setup

Same as the [Quick Start](README.md#quick-start) in the README.

```bash
git clone --recursive https://github.com/your-username/sev
cd sev
cmake -S . -B build-desktop
cmake --build build-desktop
./build-desktop/app
```

For faster iteration during development, the Makefile wraps a GCC build with useful flags:

```bash
make compile        # build and run
make build-debug    # build with AddressSanitizer
make vg             # run under Valgrind
```

## Contribution Areas

### Scheme layer (no C required)

The entire Vim modal editing system lives in `scheme/editor/vim/` — motions, operators, text objects, macros, visual mode. New vim commands, text objects, and motions are pure Scheme. Themes, commands, and modes are the same.

See `scheme/README.md` for step-by-step workflows for each type of addition.

### C core

- `src/text/` — text engine: gap buffers, marks, logical lines, undo, registers
- `src/display/` — layout and rendering: panes, tabs, Clay components
- `src/command/` — input pipeline and Scheme FFI: new C primitives exposed to Scheme go here

New C primitives must be registered in `scheme_init()` via `SDEF()` and added to the `(editor primitives)` export list in `scheme.c`.

### Documentation

We need more documentation!

- Short term goal: every subdirectory should have a README.md. Scheme documentation strings for commands / functions / variables should be more informative.
- Long term goal: expansive user documentation readable from within the app (along the lines of Emacs's `info-mode`)

The `scheme/README.md` file documents the full C FFI and module system. The per-subsystem `README.md` files describe architecture and patterns. Improvements to any of these help future contributors ramp up faster.

## Code Style

- **C**: follow patterns in the surrounding code; no enforced formatter
- **Scheme**: R7RS, `define-library` module conventions; follow the style of `scheme/editor/`
- **Commits**: short present-tense subject line; body if the change needs explanation

## Submitting Changes

1. Fork the repo and create a branch from `main`
2. Make your changes
3. Open a pull request — describe what you changed and why
4. Keep PRs focused; one logical change per PR

## Community

[Discord](https://discord.gg/u9tvBmTMwx)
