// The message area at the bottom of the screen's buffer + functions.
// Note that this message is always completely overwritten, hence the use
// of a regular, fixed-length array instead of a Buffer.

#include <string.h>

#include "../clay/clay.h"
#include "../command/scheme_internal.h"

#define MAX_MESSAGE_LENGTH 2048
static char message_buf[MAX_MESSAGE_LENGTH];

Clay_String message_string = {
    .chars = message_buf,
    .length = 0,
    .isStaticallyAllocated = true
};

static bool locked = false;

void message_clear(void) {
    if (locked) return;
    for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
        message_buf[i] = 0;
    message_string.length = 0;
}

void message_send(const char* message) {
    if (locked) return;
    message_clear();
    strncat(message_buf, message, MAX_MESSAGE_LENGTH - 1);
    message_string.length = strlen(message_buf);
}

// --- Scheme bindings ---

sexp scm_message_send(sexp ctx, sexp self, sexp n, sexp message) {
    G->needs_redraw = true;

    if (sexp_stringp(message)) {
        message_send(sexp_string_data(message));
        return SEXP_VOID;
    } else {
        return sexp_type_exception(ctx, self, SEXP_STRING, message);
    }
}

sexp scm_message_clear(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    return SEXP_VOID;
}

sexp scm_message_lock(sexp ctx, sexp self, sexp n) {
    locked = true;

    return SEXP_VOID;
}

sexp scm_message_unlock(sexp ctx, sexp self, sexp n) {
    locked = false;

    return SEXP_VOID;
}
