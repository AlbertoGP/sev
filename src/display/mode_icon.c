#include <stdio.h>
#include <string.h>

#include "icon.h"
#include "mode_icon.h"
#include "../text/buffer.h"

static ModeIconEntry entries[MODE_ICON_MAX];
static int entry_count = 0;

bool mode_icon_register(const char *mode_name, const char *filename,
                        sexp role_bg, sexp role_label, sexp role_cursor,
                        CursorType cursor_type) {
    if (entry_count >= MODE_ICON_MAX) return false;

    ModeIconEntry *e = &entries[entry_count];
    e->mode_name = strdup(mode_name);
    snprintf(e->icon_name, sizeof(e->icon_name), "%s", mode_name);
    e->role_mode_bg = role_bg;
    e->role_label = role_label;
    e->role_cursor = role_cursor;
    e->cursor_type = cursor_type;

    // Register in the general icon registry with label role for color mod
    icon_register(mode_name, filename, role_label);

    entry_count++;
    return true;
}

ModeIconEntry *mode_icon_for_current_buffer(void) {
    Buffer *buf = buffer_get_current();
    for (int i = 0; i < entry_count; i++) {
        if (buffer_has_minor_mode(buf, entries[i].mode_name))
            return &entries[i];
    }
    return NULL;
}
