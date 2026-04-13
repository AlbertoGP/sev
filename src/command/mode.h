// Major and minor mode system

#pragma once

#include <stdbool.h>
#include "../command/keymap.h"

typedef enum {
    MODE_MAJOR,
    MODE_MINOR
} ModeType;

typedef struct Mode {
    const char *name;
    ModeType type;
    Keymap *keymap;
    bool allows_input;
    struct Mode *next;  // for registry linked list
} Mode;

// Node for buffer's minor mode list (separate from registry)
typedef struct ModeListNode {
    Mode *mode;
    struct ModeListNode *next;
} ModeListNode;

typedef struct ModeList {
    ModeListNode *head;   // most-recently-enabled first
    size_t count;
} ModeList;

// Initialize a ModeList (zeroes head and count)
void modelist_init(ModeList *ml);

// Free all mode references in a ModeList (does not free Mode structs)
void modelist_destroy(ModeList *ml);

// Create a new mode with given name, type, and keymap
// Returns NULL on allocation failure
Mode *mode_create(const char *name, ModeType type, Keymap *keymap);

// Register a mode in the global registry
// Returns the mode pointer, or NULL if already registered
Mode *mode_register(Mode *mode);

// Look up a mode by name in the global registry
// type can be MODE_MAJOR or MODE_MINOR to search specific registry
Mode *mode_lookup(const char *name, ModeType type);

// Look up a mode by name in either registry
Mode *mode_lookup_any(const char *name);

// Get the default major mode (text-mode); creates a bootstrap stub if Scheme hasn't run yet
Mode *mode_get_default(void);
