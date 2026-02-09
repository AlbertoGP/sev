#include <SDL3/SDL_events.h>
#include <chibi/eval.h>
#include <chibi/sexp.h>

#include "scheme.h"
#include "mode.h"
#include "../command/keymap.h"
#include "../text/buffer.h"
#include "../text/var.h"
#include "../text/message.h"
#include "../display/tab.h"
#include "../display/scale.h"
#include "../display/icon.h"
#include "../display/mode_icon.h"

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

#define SCM_CMD(cname, action) \
    static sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        action; \
        return SEXP_VOID; \
    }

SCM_CMD(scm_next_line,           point_move_by_line(1))
SCM_CMD(scm_prev_line,           point_move_by_line(-1))

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

SCM_CMD(scm_newline,             (insert_char(buffer_get_current(), '\n'), message_clear()))
SCM_CMD(scm_insert_tab,          (insert_char(buffer_get_current(), '\t'), message_clear()))

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

SCM_CMD(scm_point_to_line_start, point_to_line_start(buffer_get_current()))
SCM_CMD(scm_point_to_line_end,   point_to_line_end(buffer_get_current()))

static sexp scm_skip_whitespace(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    while (char_at_point() == ' ' || char_at_point() == '\t')
        point_move(1);
    return SEXP_VOID;
}

#define SCM_PANE_NAV(cname, func, msg) \
    static sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        message_clear(); \
        if (func()) message_send(msg); \
        return SEXP_VOID; \
    }

SCM_PANE_NAV(scm_tab_next, tab_next, "tab-next")
SCM_PANE_NAV(scm_tab_prev, tab_prev, "tab-prev")
SCM_PANE_NAV(scm_pane_navigate_up,    pane_navigate_up,    "pane-navigate-up")
SCM_PANE_NAV(scm_pane_navigate_down,  pane_navigate_down,  "pane-navigate-down")
SCM_PANE_NAV(scm_pane_navigate_left,  pane_navigate_left,  "pane-navigate-left")
SCM_PANE_NAV(scm_pane_navigate_right, pane_navigate_right, "pane-navigate-right")

#define SCM_GLOBAL_SCALE(cname, func, label) \
    static sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        func(G); \
        message_send(label); \
        return SEXP_VOID; \
    }

SCM_GLOBAL_SCALE(scm_reset_global_scale,    reset_scale,    "reset-global-scale")
SCM_GLOBAL_SCALE(scm_increase_global_scale, increase_scale, "increase-global-scale")
SCM_GLOBAL_SCALE(scm_decrease_global_scale, decrease_scale, "decrease-global-scale")

static sexp scm_reset_buffer_scale(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    reset_buffer_scale(G, buffer_get_current());

    message_send("reset-buffer-scale");
    return SEXP_VOID;
}

#define SCM_BUFFER_SCALE(cname, func, label) \
    static sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        func(buffer_get_current()); \
        message_send(label); \
        return SEXP_VOID; \
    }

SCM_BUFFER_SCALE(scm_increase_buffer_scale, increase_buffer_scale, "increase-buffer-scale")
SCM_BUFFER_SCALE(scm_decrease_buffer_scale, decrease_buffer_scale, "decrease-buffer-scale")

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

#define SCM_PANE_RESIZE(cname, func, msg) \
    static sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        G->needs_extra_frame = true; \
        message_clear(); \
        if (func()) message_send(msg); \
        return SEXP_VOID; \
    }

SCM_PANE_RESIZE(scm_pane_v_split_increase, pane_v_split_increase, "pane-v-split-increase")
SCM_PANE_RESIZE(scm_pane_v_split_decrease, pane_v_split_decrease, "pane-v-split-decrease")
SCM_PANE_RESIZE(scm_pane_h_split_increase, pane_h_split_increase, "pane-h-split-increase")
SCM_PANE_RESIZE(scm_pane_h_split_decrease, pane_h_split_decrease, "pane-h-split-decrease")

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

