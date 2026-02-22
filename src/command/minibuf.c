#include <string.h>

#include <chibi/eval.h>

#include "minibuf.h"
#include "mode.h"
#include "scheme_internal.h"
#include "../text/buffer.h"

// Call a named command via the cached call-interactively.
// Used to push/pop evil mode on minibuffer focus transitions.
static void minibuf_invoke_command(sexp ctx, const char *name) {
    if (G->chibi.call_interactively == SEXP_FALSE) return;
    sexp sym = sexp_intern(ctx, name, -1);
    sexp result = sexp_apply(ctx, G->chibi.call_interactively,
                             sexp_list1(ctx, sym));
    if (sexp_exceptionp(result))
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
}

bool minibuf_init(AppState *state) {
    state->minibuf.on_submit   = SEXP_FALSE;
    state->minibuf.on_cancel   = SEXP_FALSE;
    state->minibuf.active      = false;
    state->minibuf.stack_depth = 0;

    Buffer *buf = buffer_create("*minibuffer*");
    if (!buf) return false;
    state->minibuf.buf = buf;

    // Enable minibuffer-mode (provides the isolated keymap + allows_input)
    Mode *mode = mode_lookup("minibuffer-mode", MODE_MINOR);
    if (mode)
        buffer_enable_minor_mode(buf, mode);

    // buffer_create() auto-enables evil-normal-mode on every new buffer;
    // remove it so the minibuf is fully isolated from evil keybindings.
    Mode *evil = mode_lookup("evil-normal-mode", MODE_MINOR);
    if (evil)
        buffer_disable_minor_mode(buf, evil);

    return true;
}

// Restore the top stack frame into the live minibuf state.
// Caller must have already saved/zeroed on_submit/on_cancel before calling.
// Requires minibuf.buf to be the current buffer (safe during submit/cancel).
static void minibuf_pop(sexp ctx) {
    G->minibuf.stack_depth--;
    MinibufFrame *frame = &G->minibuf.stack[G->minibuf.stack_depth];

    strncpy(G->minibuf.prompt, frame->prompt, MINIBUF_PROMPT_MAX);

    // Restore callbacks (still GC-preserved from when they were pushed)
    G->minibuf.on_submit = frame->on_submit;
    G->minibuf.on_cancel = frame->on_cancel;

    // Restore buffer content
    buffer_clear(G->minibuf.buf);
    if (frame->saved_text) {
        insert_string(G->minibuf.buf, frame->saved_text);
        free(frame->saved_text);
        frame->saved_text = NULL;
    }

    // Restore point (minibuf.buf must be the current buffer)
    Location loc = { frame->saved_point };
    point_set(loc);

    G->needs_redraw = true;
}

sexp scm_minibuffer_activate(sexp ctx, sexp self, sexp n,
                              sexp sprompt, sexp on_submit, sexp on_cancel) {
    if (!sexp_stringp(sprompt))
        return sexp_user_exception(ctx, self, "prompt must be a string", sprompt);

    if (G->minibuf.active) {
        // Push current state onto stack
        if (G->minibuf.stack_depth >= MINIBUF_STACK_MAX)
            return sexp_user_exception(ctx, self, "minibuffer stack overflow", SEXP_FALSE);
        MinibufFrame *frame = &G->minibuf.stack[G->minibuf.stack_depth];
        strncpy(frame->prompt, G->minibuf.prompt, MINIBUF_PROMPT_MAX);
        frame->on_submit   = G->minibuf.on_submit;   // already preserved
        frame->on_cancel   = G->minibuf.on_cancel;   // already preserved (or SEXP_FALSE)
        frame->saved_text  = buffer_text(G->minibuf.buf); // may be NULL if empty
        frame->saved_point = point_get(G->minibuf.buf).pos;
        G->minibuf.stack_depth++;
        buffer_clear(G->minibuf.buf);
    } else {
        // Fresh activation: switch the calling buffer to command-mode.
        minibuf_invoke_command(ctx, "evil-command");
        // Release any stale callbacks
        if (G->minibuf.on_submit != SEXP_FALSE)
            sexp_release_object(ctx, G->minibuf.on_submit);
        if (G->minibuf.on_cancel != SEXP_FALSE)
            sexp_release_object(ctx, G->minibuf.on_cancel);
        G->minibuf.prev_buf = buffer_get_current();
        buffer_clear(G->minibuf.buf);
    }

    const char *prompt = sexp_string_data(sprompt);
    strncpy(G->minibuf.prompt, prompt, MINIBUF_PROMPT_MAX - 1);
    G->minibuf.prompt[MINIBUF_PROMPT_MAX - 1] = '\0';

    G->minibuf.on_submit = on_submit;
    G->minibuf.on_cancel = sexp_booleanp(on_cancel) ? SEXP_FALSE : on_cancel;
    if (G->minibuf.on_submit != SEXP_FALSE)
        sexp_preserve_object(ctx, G->minibuf.on_submit);
    if (G->minibuf.on_cancel != SEXP_FALSE)
        sexp_preserve_object(ctx, G->minibuf.on_cancel);

    G->minibuf.active = true;
    G->needs_redraw   = true;

    return SEXP_VOID;
}

sexp scm_minibuffer_submit(sexp ctx, sexp self, sexp n) {
    if (!G->minibuf.active)
        return SEXP_VOID;

    char *raw = buffer_text(G->minibuf.buf);
    const char *text = raw ? raw : "";

    sexp cb_submit = G->minibuf.on_submit;
    sexp cb_cancel = G->minibuf.on_cancel;
    G->minibuf.on_submit = SEXP_FALSE;
    G->minibuf.on_cancel = SEXP_FALSE;

    if (G->minibuf.stack_depth > 0) {
        minibuf_pop(ctx);
        // active remains true; previous frame is restored
    } else {
        G->minibuf.active = false;
        buffer_set_current(G->minibuf.prev_buf);
        G->needs_redraw = true;
        // Full dismiss: return the calling buffer to normal-mode.
        minibuf_invoke_command(ctx, "evil-normal");
    }

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

    sexp cb_cancel = G->minibuf.on_cancel;
    sexp cb_submit = G->minibuf.on_submit;
    G->minibuf.on_cancel = SEXP_FALSE;
    G->minibuf.on_submit = SEXP_FALSE;

    if (G->minibuf.stack_depth > 0) {
        minibuf_pop(ctx);
        // active remains true; previous frame is restored
    } else {
        G->minibuf.active = false;
        buffer_set_current(G->minibuf.prev_buf);
        G->needs_redraw = true;
        // Full dismiss: return the calling buffer to normal-mode.
        minibuf_invoke_command(ctx, "evil-normal");
    }

    if (cb_cancel != SEXP_FALSE) {
        sexp result = sexp_apply(ctx, cb_cancel, SEXP_NULL);
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        sexp_release_object(ctx, cb_cancel);
    }

    if (cb_submit != SEXP_FALSE)
        sexp_release_object(ctx, cb_submit);

    return SEXP_VOID;
}

sexp scm_minibuffer_activep(sexp ctx, sexp self, sexp n) {
    return sexp_make_boolean(G->minibuf.active);
}
