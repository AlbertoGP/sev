#include "state.h"

static inline void set_theme_dark(AppState *state) {
    state->colors = (ColorSet){
        .background = { 30, 30, 46, 255 },
        .foreground = { 24, 24, 37, 255 },
        .bar = { 17, 17, 27, 255 },
        .text = { 205, 214, 244, 255 },
        .textFaded = { 88, 91, 112, 255 },
    };
    state->theme = THEME_DARK;
    state->needs_redraw = true;
}

static inline void set_theme_light(AppState *state) {
    state->colors = (ColorSet){
        .background = { 239, 241, 245, 255 },
        .foreground = { 230, 233, 239, 255 },
        .bar = { 220, 224, 232, 255 },
        .text = { 76, 79, 105, 255 },
        .textFaded = { 172, 176, 190, 255 },
    };
    state->theme = THEME_LIGHT;
    state->needs_redraw = true;
}

static inline void toggle_theme(AppState *state) {
    if (state->theme == THEME_DARK)
        set_theme_light(state);
    else 
        set_theme_dark(state);
}
