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

ModeIconEntry *mode_icon_get(const char *mode_name) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].mode_name, mode_name) == 0)
            return &entries[i];
    }
    return NULL;
}

// --- Scheme bindings ---

// (%register-mode-icon! mode-name filename role-bg role-label role-cursor cursor-type-sym)
sexp scm_register_mode_icon(sexp ctx, sexp self, sexp n,
                            sexp sname, sexp sfilename,
                            sexp srole_bg, sexp srole_label,
                            sexp srole_cursor, sexp scursor_type) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "mode-name must be a symbol", sname);
    if (!sexp_stringp(sfilename))
        return sexp_user_exception(ctx, self, "filename must be a string", sfilename);
    if (!sexp_symbolp(srole_bg))
        return sexp_user_exception(ctx, self, "role-bg must be a symbol", srole_bg);
    if (!sexp_symbolp(srole_label))
        return sexp_user_exception(ctx, self, "role-label must be a symbol", srole_label);
    if (!sexp_symbolp(srole_cursor))
        return sexp_user_exception(ctx, self, "role-cursor must be a symbol", srole_cursor);
    if (!sexp_symbolp(scursor_type))
        return sexp_user_exception(ctx, self, "cursor-type must be a symbol", scursor_type);

    const char *mode_name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *filename = sexp_string_data(sfilename);

    sexp role_bg = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_bg)), -1);
    sexp role_label = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_label)), -1);
    sexp role_cursor = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_cursor)), -1);
    sexp_preserve_object(ctx, role_bg);
    sexp_preserve_object(ctx, role_label);
    sexp_preserve_object(ctx, role_cursor);

    const char *ct_str = sexp_string_data(sexp_symbol_to_string(ctx, scursor_type));
    CursorType ct = CURSOR_SOLID;
    if (strcmp(ct_str, "thin") == 0) ct = CURSOR_THIN;
    else if (strcmp(ct_str, "hollow") == 0) ct = CURSOR_HOLLOW;
    else if (strcmp(ct_str, "under") == 0) ct = CURSOR_UNDER;

    if (!mode_icon_register(mode_name, filename, role_bg, role_label, role_cursor, ct))
        return SEXP_FALSE;

    return SEXP_TRUE;
}
