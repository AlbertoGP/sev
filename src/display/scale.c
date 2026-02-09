#include "scale.h"
#include "../command/scheme_internal.h"
#include "../text/buffer.h"
#include "../text/message.h"

// --- Scheme bindings ---

sexp scm_reset_global_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    reset_scale(G);
    message_send("reset-global-scale");
    return SEXP_VOID;
}

sexp scm_increase_global_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    increase_scale(G);
    message_send("increase-global-scale");
    return SEXP_VOID;
}

sexp scm_decrease_global_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    decrease_scale(G);
    message_send("decrease-global-scale");
    return SEXP_VOID;
}

sexp scm_reset_buffer_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    reset_buffer_scale(G, buffer_get_current());
    message_send("reset-buffer-scale");
    return SEXP_VOID;
}

sexp scm_increase_buffer_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    increase_buffer_scale(buffer_get_current());
    message_send("increase-buffer-scale");
    return SEXP_VOID;
}

sexp scm_decrease_buffer_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    decrease_buffer_scale(buffer_get_current());
    message_send("decrease-buffer-scale");
    return SEXP_VOID;
}
