#include "cursor.h"
#include "../subeditor/buffer.h"

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
                .width = CLAY_SIZING_FIT(2),
                .height = CLAY_SIZING_FIXED(height)
            }
        },
        .backgroundColor =  state->colors.cursor,
    };

    static Clay_TextElementConfig textConfig = {0};
    textConfig.fontId = font_id;
    textConfig.fontSize = font_size;
    textConfig.textColor = state->colors.background;

    switch (state->cursor) {
    case CURSOR_HOLLOW:
        cursorData.backgroundColor = (Clay_Color){0};
        cursorData.border = (Clay_BorderElementConfig){
                .width = CLAY_BORDER_ALL(1),
                .color = state->colors.cursor
        };
        textConfig.textColor = (Clay_Color){0};
        break;
    case CURSOR_UNDER:
        cursorData.floating.offset.y = height - 2;
        cursorData.layout.sizing.height = CLAY_SIZING_FIXED(2);
        break;
    default:
        break;
    };

    CLAY(CLAY_IDI_LOCAL("Cursor", index), cursorData) {
        if (!(state->cursor == CURSOR_THIN))
            CLAY_TEXT(state->cursor == CURSOR_SOLID ? point_str : gap_str, &textConfig);
    }
}
