#include "scheme.h"
#include "../keymap.h"
#include "../subeditor/buffer.h"
#include "../theme.h"
#include "../layout/tab.h"
#include "../subeditor/message.h"
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
    insert_char(buffer_get_current(), sexp_unbox_character(ch));

    message_clear();
    return SEXP_VOID;
}

static sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    KeyEvent last = last_event;
    
    if (last.type != KEYEVENT_CHAR) return SEXP_VOID;
    insert_char(buffer_get_current(), last.codepoint);

    message_clear();
    return SEXP_VOID;
}

static sexp scm_delete_char(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;

    int count_unboxed = sexp_unbox_fixnum(count);
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());

    message_clear();
    if (count_unboxed > (int)point)
        message_send("Beginning of buffer");
    if (count_unboxed < 0 && -count_unboxed > (int)(chars - point))
        message_send("End of buffer");

    delete_chars(buffer_get_current(), sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

static sexp scm_point_move(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;

    int count_unboxed = sexp_unbox_fixnum(count);
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());

    message_clear();
    if (count_unboxed < 0 && -count_unboxed > (int)point)
        message_send("Beginning of buffer");
    if (count_unboxed > (int)(chars - point))
        message_send("End of buffer");

    point_move(sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

static sexp scm_point_move_by_line(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;
    point_move_by_line(sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

static sexp scm_next_line(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    point_move_by_line(1);
    return SEXP_VOID;
}

static sexp scm_prev_line(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    point_move_by_line(-1);
    return SEXP_VOID;
}

static sexp scm_forward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());
    if (point == chars)
        message_send("End of buffer");
    point_move(1);
    return SEXP_VOID;
}

static sexp scm_backward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    if (point == 0)
        message_send("Beginning of buffer");
    point_move(-1);
    return SEXP_VOID;
}

static sexp scm_newline(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    insert_char(buffer_get_current(), '\n');
    message_clear();
    return SEXP_VOID;
}

static sexp scm_insert_tab(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    insert_char(buffer_get_current(), '\t');
    message_clear();
    return SEXP_VOID;
}

static sexp scm_delete_backward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    size_t point = point_get(buffer_get_current()).pos;
    message_clear();
    if (point == 0)
        message_send("Beginning of buffer");
    delete_chars(buffer_get_current(), 1);
    return SEXP_VOID;
}

static sexp scm_delete_forward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());
    message_clear();
    if (point >= chars)
        message_send("End of buffer");
    delete_chars(buffer_get_current(), -1);
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

    char message[100];
    sprintf(message, "Theme set to %s mode", G->theme == THEME_DARK ? "dark" : "light");
    message_send(message);
    return SEXP_VOID;
}

static sexp scm_set_key(sexp ctx, sexp self, sexp n,
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


static sexp scm_char_at_point(sexp ctx, sexp self, sexp n) {
    printf("point: %c\n", char_at_point());
    // print_buffer();

    return SEXP_VOID;
}

static sexp scm_set_column(sexp ctx, sexp self, sexp n, sexp column) {
    G->needs_redraw = true;
    set_column(sexp_unbox_fixnum(column), false);

    return SEXP_VOID;
}

static sexp scm_tab_next(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    if (tab_next())
        message_send("tab-next");
    return SEXP_VOID;
}

static sexp scm_tab_prev(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    if (tab_prev())
        message_send("tab-prev");
    return SEXP_VOID;
}

static sexp scm_split_vertical(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    pane_split_vertical(pane_get_active());

    message_send("split-vertical");
    return SEXP_VOID;
}

static sexp scm_split_horizontal(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    pane_split_horizontal(pane_get_active());

    message_send("split-horizontal");
    return SEXP_VOID;
}

static sexp scm_pane_close(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    pane_close();

    message_send("pane-close");
    return SEXP_VOID;
}

static sexp scm_pane_navigate_up(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    if (pane_navigate_up())
        message_send("pane-navigate-up");
    return SEXP_VOID;
}

static sexp scm_pane_navigate_down(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    if (pane_navigate_down())
        message_send("pane-navigate-down");
    return SEXP_VOID;
}

static sexp scm_pane_navigate_left(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
    if (pane_navigate_left())
        message_send("pane-navigate-left");
    return SEXP_VOID;
}

static sexp scm_pane_navigate_right(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    
    message_clear();
    if (pane_navigate_right())
        message_send("pane-navigate-right");
    return SEXP_VOID;
}

static sexp scm_pane_v_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    message_clear();
    if (pane_v_split_increase())
        message_send("pane-v-split-increase");
    return SEXP_VOID;
}

