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

`AppState` struct holds all application state: window, renderer, colors, theme, Scheme context, input state, minibuffer state. Passed through SDL3 callbacks via `void *appstate`.

### Text Model (`src/text/`)

Buffers, marks, logical lines, buffer-local variables.

- See `src/text/CLAUDE.md` for subsystem details.

### Input + Command Layer (`src/command/`)

Input handling and command execution layer. Modes, keymaps and Scheme interpreter init are defined here too.

- See `src/command/CLAUDE.md` for subsystem details.

### Scheme Integration (`src/command/scheme.c`, `scheme/`)

Chibi Scheme provides extensibility and user scripting.

- See `scheme/CLAUDE.md` for subsystem details.

### UI Rendering (`src/clay/`, `src/display/`)

Clay immediate-mode UI with SDL3 renderer backend. Layout defined in `layout.c` with:

- Editor buffer view (text + cursor visualization)
- Status bar (buffer name, position, line/column)
- Bottom strip: minibuffer (prompt + editable text + cursor) when active, echo area otherwise

- See `src/display/CLAUDE.md` for display subsystem details.
- See `src/clay/CLAUDE.md` for Clay library and renderer backend details.

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
