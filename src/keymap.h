// Keymap, binding and command types and functions.

#pragma once

#include "state.h"
#include "keyevent.h"

// Type tag for Binding payload union.
typedef enum {
    BINDING_COMMAND,
    BINDING_KEYMAP
} BindingType;

// Type tag for Command payload union.
typedef enum {
    COMMAND_C,
    COMMAND_SCHEME
} CommandType;

// Command to be called by a binding.
typedef struct {
    CommandType type;
    union {
        void (*c_fn)(AppState *, sexp args);
        sexp scheme_proc;
    };
    sexp interactive_spec;
} Command;

// Key binding payload struct. Can either point to a command, or a new keymap.
typedef struct {
    BindingType type;
    union {
        sexp command_sym;   // Scheme symbol
        struct Keymap *keymap; // Pointer to new keymap the next key will be looked up in.
    };
} Binding;

// An entry for a given key in a keymap, to associate it with a given binding.
typedef struct {
    KeyEvent key;
    Binding  binding;
} KeymapEntry;

// A dynamic array of keymap entries.
typedef struct Keymap {
    KeymapEntry *entries;
    size_t count;
    size_t cap;
} Keymap;

// Creates a new, empty keymap and returns a pointer to it.
Keymap *keymap_create(void);

// Initialise global keymap state.
bool init_input(AppState *state);
// Handle a key event via its appropriate binding (if one exists).
void key_dispatch(AppState *state, const KeyEvent *ev);

// Parse a key sequence into a series of KeyEvents.
// Returns the length of the resulting sequence.
int parse_key_sequence(const char *s, KeyEvent *out);
// Create a bunding for a key sequence
void keymap_bind_sequence(Keymap *km, KeyEvent *seq, int n, Binding final);
