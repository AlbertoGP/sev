#include <chibi/sexp.h>
#include <stdio.h>
#include <string.h>

#include "major_mode_info.h"

static MajorModeInfoEntry entries[MAJOR_MODE_INFO_MAX];
static int entry_count = 0;

bool major_mode_info_register(const char *mode_name,
                              const char *display_name,
                              const char *icon_name) {
    if (entry_count >= MAJOR_MODE_INFO_MAX) return false;

    MajorModeInfoEntry *e = &entries[entry_count];
    e->mode_name    = strdup(mode_name);
    e->display_name = strdup(display_name);
    if (icon_name && icon_name[0])
        snprintf(e->icon_name, sizeof(e->icon_name), "%s", icon_name);
    else
        e->icon_name[0] = '\0';

    entry_count++;
    return true;
}

MajorModeInfoEntry *major_mode_info_get(const char *mode_name) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].mode_name, mode_name) == 0)
            return &entries[i];
    }
    return NULL;
}

// --- Scheme binding ---

// (%register-major-mode-info! mode-name display-name icon-or-false)
sexp scm_register_major_mode_info(sexp ctx, sexp self, sexp n,
                                  sexp sname, sexp sdisplay, sexp sicon) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "mode-name must be a symbol", sname);
    if (!sexp_stringp(sdisplay))
        return sexp_user_exception(ctx, self, "display-name must be a string", sdisplay);
    if (!sexp_symbolp(sicon) && sicon != SEXP_FALSE)
        return sexp_user_exception(ctx, self, "icon must be a symbol or #f", sicon);

    const char *mode_name    = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *display_name = sexp_string_data(sdisplay);
    const char *icon_name    = (sicon != SEXP_FALSE)
        ? sexp_string_data(sexp_symbol_to_string(ctx, sicon))
        : NULL;

    if (!major_mode_info_register(mode_name, display_name, icon_name))
        return SEXP_FALSE;

    return SEXP_TRUE;
}
