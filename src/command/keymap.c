// Keymap binding function implementations.

#include <assert.h>

#include <chibi/eval.h>

#include "keymap.h"
#include "message.h"
#include "mode.h"
#include "scheme_internal.h"
#include "../text/buffer.h"

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
    state->input.key_intercept_cb  = SEXP_FALSE;
    state->input.key_intercept_map = NULL;
    state->input.key_intercept_str[0] = '\0';

    return true;
}

static void key_event_append_str(char *buf, size_t sz, const KeyEvent *ev) {
    size_t len = strlen(buf);
    if (len > 0 && len + 1 < sz) {
        buf[len++] = ' ';
        buf[len]   = '\0';
    }

    char tmp[64];
    size_t ti = 0;

    if (ev->mods & MOD_CTRL)  { tmp[ti++] = 'C'; tmp[ti++] = '-'; }
    if (ev->mods & MOD_META)  { tmp[ti++] = 'M'; tmp[ti++] = '-'; }
    if (ev->mods & MOD_SHIFT) { tmp[ti++] = 'S'; tmp[ti++] = '-'; }

    if (ev->type == KEYEVENT_SPECIAL) {
        const char *name;
        switch (ev->keycode) {
        case KEY_ESC:       name = "ESC";   break;
        case KEY_RETURN:    name = "RET";   break;
        case KEY_TAB:       name = "TAB";   break;
        case KEY_BACKSPACE: name = "BSP";   break;
        case KEY_DELETE:    name = "DEL";   break;
        case KEY_UP:        name = "UP";    break;
        case KEY_DOWN:      name = "DOWN";  break;
        case KEY_LEFT:      name = "LEFT";  break;
        case KEY_RIGHT:     name = "RIGHT"; break;
        default:            name = "?";     break;
        }
        snprintf(tmp + ti, sizeof(tmp) - ti, "%s", name);
    } else {
        if (ev->codepoint == (uint32_t)' ') {
            snprintf(tmp + ti, sizeof(tmp) - ti, "SPC");
        } else {
            tmp[ti++] = (char)ev->codepoint;
            tmp[ti]   = '\0';
        }
    }

    strncat(buf, tmp, sz - strlen(buf) - 1);
}

