#include <SDL3/SDL_events.h>
#include <chibi/eval.h>
#include <chibi/sexp.h>

#include "message.h"
#include "minibuf.h"
#include "scheme.h"
#include "scheme_bindings.h"
#include "../text/buffer.h"
#include "../text/buffer_type.h"
#include "../text/var.h"
#include "../display/pane.h"

AppState *G;   // global app state for commands

static sexp scm_quit(sexp ctx, sexp self, sexp n) {
    SDL_Event quit_event;
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);
    return SEXP_VOID;
}

static sexp scm_eval_buffer(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    sexp_gc_var2(result, str);
    sexp_gc_preserve2(ctx, result, str);

    char *eval_src = buffer_text(buffer_get_current());
    result = sexp_eval_string(ctx, eval_src, -1, NULL);
    free(eval_src);

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

static sexp scm_pop_to_buffer(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sname);
    const char *name = sexp_string_data(sname);
    Pane *pane = pane_split_horizontal(pane_get_active());
    Buffer *buf = buffer_get_by_name(name);
    if (!buf) {
        buffer_create(name);
        buf = buffer_get_by_name(name);
    } else {
        buffer_clear(buf);
    }
    pane_set_buffer(pane, buf);
    pane_set_active(pane);
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    return SEXP_VOID;
}

static sexp scm_clay_debug(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->debug_open = !G->debug_open;
    Clay_SetDebugModeEnabled(G->debug_open);

    return SEXP_VOID;
}

static sexp scm_prefix_arg(sexp ctx, sexp self, sexp n) {
    return SEXP_FALSE;  // TODO: integrate with C-u handling
}

// (ignore) - do nothing
static sexp scm_ignore(sexp ctx, sexp self, sexp n) {
    return SEXP_VOID;
}

static sexp scm_set_palette(sexp ctx, sexp self, sexp n, sexp key, sexp hexstr) {
    if (!sexp_symbolp(key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", key);
    }

    sexp_preserve_object(ctx, hexstr);
    vartable_set(&G->ui.palette_table, key, hexstr);
    return hexstr;
}

static sexp scm_set_role(sexp ctx, sexp self, sexp n, sexp role, sexp palette_key) {
    if (!sexp_symbolp(role)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", role);
    }
    if (!sexp_symbolp(palette_key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", palette_key);
    }

    sexp_preserve_object(ctx, palette_key);
    vartable_set(&G->ui.role_table, role, palette_key);
    return palette_key;
}

static sexp scm_clear_palette(sexp ctx, sexp self, sexp n) {
    vartable_destroy(&G->ui.palette_table);
    vartable_init(&G->ui.palette_table);
    return SEXP_VOID;
}

static sexp scm_clear_roles(sexp ctx, sexp self, sexp n) {
    vartable_destroy(&G->ui.role_table);
    vartable_init(&G->ui.role_table);
    return SEXP_VOID;
}

static sexp scm_clipboard_get(sexp ctx, sexp self, sexp n) {
    char *text = SDL_GetClipboardText();
    sexp result = sexp_c_string(ctx, text ? text : "", -1);
    SDL_free(text);
    return result;
}

static sexp scm_clipboard_set(sexp ctx, sexp self, sexp n, sexp stext) {
    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, stext);
    SDL_SetClipboardText(sexp_string_data(stext));
    return SEXP_VOID;
}

static sexp scm_buffer_file_name(sexp ctx, sexp self, sexp n) {
    char name[FILE_NAME_MAX] = {0};
    get_file_name(name, sizeof(name));
    if (name[0] == '\0') {
        return SEXP_FALSE;
    }
    return sexp_c_string(ctx, name, -1);
}

static sexp scm_set_buffer_file_name(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "filename must be a string", sname);
    set_file_name(sexp_string_data(sname));
    return SEXP_VOID;
}

