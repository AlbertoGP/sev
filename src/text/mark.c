#include <stdlib.h>

#include "mark.h"
#include "buffer.h"

// Marks are stored in a singly-linked list.
typedef struct Mark {
    struct Mark *next;
    Location location;
    bool is_fixed;
} Mark;

Mark *mark_create(bool is_fixed) {
    Buffer *buf = buffer_get_current();
    if (!buf) return NULL;
    Mark *m = malloc(sizeof(Mark));
    if (!m) return NULL;
    m->location = point_get(buf);
    m->is_fixed = is_fixed;
    m->next = buffer_get_marks(buf);
    buffer_set_marks(buf, m);
    return m;
}

void mark_delete(Mark *mark) {
    if (!mark) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    Mark *head = buffer_get_marks(buf);
    if (head == mark) {
        buffer_set_marks(buf, mark->next);
    } else {
        for (Mark *m = head; m; m = m->next) {
            if (m->next == mark) {
                m->next = mark->next;
                break;
            }
        }
    }
    free(mark);
}

void mark_delete_all(Mark *mark) {
    while (mark) {
        Mark *next = mark->next;
        free(mark);
        mark = next;
    }
}

bool mark_to_point(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    mark->location = point_get(buf);
    return true;
}

bool point_to_mark(Mark *mark) {
    if (!mark) return false;
    return point_set(mark->location);
}

Location mark_get(Mark *mark) {
    if (!mark) return (Location){0};
    return mark->location;
}

bool mark_set(Mark *mark, Location loc) {
    if (!mark) return false;
    mark->location = loc;
    return true;
}

bool is_point_at_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos == mark->location.pos;
}

bool is_point_before_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos < mark->location.pos;
}

bool is_point_after_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    return point_get(buf).pos > mark->location.pos;
}

bool swap_point_and_mark(Mark *mark) {
    if (!mark) return false;
    Buffer *buf = buffer_get_current();
    if (!buf) return false;
    Location old_point = point_get(buf);
    point_set(mark->location);
    mark->location = old_point;
    return true;
}