static sexp scm_pane_v_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    message_clear();
    if (pane_v_split_decrease())
        message_send("pane-v-split-decrease");
    return SEXP_VOID;
}

static sexp scm_pane_h_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    message_clear();
    if (pane_h_split_increase())
        message_send("pane-h-split-increase");
    return SEXP_VOID;
}

static sexp scm_pane_h_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    message_clear();
    if (pane_h_split_decrease())
        message_send("pane-h-split-decrease");
    return SEXP_VOID;
}

static sexp scm_eval_buffer(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    sexp_gc_var2(result, str);
    sexp_gc_preserve2(ctx, result, str);

    result = sexp_eval_string(ctx, buffer_text(buffer_get_current()), -1, NULL);

    if (sexp_exceptionp(result)) {
        G->needs_extra_frame = true;

        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        str = sexp_write_to_string(ctx, sexp_exception_message(result));
        Pane *pane = pane_split_horizontal(pane_get_active());
        const char *NAME = "*Backtrace*";
        Buffer *buf = buffer_get_by_name(NAME);
        if (!buf) {
            buffer_create(NAME);
            buf = buffer_get_by_name(NAME);
        } else {
            buffer_clear(buf);
        }
        insert_string(buf, "Error: ");
        insert_string(buf, sexp_string_data(str) + 1);
        delete_chars(buf, 1);
        str = sexp_write_to_string(ctx, sexp_exception_irritants(result));
        insert_string(buf, ": ");
        insert_string(buf, sexp_string_data(str) + 1);
        delete_chars(buf, 1);
        pane_set_buffer(pane, buf);
        pane_set_active(pane);
    } else {
        str = sexp_write_to_string(ctx, result);
        message_send(sexp_string_data(str));
    }

    sexp_gc_release2(ctx);

    return SEXP_VOID;
}

static sexp scm_clay_debug(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->debug_open = !G->debug_open;
    Clay_SetDebugModeEnabled(G->debug_open);

    return SEXP_VOID;
}

static sexp scm_toggle_cursor(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->cursor++;
    G->cursor %= 4;

    return SEXP_VOID;
}

static sexp scm_prefix_arg(sexp ctx, sexp self, sexp n) {
    return SEXP_FALSE;  // TODO: integrate with C-u handling
}

