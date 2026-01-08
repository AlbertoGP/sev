#include "keymap.h"

void init_input(AppState *state) {
    Keymap *global = keymap_create();

    state->input.global_map  = global;
    state->input.current_map = global;
}

Binding *keymap_lookup(Keymap *km, const KeyEvent *ev) {
    if (!km) return NULL;
    for (size_t i = 0; i < km->count; i++) {
        if (keyevent_equal(&km->entries[i].key, ev))
            return &km->entries[i].binding;
    }
    return NULL;
}

void execute_command(AppState *app, Binding *b) {
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
    Binding *b = keymap_lookup(state->input.current_map, ev);

    if (!b) {
        reset_key_state(state);
        // minibuffer_message("Undefined key");
        printf("Undefined key\n");
        return;
    }

    if (b->type == BINDING_KEYMAP) {
        state->input.current_map = b->keymap;
        return;
    }

    printf("this fired\n");
    execute_command(state, b);
    reset_key_state(state);
}

