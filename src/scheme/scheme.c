#include "scheme.h"
#include "../keymap.h"
#include "../subeditor/buffer.h"
#include "../theme.h"
#include <chibi/eval.h>
#include <chibi/sexp.h>

static AppState *G;   // global app state for commands
static sexp keymap_type;
static sexp buffer_type;

static sexp scm_insert_char(sexp ctx, sexp self, sexp n, sexp ch) {
    G->needs_redraw = true;
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, ch);
    insert_char(sexp_unbox_character(ch));
    return SEXP_VOID;
}

static sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
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

static sexp scm_toggle_theme(sexp ctx, sexp self, sexp n) {
    toggle_theme(G);
    return SEXP_VOID;
}

// FIXED: Removed the extra 'sexp n' parameter for 3-argument function
static sexp scm_define_key(sexp ctx, sexp self, sexp n,
                    sexp skeymap, sexp skeystr, sexp scommand) {
    // ... type checks ...
    
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
    sexp protected_sym = sexp_intern(ctx, "*protected-commands*", -1);
    sexp old_list = sexp_env_ref(ctx, G->chibi.env, protected_sym, SEXP_NULL);
    sexp new_list = sexp_cons(ctx, scommand, old_list);
    sexp_env_define(ctx, G->chibi.env, protected_sym, new_list);
    
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
    state->chibi.protected_commands = SEXP_NULL;
    
    keymap_type = sexp_register_c_type(
        ctx, sexp_intern(ctx, "keymap", -1), NULL
    );
    
    buffer_type = sexp_register_c_type(
        ctx, sexp_intern(ctx, "buffer", -1), NULL
    );
    
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
    
    sexp_define_foreign(ctx, env, "make-keymap", 0, scm_make_keymap);
    sexp_define_foreign(ctx, env, "define-key!", 3, scm_define_key);
    sexp_define_foreign(ctx, env, "insert-char", 1, scm_insert_char);
    sexp_define_foreign(ctx, env, "self-insert", 0, scm_self_insert);
    sexp_define_foreign(ctx, env, "toggle-theme", 0, scm_toggle_theme);
}
