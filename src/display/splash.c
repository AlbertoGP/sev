#include "splash.h"
#include "theme.h"

void SplashPane(AppState *state) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.tab_bar)
    }) {

    }
};
