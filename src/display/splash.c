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

static void SuggestionRow(AppState *state, Clay_String label, Clay_String key) {
    float font_size = 14 * state->ui.scale_factor;
    sexp key_role = sexp_intern(state->chibi.ctx, "text.key", -1);
    CLAY_AUTO_ID({
        .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
    }) {
        CLAY_TEXT(label, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = font_size,
            .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
        }));
        XSpacer();
        CLAY_TEXT(key, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = font_size,
            .textColor = ui_resolve_color(state, key_role),
        }));
    }
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
            SuggestionRow(state, CLAY_STRING("Create a new empty buffer"),
                                 CLAY_STRING("SPC b n"));
            SuggestionRow(state, CLAY_STRING("Help commands"),
                                 CLAY_STRING("SPC h"));
            SuggestionRow(state, CLAY_STRING("Invoke command"),
                                 CLAY_STRING("M-x"));
        }
    }
}
