#include <string.h>

#include <chibi/eval.h>

#include "minibuf.h"
#include "mode.h"
#include "scheme_internal.h"
#include "../text/buffer.h"

// ---- Provider helpers -------------------------------------------------------

// Populate items[] from a Scheme list of symbols, filtered by input substring.
static void populate_items(AppState *state, const char *input, sexp list) {
    sexp ctx = state->chibi.ctx;
    state->minibuf.item_count = 0;
    bool filter = input && input[0] != '\0';

    for (sexp p = list; sexp_pairp(p); p = sexp_cdr(p)) {
        if (state->minibuf.item_count >= MINIBUF_ITEMS_MAX) break;
        sexp sym = sexp_car(p);
        if (!sexp_symbolp(sym)) continue;
        sexp str = sexp_symbol_to_string(ctx, sym);
        const char *name = sexp_string_data(str);
        if (filter && strstr(name, input) == NULL) continue;
        MinibufItem *item = &state->minibuf.items[state->minibuf.item_count];
        strncpy(item->label,    name, MINIBUF_LABEL_MAX - 1);
        item->label[MINIBUF_LABEL_MAX - 1] = '\0';
        strncpy(item->sym_name, name, MINIBUF_LABEL_MAX - 1);
        item->sym_name[MINIBUF_LABEL_MAX - 1] = '\0';
        state->minibuf.item_count++;
    }

    if (state->minibuf.item_count == 0)
        state->minibuf.selected = 0;
    else if (state->minibuf.selected >= state->minibuf.item_count)
        state->minibuf.selected = state->minibuf.item_count - 1;
}

// Call a 0-arg Scheme function by name, populate items from the returned list.
static void scheme_list_provider(AppState *state, const char *input,
                                  const char *fn_name) {
    sexp ctx = state->chibi.ctx;
    sexp fn = sexp_env_ref(ctx, state->chibi.env,
                           sexp_intern(ctx, fn_name, -1), SEXP_FALSE);
    if (fn == SEXP_FALSE) return;
    sexp result = sexp_apply(ctx, fn, SEXP_NULL);
    if (sexp_exceptionp(result)) return;
    populate_items(state, input, result);
}

// ---- Command provider -------------------------------------------------------

static void commands_provider(AppState *state, const char *input) {
    scheme_list_provider(state, input, "list-commands");
}

// ---- Theme provider ---------------------------------------------------------

static void themes_provider(AppState *state, const char *input) {
    scheme_list_provider(state, input, "list-themes");
}

static void theme_submit(sexp ctx, const char *sym_name) {
    sexp activate = sexp_env_ref(ctx, G->chibi.env,
                                  sexp_intern(ctx, "activate-theme", -1),
                                  SEXP_FALSE);
    if (activate == SEXP_FALSE) return;
    sexp sym = sexp_intern(ctx, sym_name, -1);
    sexp result = sexp_apply(ctx, activate, sexp_list1(ctx, sym));
    if (sexp_exceptionp(result))
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
}

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
        G->minibuf.prev_buf   = buffer_get_current();
        G->minibuf.prev_focus = G->input.current_focus;
        G->input.current_focus = FOCUS_MINIBUFFER;
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

    // Provider branch: ignore raw text, execute selected item.
    if (G->minibuf.provider && G->minibuf.item_count > 0) {
        int sel = G->minibuf.selected;
        if (sel < 0) sel = 0;
        if (sel >= G->minibuf.item_count) sel = G->minibuf.item_count - 1;

        char sym_name[MINIBUF_LABEL_MAX];
        strncpy(sym_name, G->minibuf.items[sel].sym_name, MINIBUF_LABEL_MAX);
        sym_name[MINIBUF_LABEL_MAX - 1] = '\0';

        void (*action)(sexp, const char*) = G->minibuf.submit_action;
        G->minibuf.provider       = NULL;
        G->minibuf.submit_action  = NULL;
        G->minibuf.item_count     = 0;
        G->minibuf.selected       = 0;

        // Release any stale callbacks before dismissing.
        sexp cb_submit = G->minibuf.on_submit;
        sexp cb_cancel = G->minibuf.on_cancel;
        G->minibuf.on_submit = SEXP_FALSE;
        G->minibuf.on_cancel = SEXP_FALSE;

        if (G->minibuf.stack_depth > 0) {
            minibuf_pop(ctx);
        } else {
            G->minibuf.active = false;
            G->input.current_focus = G->minibuf.prev_focus;
            buffer_set_current(G->minibuf.prev_buf);
            G->needs_redraw = true;
            minibuf_invoke_command(ctx, "evil-normal");
        }

        if (cb_submit != SEXP_FALSE) sexp_release_object(ctx, cb_submit);
        if (cb_cancel != SEXP_FALSE) sexp_release_object(ctx, cb_cancel);

        if (action) {
            action(ctx, sym_name);
        } else if (G->chibi.call_interactively != SEXP_FALSE) {
            sexp sym = sexp_intern(ctx, sym_name, -1);
            sexp result = sexp_apply(ctx, G->chibi.call_interactively,
                                     sexp_list1(ctx, sym));
            if (sexp_exceptionp(result))
                sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        }
        return SEXP_VOID;
    }

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
        G->input.current_focus = G->minibuf.prev_focus;
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
        G->minibuf.active         = false;
        G->minibuf.provider       = NULL;
        G->minibuf.submit_action  = NULL;
        G->minibuf.item_count     = 0;
        G->minibuf.selected       = 0;
        G->input.current_focus = G->minibuf.prev_focus;
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

sexp scm_minibuffer_activate_commands(sexp ctx, sexp self, sexp n) {
    G->minibuf.provider       = commands_provider;
    G->minibuf.submit_action  = NULL; // use call-interactively
    G->minibuf.item_count     = 0;
    G->minibuf.selected       = 0;
    sexp prompt = sexp_c_string(ctx, "Execute command...", -1);
    return scm_minibuffer_activate(ctx, self, n, prompt, SEXP_FALSE, SEXP_FALSE);
}

sexp scm_minibuffer_activate_themes(sexp ctx, sexp self, sexp n) {
    G->minibuf.provider       = themes_provider;
    G->minibuf.submit_action  = theme_submit;
    G->minibuf.item_count     = 0;
    G->minibuf.selected       = 0;
    sexp prompt = sexp_c_string(ctx, "Select a theme...", -1);
    return scm_minibuffer_activate(ctx, self, n, prompt, SEXP_FALSE, SEXP_FALSE);
}

sexp scm_minibuffer_select_next(sexp ctx, sexp self, sexp n) {
    if (!G->minibuf.active || G->minibuf.item_count == 0) return SEXP_VOID;
    if (G->minibuf.selected < G->minibuf.item_count - 1)
        G->minibuf.selected++;
    G->needs_redraw = true;
    return SEXP_VOID;
}

sexp scm_minibuffer_select_prev(sexp ctx, sexp self, sexp n) {
    if (!G->minibuf.active || G->minibuf.item_count == 0) return SEXP_VOID;
    if (G->minibuf.selected > 0)
        G->minibuf.selected--;
    G->needs_redraw = true;
    return SEXP_VOID;
}
