#include "cursor.h"
#include "icon.h"
#include "message.h"
#include "pane.h"
#include "welcome.h"
#include "status.h"
#include "theme.h"
#include "which_key.h"
#include "../state.h"

static void GlobalHeader(AppState *state) {
    float icon_size = 24.0f * state->ui.scale_factor;
    CLAY(CLAY_ID("Global Header"), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED(icon_size + 4 * state->ui.scale_factor),
            },
            .padding = CLAY_PADDING_ALL(5 * state->ui.scale_factor),
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
    }) {
        SDL_Texture *app_tex = icon_get("tab-icon", state, (int)icon_size, (int)icon_size);
        CLAY(CLAY_ID("App Icon"), {
            .layout = {
                .sizing = {
                    .width  = CLAY_SIZING_FIXED(icon_size),
                    .height = CLAY_SIZING_FIXED(icon_size)
                },
            },
            .image = app_tex,
        }) {}
    }
}

Clay_RenderCommandArray create_app_layout(AppState *state) {
    Clay_Sizing layoutExpand = {
        .width  = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    tab_cb_reset();
    cursor_cb_reset();
    Clay_BeginLayout();

    CLAY(CLAY_ID("Layout Background"), {
        .layout = {
            .sizing = layoutExpand,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg)
    }) {
        GlobalHeader(state);
        Pane *root = pane_get_root();
        if (!root) {
            WelcomePane(state);
        } else {
            CLAY(CLAY_ID("Root Pane Area"), {
                .layout = { .sizing = layoutExpand },
                .border = { .width = { .top = 1 }, .color = ui_resolve_color(state, state->ui.roles.border_inactive) }
            }) {
                PaneContent(state, root, 1, 0, 0);
            }
        }
        StatusBar(state);
        if (state->minibuf.active)
            MinibufArea(state);
        else
            MessageArea(state);
    }

    if (state->which_key.active && state->which_key.enabled)
        WhichKey(state);

    return Clay_EndLayout();
}
