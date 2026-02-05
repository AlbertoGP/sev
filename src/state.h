#pragma once

#include <SDL3/SDL.h>
#include <chibi/eval.h>
#include "clay/renderer.h"
#include "keyevent.h"
#include "subeditor/var.h"

typedef enum FontID {
    FONT_NORMAL,
    FONT_BOLD,
    FONT_ITALIC,
    FONT_COUNT
} FontID;

typedef enum Theme {
    THEME_DARK,
    THEME_LIGHT
} Theme;

typedef struct {
    struct Keymap *global_map;
    struct Keymap *current_map;
    KeyEvent last_event;
} InputState;

typedef struct {
    sexp ctx;
    sexp env;
    sexp call_interactively;
} Chibi;

typedef struct CachedRoles {
    sexp ui_bg;
    sexp bar_bg, bar_text_active;
    sexp tab_bar, tab_active, tab_hover, tab_inactive;
    sexp text_primary, text_faded;
} CachedRoles;

typedef struct UIState {
    sexp current_theme;       // symbol, e.g. 'gruvbox-dark
    VarTable role_table;      // role-symbol -> palette-index OR color
    VarTable palette_table;   // palette-symbol -> Color
    CachedRoles roles;        // pre-interned role symbols
    float scale_factor;       // global scaling factor
} UIState;

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
    VarTable globals;
    UIState ui;
    InputState input;
} AppState;
