// General-purpose icon registry.
// Maps string names to textures with theme-aware color modulation.

#pragma once

#include <SDL3/SDL_render.h>
#include <chibi/sexp.h>

#include "../state.h"

#define ICON_MAX 64

typedef struct IconEntry {
    char name[64];          // lookup key, e.g. "tab-close-active"
    SDL_Texture *texture;
    char filename[64];      // e.g. "icon-close.png"
    sexp color_role;        // interned symbol for SDL_SetTextureColorMod
} IconEntry;

// Register an icon entry. Loads texture eagerly if renderer is stashed.
bool icon_register(const char *name, const char *filename, sexp color_role);

// Look up an icon by name. Returns NULL if not found.
IconEntry *icon_lookup(const char *name);

// Load textures for all registered entries. Stashes renderer.
bool icons_load_textures(SDL_Renderer *renderer);

// Reapply color mods from theme for all entries.
void icons_update_colors(AppState *state);