void scheme_init(AppState *state) {
    G = state;
    sexp_scheme_init();

    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 512*1024*1024);
    if (!ctx || sexp_exceptionp(ctx)) {
        fprintf(stderr, "ERROR: failed to create eval context\n");
        return;
    }

    sexp env = sexp_load_standard_env(ctx, NULL, SEXP_SEVEN);
    if (sexp_exceptionp(env)) {
        fprintf(stderr, "ERROR: failed to load standard env\n");
        // Try to print exception message directly
        sexp msg = sexp_exception_message(env);
        if (sexp_stringp(msg)) {
            fprintf(stderr, "  Exception: %s\n", sexp_string_data(msg));
        }
        sexp irritants = sexp_exception_irritants(env);
        if (sexp_pairp(irritants) && sexp_stringp(sexp_car(irritants))) {
            fprintf(stderr, "  Irritant: %s\n", sexp_string_data(sexp_car(irritants)));
        }
        return;
    }

    sexp_load_standard_ports(ctx, env, stdin, stdout, stderr, 0);

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
    sexp_define_foreign(ctx, env, "%set-key!", 3, scm_set_key);
    sexp_define_foreign(ctx, env, "insert-char", 1, scm_insert_char);
    sexp_define_foreign(ctx, env, "self-insert", 0, scm_self_insert);
    sexp_define_foreign(ctx, env, "delete-char", 1, scm_delete_char);
    sexp_define_foreign(ctx, env, "move-point", 1, scm_point_move);
    sexp_define_foreign(ctx, env, "move-point-by-line", 1, scm_point_move_by_line);
    sexp_define_foreign(ctx, env, "next-line", 0, scm_next_line);
    sexp_define_foreign(ctx, env, "prev-line", 0, scm_prev_line);
    sexp_define_foreign(ctx, env, "forward-char", 0, scm_forward_char);
    sexp_define_foreign(ctx, env, "backward-char", 0, scm_backward_char);
    sexp_define_foreign(ctx, env, "newline", 0, scm_newline);
    sexp_define_foreign(ctx, env, "insert-tab", 0, scm_insert_tab);
    sexp_define_foreign(ctx, env, "delete-backward-char", 0, scm_delete_backward_char);
    sexp_define_foreign(ctx, env, "delete-forward-char", 0, scm_delete_forward_char);
    sexp_define_foreign(ctx, env, "set-column", 1, scm_set_column);
    sexp_define_foreign(ctx, env, "toggle-theme", 0, scm_toggle_theme);
    sexp_define_foreign(ctx, env, "char-at-point", 0, scm_char_at_point);
    sexp_define_foreign(ctx, env, "tab-next", 0, scm_tab_next);
    sexp_define_foreign(ctx, env, "tab-prev", 0, scm_tab_prev);
    sexp_define_foreign(ctx, env, "split-vertical", 0, scm_split_vertical);
    sexp_define_foreign(ctx, env, "split-horizontal", 0, scm_split_horizontal);
    sexp_define_foreign(ctx, env, "pane-close", 0, scm_pane_close);
    sexp_define_foreign(ctx, env, "pane-navigate-up", 0, scm_pane_navigate_up);
    sexp_define_foreign(ctx, env, "pane-navigate-down", 0, scm_pane_navigate_down);
    sexp_define_foreign(ctx, env, "pane-navigate-left", 0, scm_pane_navigate_left);
    sexp_define_foreign(ctx, env, "pane-navigate-right", 0, scm_pane_navigate_right);
    sexp_define_foreign(ctx, env, "pane-v-split-increase", 0, scm_pane_v_split_increase);
    sexp_define_foreign(ctx, env, "pane-v-split-decrease", 0, scm_pane_v_split_decrease);
    sexp_define_foreign(ctx, env, "pane-h-split-increase", 0, scm_pane_h_split_increase);
    sexp_define_foreign(ctx, env, "pane-h-split-decrease", 0, scm_pane_h_split_decrease);
    sexp_define_foreign(ctx, env, "eval-buffer", 0, scm_eval_buffer);
    sexp_define_foreign(ctx, env, "clay-debug", 0, scm_clay_debug);
    sexp_define_foreign(ctx, env, "toggle-cursor", 0, scm_toggle_cursor);
    sexp_define_foreign(ctx, env, "prefix-arg", 0, scm_prefix_arg);

    #ifdef __EMSCRIPTEN__
    #define RESOURCES_PATH "/resources/"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char resourcesPath[1024];
    snprintf(resourcesPath, sizeof(resourcesPath), "%sresources/", basePath);
    #define RESOURCES_PATH resourcesPath
    #endif

    // Load command.scm first (provides infrastructure for init.scm)
    char commandScriptPath[1024];
    snprintf(commandScriptPath, sizeof(commandScriptPath), "%scommand.scm", RESOURCES_PATH);
    sexp result = sexp_load(ctx, sexp_c_string(ctx, commandScriptPath, -1), env);
    if (sexp_exceptionp(result)) {
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }

    // Load init.scm
    char initScriptPath[1024];
    snprintf(initScriptPath, sizeof(initScriptPath), "%sinit.scm", RESOURCES_PATH);
    result = sexp_load(ctx, sexp_c_string(ctx, initScriptPath, -1), env);
    if (sexp_exceptionp(result)) {
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }

    // Get call-interactively procedure (defined in command.scm)
    state->chibi.call_interactively = sexp_env_ref(
        ctx, env,
        sexp_intern(ctx, "call-interactively", -1),
        SEXP_FALSE
    );
    if (state->chibi.call_interactively == SEXP_FALSE) {
        fprintf(stderr, "ERROR: call-interactively not found\n");
    }
    sexp_preserve_object(ctx, state->chibi.call_interactively);
}
