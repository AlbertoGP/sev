// Dynamic mode icon registry.
// Maps mode names to icon textures, theme role symbols, and cursor types.

#pragma once

#include <SDL3/SDL_render.h>
#include <chibi/sexp.h>

#include "theme.h"
#include "../state.h"

#define MODE_ICON_MAX 32

typedef struct ModeIconEntry {
    const char *mode_name;       // e.g. "evil-normal-mode"
    SDL_Texture *texture;        // loaded PNG
    char icon_filename[64];      // e.g. "icon-normal.png"
    sexp role_mode_bg;           // interned e.g. 'mode.normal
    sexp role_label;             // interned e.g. 'label.normal
    sexp role_cursor;            // interned e.g. 'cursor.normal
    CursorType cursor_type;      // CURSOR_SOLID etc.
} ModeIconEntry;

// Register a mode icon entry. Loads texture eagerly if renderer is available.
// mode_name is copied internally. Returns true on success.
bool mode_icon_register(const char *mode_name, const char *filename,
                        sexp role_bg, sexp role_label, sexp role_cursor,
                        CursorType cursor_type);

// Load textures for all registered entries. Call after renderer is ready.
// Stashes renderer pointer for future eager loads.
bool mode_icons_load_textures(SDL_Renderer *renderer);

// Update icon color mods from current theme. Call on theme change.
void mode_icons_update_colors(AppState *state);

// Find the first matching entry for the current buffer's active minor modes.
// Returns NULL if no match.
ModeIconEntry *mode_icon_for_current_buffer(void);