static bool keymap_add(Keymap *km, KeyEvent key, Binding binding) {
    for (size_t i = 0; i < km->count; i++) {
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

// Local-only lookup (no parent chain) — used by keymap_bind_sequence
// to avoid leaking bindings into parent keymaps.
static Binding *keymap_lookup_local(Keymap *km, const KeyEvent *ev) {
    if (!km) return NULL;
    for (size_t i = 0; i < km->count; i++) {
        if (keyevent_equal(&km->entries[i].key, ev))
            return &km->entries[i].binding;
    }
    return NULL;
}

Binding *keymap_lookup(Keymap *km, const KeyEvent *ev) {
    for (Keymap *cur = km; cur; cur = cur->parent) {
        for (size_t i = 0; i < cur->count; i++) {
            if (keyevent_equal(&cur->entries[i].key, ev))
                return &cur->entries[i].binding;
        }
    }
    return NULL;
}

Binding *keymap_lookup_chain(AppState *state, const KeyEvent *ev) {
    Buffer *buf = buffer_get_current();
    Binding *b;

    // 1. Minor mode keymaps (head-first = most-recently-enabled)
    ModeList *minors = buffer_get_minor_modes(buf);
    if (minors) {
        for (ModeListNode *n = minors->head; n; n = n->next) {
            if (n->mode->keymap && (b = keymap_lookup(n->mode->keymap, ev)))
                return b;
        }
    }

    // 2. Buffer-local keymap
    Keymap *local = buffer_get_local_map(buf);
    if (local && (b = keymap_lookup(local, ev)))
        return b;

    // 3. Major mode keymap
    Mode *major = buffer_get_major_mode(buf);
    if (major && major->keymap && (b = keymap_lookup(major->keymap, ev)))
        return b;

    // 4. Global keymap
    return keymap_lookup(state->input.global_map, ev);
}

static void execute_command(AppState *state, Binding *b) {
    sexp ctx = state->chibi.ctx;

    // Call Scheme: (call-interactively sym)
    sexp result = sexp_apply(
        ctx,
        state->chibi.call_interactively,
        sexp_list1(ctx, b->command_sym)
    );

    if (sexp_exceptionp(result)) {
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }
}

static void reset_key_state(AppState *state) {
    state->input.current_map = state->input.global_map;
}

static void key_dispatch_inner(AppState *state, const KeyEvent *ev) {
    assert(state->input.current_map);

    Binding *b;

    // If mid-prefix-sequence, continue in current_map only
    if (state->input.current_map != state->input.global_map) {
        b = keymap_lookup(state->input.current_map, ev);
    } else {
        // Start of sequence: use full chain lookup
        b = keymap_lookup_chain(state, ev);
    }

    if (!b) {
        if (state->input.current_map != state->input.global_map) {
            // In prefix sequence: invalid continuation is an error
            reset_key_state(state);
            char msg[128];
            snprintf(msg, 128, "Undefined key: codepoint: %c mods: %d",
                   last_event.keycode,
                   last_event.mods);
            message_send(msg);
            return;
        }
        // Top-level unbound key
        if (ev->type == KEYEVENT_CHAR && ev->mods == MOD_NONE) {
            Buffer *buf = buffer_get_current();
            ModeList *minors = buffer_get_minor_modes(buf);
            if (minors && minors->head && minors->head->mode->allows_input) {
                static Binding si;
                si.type = BINDING_COMMAND;
                si.command_sym = sexp_intern(state->chibi.ctx, "self-insert", -1);
                execute_command(state, &si);
                reset_key_state(state);
                goto record_macro;
            }
        }
        reset_key_state(state);
        goto record_macro;  // silently ignore, but still record
    }

    if (b->type == BINDING_KEYMAP) {
        state->input.current_map = b->keymap;
        goto record_macro;  // prefix key: record for macro replay
    }

    execute_command(state, b);
    reset_key_state(state);

record_macro:
    // Record event AFTER dispatch so stop-macro can clear the flag first,
    // preventing the stop 'q' from being recorded.
    if (state->macro_recording) {
        if (state->macro_skip_next) {
            // Don't record the key that triggered start-macro (e.g. 'a' in 'qa')
            state->macro_skip_next = false;
        } else {
            if (state->macro_buf_len >= state->macro_buf_cap) {
                size_t cap = state->macro_buf_cap ? state->macro_buf_cap * 2 : 32;
                KeyEvent *nb = SDL_realloc(state->macro_buf, cap * sizeof(KeyEvent));
                if (nb) { state->macro_buf = nb; state->macro_buf_cap = cap; }
            }
            if (state->macro_buf_len < state->macro_buf_cap)
                state->macro_buf[state->macro_buf_len++] = *ev;
        }
    }
}

void key_dispatch(AppState *state, const KeyEvent *ev) {
    last_event = *ev;

    if (state->minibuf.active) {
        Buffer *saved = buffer_get_current();
        buffer_set_current(state->minibuf.buf);
        reset_key_state(state);       // clear any editor prefix state
        key_dispatch_inner(state, ev);
        if (state->minibuf.active)    // submit/cancel already restored if false
            buffer_set_current(saved);
        return;
    }

    if (state->input.key_intercept_cb != SEXP_FALSE) {
        key_event_append_str(state->input.key_intercept_str,
                             sizeof(state->input.key_intercept_str), ev);

        Binding *b = (state->input.key_intercept_map == state->input.global_map)
            ? keymap_lookup_chain(state, ev)
            : keymap_lookup(state->input.key_intercept_map, ev);

        if (b && b->type == BINDING_KEYMAP) {
            state->input.key_intercept_map = b->keymap;
            char echo[280];
            snprintf(echo, sizeof(echo), "Describe key: %s", state->input.key_intercept_str);
            message_send(echo);
            return;
        }

        // Reached command or unbound — fire callback
        sexp ctx = state->chibi.ctx;
        sexp cb  = state->input.key_intercept_cb;
        state->input.key_intercept_cb = SEXP_FALSE;
        sexp sym = (b && b->type == BINDING_COMMAND) ? b->command_sym : SEXP_FALSE;
        sexp kstr = sexp_c_string(ctx, state->input.key_intercept_str, -1);
        sexp result = sexp_apply(ctx, cb, sexp_list2(ctx, sym, kstr));
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        sexp_release_object(ctx, cb);
        return;
    }

    key_dispatch_inner(state, ev);
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

        if (s[0] == 'T' && s[1] == 'A' && s[2] == 'B') {
            out[count++] = (KeyEvent){
                .type = KEYEVENT_SPECIAL,
                .mods = mods,
                .keycode = KEY_TAB
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
        Binding *b = keymap_lookup_local(km, &seq[i]);

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

// --- Scheme bindings ---

sexp scm_make_keymap(sexp ctx, sexp self, sexp n) {
    Keymap *km = keymap_create();
    if (!km) {
        return SEXP_VOID;
    }
    sexp result = sexp_make_cpointer(ctx,
        SEXP_CPOINTER,
        km,
        SEXP_FALSE,
        0);
    return result;
}

sexp scm_set_key(sexp ctx, sexp self, sexp n,
                 sexp skeymap, sexp skeystr, sexp scommand) {
    Keymap *km = sexp_cpointer_value(skeymap);
    if (!km) {
        return sexp_user_exception(ctx, self, "null keymap pointer", skeymap);
    }

    if (!sexp_symbolp(scommand)) {
        return sexp_user_exception(ctx, self,
            "command must be a symbol", scommand);
    }

    const char *keystr = sexp_string_data(skeystr);
    KeyEvent seq[8];
    int key_count = parse_key_sequence(keystr, seq);
    if (key_count < 1) {
        return sexp_user_exception(ctx, self, "invalid key sequence", skeystr);
    }

    Binding binding = {
        .type = BINDING_COMMAND,
        .command_sym = scommand
    };

    sexp_preserve_object(ctx, binding.command_sym);

    keymap_bind_sequence(km, seq, key_count, binding);

    return SEXP_VOID;
}

// (%set-keymap-parent! keymap parent) -> void
sexp scm_set_keymap_parent(sexp ctx, sexp self, sexp n,
                           sexp skeymap, sexp sparent) {
    Keymap *km = sexp_cpointer_value(skeymap);
    if (!km) return sexp_user_exception(ctx, self, "null keymap", skeymap);
    Keymap *parent = sexp_cpointer_value(sparent);
    km->parent = parent;
    return SEXP_VOID;
}

// (%read-key-binding callback) -> void
// Activates key-intercept mode: next key sequence (until a command or unbound)
// calls callback with (sym key-str), where sym is the bound command symbol or #f.
sexp scm_read_key_binding(sexp ctx, sexp self, sexp n, sexp callback) {
    if (G->input.key_intercept_cb != SEXP_FALSE)
        sexp_release_object(ctx, G->input.key_intercept_cb);
    G->input.key_intercept_cb = callback;
    sexp_preserve_object(ctx, G->input.key_intercept_cb);
    G->input.key_intercept_map = G->input.global_map;
    G->input.key_intercept_str[0] = '\0';
    return SEXP_VOID;
}

