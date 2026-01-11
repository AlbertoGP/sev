// Keymap binding function implementations.

#include <assert.h>
#include "keymap.h"

Keymap *keymap_create(void) {
    Keymap *km = calloc(1, sizeof(Keymap));
    return km;
}

void init_input(AppState *state) {
    Keymap *global = keymap_create();

    state->input.global_map  = global;
    state->input.current_map = global;
}

static void keymap_add(Keymap *km, KeyEvent key, Binding binding) {
    if (km->count == km->cap) {
        km->cap = km->cap ? km->cap * 2 : 8;
        km->entries = realloc(km->entries,
                               km->cap * sizeof(KeymapEntry));
    }

    km->entries[km->count++] = (KeymapEntry){
        .key = key,
        .binding = binding,
    };
}

void keymap_bind_char(Keymap *km, uint32_t codepoint, void (*fn)(AppState *)) {
    KeyEvent key = {
        .type = KEYEVENT_CHAR,
        .mods = MOD_NONE,
        .codepoint = codepoint,
    };

    Binding binding = {
        .type = BINDING_COMMAND,
        .command = {
            .type = COMMAND_C,
            .c_fn = fn,
        },
    };

    keymap_add(km, key, binding);
}

void keymap_bind_ctrl(Keymap *km, uint32_t codepoint, void (*fn)(AppState *)) {
    KeyEvent key = {
        .type = KEYEVENT_CHAR,
        .mods = MOD_CTRL,
        .codepoint = codepoint,
    };

    Binding binding = {
        .type = BINDING_COMMAND,
        .command = {
            .type = COMMAND_C,
            .c_fn = fn,
        },
    };

    keymap_add(km, key, binding);
}

void keymap_bind_ctrl_prefix(Keymap *km, uint32_t codepoint, Keymap *submap) {
    KeyEvent key = {
        .type = KEYEVENT_CHAR,
        .mods = MOD_CTRL,
        .codepoint = (uint32_t)codepoint,
    };

    Binding binding = {
        .type = BINDING_KEYMAP,
        .keymap = submap,
    };

    keymap_add(km, key, binding);
}

static Binding *keymap_lookup(Keymap *km, const KeyEvent *ev) {
    if (!km) return NULL;
    for (size_t i = 0; i < km->count; i++) {
        if (keyevent_equal(&km->entries[i].key, ev))
            return &km->entries[i].binding;
    }
    return NULL;
}

static void execute_command(AppState *app, Binding *b) {
    Command *cmd = &b->command;

    if (cmd->type == COMMAND_C) {
        cmd->c_fn(app);
    } else {
        sexp_apply(app->chibi,
                   cmd->scheme_proc,
                   SEXP_NULL);
    }
}

static void reset_key_state(AppState *state) {
    state->input.current_map = state->input.global_map;
}

void key_dispatch(AppState *state, const KeyEvent *ev) {
    assert(state->input.current_map);

    state->input.last_event = *ev;

    Binding *b = keymap_lookup(state->input.current_map, ev);

    if (!b) {
        reset_key_state(state);
        // minibuffer_message("Undefined key");
        printf("Undefined key: %c   %d\n",
               state->input.last_event.codepoint,
               state->input.last_event.mods);
        return;
    }

    if (b->type == BINDING_KEYMAP) {
        state->input.current_map = b->keymap;
        return;
    }

    execute_command(state, b);
    reset_key_state(state);
}