// (%set-keymap-default! keymap command-sym) -> #t
static sexp scm_set_keymap_default(sexp ctx, sexp self, sexp n,
                                   sexp skeymap, sexp scommand) {
    Keymap *km = sexp_cpointer_value(skeymap);
    if (!km) return sexp_user_exception(ctx, self, "null keymap", skeymap);
    if (!sexp_symbolp(scommand))
        return sexp_user_exception(ctx, self, "command must be symbol", scommand);

    if (!km->default_binding)
        km->default_binding = malloc(sizeof(Binding));
    km->default_binding->type = BINDING_COMMAND;
    km->default_binding->command_sym = scommand;
    sexp_preserve_object(ctx, scommand);
    return SEXP_TRUE;
}

// (ignore) - do nothing
static sexp scm_ignore(sexp ctx, sexp self, sexp n) {
    return SEXP_VOID;
}

// (%buffer-has-minor-mode? name) -> #t or #f
static sexp scm_buffer_has_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "name must be symbol", sname);
    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    return buffer_has_minor_mode(buffer_get_current(), name) ? SEXP_TRUE : SEXP_FALSE;
}

// Mode primitives

// (%register-mode name type keymap) -> mode-ptr or #f
// type: 'major or 'minor
static sexp scm_register_mode(sexp ctx, sexp self, sexp n,
                              sexp sname, sexp stype, sexp skeymap) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }
    if (!sexp_symbolp(stype)) {
        return sexp_user_exception(ctx, self, "type must be 'major or 'minor", stype);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *type_str = sexp_string_data(sexp_symbol_to_string(ctx, stype));

    ModeType type;
    if (strcmp(type_str, "major") == 0) {
        type = MODE_MAJOR;
    } else if (strcmp(type_str, "minor") == 0) {
        type = MODE_MINOR;
    } else {
        return sexp_user_exception(ctx, self, "type must be 'major or 'minor", stype);
    }

    Keymap *km = NULL;
    if (sexp_cpointerp(skeymap)) {
        km = sexp_cpointer_value(skeymap);
    }

    Mode *mode = mode_create(name, type, km);
    if (!mode) return SEXP_FALSE;

    if (!mode_register(mode)) {
        free((void *)mode->name);
        free(mode);
        return SEXP_FALSE;
    }

    return sexp_make_cpointer(ctx, SEXP_CPOINTER, mode, SEXP_FALSE, 0);
}

// (%set-major-mode name) -> #t or #f
static sexp scm_set_major_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MAJOR);
    if (!mode) return SEXP_FALSE;

    buffer_set_major_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%enable-minor-mode name) -> #t or #f
static sexp scm_enable_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MINOR);
    if (!mode) return SEXP_FALSE;

    buffer_enable_minor_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%disable-minor-mode name) -> #t or #f
static sexp scm_disable_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MINOR);
    if (!mode) return SEXP_FALSE;

    buffer_disable_minor_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%buffer-major-mode) -> symbol or #f
static sexp scm_buffer_major_mode(sexp ctx, sexp self, sexp n) {
    Mode *mode = buffer_get_major_mode(buffer_get_current());
    if (!mode) return SEXP_FALSE;

    return sexp_intern(ctx, mode->name, -1);
}

// (%buffer-minor-modes) -> list of symbols
static sexp scm_buffer_minor_modes(sexp ctx, sexp self, sexp n) {
    ModeList *minors = buffer_get_minor_modes(buffer_get_current());
    if (!minors) return SEXP_NULL;

    sexp result = SEXP_NULL;
    sexp_gc_var1(sym);
    sexp_gc_preserve1(ctx, sym);

    // Build list in reverse (so head of ModeList is head of result list)
    for (ModeListNode *node = minors->head; node; node = node->next) {
        sym = sexp_intern(ctx, node->mode->name, -1);
        result = sexp_cons(ctx, sym, result);
    }

    // Reverse to maintain order
    sexp reversed = SEXP_NULL;
    while (sexp_pairp(result)) {
        sexp tmp = sexp_cdr(result);
        sexp_cdr(result) = reversed;
        reversed = result;
        result = tmp;
    }

    sexp_gc_release1(ctx);
    return reversed;
}

// Variable primitives

