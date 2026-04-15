// Boots a chibi context sufficient to load (editor vim) and run scheme-
// side tests against real buffer state. Real implementations for the
// point/buffer/mark/change/var bindings come from src/text/*.c (already
// linked in the test target). Everything the editor normally supplies
// from src/command or src/display is stubbed here to a no-op so the
// library graph loads without dragging in SDL, panes, keymaps, etc.

#include <stdio.h>
#include <stdlib.h>

#include <chibi/eval.h>
#include <chibi/sexp.h>

#include "scheme_test_init.h"

#include "../src/state.h"
#include "../src/command/scheme_bindings.h"
#include "../src/text/buffer.h"

extern AppState *G;

// ── Stubs for command/display primitives ─────────────────────────────────────
//
// The real editor registers ~120 FFI primitives. Tests only exercise point
// and buffer primitives (provided by src/text/*.c), so the rest are stubbed
// here. Stubs need to match chibi's foreign-procedure arity — one stub per
// arity we register.

#define STUB0(name) \
    static sexp name(sexp ctx, sexp self, sexp n) { \
        (void)ctx; (void)self; (void)n; return SEXP_VOID; }
#define STUB1(name) \
    static sexp name(sexp ctx, sexp self, sexp n, sexp a) { \
        (void)ctx; (void)self; (void)n; (void)a; return SEXP_VOID; }
#define STUB2(name) \
    static sexp name(sexp ctx, sexp self, sexp n, sexp a, sexp b) { \
        (void)ctx; (void)self; (void)n; (void)a; (void)b; return SEXP_VOID; }
#define STUB3(name) \
    static sexp name(sexp ctx, sexp self, sexp n, sexp a, sexp b, sexp c) { \
        (void)ctx; (void)self; (void)n; (void)a; (void)b; (void)c; return SEXP_VOID; }
#define STUB6(name) \
    static sexp name(sexp ctx, sexp self, sexp n, sexp a, sexp b, sexp c, sexp d, sexp e, sexp f) { \
        (void)ctx; (void)self; (void)n; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; \
        return SEXP_VOID; }

