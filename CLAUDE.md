# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Emacs-like extensible text editor built with SDL3, Clay (immediate-mode UI), and Chibi Scheme for scripting/configuration. Targets desktop (Linux/Mac/Windows) and WebAssembly (browser via Emscripten).

## Build Commands

```bash
# Desktop build (CMake)
cmake -S . -B build-desktop
cmake --build build-desktop

# WebAssembly build
emcmake cmake -S . -B build-wasm
cmake --build build-wasm

# Quick build with Makefile (GCC, includes debug options)
make compile          # Build and run
make build-debug      # With AddressSanitizer
make vg               # Run with Valgrind

# Run
./out/app             # Makefile build
./build-desktop/app   # CMake build
```

## Architecture

### SDL3 Callback-Based Lifecycle

The app uses SDL3's main callbacks pattern (no `main()` function):

- `SDL_AppInit()` in `src/init.c` - Window, fonts, Clay UI, buffers, Scheme init
- `SDL_AppIterate()` in `src/iterate.c` - Main loop with 60 FPS cap for animations
- `SDL_AppEvent()` in `src/event.c` - Event dispatch (keyboard, mouse, window)
- `SDL_AppQuit()` in `src/quit.c` - Cleanup

### Global State (`src/state.h`)

`AppState` struct holds all application state: window, renderer, colors, theme, Scheme context, input state. Passed through SDL3 callbacks via `void *appstate`.

### Text Model (`src/text/`)

- **gap.c** - Gap buffer data structure for efficient text editing at cursor
- **buffer.c** - Buffer list management, text manipulation API (insert, delete, movement)
- **line.c** - Logical line tracking for rendering and navigation
- **mark.c** - Mark/region system for selections

### Input System (`src/command/keyboard.c`, `src/command/keymap.c`)

- `KeyEvent` represents either character input (`KEYEVENT_CHAR`) or special keys (`KEYEVENT_SPECIAL`)
- Keymaps form a hierarchy supporting prefix keys (e.g., `C-x` opens a prefix keymap)
- Bindings map to C functions, Scheme procedures, or nested keymaps
- `key_dispatch()` looks up and executes the appropriate command

### Scheme Integration (`src/command/scheme.c`)

Chibi Scheme provides extensibility. C primitives exposed include:

- Text operations: `insert-char`, `insert-string`, `delete-char`, `move-point`
- Buffer/mark operations
- UI: `quit`
- Keymap: `make-keymap`, `set-key!`

Keybindings configured in `resources/init.scm`.

### UI Rendering (`src/clay/`, `src/layout/`)

Clay immediate-mode UI with SDL3 renderer backend. Layout defined in `layout.c` with:

- Editor buffer view (text + cursor visualization)
- Status bar (buffer name, position, line/column)
- Animated theme transitions (dark/light)

## Key Patterns

- **Dirty flag**: `state->needs_redraw` controls when to re-render
- **Key parsing**: `parse_key_sequence()` handles Emacs-style notation ("C-x", "M-f", "SPC", "RET")

## Dependencies

Vendored as git submodules in `vendored/`:

- SDL3 - Windowing and input
- SDL_ttf - Font rendering
- chibi-scheme - Scheme interpreter

Clone with `--recursive` or run `git submodule update --init --recursive`.

## Plan Mode

- Make the plan extremely concise. Sacrifice grammar for the sake of concision.
- At the end of each plan, give me a list of unresolved questions to answer, if any.
