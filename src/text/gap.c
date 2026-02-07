// Gap buffer implementation

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "gap.h"

#define MIN_BUF_SIZE 1024
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

GapBuf *gb_new(size_t init_size) {
    GapBuf *buf = (GapBuf*) malloc(sizeof *buf);
    if (!buf) return NULL;

    init_size   = MAX(init_size, MIN_BUF_SIZE);
    buf->buffer = (char*) malloc(init_size);
    if (!buf->buffer) {
        free(buf);
        return NULL;
    }
    buf->size           = init_size;
    buf->point = 0;
    buf->gap_end        = init_size;
    return buf;
}

void gb_free(GapBuf *buf) {
    free(buf->buffer);
    free(buf);
}

// Move backtext of `buf` to back of `new_buf`.
static void gb_move_backtext(GapBuf *buf, char *new_buf, size_t new_size) {
  memmove(new_buf + new_size - gb_back(buf), // back of new_buf
          buf->buffer + buf->gap_end,        // back of buf
          gb_back(buf));                     // size of backtext
}

// Shrink buf to new_size - never fails.
// No return value; i.e. no bool as in gb_grow(),
// because it does not matter if we cannot shrink buffer.
static void gb_shrink(GapBuf *buf, size_t new_size) {
    // we have to move first the backtext forward and then shrink
    new_size = MAX(new_size, MIN_BUF_SIZE);
    if (new_size < gb_used(buf)) {
        return; // we do not resize if we'd lose data!
    }

    gb_move_backtext(buf, buf->buffer, new_size); // move backtext forward
    // set gap_end before updating the size or gb_back(buf) will be wrong
    buf->gap_end = new_size - gb_back(buf);
    buf->size    = new_size;

    // Shrinks only the gap. The sizes remain the same.
    char *new_buf = realloc(buf->buffer, new_size); // allocate a smaller buffer
    if (new_buf) {
        buf->buffer = new_buf;
    }
}

// Grow buf to new_size. Operation can fail.
static bool gb_grow(GapBuf *buf, size_t new_size) {
    // we have to grow first and then move the backtext
    new_size = MAX(new_size, MIN_BUF_SIZE);
    if (buf->size >= new_size) {
        return false;
    }
    char *new_buf = realloc(buf->buffer, new_size); // allocate a larger buffer
    if (!new_buf) return false;

    gb_move_backtext(buf, new_buf, new_size);  // move backtext to end of new buffer
    buf->buffer  = new_buf;
    // set gap_end before updating the size or gb_back(buf) will be wrong
    buf->gap_end = new_size - gb_back(buf);
    buf->size    = new_size;

    return true;
}

#define capped_dbl_size(s) ((s) < INT_MAX / 2) ? (2 * (s)) : INT_MAX

bool gb_insert(GapBuf *buf, char c) {
    // grow buffer if there is no more space
    if (buf->point == buf->gap_end) {
        size_t new_size = capped_dbl_size(buf->size);
        if (!gb_grow(buf, new_size)) {
            return false;
        }
    }

    buf->buffer[buf->point++] = c;
    return true;
}

int gb_point_get(GapBuf *buf) {
    return buf->point;
}

void gb_point_set(GapBuf *buf, size_t target) {
    if (target > gb_used(buf))
        target = gb_used(buf);

    if (target == buf->point)
        return;

    if (target > buf->point) {
        size_t delta = target - buf->point;

        memmove(
            buf->buffer + buf->point,
            buf->buffer + buf->gap_end,
            delta
        );

        buf->point   += delta;
        buf->gap_end += delta;
    } else {
        size_t delta = buf->point - target;

        memmove(
            buf->buffer + buf->gap_end - delta,
            buf->buffer + target,
            delta
        );

        buf->point   -= delta;
        buf->gap_end -= delta;
    }
}

void gb_point_left(GapBuf *buf) {
    if (buf->point > 0) {
        buf->buffer[--buf->gap_end] = buf->buffer[--buf->point];
    }
}

void gb_point_right(GapBuf *buf) {
    if (buf->gap_end < buf->size) {
        buf->buffer[buf->point++] = buf->buffer[buf->gap_end++];
    }
}

void gb_backspace(GapBuf *buf, size_t count) {
    if (count > buf->point)
        count = buf->point;
    buf->point -= count;
    if (gb_used(buf) < buf->size / 4) {
        gb_shrink(buf, buf->size / 2);
    }
}

void gb_delete(GapBuf *buf, size_t count) {
    if (count > gb_back(buf))
        count = gb_back(buf);
    buf->gap_end += count;
    if (gb_used(buf) < buf->size / 4) {
        gb_shrink(buf, buf->size / 2);
    }
}

char *gb_text(GapBuf *buf) {
    // It is insanely unlikely to happen, but if it does then we do not have space for the zero terminal.
    if (gb_used(buf) == INT_MAX) {
        return NULL;
    }

    char *text = malloc(gb_used(buf) + 1);
    if (!text) return NULL;

    strncpy(text, buf->buffer, buf->point);
    strncpy(text + buf->point, buf->buffer + buf->gap_end, gb_back(buf));
    text[gb_used(buf)] = '\0';
    return text;
}

char gb_char_at(GapBuf *buf, size_t index) {
    if (index > gb_used(buf)) return '\0';
    if (index < gb_front(buf)) {
        return buf->buffer[index];
    } else {
        return buf->buffer[index + buf->gap_end - buf->point];
    }
}