// (%setq-local name val) -> val
static sexp scm_set_local(sexp ctx, sexp self, sexp n, sexp key, sexp sval) {
    if (!sexp_symbolp(key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", key);
    }

    VarTable *locals = buffer_get_locals(buffer_get_current());
    if (!locals) return SEXP_FALSE;

    sexp_preserve_object(ctx, sval);
    vartable_set(locals, key, sval);
    return sval;
}

// (%getq-local name default) -> value or default
static sexp scm_get_local(sexp ctx, sexp self, sexp n, sexp key, sexp sdefault) {
    if (!sexp_symbolp(key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", key);
    }

    VarTable *locals = buffer_get_locals(buffer_get_current());
    if (!locals) return sdefault;

    return vartable_get(locals, key, sdefault);
}

static sexp scm_update_icon_colors(sexp ctx, sexp self, sexp n) {
    icons_update_colors(G);
    return SEXP_VOID;
}

// (%register-icon! name filename color-role)
static sexp scm_register_icon(sexp ctx, sexp self, sexp n,
                               sexp sname, sexp sfilename, sexp scolor_role) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    if (!sexp_stringp(sfilename))
        return sexp_user_exception(ctx, self, "filename must be a string", sfilename);
    if (!sexp_symbolp(scolor_role))
        return sexp_user_exception(ctx, self, "color-role must be a symbol", scolor_role);

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *filename = sexp_string_data(sfilename);

    sexp color_role = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, scolor_role)), -1);
    sexp_preserve_object(ctx, color_role);

    if (!icon_register(name, filename, color_role))
        return SEXP_FALSE;

    return SEXP_TRUE;
}

// (%register-mode-icon! mode-name filename role-bg role-label role-cursor cursor-type-sym)
static sexp scm_register_mode_icon(sexp ctx, sexp self, sexp n,
                                   sexp sname, sexp sfilename,
                                   sexp srole_bg, sexp srole_label,
                                   sexp srole_cursor, sexp scursor_type) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "mode-name must be a symbol", sname);
    if (!sexp_stringp(sfilename))
        return sexp_user_exception(ctx, self, "filename must be a string", sfilename);
    if (!sexp_symbolp(srole_bg))
        return sexp_user_exception(ctx, self, "role-bg must be a symbol", srole_bg);
    if (!sexp_symbolp(srole_label))
        return sexp_user_exception(ctx, self, "role-label must be a symbol", srole_label);
    if (!sexp_symbolp(srole_cursor))
        return sexp_user_exception(ctx, self, "role-cursor must be a symbol", srole_cursor);
    if (!sexp_symbolp(scursor_type))
        return sexp_user_exception(ctx, self, "cursor-type must be a symbol", scursor_type);

    const char *mode_name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *filename = sexp_string_data(sfilename);

    // Intern role symbols so they persist
    sexp role_bg = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_bg)), -1);
    sexp role_label = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_label)), -1);
    sexp role_cursor = sexp_intern(ctx, sexp_string_data(sexp_symbol_to_string(ctx, srole_cursor)), -1);
    sexp_preserve_object(ctx, role_bg);
    sexp_preserve_object(ctx, role_label);
    sexp_preserve_object(ctx, role_cursor);

    // Parse cursor type symbol
    const char *ct_str = sexp_string_data(sexp_symbol_to_string(ctx, scursor_type));
    CursorType ct = CURSOR_SOLID;
    if (strcmp(ct_str, "thin") == 0) ct = CURSOR_THIN;
    else if (strcmp(ct_str, "hollow") == 0) ct = CURSOR_HOLLOW;
    else if (strcmp(ct_str, "under") == 0) ct = CURSOR_UNDER;

    if (!mode_icon_register(mode_name, filename, role_bg, role_label, role_cursor, ct))
        return SEXP_FALSE;

    return SEXP_TRUE;
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

static sexp scm_message_send(sexp ctx, sexp self, sexp n, sexp message) {
    G->needs_redraw = true;

    if (sexp_stringp(message)) {
        message_send(sexp_string_data(message));
        return SEXP_VOID;
    } else {
        return sexp_type_exception(ctx, self, SEXP_STRING, message);
    }
}

static sexp scm_message_clear(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;

    message_clear();
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
