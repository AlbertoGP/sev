#include "scheme.h"
#include "../keymap.h"
#include "../subeditor/buffer.h"
#include "../theme.h"
#include <SDL3/SDL_events.h>
#include <chibi/eval.h>
#include <chibi/sexp.h>

static AppState *G;   // global app state for commands
extern KeyEvent last_event;

static sexp scm_quit(sexp ctx, sexp self, sexp n) {
    SDL_Event quit_event;
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);
    return SEXP_VOID;
}

static sexp scm_insert_char(sexp ctx, sexp self, sexp n, sexp ch) {
    G->needs_redraw = true;
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, ch);
    insert_char(sexp_unbox_character(ch));
    return SEXP_VOID;
}

static sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    KeyEvent last = last_event;
    
    if (last.type != KEYEVENT_CHAR) return SEXP_VOID;
    insert_char(last.codepoint);
    return SEXP_VOID;
}

static sexp scm_delete_char(sexp ctx, sexp self, sexp n, sexp num) {
    G->needs_redraw = true;
    delete_chars(sexp_unbox_fixnum(num));
    return SEXP_VOID;
}

static sexp scm_point_move(sexp ctx, sexp self, sexp n, sexp num) {
    printf("this fired\n");
    G->needs_redraw = true;
    point_move(sexp_unbox_fixnum(num));
    return SEXP_VOID;
}

static sexp scm_make_keymap(sexp ctx, sexp self, sexp n) {
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

static sexp scm_toggle_theme(sexp ctx, sexp self, sexp n) {
    toggle_theme(G);
    return SEXP_VOID;
}

static sexp scm_set_key(sexp ctx, sexp self, sexp n,
                    sexp skeymap, sexp skeystr, sexp scommand) {
    Keymap *km = sexp_cpointer_value(skeymap);
    if (!km) {
        return sexp_user_exception(ctx, self, "null keymap pointer", skeymap);
    }

    const char *keystr = sexp_string_data(skeystr);
    KeyEvent seq[8];
    int key_count = parse_key_sequence(keystr, seq);
    if (key_count < 1) {
        return sexp_user_exception(ctx, self, "invalid key sequence", skeystr);
    }
    
    Binding binding = {
        .type = BINDING_COMMAND,
        .command = (Command){
            .type = COMMAND_SCHEME,
            .scheme_proc = scommand
        }
    };
    // After creating/modifying a binding with a scheme_proc:
    sexp_preserve_object(ctx, binding.command.scheme_proc);

    keymap_bind_sequence(km, seq, key_count, binding);
    
    return SEXP_VOID;
}

void scheme_init(AppState *state) {
    G = state;
    sexp_scheme_init();
    
    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 512*1024*1024);
    sexp env = sexp_load_standard_env(ctx, NULL, SEXP_SEVEN);
    sexp_load_standard_ports(ctx, env, stdin, stdout, stderr, 1);
    
    state->chibi.ctx = ctx;
    state->chibi.env = env;
    
    sexp_gc_var1(global_km);
    sexp_gc_preserve1(ctx, global_km);
    
    // DON'T register custom types for now - just use built-in CPOINTER
    global_km = sexp_make_cpointer(
        ctx,
        SEXP_CPOINTER,
        state->input.global_map,
        SEXP_FALSE,
        0
    );
    
    sexp_env_define(ctx, env,
                    sexp_intern(ctx, "global-keymap", -1),
                    global_km);
    
    sexp_gc_release1(ctx);
    
    // Register foreign functions
    sexp_define_foreign(ctx, env, "quit", 0, scm_quit);
    sexp_define_foreign(ctx, env, "make-keymap", 0, scm_make_keymap);
    sexp_define_foreign(ctx, env, "set-key!", 3, scm_set_key);
    sexp_define_foreign(ctx, env, "insert-char", 1, scm_insert_char);
    sexp_define_foreign(ctx, env, "self-insert", 0, scm_self_insert);
    sexp_define_foreign(ctx, env, "delete-char", 1, scm_delete_char);
    sexp_define_foreign(ctx, env, "move-point", 1, scm_point_move);
    sexp_define_foreign(ctx, env, "toggle-theme", 0, scm_toggle_theme);
    
    sexp result = sexp_load(ctx, sexp_c_string(ctx, "resources/init.scm", -1), env);
    if (sexp_exceptionp(result)) {
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }
}
