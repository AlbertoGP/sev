#pragma once

#include <stddef.h>
#include <chibi/sexp.h>

typedef struct Buffer Buffer;

typedef struct EditOp {
    enum { EDIT_INSERT, EDIT_DELETE } type;
    size_t pos;
    char *text;
    size_t len;
    struct EditOp *next;
} EditOp;

typedef struct Change {
    EditOp *ops;
    EditOp *ops_tail;
    size_t point_before;
    size_t point_after;
    sexp repeat_info;     // GC-protected Scheme record
    struct Change *prev;  // undo stack link
} Change;

void change_begin(Buffer *buf);
void change_end(Buffer *buf);
void change_record_insert(Buffer *buf, size_t pos, const char *text, size_t len);
void change_record_delete(Buffer *buf, size_t pos, const char *text, size_t len);
void change_undo(Buffer *buf, sexp ctx);
void change_redo(Buffer *buf, sexp ctx);
void change_clear_redo(Buffer *buf, sexp ctx);
void change_free_all(Buffer *buf, sexp ctx);
