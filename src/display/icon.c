#include <stdio.h>
#include <string.h>

#include <SDL3_image/SDL_image.h>

#include "icon.h"
#include "theme.h"

static IconEntry entries[ICON_MAX];
static int entry_count = 0;
static SDL_Renderer *stashed_renderer = NULL;

static IconEntry *icon_lookup(const char *name) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].name, name) == 0)
            return &entries[i];
    }
    return NULL;
}

static bool generate_texture(IconEntry *e, SDL_Renderer *renderer,
                             AppState *state, int w, int h) {
    if (e->texture) {
        SDL_DestroyTexture(e->texture);
        e->texture = NULL;
    }

    float scale = state->ui.scale_factor;

#ifdef __EMSCRIPTEN__
    char path[256];
    snprintf(path, sizeof(path), "/resources/%s", e->filename);
#else
    char *basePath = (char *)SDL_GetBasePath();
    char path[1024];
    snprintf(path, sizeof(path), "%sresources/%s", basePath, e->filename);
#endif

    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io) {
        fprintf(stderr, "Failed to open icon file: %s\n", path);
        return false;
    }

    int pw = (int)(w * scale);
    int ph = (int)(h * scale);
    SDL_Surface *surf = IMG_LoadSizedSVG_IO(io, pw, ph);
    SDL_CloseIO(io);

    if (!surf) {
        fprintf(stderr, "Failed to rasterize SVG: %s (%dx%d)\n",
                e->filename, pw, ph);
        return false;
    }

    e->texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!e->texture) {
        fprintf(stderr, "Failed to create texture from SVG: %s\n", e->filename);
        return false;
    }

    // Apply color mod from role
    Clay_Color c = ui_resolve_color(state, e->color_role);
    SDL_SetTextureColorMod(e->texture, c.r, c.g, c.b);

    e->cached_scale = scale;
    return true;
}

bool icon_register(const char *name, const char *filename, sexp color_role) {
    if (entry_count >= ICON_MAX) return false;

    IconEntry *e = &entries[entry_count];
    snprintf(e->name, sizeof(e->name), "%s", name);
    snprintf(e->filename, sizeof(e->filename), "%s", filename);
    e->color_role = color_role;
    e->texture = NULL;
    e->cached_scale = 0;

    entry_count++;
    return true;
}

SDL_Texture *icon_get(const char *name, AppState *state, int w, int h) {
    IconEntry *e = icon_lookup(name);
    if (!e) return NULL;

    if (!e->texture || e->cached_scale != state->ui.scale_factor) {
        if (!stashed_renderer) return NULL;
        generate_texture(e, stashed_renderer, state, w, h);
    }

    return e->texture;
}

void icons_stash_renderer(SDL_Renderer *renderer) {
    stashed_renderer = renderer;
}

void icons_update_colors(AppState *state) {
    for (int i = 0; i < entry_count; i++) {
        IconEntry *e = &entries[i];
        if (!e->texture) continue;
        Clay_Color c = ui_resolve_color(state, e->color_role);
        SDL_SetTextureColorMod(e->texture, c.r, c.g, c.b);
    }
}
