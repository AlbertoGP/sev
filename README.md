<h1 align="center">
  <img src="logo.svg" alt="Sev" width="325">

<a href="https://dylancobb.github.io/sev">Website</a> |
<a href="https://discord.gg/u9tvBmTMwx">Chat</a>

</h1>

# Sev

Sev is an Emacs-like, Vim-like graphical text editor, built from scratch using [SDL3](https://libsdl.org/) and [Clay](https://www.nicbarker.com/clay), with [Chibi Scheme](https://synthcode.com/wiki/chibi-scheme) as its scripting layer. It runs natively on Linux and macOS (Windows soon!), and in the browser via WebAssembly.

## Screenshots

<figure>
  <img width="3778" height="2047" alt="image" src="https://github.com/user-attachments/assets/5b595e0d-571f-4b2d-9ecd-ce4916cd9e6a" />
  <figcaption>Panes, tabs and syntax highlighting.</figcaption>
</figure>

<figure>
  <img width="3778" height="2047" alt="image" src="https://github.com/user-attachments/assets/84412c21-f423-4884-9566-360ca90f9b32" />  
  <figcaption>Welcome screen and command palette.</figcaption>
</figure>

<figure>
  <img width="3778" height="2047" alt="image" src="https://github.com/user-attachments/assets/87ed1065-f070-4aa8-b271-566000c7153c" />
  <figcaption>Theme picker with theme previews.</figcaption>
</figure>


## Features

- **Full Vim-style modal editing** — normal, insert, visual, replace, and operator-pending modes; motions, operators, text objects, registers, macros and marks
- **Modern VSCode/Zed-style command palette** — `ctrl-shift-p` or `:` to search and run any command by name
- **Which-key overlay** — press a prefix key and see all available continuations
- **Scheme scripting** — every command, keybinding, and mode is defined in Scheme and can be overridden; no recompile needed
- **Themes** — Catppuccin and Solarized themes are included, with many more planned; define your own in a few lines
- **Split panes** — horizontal and vertical splits, navigate and resize with the keyboard or mouse
- **Runs in the browser** — build with Emscripten for a zero-install WebAssembly version
- **Undo tree, jump list, buffer-local variables, tab bar, and so much more**

## Quick Start

### Prerequisites

- CMake 3.20+
- A C compiler (GCC or Clang)
- SDL3 system dependencies (on Debian/Ubuntu: `libgl-dev libgles-dev`)

Dependencies (SDL3, SDL_ttf, Chibi Scheme) are vendored as git submodules.

```bash
git clone --recursive https://github.com/your-username/sev
cd sev
```

### Desktop

```bash
cmake -S . -B build-desktop
cmake --build build-desktop
./build-desktop/app
```

### WebAssembly

Requires [Emscripten](https://emscripten.org/docs/getting_started/downloads.html).

```bash
emcmake cmake -S . -B build-wasm
cmake --build build-wasm
python3 -m http.server   # then open http://localhost:8000
```

## Scriptable with Scheme

All commands, keybindings, modes, and themes are defined in Scheme. Drop your configuration in `scheme/init.scm` or load it from there.

**Define and bind a custom command:**

```scheme
(defcommand (greet)
  "editor: greet\nShow a personalised message in the echo area."
  (interactive)
  (minibuffer-read "Your name: "
    (lambda (name)
      (message (string-append "Hello, " name "!")))))

(set-key! global-keymap "ctrl-shift-g" 'greet)
```

**Define a custom theme:**

```scheme
(define-theme
  'my-theme
  "My Theme Display Name"
  'palette
  '((bg-0    . "#1a1b26")
    (bg-1    . "#16161e")
    (bg-2    . "#13131a")
    (fg-0    . "#2a2b3d")
    (fg-1    . "#3b3c52")
    (fg-2    . "#414868")
    (accent-0 . "#565f89")
    (accent-1 . "#6b7394")
    (accent-2 . "#828bb8")
    (text-0  . "#c0caf5")
    (text-1  . "#a9b1d6")
    (text-2  . "#9aa5ce")
    (select  . (alpha accent-2 0.25))
    (blue    . "#7aa2f7")
    (cyan    . "#7dcfff")
    (green   . "#9ece6a")
    (yellow  . "#e0af68")
    (orange  . "#ff9e64")
    (red     . "#f7768e")
    (purple  . "#bb9af7")
    (pink    . "#ff007c"))
  'canonical-map
  '((fg-text      . text-0)
    (fg-text-dim  . text-2)
    (fg-warm      . text-1)
    (selection-bg . select))
  'overrides
  '((mode.normal . blue)))   ; omit to keep *default-roles* value

(activate-theme 'my-theme)
```

The full Vim modal editing system — motions, operators, text objects, macros — is implemented in `scheme/editor/evil/`. It's a good reference for what's possible without touching C.

## Project Organisation

```
├── resources/    fonts and SVG icons
├── scheme/       all Scheme scripts (commands, modes, keybindings, themes)
│   └── editor/   R7RS modules: command, mode, evil, theme, minibuffer, …
├── src/          C source (~12k LOC)
│   ├── command/  input handling, keymaps, command dispatch, Scheme bridge
│   ├── text/     buffers, marks, lines, registers, undo
│   └── display/  layout, panes, tabs, status bar, Clay components
└── vendored/     SDL3, SDL_ttf, Chibi Scheme (git submodules)
```

## Roadmap

Beyond standard text editing, the longer-term vision includes:

- **Collaborative editing** — real-time sessions using the [Eg-walker](https://arxiv.org/pdf/2409.14252) algorithm
- **Declarative UI-builder DSL** — enable users to create custom UIs for their plugins directly via the Scheme layer
- **Alternative text display** — better use of modern monitors and typography (see [this paper](https://arxiv.org/pdf/2008.06030))
- **Wiki / hypermedia** — Org Mode-style directives and wiki links, inspired by Obsidian and the original hypertext vision
- **Embeddable core** — Sev is designed to be usable as a foundation for non-text-editor apps (smart notes software, data tools, etc.) where rich text editing is a first-class feature

## Project Status

Sev is **early / experimental**. Core editing works and it is usable, but there are a few specific features that will make it more viable as a daily driver. Namely:

- Text search and replace
- A file picker / file tree
- LSP integration
- In-app documentation

Expect rough edges, missing features, and breaking changes. Feedback and contributions are very welcome.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for how to get started. The Scheme layer (`scheme/`) is a great place to contribute without needing to touch C — new commands, themes, motions, and modes can all be written in pure Scheme.

## Community

[Discord](https://discord.gg/u9tvBmTMwx) — join the conversation as the community gets off the ground.

## License

[MIT](LICENSE) © 2026 Dylan Cobb
