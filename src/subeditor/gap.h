// Gap buffer type and function declarations

#pragma once

#include <stdbool.h>

typedef struct GapBuf {
    int size;
    int point;
    int gap_end;
    char  *buffer;
} GapBuf;

// Create a new gap buffer.
GapBuf *gb_new(int init_size);
// Free the specified gap buffer.
void gb_free(GapBuf *buf);
// Returns size of text before point.
#define gb_front(buf) ((buf)->point)
// Returns size of text after gap.
#define gb_back(buf) ((buf)->size - (buf)->gap_end)
// Returns total number of used characters.
#define gb_used(buf) (gb_front(buf) + gb_back(buf))
// Move backtext of `buf` to back of `new_buf`.
void gb_move_backtext(GapBuf *buf, char *new_buf, int new_size);
// Shrink buf to new_size - never fails.
// No return value; i.e. no bool as in grow_buffer(),
// because it does not matter if we cannot shrink buffer.
void gb_shrink(GapBuf *buf, int new_size);
// Grow buf to new_size. Operation can fail.
bool gb_grow(GapBuf *buf, int new_size);
// Insert a character at the point and advance point by 1.
// Can fail exceptionally if buffer is full, and the attempt
// to grow it fails.
bool gb_insert(GapBuf *buf, char c);
// Get the position of the point.
int gb_point_get(GapBuf *buf);
// Set the point to an arbitrary position.
// Will not exceed the start or end of buffer content.
void gb_point_set(GapBuf *buf, int target);
// Move point left by a single character.
void gb_point_left(GapBuf *buf);
// Move point right by a single character.
void gb_point_right(GapBuf *buf);
// Delete the chracter before the point and move the point back by 1.
void gb_backspace(GapBuf *buf, int count);
// Delete the chracter after the gap.
void gb_delete(GapBuf *buf, int count);
// Returns the text of the gap buffer as a nul-terminated string.
char *gb_text(GapBuf *buf);
// Returns the specified character, as if there is no gap.
char gb_char_at(GapBuf *buf, int i);
