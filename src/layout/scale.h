#pragma once

#include "../state.h"
#include <chibi/sexp.h>

// void update_icons(void);

static inline void reset_scale(AppState *state) {
    sexp default_symbol = sexp_intern(state->chibi.ctx, "default-scale", -1);
    
    float default_scale = sexp_flonum_value(vartable_get(&state->globals,
                 default_symbol,
                 sexp_make_flonum(state->chibi.ctx, 1.0)));
    state->ui.scale_factor = default_scale;
    // update_icons();
}

static inline void increase_scale(AppState *state) {
    state->ui.scale_factor *= 1.1;
}

static inline void decrease_scale(AppState *state) {
    state->ui.scale_factor /= 1.1;
}

#define BUF_TEXT_SCALE
