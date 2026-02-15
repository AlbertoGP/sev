# src/clay/ Subsystem

## Purpose

Bridges the vendored Clay immediate-mode UI library to SDL3. Provides arena initialization, a text measurement callback, and a renderer that converts Clay layout commands into SDL3 draw calls. All UI in the editor flows through this layer — `src/display/` declares layout with `CLAY()` macros, and this module computes and renders it.

## Files

- **clay.h** — Vendored Clay v0.14 single-header library (~270K, do not edit). Provides `CLAY()` / `CLAY_TEXT()` declarative layout macros. Computes layout each frame and emits a `Clay_RenderCommandArray`. Activated by `#define CLAY_IMPLEMENTATION` in `init.h` (must be included exactly once).
- **init.c / init.h** — `clay_init(state)`: allocates Clay's arena, calls `Clay_Initialize()`, registers an `SDL_MeasureText()` callback that uses `TTF_GetStringSize()` for layout computation.
- **renderer.c / renderer.h** — `SDL_Clay_RenderClayCommands()`: iterates render commands and draws them via SDL3 (rectangles, text, borders, images, scissor regions, custom shapes). Internally caches fonts (8 slots) and TTF_Text objects (256 slots, FNV-1a hashed) to avoid per-frame recreation. `Clay_SDL3RendererData` struct holds `SDL_Renderer`, `TTF_TextEngine`, font array, and font paths.

## Key Invariants

- Layout is fully recomputed every frame — no persistent layout state
- `Clay_BeginLayout()` / `Clay_EndLayout()` must bracket all `CLAY()` calls each frame
- Clay does not own string data — callers must keep strings alive until after rendering
- Font IDs are indices into `rendererData.fonts[]` (FONT_NORMAL=0, FONT_BOLD=1, FONT_ITALIC=2)
- All `Clay_Color` values use 0–255 range

## Per-Frame Pipeline

```
Clay_BeginLayout()
  → CLAY() macro calls (in src/display/)
  → Clay_EndLayout()                    // computes layout, returns render commands
  → SDL_Clay_RenderClayCommands()       // draws to SDL_Renderer
  → SDL_RenderPresent()
```

## Common Modification Workflows

### Adding a new custom render type
1. Add variant to `CustomRenderType` enum in `renderer.h`
2. Handle in `CLAY_RENDER_COMMAND_TYPE_CUSTOM` branch of `SDL_Clay_RenderClayCommands()`

### Adding a new font slot
1. Add enum value in font enum (see `init.c`)
2. Load font in `SDL_AppInit()` (`src/init.c`), store in `rendererData.fonts[]`
3. Reference by font ID in `CLAY_TEXT()` config
