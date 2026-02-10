![Banner image](banner.png)

# `sev`

`sev` is an Emacs-like, Vim-like extensible graphical text editor powered by:

- [SDL3](https://libsdl.org/) for windowing, rendering and low level device I/O.
- [Clay](https://www.nicbarker.com/clay) for layout and UI.
- [Chibi Scheme](https://synthcode.com/wiki/chibi-scheme) as an embedded interpreted language, filling the same role as elisp in Emacs or Lua in Neovim.

This application is _not_ an attempt to build a feature-complete clone of Emacs, but does aim to be:

- Genuinely usable.
- Lightweight / performant.
- Portable. More specifically, able to run on Windows, Mac, Linux _and_ in the browser via Emscripten and WebAssembly.
- Highly configurable and extensible via Scheme.

The ultimate goal of `sev` is to bring together a core set of features from Emacs and Vim (and a couple from more modern editors like Helix and Zed), which can then serve as a foundation for _non_-text-editor applications where text editing needs to be a first-class application capability. This leads to a couple of points of departure from the Emacs philosophy:

1. Everything is _not_ a buffer. _Many_ things are buffers, but `sev` will also make use of other forms of graphical display. For example, I plan on using it to create a music composition environment with notation and all kinds of other non-text graphical elements.
2. `sev` does not try to be unopinionated. Defaults should be sane, and batteries should usually be included.

Emacs's power comes from the homogeneity of its internals and the fact that _everything_ is exposed via Lisp and available to customise. But the end user of Emacs is the person using it as a text editor, and they need to be able to change everything _at runtime_.

The end user of `sev` is the application developer, who has direct access to the C internals. _Their_ end user is using whatever they build with it! _Most_ things should still be customisable, but the application developer is the user `sev` will be specially oriented towards. Hopefully it will still be an excellent text editor by itself, and perfectly viable as a main driver.

A couple of features are also on the roadmap for `sev` beyond standard text editing capabilities:

- I'd like `sev` to support collaborative text editing sessions ASAP. [Eg-walker](https://arxiv.org/pdf/2409.14252) is a fairly recent algorithm and looks very exciting.
- I'm keen to explore [alternative ways of displaying text](https://arxiv.org/pdf/2008.06030) that make better use of modern monitors and typography.
- As an admirer of [Obsidian](https://obsidian.md/), Emacs's [Org Mode](https://orgmode.org/) and the original idea of [hypermedia](https://en.wikipedia.org/wiki/Hypermedia) that gave us the World Wide Web, I'd like to build some wiki-like capabilities directly into the editor. Possibly via directives that are usable anywhere recognisable as inside a code comment in text buffers (or anywhere if the buffer holds non-code text like Markdown).

## Project Organisation

At the the top level:

```
├── resources/
├── scheme/
├── src/
│   ├── clay/
│   ├── command/
│   ├── display/
│   ├── text/
│   ├── event.c
│   ├── init.c
│   ├── iterate.c
│   ├── quit.c
│   └── state.h
└── vendored/
```

- `resources/` is where font and icon files are located.
- `scheme/` is where all `sev`-specific Scheme scripts are located.
- `src/` is where all C code resides.
- `vendored/` is where external repositories such as [SDL3](https://github.com/libsdl-org/sdl) and [Chibi Scheme](https://github.com/ashinn/chibi-scheme) live as git submodules.

Within the `src/` directory, the application is divided into three main layers: `command/`, `text/` and `display/`.

- The command layer manages the mapping of user input to keymaps and command invocation, as well as setting up the Scheme interpreter.
- The text layer defines things like text buffers, marks, logical lines and buffer-local variables, and exposes the functions that manipulate them.
- The display layer defines the application layout, reusable UI components and the display model's notion of things such as _tabs_, _panes_ and _visual lines_.

Several files also live at the top level of the `src/` directory:

- `init.c`, `event.c`, `iterate.c` and `quit.c` define functions called by SDL.
  - `init.c` is where all code called during application startup should go.
  - `event.c` is where handling is set up for various events.
  - `iterate.c` is the entry point for rendering and post-rendering cleanup.
  - `quit.c` is where functions are called to clean up allocated resources on app shutdown.
- `state.h` defines the global `AppState` object.

## Building

### Fresh build commands

```bash
# Desktop
cmake -S . -B build-desktop
cmake --build build-desktop

# WASM
emcmake cmake -S . -B build-wasm
cmake --build build-wasm

# WASM with optimisations
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
```

### Rebuild commands

```bash
## Desktop
cd build-desktop
make -j$(nproc)

## WASM
cd build-wasm
make -j$(nproc)
```

### Running WASM build

```bash
python3 -m http.server
```
