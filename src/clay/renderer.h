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
    CUSTOM_TYPE_SCISSORED_RECT,
    CUSTOM_TYPE_TRIANGLE,
} CustomRenderType;

typedef enum {
    TRIANGLE_HALF_TL = 0,  // right angle at top-left:     TL, TR, BL
    TRIANGLE_HALF_TR,      // right angle at top-right:    TR, BR, TL
    TRIANGLE_HALF_BL,      // right angle at bottom-left:  BL, TL, BR
    TRIANGLE_HALF_BR,      // right angle at bottom-right: BR, BL, TR
} TriangleHalf;

// Per-frame pool for cursor render data; up to CURSOR_POOL_SIZE cursors/frame.
#define CURSOR_POOL_SIZE 16

typedef struct {
    int   type;           // CustomRenderType (must be first field)
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

// Scissored filled rectangle — used for selection highlights.
#define SCISSORED_RECT_POOL_SIZE 512

typedef struct {
    int        type;   // CUSTOM_TYPE_SCISSORED_RECT (must be first field)
    float      clip_x, clip_y, clip_w, clip_h;
    Clay_Color color;
} ScissoredRectData;

// Solid-color right triangle filling one half of the element's bounding box.
#define TRIANGLE_POOL_SIZE 256

typedef struct {
    int        type;   // CUSTOM_TYPE_TRIANGLE (must be first field)
    int        half;   // TriangleHalf
    Clay_Color color;
} TriangleRenderData;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);
void SDL_Clay_DestroyTextCache(void);
// Returns the font instance used for rendering at the given size.
// Use this (not fonts[id] + TTF_SetFontSize) for measurements so that
// measurement and rendering use identical metrics.
TTF_Font *SDL_Clay_GetRenderFont(Clay_SDL3RendererData *rd, uint16_t font_id, float size);
