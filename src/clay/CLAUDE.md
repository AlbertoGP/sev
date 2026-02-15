# src/clay/ Subsystem

## Purpose

Bridges the vendored Clay immediate-mode UI library to SDL3. Provides arena initialization, text measurement callback, and a renderer that converts Clay layout commands into SDL3 draw calls. All UI in the editor flows through this layer.

## Files

### clay.h (vendored, ~270K, do not edit)
- Single-header Clay v0.14 library
- Provides `CLAY()` / `CLAY_TEXT()` declarative layout macros
- Computes layout each frame; emits `Clay_RenderCommandArray` of draw commands
- Activated by `#define CLAY_IMPLEMENTATION` in `init.h`

### init.c / init.h
- `clay_init(state)` — allocates arena via `Clay_MinMemorySize()`, calls `Clay_Initialize()`
- Registers `SDL_MeasureText()` callback — Clay calls this to measure text via `TTF_GetStringSize()`
- Font array (`state->rendererData.fonts`) passed as userData to measurement function
- `init.h` defines `CLAY_IMPLEMENTATION` (must be included exactly once)

### renderer.c / renderer.h
- `SDL_Clay_RenderClayCommands()` — iterates render command array, dispatches by type:
  - `RECTANGLE` — solid fill, optional rounded corners via triangle-fan geometry
  - `TEXT` — cached TTF_Text objects rendered with `TTF_DrawRendererText()`
  - `BORDER` — per-edge strokes with arc corners
  - `IMAGE` — `SDL_RenderTexture()` (imageData cast to `SDL_Texture*`)
  - `SCISSOR_START/END` — `SDL_SetRenderClipRect()` pairs
  - `CUSTOM` — concave fan shapes (`CUSTOM_RENDER_CONCAVE_LEFT/RIGHT`)
- `SDL_Clay_DestroyTextCache()` — cleanup on shutdown
- `Clay_SDL3RendererData` struct: holds `SDL_Renderer`, `TTF_TextEngine`, font array, font paths

#### Text caching (internal to renderer.c)
- **Font cache** (8 slots): avoids reopening `TTF_Font` at different sizes; evicts slot 0 when full
- **Text cache** (256 slots, direct-mapped): FNV-1a hash on (string, font_id, font_size); avoids recreating `TTF_Text` objects
- Evicting a font flushes all dependent text cache entries

## Key Invariants

- Layout is fully recomputed every frame — no persistent layout state between frames
- `Clay_BeginLayout()` and `Clay_EndLayout()` bracket all `CLAY()` macro calls per frame
- Clay does not own string data — callers must keep strings alive until after `SDL_Clay_RenderClayCommands()` returns
- Scissor START/END pairs must balance; renderer tracks current clip rect globally
- All `Clay_Color` values use 0–255 range
- Font IDs are indices into `rendererData.fonts[]` (FONT_NORMAL=0, FONT_BOLD=1, FONT_ITALIC=2)

## Per-Frame Pipeline

```
Clay_BeginLayout()
  → CLAY() macro calls (in src/display/)
  → Clay_EndLayout()          // computes layout, returns render commands
  → SDL_Clay_RenderClayCommands()  // draws to SDL_Renderer
  → SDL_RenderPresent()
```

## Common Modification Workflows

### Adding a new custom render type
1. Add variant to `CustomRenderType` enum in `renderer.h`
2. Handle new case in the `CLAY_RENDER_COMMAND_TYPE_CUSTOM` branch of `SDL_Clay_RenderClayCommands()`

### Adding a new font slot
1. Add enum value in font enum (see `init.c`)
2. Load font in `SDL_AppInit()` (`src/init.c`), store in `rendererData.fonts[]`
3. Reference by font ID in `CLAY_TEXT()` config

### Changing text cache size
- Adjust `TEXT_CACHE_SIZE` constant in `renderer.c`; must be power of 2 (direct-mapped hash)

## Relationship to src/display/

- `src/display/` files call `CLAY()` macros to declare layout each frame
- `src/clay/` computes layout and renders the result
- Display components never call SDL3 draw functions directly — all rendering goes through Clay render commands
- See `src/display/CLAUDE.md` for the layout and component layer
