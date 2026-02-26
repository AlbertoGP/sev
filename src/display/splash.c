#include "icon.h"
#include "splash.h"
#include "theme.h"

void SplashPane(AppState *state) {
    float icon_size = 80.0f * state->ui.scale_factor;
    CLAY(CLAY_ID("Splash"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.tab_bar)
    }) {
        SDL_Texture *icon = icon_get("splash-icon", state, (int)icon_size, (int)icon_size);
        CLAY(CLAY_ID("Splash Icon"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(icon_size),
                    .height = CLAY_SIZING_FIXED(icon_size)
                }
            },
            .image = icon
        }) {}
    }
}
