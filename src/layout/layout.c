#include "../state.h"
#include "status.h"
#include "tab.h"

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
        .backgroundColor = state->colors.background
    }) {
        TabBar(state);
        TabContent(state);
        StatusBar(state);
        CLAY(CLAY_ID("echo area"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIXED(25)
                }
            }
        }){}
    }

    return Clay_EndLayout();
}
