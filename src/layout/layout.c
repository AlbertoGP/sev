#include "../state.h"
#include "tab.h"
#include "theme.h"

extern Clay_String message_string;

static void MessageArea(AppState *state) {
    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(25)
            },
            .padding = { .left = 10, .right = 10 }
        },
        .clip = CLAY_CLIP_TO_NONE
    }){
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}

Clay_RenderCommandArray create_app_layout(AppState *state) {
    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    Clay_BeginLayout();

    CLAY(CLAY_ID("Layout Background"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = layoutExpand,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.ui_bg)
    }) {
        TabBar(state);
        CLAY(CLAY_ID_LOCAL("Tab Content"), {
            .layout = {
                .sizing = layoutExpand,
                .padding = CLAY_PADDING_ALL(2)
            }
        }) {
            TabContent(state);
        }
        MessageArea(state);
    }

    return Clay_EndLayout();
}
