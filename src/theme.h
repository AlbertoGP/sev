#pragma once

#include "state.h"

#define FRAMES 15

static Clay_Color generate_color_delta(Clay_Color current, Clay_Color target) {
    return (Clay_Color){
        .r = (target.r - current.r) / FRAMES,
        .g = (target.g - current.g) / FRAMES,
        .b = (target.b - current.b) / FRAMES,
        .a = (target.a - current.a) / FRAMES,
    };
}

static ColorSet generate_set_delta(AppState *state) {
    return (ColorSet){
        .background = generate_color_delta(state->colors.background, state->colors_target.background),
        .foreground = generate_color_delta(state->colors.foreground, state->colors_target.foreground),
        .bar = generate_color_delta(state->colors.bar, state->colors_target.bar),
        .text = generate_color_delta(state->colors.text, state->colors_target.text),
        .textFaded = generate_color_delta(state->colors.textFaded, state->colors_target.textFaded),
    };
}

static inline void set_theme_dark(AppState *state) {
    state->colors_target = (ColorSet){
        .background = { 30, 30, 46, 255 },
        .foreground = { 24, 24, 37, 255 },
        .bar = { 17, 17, 27, 255 },
        .text = { 205, 214, 244, 255 },
        .textFaded = { 88, 91, 112, 255 },
    };
    state->colors_delta = generate_set_delta(state);
    state->color_frames = FRAMES;
    state->theme = THEME_DARK;
    state->needs_redraw = true;
    state->animating = true;
}

static inline void set_theme_light(AppState *state) {
    state->colors_target = (ColorSet){
        .background = { 239, 241, 245, 255 },
        .foreground = { 230, 233, 239, 255 },
        .bar = { 220, 224, 232, 255 },
        .text = { 76, 79, 105, 255 },
        .textFaded = { 172, 176, 190, 255 },
    };
    state->colors_delta = generate_set_delta(state);
    state->color_frames = FRAMES;
    state->theme = THEME_LIGHT;
    state->needs_redraw = true;
    state->animating = true;
}

static inline Clay_Color add_colors(Clay_Color color_1, Clay_Color color_2) {
    return (Clay_Color){
        color_1.r + color_2.r,
        color_1.g + color_2.g,
        color_1.b + color_2.b,
        color_1.a + color_2.a,
    };
}

static inline void add_color_delta(AppState *state) {
    if (!state->color_frames) {
        state->colors = state->colors_target;
        state->animating = false;
        state->needs_redraw = false;
        return;
    }
    // add delta
    state->colors = (ColorSet){
        .background = add_colors(state->colors.background, state->colors_delta.background),
        .foreground = add_colors(state->colors.foreground, state->colors_delta.foreground),
        .bar = add_colors(state->colors.bar, state->colors_delta.bar),
        .text = add_colors(state->colors.text, state->colors_delta.text),
        .textFaded = add_colors(state->colors.textFaded, state->colors_delta.textFaded),
    };
    state->needs_redraw = true;
    state->color_frames--;

}

static inline void toggle_theme(AppState *state) {
    if (state->theme == THEME_DARK)
        set_theme_light(state);
    else 
        set_theme_dark(state);
}
