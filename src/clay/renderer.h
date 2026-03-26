#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "clay.h"

typedef struct {
    SDL_Renderer *renderer;
    TTF_TextEngine *textEngine;
    TTF_Font **fonts;
    const char **font_paths;
} Clay_SDL3RendererData;

// ── Custom render types ───────────────────────────────────────────────────────
typedef enum {
    CUSTOM_TYPE_CURSOR,
} CustomRenderType;

// Per-frame pool for cursor render data; up to CURSOR_POOL_SIZE cursors/frame.
#define CURSOR_POOL_SIZE 16

typedef struct {
    // Clip rect (screen coords, absolute)
    float clip_x, clip_y, clip_w, clip_h;
    // Cursor geometry (element bounding box gives x,y)
    float width, height;
    // Appearance
    int   cursor_type;    // CursorType enum value
    Clay_Color bg_color;
    Clay_Color border_color;
    float border_width;
    // Character overlay (CURSOR_SOLID only)
    Clay_Color text_color;
    uint16_t font_id, font_size;
    char  char_buf[5];    // UTF-8 + null
    int   char_len;
} CursorRenderData;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);
void SDL_Clay_DestroyTextCache(void);
// Returns the font instance used for rendering at the given size.
// Use this (not fonts[id] + TTF_SetFontSize) for measurements so that
// measurement and rendering use identical metrics.
TTF_Font *SDL_Clay_GetRenderFont(Clay_SDL3RendererData *rd, uint16_t font_id, float size);
