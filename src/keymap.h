#pragma once

#include "state.h"
#include "keyevent.h"

typedef enum {
    BINDING_COMMAND,
    BINDING_KEYMAP
} BindingType;

typedef enum {
    COMMAND_C,
    COMMAND_SCHEME
} CommandType;

typedef struct {
    CommandType type;
    union {
        void (*c_fn)(AppState *);
        sexp scheme_proc;
    };
} Command;

typedef struct {
    BindingType type;
    union {
        Command command;   // C or Scheme wrapper
        struct Keymap *keymap;
    };
} Binding;

typedef struct {
    KeyEvent key;
    Binding  binding;
} KeymapEntry;

typedef struct Keymap {
    KeymapEntry *entries;
    size_t count;
    size_t cap;
} Keymap;

Keymap *keymap_create(void);
void init_input(AppState *state);
void keymap_bind_char(Keymap *km, uint32_t codepoint, void (*fn)(AppState *));
void keymap_bind_ctrl(Keymap *km, uint32_t codepoint, void (*fn)(AppState *));
void keymap_bind_ctrl_prefix(Keymap *km, uint32_t codepoint, Keymap *submap);
Binding *keymap_lookup(Keymap *km, const KeyEvent *ev);
void execute_command(AppState *app, Binding *b);
void key_dispatch(AppState *state, const KeyEvent *ev);
