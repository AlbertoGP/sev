#pragma once

#include <chibi/sexp.h>
#include "../state.h"

bool minibuf_init(AppState *state);

sexp scm_minibuffer_activate(sexp ctx, sexp self, sexp n, sexp sprompt, sexp on_submit, sexp on_cancel);
sexp scm_minibuffer_submit(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_cancel(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_activep(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_activate_commands(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_activate_themes(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_select_next(sexp ctx, sexp self, sexp n);
sexp scm_minibuffer_select_prev(sexp ctx, sexp self, sexp n);
