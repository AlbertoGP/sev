#include "icon.h"
#include "splash.h"
#include "theme.h"

static void XSpacer(void) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0)
            }
        }
    }){}
}

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
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = 10.0 * state->ui.scale_factor,
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
        CLAY_TEXT(CLAY_STRING("sev"), CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 22.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state,
                      sexp_intern(state->chibi.ctx, "text.splash", -1)),
        }));
        CLAY_AUTO_ID({
            .layout = { .sizing = { .height = CLAY_SIZING_FIXED(15 * state->ui.scale_factor) } }
        }) {}
        CLAY(CLAY_ID("Suggestions"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(300 * state->ui.scale_factor)
                },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 5.0 * state->ui.scale_factor,
            }
        }) {
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
            }) {
            
                CLAY_TEXT(CLAY_STRING("Create a new empty buffer"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
                }));
                XSpacer();
                CLAY_TEXT(CLAY_STRING("SPC b n"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state,
                                    sexp_intern(state->chibi.ctx, "text.key", -1))
                }));
            }
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
            }) {
            
                CLAY_TEXT(CLAY_STRING("Help commands"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
                }));
                XSpacer();
                CLAY_TEXT(CLAY_STRING("SPC h"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state,
                                    sexp_intern(state->chibi.ctx, "text.key", -1))
                }));
            }
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
            }) {
            
                CLAY_TEXT(CLAY_STRING("Invoke command"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
                }));
                XSpacer();
                CLAY_TEXT(CLAY_STRING("M-x"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state,
                                    sexp_intern(state->chibi.ctx, "text.key", -1))
                }));
            }
        }
    }
}
