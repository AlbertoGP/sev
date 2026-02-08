#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "clay.h"

typedef struct {
    SDL_Renderer *renderer;
    TTF_TextEngine *textEngine;
    TTF_Font **fonts;
} Clay_SDL3RendererData;

typedef enum {
    CUSTOM_RENDER_CONCAVE_LEFT = 1,
    CUSTOM_RENDER_CONCAVE_RIGHT,
} CustomRenderType;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);
