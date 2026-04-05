#include <SDL3_ttf/SDL_ttf.h>

#include "message.h"
#include "tab.h"
#include "theme.h"
#include "tooltip.h"
#include "../clay/renderer.h"
#include "../state.h"

extern Clay_String message_string;

static void HandleMessageAreaClick(Clay_ElementId elementId,
                                   Clay_PointerData pointerInfo,
                                   void *userData) {
    if (pointerInfo.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    tab_new_with_buffer("*Messages*", false);
}

void MessageArea(AppState *state) {
    bool hovered = false;
    Clay_Color c = ui_resolve_color(state, state->ui.roles.line_bg);
    Clay_Color hovered_bg = { .r = c.r, .g = c.g, .b = c.b, .a = 128 };
    uint16_t padding = 10.0 * state->ui.scale_factor;
    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIT(0, 500.0 * state->ui.scale_factor),
                .height = CLAY_SIZING_FIXED(16 * state->ui.scale_factor)
            },
            .padding = { .left = padding },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .childGap = padding
        },
        .clip = CLAY_CLIP_TO_NONE,
        .backgroundColor = Clay_Hovered() ? hovered_bg : (Clay_Color){0}
    }){
        Clay_OnHover(HandleMessageAreaClick, NULL);
        hovered = Clay_Hovered();
        if (hovered) state->input.desired_cursor = SDL_SYSTEM_CURSOR_POINTER;
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_UI_NORMAL,
            .fontSize = 10.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .height = 12.0 * state->ui.scale_factor,
                    .width = CLAY_SIZING_FIXED(2),
                }
            },
            .backgroundColor = ui_resolve_color(state, state->ui.roles.border_inactive)
        }) {}
    }
    TextTooltip(state, hovered, 999, "View Message Log");
}
