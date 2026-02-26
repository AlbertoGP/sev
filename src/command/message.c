// The message area at the bottom of the screen's buffer + functions.
// Note that this message is always completely overwritten, hence the use
// of a regular, fixed-length array instead of a Buffer.

#include <string.h>

#include "../clay/clay.h"
#include "../command/scheme_internal.h"
#include "../text/buffer.h"

#define MAX_MESSAGE_LENGTH 2048
static char message_buf[MAX_MESSAGE_LENGTH];

Clay_String message_string = {
    .chars = message_buf,
    .length = 0,
    .isStaticallyAllocated = true
};

static bool locked = false;
static bool is_transient = false;

void message_clear(void) {
    if (locked) return;
    is_transient = false;
    for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
        message_buf[i] = 0;
    message_string.length = 0;
}

void message_echo(const char *msg) {
    if (locked) return;
    message_buf[0] = '\0';
    strncat(message_buf, msg, MAX_MESSAGE_LENGTH - 1);
    message_string.length = strlen(message_buf);
    is_transient = true;
}

void message_echo_clear(void) {
    if (!is_transient) return;
    is_transient = false;
    for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
        message_buf[i] = 0;
    message_string.length = 0;
}

static void messages_buffer_append(const char *msg) {
    Buffer *msgs = buffer_get_by_name("*Messages*");
    if (!msgs) {
        msgs = buffer_create("*Messages*");
        if (!msgs) return;
    }
    Buffer *saved = buffer_get_current();
    buffer_set_current(msgs);
    point_set(buffer_end());
    insert_string(msgs, msg);
    insert_string(msgs, "\n");
    buffer_set_current(saved);
}

void message_send(const char* message) {
    if (locked) return;
    is_transient = false;
    message_clear();
    strncat(message_buf, message, MAX_MESSAGE_LENGTH - 1);
    message_string.length = strlen(message_buf);
    messages_buffer_append(message);
}

// --- Scheme bindings ---

sexp scm_message_echo_scm(sexp ctx, sexp self, sexp n, sexp sarg) {
    G->needs_redraw = true;
    if (!sexp_stringp(sarg))
        return sexp_type_exception(ctx, self, SEXP_STRING, sarg);
    const char *str = sexp_string_data(sarg);
    message_echo(str);
    return SEXP_VOID;
}

sexp scm_message(sexp ctx, sexp self, sexp n, sexp sarg) {
    G->needs_redraw = true;
    if (sarg == SEXP_FALSE) {
        message_clear();
        return sexp_c_string(ctx, "", -1);
    }
    if (!sexp_stringp(sarg))
        return sexp_type_exception(ctx, self, SEXP_STRING, sarg);
    const char *str = sexp_string_data(sarg);
    message_send(str);
    return sexp_c_string(ctx, str, -1);
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
