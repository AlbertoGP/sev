#include <stdio.h>
#include <string.h>

#include <SDL3_image/SDL_image.h>

#include "icon.h"
#include "theme.h"

static IconEntry entries[ICON_MAX];
static int entry_count = 0;
static SDL_Renderer *stashed_renderer = NULL;

static bool load_texture_for_entry(IconEntry *e, SDL_Renderer *renderer) {
    if (e->texture) return true;

#ifdef __EMSCRIPTEN__
    char path[256];
    snprintf(path, sizeof(path), "/resources/%s", e->filename);
#else
    char *basePath = (char *)SDL_GetBasePath();
    char path[1024];
    snprintf(path, sizeof(path), "%sresources/%s", basePath, e->filename);
#endif

    e->texture = IMG_LoadTexture(renderer, path);
    return e->texture != NULL;
}

bool icon_register(const char *name, const char *filename, sexp color_role) {
    if (entry_count >= ICON_MAX) return false;

    IconEntry *e = &entries[entry_count];
    snprintf(e->name, sizeof(e->name), "%s", name);
    snprintf(e->filename, sizeof(e->filename), "%s", filename);
    e->color_role = color_role;
    e->texture = NULL;

    if (stashed_renderer) {
        load_texture_for_entry(e, stashed_renderer);
    }

    entry_count++;
    return true;
}

IconEntry *icon_lookup(const char *name) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].name, name) == 0)
            return &entries[i];
    }
    return NULL;
}

bool icons_load_textures(SDL_Renderer *renderer) {
    stashed_renderer = renderer;
    for (int i = 0; i < entry_count; i++) {
        if (!load_texture_for_entry(&entries[i], renderer)) {
            fprintf(stderr, "Failed to load icon: %s (%s)\n",
                    entries[i].name, entries[i].filename);
            return false;
        }
    }
    return true;
}

void icons_update_colors(AppState *state) {
    for (int i = 0; i < entry_count; i++) {
        IconEntry *e = &entries[i];
        if (!e->texture) continue;
        Clay_Color c = ui_resolve_color(state, e->color_role);
        SDL_SetTextureColorMod(e->texture, c.r, c.g, c.b);
    }
}
