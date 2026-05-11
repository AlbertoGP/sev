#include "buf_render.h"
#include "cursor.h"
#include "icon.h"
#include "minibuf.h"
#include "pane.h"
#include "welcome.h"
#include "status.h"
#include "theme.h"
#include "which_key.h"
#include "../state.h"

static void GlobalHeader(AppState *state) {
    float scale     = state->ui.scale_factor;
    float icon_size = 24.0f * scale;
    float natural_h = icon_size + 4.0f * scale;
    float height    = (state->ui.titlebar_height > natural_h)
                      ? state->ui.titlebar_height : natural_h;
    float pad_left  = (state->ui.titlebar_left_inset > 0.0f)
                      ? state->ui.titlebar_left_inset : 5.0f * scale;
    float pad       = 5.0f * scale;

    CLAY(CLAY_ID("Global Header"), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED(height),
            },
            .padding = {
                .left   = (uint16_t)pad_left,
                .right  = (uint16_t)pad,
                .top    = (uint16_t)pad,
                .bottom = (uint16_t)pad,
            },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
    }) {
        if (state->ui.titlebar_height > 0.0f &&
                Clay_Hovered() &&
                state->input.left_double_click_this_frame) {
            if (SDL_GetWindowFlags(state->window) & SDL_WINDOW_MAXIMIZED)
                SDL_RestoreWindow(state->window);
            else
                SDL_MaximizeWindow(state->window);
        }

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

Clay_RenderCommandArray create_app_layout(AppState *state, float delta_time) {
    Clay_Sizing layoutExpand = {
        .width  = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    tab_cb_reset();
    buf_render_reset();
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
    }

    if (state->which_key.active && state->which_key.enabled)
        WhichKey(state);
    MinibufPalette(state);

    return Clay_EndLayout(delta_time);
}
