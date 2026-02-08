#include <stdio.h>
#include <string.h>

#include <SDL3_image/SDL_image.h>

#include "mode_icon.h"
#include "../text/buffer.h"
#include "tab.h"

static ModeIconEntry entries[MODE_ICON_MAX];
static int entry_count = 0;
static SDL_Renderer *stashed_renderer = NULL;

static bool load_texture_for_entry(ModeIconEntry *e, SDL_Renderer *renderer) {
    if (e->texture) return true;

#ifdef __EMSCRIPTEN__
    char path[256];
    snprintf(path, sizeof(path), "/resources/%s", e->icon_filename);
#else
    char *basePath = (char *)SDL_GetBasePath();
    char path[1024];
    snprintf(path, sizeof(path), "%sresources/%s", basePath, e->icon_filename);
#endif

    e->texture = IMG_LoadTexture(renderer, path);
    return e->texture != NULL;
}

bool mode_icon_register(const char *mode_name, const char *filename,
                        sexp role_bg, sexp role_label, sexp role_cursor,
                        CursorType cursor_type) {
    if (entry_count >= MODE_ICON_MAX) return false;

    ModeIconEntry *e = &entries[entry_count];
    e->mode_name = strdup(mode_name);
    snprintf(e->icon_filename, sizeof(e->icon_filename), "%s", filename);
    e->role_mode_bg = role_bg;
    e->role_label = role_label;
    e->role_cursor = role_cursor;
    e->cursor_type = cursor_type;
    e->texture = NULL;

    if (stashed_renderer) {
        load_texture_for_entry(e, stashed_renderer);
    }

    entry_count++;
    return true;
}

bool mode_icons_load_textures(SDL_Renderer *renderer) {
    stashed_renderer = renderer;
    for (int i = 0; i < entry_count; i++) {
        if (!load_texture_for_entry(&entries[i], renderer)) {
            fprintf(stderr, "Failed to load icon: %s\n", entries[i].icon_filename);
            return false;
        }
    }
    return true;
}

void mode_icons_update_colors(AppState *state) {
    for (int i = 0; i < entry_count; i++) {
        ModeIconEntry *e = &entries[i];
        if (!e->texture) continue;
        Clay_Color c = ui_resolve_color(state, e->role_label);
        SDL_SetTextureColorMod(e->texture, c.r, c.g, c.b);
    }
    update_tab_cross_colors(state);
}

ModeIconEntry *mode_icon_for_current_buffer(void) {
    Buffer *buf = buffer_get_current();
    for (int i = 0; i < entry_count; i++) {
        if (buffer_has_minor_mode(buf, entries[i].mode_name))
            return &entries[i];
    }
    return NULL;
}
