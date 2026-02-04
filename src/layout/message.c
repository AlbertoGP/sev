#include "message.h"
#include "theme.h"

extern Clay_String message_string;

void MessageArea(AppState *state) {
    static int FONT_SIZE, FIT_MIN, PADDING;
    if (state->scale_change) {
        FONT_SIZE = (int)(14.0 * state->ui.scale_factor);
        FIT_MIN = (int)(25.0 * state->ui.scale_factor);
        PADDING = (int)(10.0 * state->ui.scale_factor);
    }

    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(16.0 * state->ui.scale_factor + 8)
            },
            .padding = { .left = PADDING, .right = PADDING }
        },
        .clip = CLAY_CLIP_TO_NONE
    }){
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = FONT_SIZE,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}