static sexp scm_buffer_write(sexp ctx, sexp self, sexp n) {
    bool ok = buffer_write();
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_buffer_modified_p(sexp ctx, sexp self, sexp n) {
    return get_modified() ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_set_buffer_modified(sexp ctx, sexp self, sexp n, sexp val) {
    set_modified(sexp_truep(val));
    return SEXP_VOID;
}

static sexp scm_buffer_set_name(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sname);
    bool ok = buffer_set_name(sexp_string_data(sname));
    if (ok) G->needs_redraw = true; // since tab name might change
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_buffer_insert(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "filename must be a string", sname);
    bool ok = buffer_insert(sexp_string_data(sname));
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_buffer_read(sexp ctx, sexp self, sexp n) {
    bool ok = buffer_read();
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_buffer_create(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sname);
    Buffer *buf = buffer_create(sexp_string_data(sname));
    return buf ? SEXP_TRUE : SEXP_FALSE;
}

static sexp scm_pane_set_buffer(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sname);
    Buffer *buf = buffer_get_by_name(sexp_string_data(sname));
    if (!buf) return SEXP_FALSE;
    Pane *pane = pane_get_active();
    pane_set_buffer(pane, buf);
    G->needs_redraw = true;
    return SEXP_TRUE;
}


void scheme_init(AppState *state) {
    G = state;
    sexp_scheme_init();

#ifdef __EMSCRIPTEN__
    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 16*1024*1024);
#else
    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 512*1024*1024);