STUB0(stub_quit)
STUB1(stub_message)
STUB1(stub_message_echo)
STUB0(stub_message_clear)
STUB0(stub_message_lock)
STUB0(stub_message_unlock)
STUB0(stub_make_keymap_ret)
static sexp stub_make_keymap(sexp ctx, sexp self, sexp n) {
    (void)ctx; (void)self; (void)n;
    // Return a unique dummy cpointer so scheme values compare distinctly.
    static int slot;
    return sexp_make_cpointer(ctx, SEXP_CPOINTER, &slot, SEXP_FALSE, 0);
}
STUB3(stub_set_key)
STUB2(stub_set_keymap_parent)
STUB2(stub_set_keymap_name)
STUB3(stub_bind_prefix)
STUB1(stub_read_key_binding)
STUB1(stub_set_key_unbound_cb)
STUB2(stub_set_mode_allows_input)
static sexp stub_no_panes_p(sexp ctx, sexp self, sexp n) {
    (void)ctx; (void)self; (void)n; return SEXP_FALSE;
}
STUB0(stub_tab_next)
STUB0(stub_tab_prev)
STUB1(stub_tab_new)
STUB1(stub_tab_new_fresh)
STUB1(stub_tab_set_buffer)
STUB1(stub_tab_set_preview)
STUB0(stub_tab_close)
STUB1(stub_pop_to_buffer)
STUB0(stub_pane_up)
STUB0(stub_pane_down)
STUB0(stub_pane_left)
STUB0(stub_pane_right)
STUB0(stub_pane_vinc)
STUB0(stub_pane_vdec)
STUB0(stub_pane_hinc)
STUB0(stub_pane_hdec)
STUB0(stub_split_v)
STUB0(stub_split_h)
STUB0(stub_reset_global_scale)
STUB0(stub_inc_global_scale)
STUB0(stub_dec_global_scale)
STUB0(stub_reset_buffer_scale)
STUB0(stub_inc_buffer_scale)
STUB0(stub_dec_buffer_scale)
STUB0(stub_eval_buffer)
STUB0(stub_ts_tree_string)
STUB0(stub_ts_enable)
STUB0(stub_ts_disable)
STUB0(stub_clay_debug)
STUB0(stub_prefix_arg)
STUB0(stub_ignore)
static sexp stub_buffer_has_minor_mode_p(sexp ctx, sexp self, sexp n, sexp a) {
    (void)ctx; (void)self; (void)n; (void)a; return SEXP_FALSE;
}
STUB0(stub_buffer_file_name)
STUB1(stub_set_buffer_file_name)
STUB0(stub_buffer_write)
STUB0(stub_buffer_modified_p)
STUB1(stub_set_buffer_modified)
STUB1(stub_buffer_set_name)
STUB1(stub_buffer_insert_stub)
STUB0(stub_buffer_read_stub)
STUB1(stub_buffer_create_stub)
STUB1(stub_buffer_close_stub)
STUB1(stub_set_welcome_keymap)
STUB3(stub_register_mode)
STUB1(stub_set_major_mode)
STUB1(stub_enable_minor_mode)
STUB1(stub_disable_minor_mode)
STUB0(stub_buffer_major_mode)
static sexp stub_buffer_minor_modes(sexp ctx, sexp self, sexp n) {
    (void)ctx; (void)self; (void)n; return SEXP_NULL;
}
STUB2(stub_set_palette)
STUB0(stub_update_icon_colors)
STUB3(stub_register_icon)
STUB6(stub_register_mode_icon)
STUB3(stub_register_major_mode_info)
STUB1(stub_set_cursor_override)
STUB2(stub_set_role)
STUB0(stub_clear_palette)
STUB0(stub_clear_roles)
STUB0(stub_clipboard_get)
STUB1(stub_clipboard_set)
STUB1(stub_macro_start)
STUB0(stub_macro_stop)
STUB1(stub_macro_play)
static sexp stub_macro_recording_p(sexp ctx, sexp self, sexp n) {
    (void)ctx; (void)self; (void)n; return SEXP_FALSE;
}
STUB3(stub_minibuffer_activate)
STUB0(stub_minibuffer_submit)
STUB0(stub_minibuffer_cancel)
static sexp stub_minibuffer_active_p(sexp ctx, sexp self, sexp n) {
    (void)ctx; (void)self; (void)n; return SEXP_FALSE;
}
STUB0(stub_mb_act_commands)
STUB0(stub_mb_act_desc_func)
STUB0(stub_mb_act_desc_cmd)
STUB0(stub_mb_act_desc_var)
STUB0(stub_mb_act_themes)
STUB0(stub_mb_act_major_modes)
STUB0(stub_mb_select_next)
STUB0(stub_mb_select_prev)
STUB0(stub_which_key_toggle)
STUB1(stub_record_command_usage)
STUB1(stub_update_recent_project)
STUB1(stub_chdir)
STUB1(stub_open_recent_project)
STUB0(stub_jump_push)
STUB0(stub_jump_backward)
STUB0(stub_jump_forward)
STUB1(stub_mouse_click)
STUB1(stub_mouse_drag)

// ── Test-only primitive: reset the current buffer to a known string ──────────

static sexp scm_test_reset_buffer(sexp ctx, sexp self, sexp n, sexp stext) {
    (void)self; (void)n;
    if (!sexp_stringp(stext))
        return sexp_user_exception(ctx, self, "expected string", stext);
    Buffer *buf = buffer_get_current();
    if (!buf) return sexp_user_exception(ctx, self, "no current buffer", SEXP_FALSE);
    buffer_clear(buf);
    insert_string(buf, sexp_string_data(stext));
    point_set((Location){.pos = 0});
    return SEXP_VOID;
}

