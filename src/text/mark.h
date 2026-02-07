// The mark / mark list data structure, and associated functions
// that manipulate it.

#pragma once

#include "location.h"

typedef struct Mark Mark;

// Creates a new mark of the specified type and returns a pointer to it.
// The new mark is positioned at the point.
Mark *mark_create(bool is_fixed);
// Deletes the specified mark.
void mark_delete(Mark *mark);
// Deletes an entire mark list.
void mark_delete_all(Mark *mark);
// Sets the location of the specified mark to the point.
bool mark_to_point(Mark *mark);
// Sets the location of the point to the specified mark.
bool point_to_mark(Mark *mark);
// Returns the location of the specified mark.
Location mark_get(Mark *mark);
// Moves the specified mark to the current location.
bool mark_set(Mark *mark, Location loc);
// Returns true if the point is at the specified mark.
bool is_point_at_mark(Mark *mark);
// Returns true if the point is before the specified mark.
bool is_point_before_mark(Mark *mark);
// Returns true if the point is after the specified mark.
bool is_point_after_mark(Mark *mark);
// Swaps the locations of the point and the specified mark.
bool swap_point_and_mark(Mark *mark);
