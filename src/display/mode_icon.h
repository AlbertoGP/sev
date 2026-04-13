// Mode icon registry.
// Maps mode names to theme role symbols and cursor types.
// Textures are managed by the general icon registry (icon.h).

#pragma once

#include <chibi/sexp.h>

#include "theme.h"
#include "../text/buffer.h"

#define MODE_ICON_MAX 32

typedef struct ModeIconEntry {
    const char *mode_name;       // e.g. "evil-normal-mode"
    char icon_name[64];          // references icon registry
    sexp role_mode_bg;           // interned e.g. 'mode.normal
    sexp role_label;             // interned e.g. 'label.normal
    sexp role_cursor;            // interned e.g. 'cursor.normal
    CursorType cursor_type;      // CURSOR_SOLID etc.
} ModeIconEntry;

// Register a mode icon entry. Delegates texture to icon registry.
// mode_name is copied internally. Returns true on success.
bool mode_icon_register(const char *mode_name, const char *filename,
                        sexp role_bg, sexp role_label, sexp role_cursor,
                        CursorType cursor_type);

// Find the first matching entry for the current buffer's active minor modes.
// Returns NULL if no match.
ModeIconEntry *mode_icon_for_current_buffer(void);

// Find an entry by exact mode name. Returns NULL if no match.
ModeIconEntry *mode_icon_get(const char *mode_name);
