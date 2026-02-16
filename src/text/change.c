#include <stdlib.h>
#include <string.h>

#include "change.h"
#include "buffer.h"
#include "buffer_type.h"
#include "../command/scheme_internal.h"

void change_begin(Buffer *buf) {
    // Guard against nested begins — keep existing transaction
    if (buf->current_change) return;

    Change *c = calloc(1, sizeof(Change));
    if (!c) return;

    c->point_before = point_get(buf).pos;
    c->repeat_info = SEXP_FALSE;
    buf->current_change = c;
}

void change_end(Buffer *buf) {
    Change *c = buf->current_change;
    if (!c) return;

    buf->current_change = NULL;

    // Discard empty changes
    if (!c->ops) {
        if (c->repeat_info != SEXP_FALSE) {
            // Need ctx to release — get it from global state
            extern AppState *G;
            if (G && G->chibi.ctx)
                sexp_release_object(G->chibi.ctx, c->repeat_info);
        }
        free(c);
        return;
    }

    c->point_after = point_get(buf).pos;
    c->prev = buf->undo_head;
    buf->undo_head = c;
}

void change_record_insert(Buffer *buf, size_t pos, const char *text, size_t len) {
    if (buf->suppress_recording || !buf->current_change) return;

    EditOp *op = malloc(sizeof(EditOp));
    if (!op) return;

    op->type = EDIT_INSERT;
    op->pos = pos;
    op->text = malloc(len);
    if (!op->text) { free(op); return; }
    memcpy(op->text, text, len);
    op->len = len;
    op->next = NULL;

    if (buf->current_change->ops_tail) {
        buf->current_change->ops_tail->next = op;
    } else {
        buf->current_change->ops = op;
    }
    buf->current_change->ops_tail = op;
}

void change_record_delete(Buffer *buf, size_t pos, const char *text, size_t len) {
    if (buf->suppress_recording || !buf->current_change) return;

    EditOp *op = malloc(sizeof(EditOp));
    if (!op) return;

    op->type = EDIT_DELETE;
    op->pos = pos;
    op->text = malloc(len);
    if (!op->text) { free(op); return; }
    memcpy(op->text, text, len);
    op->len = len;
    op->next = NULL;

    if (buf->current_change->ops_tail) {
        buf->current_change->ops_tail->next = op;
    } else {
        buf->current_change->ops = op;
    }
    buf->current_change->ops_tail = op;
}

// Count ops in a change (to build reversal array)
static int count_ops(Change *c) {
    int n = 0;
    for (EditOp *op = c->ops; op; op = op->next) n++;
    return n;
}

void change_undo(Buffer *buf, sexp ctx) {
    Change *c = buf->undo_head;
    if (!c) return;

    buf->undo_head = c->prev;

    // Build array of ops for reverse iteration
    int n = count_ops(c);
    EditOp **arr = malloc(n * sizeof(EditOp *));
    if (!arr) return;
    EditOp *op = c->ops;
    for (int i = 0; i < n; i++, op = op->next) arr[i] = op;

    buf->suppress_recording++;

    // Replay in reverse: inserts become deletes, deletes become inserts
    for (int i = n - 1; i >= 0; i--) {
        op = arr[i];
        if (op->type == EDIT_INSERT) {
            // Undo an insert: move point to pos + len, delete backward len chars
            point_set((Location){.pos = op->pos + op->len});
            update_line(buf);
            delete_chars(buf, (int)op->len);
        } else {
            // Undo a delete: move point to pos, re-insert text
            point_set((Location){.pos = op->pos});
            update_line(buf);
            for (size_t j = 0; j < op->len; j++) {
                insert_char(buf, op->text[j]);
            }
        }
    }

    buf->suppress_recording--;

    // Restore cursor
    point_set((Location){.pos = c->point_before});
    update_line(buf);
    save_current_column(buf);

    free(arr);

    // Free the change
    if (c->repeat_info != SEXP_FALSE && ctx) {
        sexp_release_object(ctx, c->repeat_info);
    }
    op = c->ops;
    while (op) {
        EditOp *next = op->next;
        free(op->text);
        free(op);
        op = next;
    }
    free(c);
}

void change_free_all(Buffer *buf, sexp ctx) {
    Change *c = buf->undo_head;
    while (c) {
        Change *prev = c->prev;
        if (c->repeat_info != SEXP_FALSE && ctx) {
            sexp_release_object(ctx, c->repeat_info);
        }
        EditOp *op = c->ops;
        while (op) {
            EditOp *next = op->next;
            free(op->text);
            free(op);
            op = next;
        }
        free(c);
        c = prev;
    }
    buf->undo_head = NULL;

    // Also free any in-progress change
    if (buf->current_change) {
        Change *cc = buf->current_change;
        if (cc->repeat_info != SEXP_FALSE && ctx) {
            sexp_release_object(ctx, cc->repeat_info);
        }
        EditOp *op = cc->ops;
        while (op) {
            EditOp *next = op->next;
            free(op->text);
            free(op);
            op = next;
        }
        free(cc);
        buf->current_change = NULL;
    }
}
