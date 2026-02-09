#pragma once

#include <chibi/sexp.h>

#include "../state.h"
#include "../text/buffer.h"

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

static inline void reset_buffer_scale(AppState *state, Buffer *buf) {
    sexp default_symbol = sexp_intern(state->chibi.ctx, "default-buffer-scale", -1);
    
    float default_scale = sexp_flonum_value(vartable_get(&state->globals,
                 default_symbol,
                 sexp_make_flonum(state->chibi.ctx, 1.0)));
    buffer_set_scale(buf, default_scale);
}

static inline void increase_buffer_scale(Buffer *buf) {
    float current = buffer_get_scale(buf);
    buffer_set_scale(buf, current * 1.1);
}

static inline void decrease_buffer_scale(Buffer *buf) {
    float current = buffer_get_scale(buf);
    buffer_set_scale(buf, current / 1.1);
}
