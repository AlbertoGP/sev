#include "theme.h"
#include "mode_icon.h"
#include <chibi/sexp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Clay_Color hex_to_rgba(const char* str) {
    int r, g, b, a;
    // Determine if the string has a leading '#' and adjust the pointer
    const char* hex = (str[0] == '#') ? str + 1 : str;
    size_t length = strlen(hex);

    // Default alpha to 255 (opaque) if not provided
    if (length == 6) {
        a = 255;
    } else if (length != 8) {
        // Handle invalid string length (e.g., 3-digit hex is also possible but requires more logic)
        fprintf(stderr, "Invalid hex string length\n");
        return (Clay_Color){255, 0, 255, 255};
    }

    // Use sscanf to parse hex values into integers. 
    // %02x reads exactly two hexadecimal characters.
    if (length == 6) {
        sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    } else if (length == 8) {
        sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a);
    }
    return (Clay_Color){ r, g, b, a };
}

static sexp resolve_role_symbol(AppState *G, sexp role) {
    return vartable_get(&G->ui.role_table, role, SEXP_FALSE);
}

static Clay_Color resolve_palette_color(AppState *G, sexp palette_key) {
    sexp val = vartable_get(&G->ui.palette_table, palette_key, SEXP_FALSE);

    if (!sexp_stringp(val))
        return (Clay_Color){255, 0, 255, 255}; // loud error pink

    return hex_to_rgba(sexp_string_data(val));
}

Clay_Color ui_resolve_color(AppState *G, sexp role) {
    sexp palette_key = resolve_role_symbol(G, role);

    if (palette_key == SEXP_FALSE)
        return (Clay_Color){255, 0, 255, 255}; // missing role

    return resolve_palette_color(G, palette_key);
}

Clay_Color ui_get_cursor_color(AppState *G) {
    ModeIconEntry *icon = mode_icon_for_current_buffer();
    if (icon)
        return ui_resolve_color(G, icon->role_cursor);
    return (Clay_Color){255, 0, 255, 255};
}

CursorType get_cursor_type(void) {
    ModeIconEntry *icon = mode_icon_for_current_buffer();
    if (icon)
        return icon->cursor_type;
    return CURSOR_SOLID;
}
