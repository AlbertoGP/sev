#include <SDL3_ttf/SDL_ttf.h>

#include "message.h"
#include "theme.h"
#include "../clay/renderer.h"
#include "../state.h"

extern Clay_String message_string;

void MessageArea(AppState *state) {
    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIT(0, 500.0 * state->ui.scale_factor),
                .height = CLAY_SIZING_FIXED(16 * state->ui.scale_factor)
            },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .clip = CLAY_CLIP_TO_NONE
    }){
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_UI_NORMAL,
            .fontSize = 10.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}
