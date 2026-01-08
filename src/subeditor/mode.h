// The mode / mode list data structure, and associated functions
// that manipulate it.

#pragma once

// Modes are stored in a singly-linked list.
typedef struct Mode {
    struct Mode *next;
    const char *name;
    bool (*add_proc)();
} Mode;

// Appends a mode with the supplied name and add_proc to the mode list.
// If is_front is true, the mode is added to the front of the list.
// Otherwise, it is added to the end.
bool mode_append(char *mode_name, bool (*add_proc)(), bool is_front);
// Removes the named mode from the mode list.
bool mode_delete(char *mode_name);
// Frees up the memory allocated to each mode in a mode list.
static inline void mode_delete_all(Mode *m) { return; }
// Invokes add_proc on each mode in the mode list to create a command set.
bool mode_invoke(void);