#endif
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

    // Import (scheme base) for when, unless, guard, etc.
    // Exclude bindings already provided by sexp_load_standard_env to avoid warnings.
    sexp_eval_string(ctx, "(import (except (scheme base) equal? let-syntax letrec-syntax))", -1, env);

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

    #define SDEF(a, b, c) sexp_define_foreign(ctx, env, a, b, c)

    // Register foreign functions
    SDEF("quit", 0, scm_quit);
    SDEF("message", 1, scm_message_send);
    SDEF("message-clear", 0, scm_message_clear);
    SDEF("message-lock", 0, scm_message_lock);
    SDEF("message-unlock", 0, scm_message_unlock);
    SDEF("make-keymap", 0, scm_make_keymap);
    SDEF("%set-key!", 3, scm_set_key);
    SDEF("insert-char", 1, scm_insert_char);
    SDEF("self-insert", 0, scm_self_insert);
    SDEF("delete-char", 1, scm_delete_char);
    SDEF("move-point", 1, scm_point_move);
    SDEF("move-point-by-line", 1, scm_point_move_by_line);
    SDEF("next-line", 0, scm_next_line);
    SDEF("prev-line", 0, scm_prev_line);
    SDEF("forward-char", 0, scm_forward_char);
    SDEF("backward-char", 0, scm_backward_char);
    SDEF("newline", 0, scm_newline);
    SDEF("insert-tab", 0, scm_insert_tab);
    SDEF("delete-backward-char", 0, scm_delete_backward_char);
    SDEF("delete-forward-char", 0, scm_delete_forward_char);
    SDEF("set-column", 1, scm_set_column);
    SDEF("line-start", 0, scm_point_to_line_start);
    SDEF("line-end", 0, scm_point_to_line_end);
    SDEF("skip-whitespace", 0, scm_skip_whitespace);
    SDEF("set-column", 1, scm_set_column);
    SDEF("char-at-point", 0, scm_char_at_point);
    SDEF("point-get", 0, scm_point_get);
    SDEF("point-set!", 1, scm_point_set_to);
    SDEF("buffer-length", 0, scm_buffer_length);
    SDEF("delete-range", 2, scm_delete_range);
    SDEF("char-at", 1, scm_char_at);
    SDEF("last-key-char", 0, scm_last_key_char);
    SDEF("%set-replace-mode!", 1, scm_set_replace_mode);
    SDEF("tab-next", 0, scm_tab_next);
    SDEF("tab-prev", 0, scm_tab_prev);
    SDEF("%tab-new!", 1, scm_tab_new);
    SDEF("reset-global-scale", 0, scm_reset_global_scale);
    SDEF("increase-global-scale", 0, scm_increase_global_scale);
    SDEF("decrease-global-scale", 0, scm_decrease_global_scale);
    SDEF("reset-buffer-scale", 0, scm_reset_buffer_scale);
    SDEF("increase-buffer-scale", 0, scm_increase_buffer_scale);
    SDEF("decrease-buffer-scale", 0, scm_decrease_buffer_scale);
    SDEF("split-vertical", 0, scm_split_vertical);
    SDEF("split-horizontal", 0, scm_split_horizontal);
    SDEF("pane-close", 0, scm_pane_close);
    SDEF("%pop-to-buffer", 1, scm_pop_to_buffer);
    SDEF("pane-navigate-up", 0, scm_pane_navigate_up);
    SDEF("pane-navigate-down", 0, scm_pane_navigate_down);
    SDEF("pane-navigate-left", 0, scm_pane_navigate_left);
    SDEF("pane-navigate-right", 0, scm_pane_navigate_right);
    SDEF("pane-v-split-increase", 0, scm_pane_v_split_increase);
    SDEF("pane-v-split-decrease", 0, scm_pane_v_split_decrease);
    SDEF("pane-h-split-increase", 0, scm_pane_h_split_increase);
    SDEF("pane-h-split-decrease", 0, scm_pane_h_split_decrease);
    SDEF("eval-buffer", 0, scm_eval_buffer);
    SDEF("clay-debug", 0, scm_clay_debug);
    SDEF("prefix-arg", 0, scm_prefix_arg);
    SDEF("%set-keymap-parent!", 2, scm_set_keymap_parent);
    SDEF("%set-keymap-name!", 2, scm_set_keymap_name);
    SDEF("%bind-prefix!", 3, scm_bind_prefix);
    SDEF("%read-key-binding", 1, scm_read_key_binding);
    SDEF("%set-mode-allows-input!", 2, scm_set_mode_allows_input);
    SDEF("ignore", 0, scm_ignore);
    SDEF("%buffer-has-minor-mode?", 1, scm_buffer_has_minor_mode);
    SDEF("%buffer-file-name", 0, scm_buffer_file_name);
    SDEF("%set-buffer-file-name!", 1, scm_set_buffer_file_name);
    SDEF("%buffer-write", 0, scm_buffer_write);
    SDEF("%buffer-modified?", 0, scm_buffer_modified_p);
    SDEF("%set-buffer-modified!", 1, scm_set_buffer_modified);
    SDEF("%buffer-set-name!", 1, scm_buffer_set_name);
    SDEF("%buffer-insert", 1, scm_buffer_insert);
    SDEF("%buffer-read", 0, scm_buffer_read);
    SDEF("%buffer-create", 1, scm_buffer_create);
    SDEF("%pane-set-buffer!", 1, scm_pane_set_buffer);

    // Mode primitives
    SDEF("%register-mode", 3, scm_register_mode);
    SDEF("%set-major-mode", 1, scm_set_major_mode);
    SDEF("%enable-minor-mode", 1, scm_enable_minor_mode);
    SDEF("%disable-minor-mode", 1, scm_disable_minor_mode);
    SDEF("%buffer-major-mode", 0, scm_buffer_major_mode);
    SDEF("%buffer-minor-modes", 0, scm_buffer_minor_modes);

    // Variable primitives
    SDEF("%set-local!", 2, scm_set_local);
    SDEF("%get-local", 2, scm_get_local);
    SDEF("%set-palette!", 2, scm_set_palette);
    SDEF("%update-icon-colors!", 0, scm_update_icon_colors);
    SDEF("%register-icon!", 3, scm_register_icon);
    SDEF("%register-mode-icon!", 6, scm_register_mode_icon);
    SDEF("%set-role!", 2, scm_set_role);
    SDEF("%clear-palette!", 0, scm_clear_palette);
    SDEF("%clear-roles!", 0, scm_clear_roles);

    // Change / undo primitives
    SDEF("%begin-change", 0, scm_begin_change);
    SDEF("%end-change", 0, scm_end_change);
    SDEF("%undo", 0, scm_undo);
    SDEF("%redo", 0, scm_redo);
    SDEF("%change-active?", 0, scm_change_active);
    SDEF("%change-set-repeat-info!", 1, scm_change_set_repeat_info);
    SDEF("%change-last-repeat-info", 0, scm_change_last_repeat_info);
    SDEF("%line-restore", 0, scm_line_restore);
    SDEF("%change-current-inserts", 0, scm_change_current_inserts);

    // Clipboard primitives
    SDEF("%clipboard-get",  0, scm_clipboard_get);
    SDEF("%clipboard-set!", 1, scm_clipboard_set);

    // Register primitives
    SDEF("%register-set!",       2, scm_register_set);
    SDEF("%register-append!",    2, scm_register_append);
    SDEF("%register-get",        1, scm_register_get);
    SDEF("%register-set-shape!",       2, scm_register_set_shape);
    SDEF("%register-shape",            1, scm_register_get_shape);
    SDEF("%register-set-block-width!", 2, scm_register_set_block_width);
    SDEF("%register-block-width",      1, scm_register_get_block_width);
    SDEF("%buffer-substring", 2, scm_buffer_substring);
    SDEF("%insert-string",    1, scm_insert_string);

    // Macro primitives
    SDEF("%macro-start!",     1, scm_macro_start);
    SDEF("%macro-stop!",      0, scm_macro_stop);
    SDEF("%macro-play",       1, scm_macro_play);
    SDEF("%macro-recording?", 0, scm_macro_is_recording);

    // Minibuffer primitives
    SDEF("%minibuffer-activate", 3, scm_minibuffer_activate);
    SDEF("%minibuffer-submit",   0, scm_minibuffer_submit);
    SDEF("%minibuffer-cancel",   0, scm_minibuffer_cancel);
    SDEF("%minibuffer-active?",  0, scm_minibuffer_activep);

    // Which-key primitives
    SDEF("%which-key-toggle", 0, scm_which_key_toggle);

    // Jump list primitives
    SDEF("%jump-push!",     0, scm_jump_push);
    SDEF("%jump-backward!", 0, scm_jump_backward);
    SDEF("%jump-forward!",  0, scm_jump_forward);

    // Mark / selection primitives
    SDEF("%mark-set-to-point!", 1, scm_mark_set_to_point);
    SDEF("%select-mode-set!", 1, scm_select_mode_set);
    SDEF("%select-mode-get", 0, scm_select_mode_get);
    SDEF("exchange-point-and-mark", 0, scm_swap_point_and_mark);
    SDEF("%point-to-mark!", 1, scm_point_to_named_mark);
    SDEF("%goto-line", 1, scm_goto_line);
    SDEF("%line-count", 0, scm_line_count);
    SDEF("%mark-position", 1, scm_mark_position);
    SDEF("%position-line", 1, scm_position_line);
    SDEF("%line-start-position", 1, scm_line_start_position);
    SDEF("%line-end-position", 1, scm_line_end_position);

    // Resolve scheme resource path
    #ifdef __EMSCRIPTEN__
    #define RESOURCES_PATH "/resources/"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char resourcesPath[1024];
    snprintf(resourcesPath, sizeof(resourcesPath), "%sscheme/", basePath);
    #define RESOURCES_PATH resourcesPath
    #endif

    // Add scheme dir to module search path so Chibi finds editor/*.sld
    {
        sexp_gc_var2(dir_str, new_path);
        sexp_gc_preserve2(ctx, dir_str, new_path);
        dir_str = sexp_c_string(ctx, RESOURCES_PATH, -1);
        new_path = sexp_cons(ctx, dir_str, sexp_global(ctx, SEXP_G_MODULE_PATH));
        sexp_global(ctx, SEXP_G_MODULE_PATH) = new_path;
        sexp_gc_release2(ctx);
    }

    // Register (editor primitives) module — makes all C bindings
    // available to .sld libraries via (import (editor primitives))
    sexp meta = sexp_global(ctx, SEXP_G_META_ENV);
    sexp_env_define(ctx, meta, sexp_intern(ctx, "%editor-env", -1), env);

    sexp result;
    result = sexp_eval_string(ctx,
        "(add-module! '(editor primitives) "
        "(make-module "
        "'(quit message message-clear message-lock message-unlock "
        "make-keymap %set-key! insert-char self-insert delete-char "
        "move-point move-point-by-line next-line prev-line "
        "forward-char backward-char newline insert-tab "
        "delete-backward-char delete-forward-char set-column "
        "line-start line-end skip-whitespace char-at-point "
        "point-get point-set! buffer-length delete-range char-at "
        "last-key-char %set-replace-mode! tab-next tab-prev %tab-new! "
        "reset-global-scale increase-global-scale decrease-global-scale "
        "reset-buffer-scale increase-buffer-scale decrease-buffer-scale "
        "split-vertical split-horizontal pane-close %pop-to-buffer "
        "pane-navigate-up pane-navigate-down "
        "pane-navigate-left pane-navigate-right "
        "pane-v-split-increase pane-v-split-decrease "
        "pane-h-split-increase pane-h-split-decrease "
        "eval-buffer clay-debug prefix-arg "
        "%set-keymap-parent! %set-keymap-name! %bind-prefix! "
        "%read-key-binding %set-mode-allows-input! ignore "
        "%buffer-has-minor-mode? "
        "%buffer-file-name %set-buffer-file-name! %buffer-write "
        "%buffer-modified? %set-buffer-modified! %buffer-set-name! "
        "%buffer-insert %buffer-read %buffer-create %pane-set-buffer! "
        "%register-mode %set-major-mode "
        "%enable-minor-mode %disable-minor-mode "
        "%buffer-major-mode %buffer-minor-modes "
        "%set-local! %get-local %set-palette! "
        "%update-icon-colors! %register-icon! "
        "%register-mode-icon! %set-role! "
        "%clear-palette! %clear-roles! "
        "%begin-change %end-change %undo %redo "
        "%change-active? %change-set-repeat-info! %change-last-repeat-info "
        "%line-restore %change-current-inserts "
        "%mark-set-to-point! %select-mode-set! %select-mode-get "
        "exchange-point-and-mark %point-to-mark! %goto-line "
        "%line-count %mark-position %position-line "
        "%line-start-position %line-end-position "
        "%clipboard-get %clipboard-set! "
        "%register-set! %register-append! %register-get "
        "%register-set-shape! %register-shape "
        "%register-set-block-width! %register-block-width "
        "%buffer-substring %insert-string "
        "%macro-start! %macro-stop! %macro-play %macro-recording? "
        "%minibuffer-activate %minibuffer-submit "
        "%minibuffer-cancel %minibuffer-active? "
        "%which-key-toggle "
        "%jump-push! %jump-backward! %jump-forward! "
        "global-keymap eval) "
        "%editor-env '()))",
        -1, meta);
    if (sexp_exceptionp(result)) {
        fprintf(stderr, "ERROR: failed to register (editor primitives)\n");
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }

    // Cache role symbols for fast lookup
    #define INTERN_ROLE(field, name) do { \
        state->ui.roles.field = sexp_intern(ctx, name, -1); \
        sexp_preserve_object(ctx, state->ui.roles.field); \
    } while(0)

    INTERN_ROLE(ui_bg, "ui.bg");
    INTERN_ROLE(bar_bg, "bar.bg");
    INTERN_ROLE(bar_text_active, "bar.text.active");
    INTERN_ROLE(tab_bar, "tab.bar");
    INTERN_ROLE(tab_active, "tab.active");
    INTERN_ROLE(tab_hover, "tab.hover");
    INTERN_ROLE(tab_inactive, "tab.inactive");
    INTERN_ROLE(text_primary, "text.primary");
    INTERN_ROLE(text_faded, "text.faded");
    INTERN_ROLE(selection, "selection");

    #undef INTERN_ROLE

    // Load init.scm — its (import ...) forms trigger library loading
    #define LOAD_SCRIPT(name) do { \
        char path_[1024]; \
        snprintf(path_, sizeof(path_), "%s" name ".scm", RESOURCES_PATH); \
        result = sexp_load(ctx, sexp_c_string(ctx, path_, -1), env); \
        if (sexp_exceptionp(result)) \
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx)); \
    } while(0)

    LOAD_SCRIPT("init");

    #undef LOAD_SCRIPT

    // Get call-interactively procedure (imported from (editor command) by init.scm)
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
