#pragma once

#include "../state.h"

typedef enum CursorType {
    CURSOR_SOLID,
    CURSOR_HOLLOW,
    CURSOR_THIN,
    CURSOR_UNDER
} CursorType;

typedef struct {
    Clay_Color color;
    Clay_Color bg_color;   // background fill; {0,0,0,0} = transparent (no fill)
    uint16_t   font_id;    // FontID
    uint16_t   font_size;  // final computed size (base_font_size × :size multiplier)
} TextStyle;

Clay_Color ui_resolve_color(AppState *G, sexp role);
// Resolves a role to a full TextStyle. base_font_size should already incorporate
// global and buffer scale factors; :size in a plist role is a multiplier on top.
TextStyle  ui_resolve_text_style(AppState *G, sexp role,
                                 uint16_t default_font_id, uint16_t base_font_size);
Clay_Color ui_get_cursor_color(AppState *G);
CursorType get_cursor_type(void);
