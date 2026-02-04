#pragma once

#include "../state.h"

typedef enum CursorType {
    CURSOR_SOLID,
    CURSOR_HOLLOW,
    CURSOR_THIN,
    CURSOR_UNDER
} CursorType;

Clay_Color ui_resolve_color(AppState *G, sexp role);
Clay_Color ui_get_cursor_color(AppState *G);
Clay_Color ui_get_mode_color(AppState *G);
CursorType get_cursor_type(void);
