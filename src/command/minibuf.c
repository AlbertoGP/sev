#include <string.h>

#include <chibi/eval.h>

#include "minibuf.h"
#include "mode.h"
#include "scheme_internal.h"
#include "../text/buffer.h"

bool minibuf_init(AppState *state) {
    state->minibuf.on_submit = SEXP_FALSE;
    state->minibuf.on_cancel = SEXP_FALSE;
    state->minibuf.active    = false;

    Buffer *buf = buffer_create("*minibuffer*");
    if (!buf) return false;
    state->minibuf.buf = buf;

    Mode *mode = mode_lookup("minibuffer-mode", MODE_MINOR);
    if (mode)
        buffer_enable_minor_mode(buf, mode);

    return true;
}

sexp scm_minibuffer_activate(sexp ctx, sexp self, sexp n,
                              sexp sprompt, sexp on_submit) {
    if (!sexp_stringp(sprompt))
        return sexp_user_exception(ctx, self, "prompt must be a string", sprompt);

    buffer_clear(G->minibuf.buf);
    G->minibuf.prev_buf = buffer_get_current();

    const char *prompt = sexp_string_data(sprompt);
    strncpy(G->minibuf.prompt, prompt, MINIBUF_PROMPT_MAX - 1);
    G->minibuf.prompt[MINIBUF_PROMPT_MAX - 1] = '\0';

    // Release old callbacks, preserve new ones
    if (G->minibuf.on_submit != SEXP_FALSE)
        sexp_release_object(ctx, G->minibuf.on_submit);
    if (G->minibuf.on_cancel != SEXP_FALSE)
        sexp_release_object(ctx, G->minibuf.on_cancel);

    G->minibuf.on_submit = on_submit;
    G->minibuf.on_cancel = SEXP_FALSE;
    if (G->minibuf.on_submit != SEXP_FALSE)
        sexp_preserve_object(ctx, G->minibuf.on_submit);

    G->minibuf.active    = true;
    G->needs_redraw      = true;

    return SEXP_VOID;
}

sexp scm_minibuffer_submit(sexp ctx, sexp self, sexp n) {
    if (!G->minibuf.active)
        return SEXP_VOID;

    char *raw = buffer_text(G->minibuf.buf);
    const char *text = raw ? raw : "";

    G->minibuf.active = false;
    buffer_set_current(G->minibuf.prev_buf);
    G->needs_redraw = true;

    sexp cb_submit = G->minibuf.on_submit;
    sexp cb_cancel = G->minibuf.on_cancel;
    G->minibuf.on_submit = SEXP_FALSE;
    G->minibuf.on_cancel = SEXP_FALSE;

    if (cb_submit != SEXP_FALSE) {
        sexp_gc_var1(str);
        sexp_gc_preserve1(ctx, str);
        str = sexp_c_string(ctx, text, -1);
        free(raw);
        raw = NULL;
        sexp result = sexp_apply(ctx, cb_submit, sexp_list1(ctx, str));
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        sexp_gc_release1(ctx);
        sexp_release_object(ctx, cb_submit);
    }

    if (cb_cancel != SEXP_FALSE)
        sexp_release_object(ctx, cb_cancel);

    free(raw);
    return SEXP_VOID;
}

sexp scm_minibuffer_cancel(sexp ctx, sexp self, sexp n) {
    if (!G->minibuf.active)
        return SEXP_VOID;

    G->minibuf.active = false;
    buffer_set_current(G->minibuf.prev_buf);
    G->needs_redraw = true;

    if (G->minibuf.on_cancel != SEXP_FALSE) {
        sexp result = sexp_apply(ctx, G->minibuf.on_cancel, SEXP_NULL);
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        sexp_release_object(ctx, G->minibuf.on_cancel);
        G->minibuf.on_cancel = SEXP_FALSE;
    }

    if (G->minibuf.on_submit != SEXP_FALSE) {
        sexp_release_object(ctx, G->minibuf.on_submit);
        G->minibuf.on_submit = SEXP_FALSE;
    }

    return SEXP_VOID;
}

sexp scm_minibuffer_activep(sexp ctx, sexp self, sexp n) {
    return sexp_make_boolean(G->minibuf.active);
}
