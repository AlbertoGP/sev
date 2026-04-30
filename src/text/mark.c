#include "mark.h"
#include "buffer.h"
#include "buffer_type.h"
#include "../command/scheme_internal.h"

Mark *mark_get(int c) {
    Buffer *buf = buffer_get_current();
    if (!buf) return NULL;

    if ((unsigned)(c) >= 'a' && (unsigned)(c) <= 'z') {
        return &buf->named_marks[c - 'a'];
    }

    switch (c) {
    case '<':
        return &buf->select_start;
    case '>':
        return &buf->select_end;
    }

    return NULL;
}

bool mark_set(int c, Location loc) {
    Mark *mark = mark_get(c);
    if (!mark) return false;

    *mark = loc;
    return true;
}

bool mark_set_to_point(int c) {
    return mark_set(c, point_get(buffer_get_current()));
}

bool point_to_mark(Mark *mark) {
    if (!mark) return false;
    return point_set(*mark);
}

bool is_point_at_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos == mark->pos;
}

bool is_point_before_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos < mark->pos;
}

bool is_point_after_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos > mark->pos;
}

bool swap_point_and_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    Location old_point = point_get(buf);
    point_set(*mark);
    update_line(buf);
    save_current_column(buf);
    *mark = old_point;
    return true;
}

// --- Scheme bindings ---

sexp scm_mark_set_to_point(sexp ctx, sexp self, sexp n, sexp mark_char) {
    if (!sexp_charp(mark_char))
        return sexp_user_exception(ctx, self, "expected char", mark_char);
    mark_set_to_point((int)sexp_unbox_character(mark_char));
    return SEXP_VOID;
}

sexp scm_select_mode_set(sexp ctx, sexp self, sexp n, sexp mode_int) {
    if (!sexp_fixnump(mode_int))
        return sexp_user_exception(ctx, self, "expected integer", mode_int);
    Buffer *buf = buffer_get_current();
    if (buf) buf->select_mode = (SelectMode)sexp_unbox_fixnum(mode_int);
    return SEXP_VOID;
}

sexp scm_select_mode_get(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (!buf) return sexp_make_fixnum(0);
    return sexp_make_fixnum((int)buf->select_mode);
}

sexp scm_swap_point_and_mark(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (buf) swap_point_and_mark(&buf->select_start);
    return SEXP_VOID;
}

sexp scm_mark_position(sexp ctx, sexp self, sexp n, sexp mark_char) {
    if (!sexp_charp(mark_char))
        return sexp_user_exception(ctx, self, "expected char", mark_char);
    Mark *mark = mark_get((int)sexp_unbox_character(mark_char));
    if (!mark) return SEXP_FALSE;
    return sexp_make_fixnum((int)mark->pos);
}

sexp scm_point_to_named_mark(sexp ctx, sexp self, sexp n, sexp mark_char) {
    if (!sexp_charp(mark_char))
        return sexp_user_exception(ctx, self, "expected char", mark_char);
    Mark *mark = mark_get((int)sexp_unbox_character(mark_char));
    if (!mark) return SEXP_VOID;
    Buffer *buf = buffer_get_current();
    point_set(*mark);
    update_line(buf);
    save_current_column(buf);
    return SEXP_VOID;
}
