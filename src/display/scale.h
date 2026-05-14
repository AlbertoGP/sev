#pragma once

#include <chibi/eval.h>

#include "../state.h"
#include "../text/buffer.h"

#define UI_PRESCALE 1.25f

static inline void ui_recompute_scale(AppState *state) {
    state->ui.scale_factor = state->ui.dpi_scale * UI_PRESCALE * state->ui.user_scale;
}

static inline void reset_scale(AppState *state) {
    sexp default_symbol = sexp_intern(state->chibi.ctx, "default-scale", -1);
    sexp val = sexp_env_ref(state->chibi.ctx, state->chibi.env, default_symbol,
                            sexp_make_flonum(state->chibi.ctx, 1.0));
    state->ui.user_scale = sexp_flonum_value(val);
    ui_recompute_scale(state);
}

static inline void increase_scale(AppState *state) {
    state->ui.user_scale *= 1.1;
    ui_recompute_scale(state);
}

static inline void decrease_scale(AppState *state) {
    state->ui.user_scale /= 1.1;
    ui_recompute_scale(state);
}

static inline void reset_buffer_scale(AppState *state, Buffer *buf) {
    sexp default_symbol = sexp_intern(state->chibi.ctx, "default-buffer-scale", -1);
    sexp val = sexp_env_ref(state->chibi.ctx, state->chibi.env, default_symbol,
                            sexp_make_flonum(state->chibi.ctx, 1.0));
    buffer_set_scale(buf, sexp_flonum_value(val));
}

static inline void increase_buffer_scale(Buffer *buf) {
    float current = buffer_get_scale(buf);
    buffer_set_scale(buf, current * 1.1);
}

static inline void decrease_buffer_scale(Buffer *buf) {
    float current = buffer_get_scale(buf);
    buffer_set_scale(buf, current / 1.1);
}
