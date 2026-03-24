#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "clay.h"

typedef struct {
    SDL_Renderer *renderer;
    TTF_TextEngine *textEngine;
    TTF_Font **fonts;
    const char **font_paths;
} Clay_SDL3RendererData;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);
void SDL_Clay_DestroyTextCache(void);
// Returns the font instance used for rendering at the given size.
// Use this (not fonts[id] + TTF_SetFontSize) for measurements so that
// measurement and rendering use identical metrics.
TTF_Font *SDL_Clay_GetRenderFont(Clay_SDL3RendererData *rd, uint16_t font_id, float size);
