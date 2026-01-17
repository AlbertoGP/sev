// Keymap binding function implementations.

#include <assert.h>
#include <chibi/eval.h>
#include "keymap.h"
#include "keyevent.h"

KeyEvent last_event;

Keymap *keymap_create(void) {
    Keymap *km = calloc(1, sizeof(Keymap));
    return km;
}

bool init_input(AppState *state) {
    Keymap *global = keymap_create();
    if (!global) return false;

    state->input.global_map  = global;
    state->input.current_map = global;

    return true;
}

static bool keymap_add(Keymap *km, KeyEvent key, Binding binding) {
    for (int i = 0; i < km->count; i++) {
        if (keyevent_equal(&km->entries[i].key, &key)) {
            km->entries[i].binding = binding; // overwrite
            return true;
        }
    }

    if (km->count == km->cap) {
        km->cap = km->cap ? km->cap * 2 : 8;
        KeymapEntry *temp = realloc(km->entries,
                               km->cap * sizeof(KeymapEntry));
        if (temp == NULL) {
            fprintf(stderr, "Error adding binding: memory reallocation failed!\n");
            return false;
        } else {
            km->entries = temp;
        }
    }

    km->entries[km->count++] = (KeymapEntry){
        .key = key,
        .binding = binding,
    };
    return true;
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

static void execute_command(AppState *state, Binding *b) {
    Command *cmd = &b->command;

    if (cmd->type == COMMAND_C) {
        cmd->c_fn(state);
    } else {
        sexp_apply(state->chibi.ctx,
                   cmd->scheme_proc,
                   SEXP_NULL);
    }
}

static void reset_key_state(AppState *state) {
    state->input.current_map = state->input.global_map;
}

void key_dispatch(AppState *state, const KeyEvent *ev) {
    assert(state->input.current_map);

    last_event = *ev;

    Binding *b = keymap_lookup(state->input.current_map, ev);

    if (!b) {
        reset_key_state(state);
        // minibuffer_message("Undefined key");
        printf("Undefined key:\n\tcodepoint: %c\n\tmods: %d\n",
               last_event.codepoint,
               last_event.mods);
        // if (ev->type == KEYEVENT_CHAR)
        //     insert_char(ev->codepoint);
        return;
    }

    if (b->type == BINDING_KEYMAP) {
        state->input.current_map = b->keymap;
        return;
    }

    execute_command(state, b);
    reset_key_state(state);
}

int parse_key_sequence(const char *s, KeyEvent *out) {
    int count = 0;
    uint16_t mods = 0;

    while (*s) {
        if (*s == ' ') {
            s++;
            continue;
        }

        mods = 0;

        /* modifiers */
        while (s[1] == '-') {
            switch (*s) {
            case 'C': mods |= MOD_CTRL; break;
            case 'M': mods |= MOD_META; break;
            case 'S': mods |= MOD_SHIFT; break;
            }
            s += 2; // skip "C-"
        }

        if (s[0] == 'S' && s[1] == 'P' && s[2] == 'C') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_CHAR,
                .mods = mods,
                .codepoint = (uint32_t)' '
            };
            s += 3;
            continue;
        }

        if (s[0] == 'E' && s[1] == 'S' && s[2] == 'C') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_ESC
            };
            s += 3;
            continue;
        }

        if (s[0] == 'R' && s[1] == 'E' && s[2] == 'T') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_RETURN
            };
            s += 3;
            continue;
        }

        if (s[0] == 'B' && s[1] == 'S' && s[2] == 'P') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_BACKSPACE
            };
            s += 3;
            continue;
        }

        if (s[0] == 'D' && s[1] == 'E' && s[2] == 'L') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_DELETE
            };
            s += 3;
            continue;
        }

        if (s[0] == 'U' && s[1] == 'P') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_UP
            };
            s += 2;
            continue;
        }

        if (s[0] == 'D' && s[1] == 'O' && s[2] == 'W' && s[3] == 'N') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_DOWN
            };
            s += 4;
            continue;
        }

        if (s[0] == 'L' && s[1] == 'E' && s[2] == 'F' && s[3] == 'T') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_LEFT
            };
            s += 4;
            continue;
        }

        if (s[0] == 'R' && s[1] == 'I' && s[2] == 'G' && s[3] == 'H' && s[4] == 'T') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_RIGHT
            };
            s += 5;
            continue;
        }

        /* key */
        if (*s) {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_CHAR,
                .mods = mods,
                .codepoint = (uint32_t)*s
            };
            s++;
        }
    }

    return count;
}

void keymap_bind_sequence(Keymap *km, KeyEvent *seq, int n, Binding final) {
    for (int i = 0; i < n - 1; i++) {
        Binding *b = keymap_lookup(km, &seq[i]);

        if (!b || b->type != BINDING_KEYMAP) {
            Keymap *next = keymap_create();

            Binding nb = {
                .type = BINDING_KEYMAP,
                .keymap = next
            };

            keymap_add(km, seq[i], nb);
            km = next;
        } else {
            km = b->keymap;
        }
    }

    /* final key */
    keymap_add(km, seq[n - 1], final);
}

