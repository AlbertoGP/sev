#include "message.h"
#include "theme.h"

extern Clay_String message_string;

void MessageArea(AppState *state) {
    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(16.0 * state->ui.scale_factor + 8)
            },
            .padding = {
                .left = 10.0 * state->ui.scale_factor, 
                .right = 10.0 * state->ui.scale_factor 
            }
        },
        .clip = CLAY_CLIP_TO_NONE
    }){
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}