// ── Init / teardown ──────────────────────────────────────────────────────────

static Buffer *test_buf = NULL;

void scheme_test_init(void) {
    sexp_scheme_init();

    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 64*1024*1024);
    if (!ctx || sexp_exceptionp(ctx)) {
        fprintf(stderr, "scheme_test_init: failed to create ctx\n");
        exit(1);
    }
    sexp env = sexp_load_standard_env(ctx, NULL, SEXP_SEVEN);
    if (sexp_exceptionp(env)) {
        fprintf(stderr, "scheme_test_init: failed to load standard env\n");
        sexp_print_exception(ctx, env, sexp_current_error_port(ctx));
        exit(1);
    }
    sexp_load_standard_ports(ctx, env, stdin, stdout, stderr, 0);

    sexp_eval_string(ctx,
        "(import (except (scheme base) equal? let-syntax letrec-syntax))",
        -1, env);

    G->chibi.ctx = ctx;
    G->chibi.env = env;

    // global-keymap / pane-keymap placeholders — (editor vim) references them
    sexp_env_define(ctx, env, sexp_intern(ctx, "global-keymap", -1), SEXP_FALSE);
    sexp_env_define(ctx, env, sexp_intern(ctx, "pane-keymap",   -1), SEXP_FALSE);

    #define SDEF(a, b, c) sexp_define_foreign(ctx, env, a, b, (sexp_proc1)c)
    #define SDEF_OPT(a, b, c) \
        sexp_define_foreign_opt(ctx, env, a, b, (sexp_proc1)c, SEXP_FALSE)

    // Real bindings from src/text/*.c ────────────────────────────────────────
    SDEF("move-point",          1, scm_point_move);
    SDEF("move-point-by-line",  1, scm_point_move_by_line);
    SDEF("next-line",           0, scm_next_line);
    SDEF("prev-line",           0, scm_prev_line);
    SDEF("forward-char",        0, scm_forward_char);
    SDEF("backward-char",       0, scm_backward_char);
    SDEF("line-start",          0, scm_point_to_line_start);
    SDEF("line-end",            0, scm_point_to_line_end);
    SDEF("skip-whitespace",     0, scm_skip_whitespace);
    SDEF("set-column",          1, scm_set_column);
    SDEF("char-at-point",       0, scm_char_at_point);
    SDEF("point-get",           0, scm_point_get);
    SDEF("point-set!",          1, scm_point_set_to);
    SDEF("buffer-length",       0, scm_buffer_length);
    SDEF("char-at",             1, scm_char_at);
    SDEF("last-key-char",       0, scm_last_key_char);
    SDEF("%goto-line",          1, scm_goto_line);
    SDEF("%line-count",         0, scm_line_count);
    SDEF("%position-line",      1, scm_position_line);
    SDEF("%line-start-position",1, scm_line_start_position);
    SDEF("%line-end-position",  1, scm_line_end_position);
    SDEF("jump-to-matching-bracket", 0, scm_jump_to_matching_bracket);

    SDEF("insert-char",         1, scm_insert_char);
    SDEF("self-insert",         0, scm_self_insert);
    SDEF("delete-char",         1, scm_delete_char);
    SDEF("delete-backward-char",0, scm_delete_backward_char);
    SDEF("delete-forward-char", 0, scm_delete_forward_char);
    SDEF("newline",             0, scm_newline);
    SDEF("insert-tab",          0, scm_insert_tab);
    SDEF("delete-range",        2, scm_delete_range);
    SDEF("%set-replace-mode!",  1, scm_set_replace_mode);
    SDEF("%insert-string",      1, scm_insert_string);
    SDEF("%buffer-substring",   2, scm_buffer_substring);

    SDEF("%begin-change",       0, scm_begin_change);
    SDEF("%end-change",         0, scm_end_change);
    SDEF("%undo",               0, scm_undo);
    SDEF("%redo",               0, scm_redo);
    SDEF("%change-active?",     0, scm_change_active);
    SDEF("%change-set-repeat-info!", 1, scm_change_set_repeat_info);
    SDEF("%change-last-repeat-info", 0, scm_change_last_repeat_info);
    SDEF("%line-restore",       0, scm_line_restore);
    SDEF("%change-current-inserts", 0, scm_change_current_inserts);

    SDEF("%mark-set-to-point!", 1, scm_mark_set_to_point);
    SDEF("%select-mode-set!",   1, scm_select_mode_set);
    SDEF("%select-mode-get",    0, scm_select_mode_get);
    SDEF("exchange-point-and-mark", 0, scm_swap_point_and_mark);
    SDEF("%point-to-mark!",     1, scm_point_to_named_mark);
    SDEF("%mark-position",      1, scm_mark_position);

    SDEF("%register-set!",      2, scm_register_set);
    SDEF("%register-append!",   2, scm_register_append);
    SDEF("%register-get",       1, scm_register_get);
    SDEF("%register-set-shape!",2, scm_register_set_shape);
    SDEF("%register-shape",     1, scm_register_get_shape);
    SDEF("%register-set-block-width!", 2, scm_register_set_block_width);
    SDEF("%register-block-width",      1, scm_register_get_block_width);

    SDEF("%set-local!",         2, scm_set_local);
    SDEF("%get-local",          2, scm_get_local);

    SDEF("%ts-tree-string",     0, stub_ts_tree_string);
    SDEF("%ts-enable!",         0, stub_ts_enable);
    SDEF("%ts-disable!",        0, stub_ts_disable);

    // Stubbed bindings ───────────────────────────────────────────────────────
    SDEF("quit",                0, stub_quit);
    SDEF_OPT("message",         1, stub_message);
    SDEF("message-echo",        1, stub_message_echo);
    SDEF("message-clear",       0, stub_message_clear);
    SDEF("message-lock",        0, stub_message_lock);
    SDEF("message-unlock",      0, stub_message_unlock);
    SDEF("make-keymap",         0, stub_make_keymap);
    SDEF("%set-key!",           3, stub_set_key);
    SDEF("%set-keymap-parent!", 2, stub_set_keymap_parent);
    SDEF("%set-keymap-name!",   2, stub_set_keymap_name);
    SDEF("%bind-prefix!",       3, stub_bind_prefix);
    SDEF("%read-key-binding",   1, stub_read_key_binding);
    SDEF("%set-key-unbound-cb!",1, stub_set_key_unbound_cb);
    SDEF("%set-mode-allows-input!", 2, stub_set_mode_allows_input);
    SDEF("no-panes?",           0, stub_no_panes_p);
    SDEF("%tab-next",           0, stub_tab_next);
    SDEF("%tab-prev",           0, stub_tab_prev);
    SDEF("%tab-new!",           1, stub_tab_new);
    SDEF("%tab-new-fresh!",     1, stub_tab_new_fresh);
    SDEF("%tab-set-buffer!",    1, stub_tab_set_buffer);
    SDEF("%tab-set-preview!",   1, stub_tab_set_preview);
    SDEF("%tab-close",          0, stub_tab_close);
    SDEF("%pop-to-buffer",      1, stub_pop_to_buffer);
    SDEF("%pane-navigate-up",   0, stub_pane_up);
    SDEF("%pane-navigate-down", 0, stub_pane_down);
    SDEF("%pane-navigate-left", 0, stub_pane_left);
    SDEF("%pane-navigate-right",0, stub_pane_right);
    SDEF("%pane-v-split-increase", 0, stub_pane_vinc);
    SDEF("%pane-v-split-decrease", 0, stub_pane_vdec);
    SDEF("%pane-h-split-increase", 0, stub_pane_hinc);
    SDEF("%pane-h-split-decrease", 0, stub_pane_hdec);
    SDEF("%split-vertical",     0, stub_split_v);
    SDEF("%split-horizontal",   0, stub_split_h);
    SDEF("reset-global-scale",     0, stub_reset_global_scale);
    SDEF("increase-global-scale",  0, stub_inc_global_scale);
    SDEF("decrease-global-scale",  0, stub_dec_global_scale);
    SDEF("%reset-buffer-scale",    0, stub_reset_buffer_scale);
    SDEF("%increase-buffer-scale", 0, stub_inc_buffer_scale);
    SDEF("%decrease-buffer-scale", 0, stub_dec_buffer_scale);
    SDEF("eval-buffer",         0, stub_eval_buffer);
    SDEF("clay-debug",          0, stub_clay_debug);
    SDEF("prefix-arg",          0, stub_prefix_arg);
    SDEF("ignore",              0, stub_ignore);
    SDEF("%buffer-has-minor-mode?", 1, stub_buffer_has_minor_mode_p);
    SDEF("%buffer-file-name",   0, stub_buffer_file_name);
    SDEF("%set-buffer-file-name!", 1, stub_set_buffer_file_name);
    SDEF("%buffer-write",       0, stub_buffer_write);
    SDEF("%buffer-modified?",   0, stub_buffer_modified_p);
    SDEF("%set-buffer-modified!", 1, stub_set_buffer_modified);
    SDEF("%buffer-set-name!",   1, stub_buffer_set_name);
    SDEF("%buffer-insert",      1, stub_buffer_insert_stub);
    SDEF("%buffer-read",        0, stub_buffer_read_stub);
    SDEF("%buffer-create",      1, stub_buffer_create_stub);
    SDEF("%buffer-close!",      1, stub_buffer_close_stub);
    SDEF("%set-welcome-keymap!",1, stub_set_welcome_keymap);
    SDEF("%register-mode",      3, stub_register_mode);
    SDEF("%set-major-mode",     1, stub_set_major_mode);
    SDEF("%enable-minor-mode",  1, stub_enable_minor_mode);
    SDEF("%disable-minor-mode", 1, stub_disable_minor_mode);
    SDEF("%buffer-major-mode",  0, stub_buffer_major_mode);
    SDEF("%buffer-minor-modes", 0, stub_buffer_minor_modes);
    SDEF("%set-palette!",       2, stub_set_palette);
    SDEF("%update-icon-colors!",0, stub_update_icon_colors);
    SDEF("%register-icon!",     3, stub_register_icon);
    SDEF("%register-mode-icon!",6, stub_register_mode_icon);
    SDEF("%register-major-mode-info!", 3, stub_register_major_mode_info);
    SDEF("%set-cursor-override!", 1, stub_set_cursor_override);
    SDEF("%set-role!",          2, stub_set_role);
    SDEF("%clear-palette!",     0, stub_clear_palette);
    SDEF("%clear-roles!",       0, stub_clear_roles);
    SDEF("%clipboard-get",      0, stub_clipboard_get);
    SDEF("%clipboard-set!",     1, stub_clipboard_set);
    SDEF("%macro-start!",       1, stub_macro_start);
    SDEF("%macro-stop!",        0, stub_macro_stop);
    SDEF("%macro-play",         1, stub_macro_play);
    SDEF("%macro-recording?",   0, stub_macro_recording_p);
    SDEF("%minibuffer-activate",3, stub_minibuffer_activate);
    SDEF("%minibuffer-submit",  0, stub_minibuffer_submit);
    SDEF("%minibuffer-cancel",  0, stub_minibuffer_cancel);
    SDEF("%minibuffer-active?", 0, stub_minibuffer_active_p);
    SDEF("%minibuffer-activate-commands",         0, stub_mb_act_commands);
    SDEF("%minibuffer-activate-describe-function",0, stub_mb_act_desc_func);
    SDEF("%minibuffer-activate-describe-command", 0, stub_mb_act_desc_cmd);
    SDEF("%minibuffer-activate-describe-variable",0, stub_mb_act_desc_var);
    SDEF("%minibuffer-activate-themes",           0, stub_mb_act_themes);
    SDEF("%minibuffer-activate-major-modes!",     0, stub_mb_act_major_modes);
    SDEF("%minibuffer-select-next", 0, stub_mb_select_next);
    SDEF("%minibuffer-select-prev", 0, stub_mb_select_prev);
    SDEF("%which-key-toggle",   0, stub_which_key_toggle);
    SDEF("%record-command-usage",   1, stub_record_command_usage);
    SDEF("%update-recent-project!", 1, stub_update_recent_project);
    SDEF("%chdir",                  1, stub_chdir);
    SDEF("%open-recent-project!",   1, stub_open_recent_project);
    SDEF("%jump-push!",         0, stub_jump_push);
    SDEF("%jump-backward!",     0, stub_jump_backward);
    SDEF("%jump-forward!",      0, stub_jump_forward);
    SDEF("%set-mouse-click-handler!", 1, stub_mouse_click);
    SDEF("%set-mouse-drag-handler!",  1, stub_mouse_drag);

    // Test-only primitive
    SDEF("test-reset-buffer!",  1, scm_test_reset_buffer);

    #undef SDEF
    #undef SDEF_OPT

    // Extend module search path so chibi finds editor/*.sld under ./out/scheme
    {
        sexp_gc_var2(dir_str, new_path);
        sexp_gc_preserve2(ctx, dir_str, new_path);
        dir_str = sexp_c_string(ctx, "./out/scheme/", -1);
        new_path = sexp_cons(ctx, dir_str, sexp_global(ctx, SEXP_G_MODULE_PATH));
        sexp_global(ctx, SEXP_G_MODULE_PATH) = new_path;
        sexp_gc_release2(ctx);
    }

    // Register (editor primitives) pointing at our test env so library
    // imports resolve against the bindings we just defined.
    sexp meta = sexp_global(ctx, SEXP_G_META_ENV);
    sexp_env_define(ctx, meta, sexp_intern(ctx, "%editor-test-env", -1), env);

    sexp result = sexp_eval_string(ctx,
        "(add-module! '(editor primitives) "
        " (make-module "
        "  '(quit message message-echo message-clear message-lock message-unlock "
        "    make-keymap %set-key! insert-char self-insert delete-char "
        "    move-point move-point-by-line next-line prev-line "
        "    forward-char backward-char newline insert-tab "
        "    delete-backward-char delete-forward-char set-column "
        "    line-start line-end skip-whitespace char-at-point "
        "    point-get point-set! buffer-length delete-range char-at "
        "    last-key-char %set-replace-mode! %tab-next %tab-prev %tab-new! "
        "    %tab-new-fresh! no-panes? "
        "    reset-global-scale increase-global-scale decrease-global-scale "
        "    %reset-buffer-scale %increase-buffer-scale %decrease-buffer-scale "
        "    %split-vertical %split-horizontal %tab-close %pop-to-buffer "
        "    %pane-navigate-up %pane-navigate-down "
        "    %pane-navigate-left %pane-navigate-right "
        "    %pane-v-split-increase %pane-v-split-decrease "
        "    %pane-h-split-increase %pane-h-split-decrease "
        "    eval-buffer %ts-tree-string %ts-enable! %ts-disable! clay-debug prefix-arg "
        "    %set-keymap-parent! %set-keymap-name! %bind-prefix! "
        "    %read-key-binding %set-key-unbound-cb! %set-mode-allows-input! ignore "
        "    %buffer-has-minor-mode? "
        "    %buffer-file-name %set-buffer-file-name! %buffer-write "
        "    %buffer-modified? %set-buffer-modified! %buffer-set-name! "
        "    %buffer-insert %buffer-read %buffer-create "
        "    %tab-set-buffer! %tab-set-preview! %buffer-close! "
        "    %register-mode %set-major-mode "
        "    %enable-minor-mode %disable-minor-mode "
        "    %buffer-major-mode %buffer-minor-modes "
        "    %set-local! %get-local %set-palette! "
        "    %update-icon-colors! %register-icon! "
        "    %register-mode-icon! %register-major-mode-info! %set-cursor-override! %set-role! "
        "    %clear-palette! %clear-roles! "
        "    %begin-change %end-change %undo %redo "
        "    %change-active? %change-set-repeat-info! %change-last-repeat-info "
        "    %line-restore %change-current-inserts "
        "    %mark-set-to-point! %select-mode-set! %select-mode-get "
        "    exchange-point-and-mark %point-to-mark! %goto-line "
        "    %line-count %mark-position %position-line "
        "    %line-start-position %line-end-position "
        "    %clipboard-get %clipboard-set! "
        "    %register-set! %register-append! %register-get "
        "    %register-set-shape! %register-shape "
        "    %register-set-block-width! %register-block-width "
        "    %buffer-substring %insert-string "
        "    %macro-start! %macro-stop! %macro-play %macro-recording? "
        "    %minibuffer-activate %minibuffer-submit "
        "    %minibuffer-cancel %minibuffer-active? "
        "    %minibuffer-activate-commands %minibuffer-activate-describe-function "
        "    %minibuffer-activate-describe-command "
        "    %minibuffer-activate-describe-variable %minibuffer-activate-themes "
        "    %minibuffer-activate-major-modes! "
        "    %minibuffer-select-next %minibuffer-select-prev "
        "    %which-key-toggle "
        "    %record-command-usage %update-recent-project! %chdir %open-recent-project! "
        "    %jump-push! %jump-backward! %jump-forward! "
        "    jump-to-matching-bracket "
        "    %set-mouse-click-handler! %set-mouse-drag-handler! "
        "    %set-welcome-keymap! "
        "    global-keymap pane-keymap eval "
        "    test-reset-buffer!) "
        "  %editor-test-env '()))",
        -1, meta);
    if (sexp_exceptionp(result)) {
        fprintf(stderr, "scheme_test_init: failed to register (editor primitives)\n");
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        exit(1);
    }

    // Import (editor vim) into the test env. This transitively loads
    // (editor command), (editor mode), and all vim/*.scm files.
    result = sexp_eval_string(ctx, "(import (editor vim))", -1, env);
    if (sexp_exceptionp(result)) {
        fprintf(stderr, "scheme_test_init: failed to import (editor vim)\n");
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        exit(1);
    }

    // Create a test buffer and make it current.
    test_buf = buffer_create("*test*");
    if (!test_buf) {
        fprintf(stderr, "scheme_test_init: failed to create test buffer\n");
        exit(1);
    }
    buffer_set_current(test_buf);
}

void scheme_test_quit(void) {
    if (test_buf) {
        buffer_delete(test_buf);
        test_buf = NULL;
    }
    // chibi context is freed when the process exits; tests run in a single
    // process so we don't bother destroying it here.
}

int scheme_test_run(const char *path) {
    sexp ctx = G->chibi.ctx;
    sexp env = G->chibi.env;

    sexp_gc_var2(path_str, result);
    sexp_gc_preserve2(ctx, path_str, result);

    path_str = sexp_c_string(ctx, path, -1);
    result = sexp_load(ctx, path_str, env);

    if (sexp_exceptionp(result)) {
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        sexp_gc_release2(ctx);
        return -1;
    }

    // After the suite runs, main.scm leaves (*fail-count*) defined.
    result = sexp_eval_string(ctx, "*fail-count*", -1, env);
    int fails = sexp_fixnump(result) ? (int)sexp_unbox_fixnum(result) : -1;

    sexp_gc_release2(ctx);
    return fails;
}
