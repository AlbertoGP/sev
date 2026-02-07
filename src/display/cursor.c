#include "cursor.h"
#include "theme.h"
#include "../text/buffer.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

void Cursor(AppState *state, int32_t index,
            float offset, int height,
            FontID font_id, uint16_t font_size ) {
    static char point_char[1];
    static Clay_String point_str = {
        .chars = point_char,
        .length = 1,
        .isStaticallyAllocated = true
    };
    static Clay_String gap_str = {
        .chars = " ",
        .length = 1,
        .isStaticallyAllocated = true
    };
    point_char[0] = char_at_point() ? char_at_point() : ' ';

    Clay_Color cursorColor = ui_get_cursor_color(state);

    Clay_ElementDeclaration cursorData = {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .offset = {
                .x = offset,
                .y = 0
            }
        },
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIT(MAX(1, 2 * state->ui.scale_factor)),
                .height = CLAY_SIZING_FIXED(height)
            }
        },
        .backgroundColor =  cursorColor,
    };

    static Clay_TextElementConfig textConfig = {0};
    textConfig.fontId = font_id;
    textConfig.fontSize = font_size;
    textConfig.textColor = ui_resolve_color(state, state->ui.roles.ui_bg);

    CursorType cursor = get_cursor_type();

    switch (cursor) {
    case CURSOR_HOLLOW:
        cursorData.backgroundColor = (Clay_Color){0};
        cursorData.border = (Clay_BorderElementConfig){
                .width = CLAY_BORDER_ALL(MAX(1, state->ui.scale_factor)),
                .color = ui_get_cursor_color(state)
        };
        textConfig.textColor = (Clay_Color){0};
        break;
    case CURSOR_UNDER:
        cursorData.floating.offset.y = height - 2;
        cursorData.layout.sizing.height = CLAY_SIZING_FIXED(MAX(1, 2 * state->ui.scale_factor));
        break;
    default:
        break;
    };

    CLAY(CLAY_IDI_LOCAL("Cursor", index), cursorData) {
        if (cursor != CURSOR_THIN)
            CLAY_TEXT(cursor == CURSOR_SOLID ? point_str : gap_str, &textConfig);
    }
}
