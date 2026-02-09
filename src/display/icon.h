// General-purpose icon registry.
// Maps string names to textures with theme-aware color modulation.
// Textures are lazily rasterized from SVGs at the requested display size.

#pragma once

#include <SDL3/SDL_render.h>
#include <chibi/sexp.h>

#include "../state.h"

#define ICON_MAX 64

typedef struct IconEntry {
    char name[64];          // lookup key, e.g. "tab-close-active"
    SDL_Texture *texture;
    char filename[64];      // e.g. "icon-close.svg"
    sexp color_role;        // interned symbol for SDL_SetTextureColorMod
    float cached_scale;     // scale_factor when texture was last generated
} IconEntry;

// Register an icon entry (texture created lazily on first icon_get).
bool icon_register(const char *name, const char *filename, sexp color_role);

// Returns texture, regenerating from SVG if scale_factor changed.
// w/h are target display size in logical pixels (pre-scale).
SDL_Texture *icon_get(const char *name, AppState *state, int w, int h);

// Stash the renderer pointer for lazy texture creation.
void icons_stash_renderer(SDL_Renderer *renderer);

// Reapply color mods from theme for all entries.
void icons_update_colors(AppState *state);
