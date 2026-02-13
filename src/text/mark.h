// The mark / mark list data structure, and associated functions
// that manipulate it.

#pragma once

#include <stdbool.h>

#include "location.h"

// Marks are just a wrapper around locations for now.
typedef struct Location Mark;

typedef enum SelectMode {
    SELECT_NONE = 0,
    SELECT_REGULAR,
    SELECT_LINE,
    SELECT_RECTANGLE
} SelectMode;

// Looks up the specified mark and sets it to the specified location.
// Returns false if bad mark specified.
bool mark_set(int c, Location loc);
// Sets the named mark (by char) to the current point location.
bool mark_set_to_point(int c);
// Sets the location of the specified mark to the point.
bool mark_to_point(Mark *mark);
// Sets the location of the point to the specified mark.
bool point_to_mark(Mark *mark);
// Returns the location of the specified mark.
Mark *mark_get(int c);
// Returns true if the point is at the specified mark.
bool is_point_at_mark(Mark *mark);
// Returns true if the point is before the specified mark.
bool is_point_before_mark(Mark *mark);
// Returns true if the point is after the specified mark.
bool is_point_after_mark(Mark *mark);
// Swaps the locations of the point and the specified mark.
bool swap_point_and_mark(Mark *mark);
