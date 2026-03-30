#pragma once

#include <stddef.h>

#include <SDL3/SDL.h>
#include <chibi/eval.h>

#include "clay/renderer.h"
#include "command/keyevent.h"
#include "text/register.h"
#include "text/var.h"

// Forward declaration — pane.h includes state.h, so we can't include it here.
struct Pane;

typedef enum FontID {
    FONT_UI_NORMAL,
    FONT_UI_BOLD,
    FONT_UI_ITALIC,
    FONT_BUF_NORMAL,
    FONT_BUF_BOLD,
    FONT_BUF_ITALIC,
    FONT_COUNT
} FontID;

typedef enum Theme {
    THEME_DARK,
    THEME_LIGHT
} Theme;

typedef enum {
    FOCUS_PANE,
    FOCUS_WELCOME,
    FOCUS_MINIBUFFER,
} FocusTarget;

typedef struct {
    struct Keymap *global_map;
    struct Keymap *pane_map;       // searched only when pane_get_root() != NULL; parent = global_map
    struct Keymap *current_map;
    KeyEvent last_event;
    sexp         key_intercept_cb;         // SEXP_FALSE if inactive
    struct Keymap *key_intercept_map;      // current traversal position
    char         key_intercept_str[256];   // accumulated display string
    sexp         key_unbound_cb;           // called on silently-ignored key; SEXP_FALSE if inactive
    // Mouse
    sexp         mouse_click_cb;           // (lambda (button buf-pos clicks) ...) or SEXP_FALSE
    sexp         mouse_drag_cb;            // (lambda (current-pos start-pos) ...) or SEXP_FALSE
    float        mouse_x, mouse_y;           // current pointer position in render coords
    bool         mouse_button_down;
    struct Pane *mouse_down_pane;          // locked pane for drag; NULL if not dragging
    float        mouse_down_x, mouse_down_y;
    size_t       mouse_down_buf_pos;       // buffer byte pos at click
    bool         mouse_drag_active;        // true once motion exceeds 3px threshold
    bool         middle_pressed_this_frame;       // true for one layout pass after middle-click
    bool         left_double_click_this_frame;    // true for one layout pass after left double-click
    struct Pane *scrollbar_drag_pane;      // non-NULL while dragging a vertical scrollbar
    float        scrollbar_drag_offset;   // y distance from cursor to thumb top at click time
    struct Pane *hscrollbar_drag_pane;    // non-NULL while dragging a horizontal scrollbar
    float        hscrollbar_drag_offset;  // x distance from cursor to thumb left at click time
    struct Pane *split_drag_pane;         // non-NULL while dragging a split divider
    float        split_drag_start_x;      // mouse x at split drag start
    float        split_drag_start_y;      // mouse y at split drag start
    FocusTarget  current_focus;
    struct Keymap *welcome_map;             // NULL until registered from Scheme
} InputState;

typedef struct {
    sexp ctx;
    sexp env;
    sexp call_interactively;
} Chibi;

typedef struct CachedRoles {
    sexp ui_bg, pane_bg, border_inactive, border_active, border_bell;
    sexp bar_bg, line_bg, scrollbar, scrollbar_hover;
    sexp tab_active, tab_hover, tab_inactive, tab_close;
    sexp text_primary, text_faded, text_key, text_command, text_prefix;
    sexp text_linenum;
    sexp selection;
    sexp mode_normal, mode_insert, mode_replace, mode_select;
    sexp mode_command, mode_pending, mode_minibuffer, mode_help;
    sexp label_normal, label_insert, label_replace, label_select;
    sexp label_command, label_pending, label_minibuffer, label_help;
    sexp cursor_normal, cursor_insert, cursor_replace, cursor_select;
    sexp cursor_command, cursor_pending, cursor_minibuffer, cursor_help;
    sexp macro_indicator, macro_bg;
    sexp hl_keyword, hl_string, hl_comment, hl_number;
    sexp hl_constant, hl_function, hl_builtin, hl_operator;
    sexp hl_bracket;
    sexp hl_bracket_match;
} CachedRoles;

typedef struct CachedSymbols {
    // plist keywords
    sexp kw_color;    // ':color
    sexp kw_font;     // ':font
    sexp kw_size;     // ':size
    sexp kw_bg;       // ':bg
    // font name symbols
    sexp buf_normal;  // 'buf-normal
    sexp buf_bold;    // 'buf-bold
    sexp buf_italic;  // 'buf-italic
    sexp ui_normal;   // 'ui-normal
    sexp ui_bold;     // 'ui-bold
    sexp ui_italic;   // 'ui-italic
} CachedSymbols;

typedef struct UIState {
    sexp current_theme;       // symbol, e.g. 'gruvbox-dark
    VarTable role_table;      // role-symbol -> palette-index OR color
    VarTable palette_table;   // palette-symbol -> Color
    CachedRoles roles;        // pre-interned role symbols
    CachedSymbols symbols;    // pre-interned misc symbols for theme resolution
    float dpi_scale;          // display pixel density (e.g. 2.0 on retina)
    float user_scale;         // user zoom level (default from Scheme)
    float scale_factor;       // effective scale = dpi_scale * user_scale
} UIState;

#define MINIBUF_PROMPT_MAX 256
#define MINIBUF_STACK_MAX 8

typedef struct {
    char   prompt[MINIBUF_PROMPT_MAX];
    sexp   on_submit;   // GC-preserved; SEXP_FALSE if none
    sexp   on_cancel;   // GC-preserved; SEXP_FALSE if none
    char  *saved_text;  // malloc'd snapshot of buffer content; NULL if empty
    size_t saved_point;
} MinibufFrame;

typedef struct {
    struct Buffer *buf;
    bool active;
    char prompt[MINIBUF_PROMPT_MAX];
    sexp on_submit;   // Scheme (lambda (text) ...), GC-preserved; SEXP_FALSE if none
    sexp on_cancel;   // Scheme (lambda () ...), GC-preserved; SEXP_FALSE if none
    struct Buffer *prev_buf;
    FocusTarget  prev_focus;   // saved focus before minibuf activation
    MinibufFrame stack[MINIBUF_STACK_MAX];
    int          stack_depth;  // 0 = no pushed frames
} Minibuf;

typedef struct {
    bool active;
    bool enabled;
    struct Keymap *keymap;
    char prefix_str[256];   // accumulated display string, e.g. "SPC h"
} WhichKeyState;

typedef struct {
    uint32_t    tracked_id;  // unique_id+1 of hovered element (0 = none)
    bool        visible;
    SDL_TimerID timer;       // pending show timer (0 = none)
    float       spawn_x, spawn_y; // pointer position at the moment tooltip became visible
} TooltipState;

typedef struct AppState {
    SDL_Window *window;
    Clay_SDL3RendererData rendererData;
    Theme theme;
    bool needs_redraw;
    bool needs_extra_frame;
    bool animating;
    Uint64 last_frame_ns;
    bool debug_open;
    Chibi chibi;
    UIState ui;
    InputState input;
    Register registers[REGISTER_COUNT];

    Minibuf minibuf;

    WhichKeyState which_key;
    TooltipState  tooltip;

    // Cursor flashing
    bool        cursor_visible;
    SDL_TimerID cursor_flash_timer;
    uint32_t    cursor_flash_gen;

    // Macro recording
    bool macro_recording;
    bool macro_skip_next;       // skip recording the key that triggered start-macro
    int  macro_target_reg;      // 0-25 = a-z
    KeyEvent *macro_buf;
    size_t macro_buf_len;
    size_t macro_buf_cap;
} AppState;
