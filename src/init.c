#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "clay/init.h"

#include "state.h"
#include "theme.h"

#include "subeditor/buffer.h"
#include "keymap.h"

#include "scheme/scheme.h"

#include <stdio.h>

#include <chibi/eval.h>

extern KeyEvent last_event;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_Log("Failed to allocate memory for AppState = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    if (!SDL_CreateWindowAndRenderer("SDL3 + Clay", 800, 600, SDL_WINDOW_RESIZABLE,
                                     &state->window, &state->rendererData.renderer)) {
        SDL_Log("Failed to create window and renderer = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_StartTextInput(state->window);

    if (!TTF_Init()) {
        SDL_Log("Failed to initialise SDL_ttf = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.textEngine = TTF_CreateRendererTextEngine(state->rendererData.renderer);
    if (!state->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts = SDL_calloc(FONT_COUNT, sizeof(TTF_Font *));
    if (!state->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    #ifdef __EMSCRIPTEN__
    #define FONT_PATH_REGULAR "/resources/VictorMono-Regular.ttf"
    #define FONT_PATH_BOLD "/resources/VictorMono-Bold.ttf"
    #define FONT_PATH_ITALIC "/resources/VictorMono-Italic.ttf"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char fontPathRegular[1024];
    char fontPathBold[1024];
    char fontPathItalic[1024];
    snprintf(fontPathRegular, sizeof(fontPathRegular), "%sresources/VictorMono-Regular.ttf", basePath);
    snprintf(fontPathBold, sizeof(fontPathBold), "%sresources/VictorMono-Bold.ttf", basePath);
    snprintf(fontPathItalic, sizeof(fontPathItalic), "%sresources/VictorMono-Italic.ttf", basePath);
    SDL_free(basePath);
    #define FONT_PATH_REGULAR fontPathRegular
    #define FONT_PATH_BOLD fontPathBold
    #define FONT_PATH_ITALIC fontPathItalic
    #endif
    
    TTF_Font* font_normal = TTF_OpenFont(FONT_PATH_REGULAR, 24);
    TTF_Font* font_bold   = TTF_OpenFont(FONT_PATH_BOLD, 24);
    TTF_Font* font_italic = TTF_OpenFont(FONT_PATH_ITALIC, 24);
    
    if (!font_normal || !font_bold || !font_italic) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    TTF_SetFontStyle(font_normal, TTF_STYLE_NORMAL);
    TTF_SetFontStyle(font_bold,   TTF_STYLE_BOLD);
    TTF_SetFontStyle(font_italic, TTF_STYLE_ITALIC);
    
    state->rendererData.fonts[FONT_NORMAL] = font_normal;
    state->rendererData.fonts[FONT_BOLD]   = font_bold;
    state->rendererData.fonts[FONT_ITALIC] = font_italic;

    if (!clay_init(state)) {
        fprintf(stderr, "Failed to initialise Clay UI.");
        return SDL_APP_FAILURE;
    }

    if (!init_input(state)) {
        fprintf(stderr, "Failed to initialise keybinding module.");
        return SDL_APP_FAILURE;
    }

    if (!buffer_list_init()) {
        fprintf(stderr, "Failed to initialise buffer list.");
        return SDL_APP_FAILURE;
    }

    set_theme_dark(state);

    state->needs_redraw = true;
    state->animating = false;
    state->last_frame_ns = 0;

    scheme_init(state);
    sexp result = sexp_eval_string(state->chibi.ctx, 
        "(define-key! global-keymap \"M-x a\" (lambda () (insert-char #\\a)))", 
        -1, 
        state->chibi.env);

    if (sexp_exceptionp(result)) {
        printf("ERROR: define-key! failed\n");
        sexp_print_exception(state->chibi.ctx, result, sexp_current_error_port(state->chibi.ctx));
        return SDL_APP_FAILURE;
    }
    
    return SDL_APP_CONTINUE;
}
