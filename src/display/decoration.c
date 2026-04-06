#include "decoration.h"
#include "theme.h"

void XSpacer() {
    CLAY_AUTO_ID({
        .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }}
    }) {}
}

void BarDivider(AppState *state) {
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

