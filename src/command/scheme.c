#include <SDL3/SDL_events.h>
#include <chibi/eval.h>
#include <chibi/sexp.h>

#include "scheme.h"
#include "scheme_bindings.h"
#include "../text/buffer.h"
#include "../text/message.h"
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

    #define SDEF(a, b, c) sexp_define_foreign(ctx, env, a, b, c)

    // Register foreign functions
    SDEF("quit", 0, scm_quit);
    SDEF("message", 1, scm_message_send);
    SDEF("message-clear", 0, scm_message_clear);
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
    SDEF("tab-next", 0, scm_tab_next);
    SDEF("tab-prev", 0, scm_tab_prev);
    SDEF("reset-global-scale", 0, scm_reset_global_scale);
    SDEF("increase-global-scale", 0, scm_increase_global_scale);
    SDEF("decrease-global-scale", 0, scm_decrease_global_scale);
    SDEF("reset-buffer-scale", 0, scm_reset_buffer_scale);
    SDEF("increase-buffer-scale", 0, scm_increase_buffer_scale);
    SDEF("decrease-buffer-scale", 0, scm_decrease_buffer_scale);
    SDEF("split-vertical", 0, scm_split_vertical);
    SDEF("split-horizontal", 0, scm_split_horizontal);
    SDEF("pane-close", 0, scm_pane_close);
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
    SDEF("%set-keymap-default!", 2, scm_set_keymap_default);
    SDEF("ignore", 0, scm_ignore);
    SDEF("%buffer-has-minor-mode?", 1, scm_buffer_has_minor_mode);

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

    #ifdef __EMSCRIPTEN__
    #define RESOURCES_PATH "/resources/"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char resourcesPath[1024];
    snprintf(resourcesPath, sizeof(resourcesPath), "%sscheme/", basePath);
    #define RESOURCES_PATH resourcesPath
    #endif

    #define LOAD_SCRIPT(name) do { \
        char path_[1024]; \
        snprintf(path_, sizeof(path_), "%s" name ".scm", RESOURCES_PATH); \
        result = sexp_load(ctx, sexp_c_string(ctx, path_, -1), env); \
        if (sexp_exceptionp(result)) \
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx)); \
    } while(0)

    sexp result;
    LOAD_SCRIPT("command");
    LOAD_SCRIPT("mode");
    LOAD_SCRIPT("icon");
    LOAD_SCRIPT("built-in");
    LOAD_SCRIPT("evil");

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

    #undef INTERN_ROLE

    LOAD_SCRIPT("theme");
    LOAD_SCRIPT("init");

    #undef LOAD_SCRIPT

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
