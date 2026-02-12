#include "mark.h"
#include "buffer.h"
#include "buffer_type.h"

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
    *mark = old_point;
    return true;
}
