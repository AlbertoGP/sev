#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chibi/sexp.h>

#include "theme.h"
#include "mode_icon.h"
#include "../command/scheme_internal.h"

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

static Clay_Color resolve_palette_color(AppState *G, sexp palette_key) {
    sexp val = vartable_get(&G->ui.palette_table, palette_key, SEXP_FALSE);

    if (!sexp_stringp(val))
        return (Clay_Color){255, 0, 255, 255}; // loud error pink

    return hex_to_rgba(sexp_string_data(val));
}

// Walk a plist '(:key val :key val ...) and return the value for key, or SEXP_FALSE.
static sexp plist_get(sexp plist, sexp key) {
    for (sexp p = plist;
         sexp_pairp(p) && sexp_pairp(sexp_cdr(p));
         p = sexp_cdr(sexp_cdr(p))) {
        if (sexp_car(p) == key) return sexp_car(sexp_cdr(p));
    }
    return SEXP_FALSE;
}

static uint16_t symbol_to_font_id(AppState *G, sexp sym, uint16_t default_id) {
    CachedSymbols *s = &G->ui.symbols;
    if (sym == s->buf_normal) return FONT_BUF_NORMAL;
    if (sym == s->buf_bold)   return FONT_BUF_BOLD;
    if (sym == s->buf_italic) return FONT_BUF_ITALIC;
    if (sym == s->ui_normal)  return FONT_UI_NORMAL;
    if (sym == s->ui_bold)    return FONT_UI_BOLD;
    if (sym == s->ui_italic)  return FONT_UI_ITALIC;
    return default_id;
}

Clay_Color ui_resolve_color(AppState *G, sexp role) {
    sexp role_val = vartable_get(&G->ui.role_table, role, SEXP_FALSE);

    if (role_val == SEXP_FALSE)
        return (Clay_Color){255, 0, 255, 255}; // missing role

    if (sexp_symbolp(role_val))
        return resolve_palette_color(G, role_val);

    if (sexp_pairp(role_val)) {
        sexp color_key = plist_get(role_val, G->ui.symbols.kw_color);
        if (sexp_symbolp(color_key))
            return resolve_palette_color(G, color_key);
    }
    return (Clay_Color){255, 0, 255, 255};
}

TextStyle ui_resolve_text_style(AppState *G, sexp role,
                                uint16_t default_font_id, uint16_t base_font_size) {
    TextStyle style = {
        .color     = (Clay_Color){255, 0, 255, 255},
        .bg_color  = (Clay_Color){0, 0, 0, 0},
        .font_id   = default_font_id,
        .font_size = base_font_size,
    };
    sexp role_val = vartable_get(&G->ui.role_table, role, SEXP_FALSE);
    if (role_val == SEXP_FALSE) return style;

    if (sexp_symbolp(role_val)) {
        style.color = resolve_palette_color(G, role_val);
        return style;
    }

    if (!sexp_pairp(role_val)) return style;

    sexp color_key = plist_get(role_val, G->ui.symbols.kw_color);
    if (sexp_symbolp(color_key))
        style.color = resolve_palette_color(G, color_key);

    sexp font_val = plist_get(role_val, G->ui.symbols.kw_font);
    if (sexp_symbolp(font_val))
        style.font_id = symbol_to_font_id(G, font_val, default_font_id);

    sexp size_val = plist_get(role_val, G->ui.symbols.kw_size);
    if (sexp_flonump(size_val))
        style.font_size = (uint16_t)(sexp_flonum_value(size_val) * base_font_size);
    else if (sexp_fixnump(size_val))
        style.font_size = (uint16_t)(sexp_unbox_fixnum(size_val) * base_font_size);

    sexp bg_val = plist_get(role_val, G->ui.symbols.kw_bg);
    if (sexp_symbolp(bg_val))
        style.bg_color = resolve_palette_color(G, bg_val);

    return style;
}

Clay_Color ui_get_cursor_color(AppState *G) {
    ModeIconEntry *icon = mode_icon_for_current_buffer();
    if (icon)
        return ui_resolve_color(G, icon->role_cursor);
    return (Clay_Color){255, 0, 255, 255};
}

static int cursor_type_override = -1; // -1 = no override

CursorType get_cursor_type(void) {
    if (cursor_type_override >= 0)
        return (CursorType)cursor_type_override;
    ModeIconEntry *icon = mode_icon_for_current_buffer();
    if (icon)
        return icon->cursor_type;
    return CURSOR_SOLID;
}

sexp scm_set_cursor_override(sexp ctx, sexp self, sexp n, sexp stype) {
    if (stype == SEXP_FALSE) {
        cursor_type_override = -1;
    } else {
        const char *s = sexp_string_data(sexp_symbol_to_string(ctx, stype));
        CursorType ct = CURSOR_SOLID;
        if (strcmp(s, "thin") == 0)   ct = CURSOR_THIN;
        else if (strcmp(s, "hollow") == 0) ct = CURSOR_HOLLOW;
        else if (strcmp(s, "under") == 0)  ct = CURSOR_UNDER;
        cursor_type_override = (int)ct;
    }
    return SEXP_VOID;
}
