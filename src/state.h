#pragma once

#include <SDL3/SDL.h>
#include <chibi/eval.h>
#include "clay/renderer.h"
#include "keyevent.h"

typedef enum FontID {
    FONT_NORMAL,
    FONT_BOLD,
    FONT_ITALIC,
    FONT_COUNT
} FontID;

typedef struct ColorSet {
    Clay_Color background;
    Clay_Color foreground;
    Clay_Color bar;
    Clay_Color text;
    Clay_Color textFaded;
    Clay_Color cursor;
} ColorSet;

typedef enum Theme {
    THEME_DARK,
    THEME_LIGHT
} Theme;

typedef enum CursorType {
    CURSOR_SOLID,
    CURSOR_HOLLOW,
    CURSOR_THIN,
    CURSOR_UNDER
} CursorType;

typedef struct {
    struct Keymap *global_map;
    struct Keymap *current_map;
    KeyEvent last_event;
} InputState;

typedef struct {
    sexp ctx;
    sexp env;
} Chibi;

typedef struct AppState {
    SDL_Window *window;
    Clay_SDL3RendererData rendererData;
    ColorSet colors;
    ColorSet colors_target;
    ColorSet colors_delta;
    int color_frames;
    Theme theme;
    CursorType cursor;
    bool needs_redraw;
    bool needs_extra_frame;
    bool animating;
    Uint64 last_frame_ns;
    bool debug_open;
    Chibi chibi;
    InputState input;
} AppState;
