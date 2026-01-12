#include "scheme.h"
#include "../keymap.h"
#include "../subeditor/buffer.h"

#include <chibi/eval.h>
#include <chibi/sexp.h>

static AppState *G;   // global app state for commands

static sexp keymap_type;
static sexp buffer_type;

static sexp scm_insert_char(sexp ctx, sexp self, sexp n, sexp ch) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, ch);

    insert_char(sexp_unbox_character(ch));
    return SEXP_VOID;
}

static sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    KeyEvent last = G->input.last_event;
    
    if (last.type != KEYEVENT_CHAR) return SEXP_VOID;

    insert_char(last.codepoint);
    return SEXP_VOID;
}

static sexp scm_make_keymap(sexp ctx, sexp self, sexp n) {
    Keymap *km = keymap_create();
    return sexp_make_cpointer(ctx,
        sexp_type_tag(keymap_type),
        km,
        SEXP_FALSE,
        0);
}

sexp scm_define_key(sexp ctx, sexp self, sexp n,
                    sexp skeymap, sexp skeystr, sexp scommand) {
    // Type checks
    // if (!sexp_cpointerp(skeymap) || 
    //     sexp_pointer_tag(skeymap) != keymap_type) {
    //     return sexp_type_exception(ctx, self, keymap_type, skeymap);
    // }
    
    if (!sexp_stringp(skeystr)) {
        return sexp_type_exception(ctx, self, SEXP_STRING, skeystr);
    }
    
    if (!sexp_procedurep(scommand)) {
        return sexp_type_exception(ctx, self, SEXP_PROCEDURE, scommand);
    }
    
    Keymap *km = sexp_cpointer_value(skeymap);
    if (!km) {
        return sexp_user_exception(ctx, self, "null keymap pointer", skeymap);
    }
    
    const char *keystr = sexp_string_data(skeystr);
    KeyEvent seq[8];
    int key_count = parse_key_sequence(keystr, seq);
    
    if (key_count <= 0) {
        return sexp_user_exception(ctx, self, "invalid key sequence", skeystr);
    }
    
    // Protect the command from GC by adding to protected list
    // Use sexp_gc_preserve to ensure the cons cell itself is protected
    sexp new_list = sexp_cons(ctx, scommand, G->chibi.protected_commands);
    G->chibi.protected_commands = new_list;
    
    Binding binding = {
        .type = BINDING_COMMAND,
        .command = (Command){
            .type = COMMAND_SCHEME,
            .scheme_proc = scommand
        }
    };
    
    keymap_bind_sequence(km, seq, key_count, binding);
    
    return SEXP_VOID;
}

void scheme_init(AppState *state) {
    G = state;

    sexp_scheme_init();

    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 0);
    sexp env = sexp_context_env(ctx);

    state->chibi.ctx = ctx;
    state->chibi.env = env;
    state->chibi.protected_commands = NULL;

    /* register types */
    keymap_type = sexp_register_c_type(
        ctx, sexp_intern(ctx, "keymap", -1), NULL
    );

    buffer_type = sexp_register_c_type(
        ctx, sexp_intern(ctx, "buffer", -1), NULL
    );

    sexp global_km = scm_make_keymap(ctx, SEXP_FALSE, 0);
    sexp_env_define(ctx, env, sexp_intern(ctx, "global-keymap", -1), global_km);

    sexp_env_define(
    ctx,
    env,
    sexp_intern(ctx, "global-keymap", -1),
    sexp_make_cpointer(
        ctx,
        sexp_type_tag(keymap_type),
            state->input.global_map,
            SEXP_FALSE,
            0
        )
    );

    /* register primitives */
    sexp_define_foreign(ctx, env, "make-keymap", 0, scm_make_keymap);
    sexp_define_foreign(ctx, env, "define-key!", 3, scm_define_key);
    sexp_define_foreign(ctx, env, "insert-char", 1, scm_insert_char);
    sexp_define_foreign(ctx, env, "self-insert", 0, scm_self_insert);
}

