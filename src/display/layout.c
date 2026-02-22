#include "message.h"
#include "tab.h"
#include "theme.h"
#include "which_key.h"
#include "../state.h"

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
        if (state->minibuf.active)
            MinibufArea(state);
        else
            MessageArea(state);
    }

    if (state->which_key.active && state->which_key.enabled)
        WhichKey(state);

    return Clay_EndLayout();
}
