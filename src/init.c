#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <chibi/eval.h>

#include "state.h"
#include "clay/init.h"
#include "command/keymap.h"
#include "command/minibuf.h"
#include "command/scheme.h"
#include "display/icon.h"
#include "display/pane.h"
#include "text/buffer.h"

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_Log("Failed to allocate memory for AppState = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    if (!SDL_CreateWindowAndRenderer("sev", 800, 600, SDL_WINDOW_RESIZABLE,
                                     &state->window, &state->rendererData.renderer)) {
        SDL_Log("Failed to create window and renderer = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

#ifdef __EMSCRIPTEN__
    /* Enable vsync for proper requestAnimationFrame timing in browser */
    SDL_SetRenderVSync(state->rendererData.renderer, 1);
#endif

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
    // #define FONT_PATH_REGULAR "/resources/VictorMono-Regular.ttf"
    // #define FONT_PATH_BOLD "/resources/VictorMono-Bold.ttf"
    // #define FONT_PATH_ITALIC "/resources/VictorMono-Italic.ttf"
    #define FONT_PATH_UI_REGULAR "/resources/IBMPlexSans-Regular.ttf"
    #define FONT_PATH_UI_BOLD "/resources/IBMPlexSans-SemiBold.ttf"
    #define FONT_PATH_UI_ITALIC "/resources/IBMPlexSans-Italic.ttf"
    #define FONT_PATH_BUF_REGULAR "/resources/JetBrainsMono-Regular.ttf"
    #define FONT_PATH_BUF_BOLD "/resources/JetBrainsMono-SemiBold.ttf"
    #define FONT_PATH_BUF_ITALIC "/resources/JetBrainsMono-Italic.ttf"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char fontPathUIRegular[1024];
    char fontPathUIBold[1024];
    char fontPathUIItalic[1024];
    char fontPathBufRegular[1024];
    char fontPathBufBold[1024];
    char fontPathBufItalic[1024];
    // snprintf(fontPathRegular, sizeof(fontPathRegular), "%sresources/VictorMono-Regular.ttf", basePath);
    // snprintf(fontPathBold, sizeof(fontPathBold), "%sresources/VictorMono-Bold.ttf", basePath);
    // snprintf(fontPathItalic, sizeof(fontPathItalic), "%sresources/VictorMono-Italic.ttf", basePath);
    snprintf(fontPathUIRegular, sizeof(fontPathUIRegular), "%sresources/IBMPlexSans-Regular.ttf", basePath);
    snprintf(fontPathUIBold, sizeof(fontPathUIBold), "%sresources/IBMPlexSans-SemiBold.ttf", basePath);
    snprintf(fontPathUIItalic, sizeof(fontPathUIItalic), "%sresources/IBMPlexSans-Italic.ttf", basePath);
    snprintf(fontPathBufRegular, sizeof(fontPathBufRegular), "%sresources/JetBrainsMono-Regular.ttf", basePath);
    snprintf(fontPathBufBold, sizeof(fontPathBufBold), "%sresources/JetBrainsMono-SemiBold.ttf", basePath);
    snprintf(fontPathBufItalic, sizeof(fontPathBufItalic), "%sresources/JetBrainsMono-Italic.ttf", basePath);
    #define FONT_PATH_UI_REGULAR fontPathUIRegular
    #define FONT_PATH_UI_BOLD fontPathUIBold
    #define FONT_PATH_UI_ITALIC fontPathUIItalic
    #define FONT_PATH_BUF_REGULAR fontPathBufRegular
    #define FONT_PATH_BUF_BOLD fontPathBufBold
    #define FONT_PATH_BUF_ITALIC fontPathBufItalic
    #endif
    
    TTF_Font* font_ui_normal = TTF_OpenFont(FONT_PATH_UI_REGULAR, 24);
    TTF_Font* font_ui_bold   = TTF_OpenFont(FONT_PATH_UI_BOLD, 24);
    TTF_Font* font_ui_italic = TTF_OpenFont(FONT_PATH_UI_ITALIC, 24);
    TTF_Font* font_buf_normal = TTF_OpenFont(FONT_PATH_BUF_REGULAR, 24);
    TTF_Font* font_buf_bold   = TTF_OpenFont(FONT_PATH_BUF_BOLD, 24);
    TTF_Font* font_buf_italic = TTF_OpenFont(FONT_PATH_BUF_ITALIC, 24);
    
    if (!font_ui_normal || !font_ui_bold || !font_ui_italic || !font_buf_normal || !font_buf_bold || !font_buf_italic) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font = %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    TTF_SetFontStyle(font_ui_normal, TTF_STYLE_NORMAL);
    TTF_SetFontStyle(font_ui_bold,   TTF_STYLE_NORMAL);
    TTF_SetFontStyle(font_ui_italic, TTF_STYLE_ITALIC);
    TTF_SetFontStyle(font_buf_normal, TTF_STYLE_NORMAL);
    TTF_SetFontStyle(font_buf_bold,   TTF_STYLE_NORMAL);
    TTF_SetFontStyle(font_buf_italic, TTF_STYLE_ITALIC);
    
    state->rendererData.fonts[FONT_UI_NORMAL] = font_ui_normal;
    state->rendererData.fonts[FONT_UI_BOLD]   = font_ui_bold;
    state->rendererData.fonts[FONT_UI_ITALIC] = font_ui_italic;
    state->rendererData.fonts[FONT_BUF_NORMAL] = font_buf_normal;
    state->rendererData.fonts[FONT_BUF_BOLD]   = font_buf_bold;
    state->rendererData.fonts[FONT_BUF_ITALIC] = font_buf_italic;

    state->rendererData.font_paths = SDL_calloc(FONT_COUNT, sizeof(const char *));
    state->rendererData.font_paths[FONT_UI_NORMAL] = SDL_strdup(FONT_PATH_UI_REGULAR);
    state->rendererData.font_paths[FONT_UI_BOLD]   = SDL_strdup(FONT_PATH_UI_BOLD);
    state->rendererData.font_paths[FONT_UI_ITALIC] = SDL_strdup(FONT_PATH_UI_ITALIC);
    state->rendererData.font_paths[FONT_BUF_NORMAL] = SDL_strdup(FONT_PATH_BUF_REGULAR);
    state->rendererData.font_paths[FONT_BUF_BOLD]   = SDL_strdup(FONT_PATH_BUF_BOLD);
    state->rendererData.font_paths[FONT_BUF_ITALIC] = SDL_strdup(FONT_PATH_BUF_ITALIC);

    if (!clay_init(state)) {
        fprintf(stderr, "Failed to initialise Clay UI.");
        return SDL_APP_FAILURE;
    }

    if (!init_input(state)) {
        fprintf(stderr, "Failed to initialise keybinding module.");
        return SDL_APP_FAILURE;
    }

    state->ui.scale_factor = 1.0;
    scheme_init(state);

    if (!buffer_list_init()) {
        fprintf(stderr, "Failed to initialise buffer list.");
        return SDL_APP_FAILURE;
    }

    if (!minibuf_init(state)) {
        fprintf(stderr, "Failed to initialise minibuffer.");
        return SDL_APP_FAILURE;
    }

    if (!pane_init(state)) {
        fprintf(stderr, "Failed to initialise pane system.");
        return SDL_APP_FAILURE;
    }

    state->needs_redraw = true;
    state->needs_extra_frame = true;
    state->animating = false;
    state->last_frame_ns = 0;

    /* Start in event-driven mode; iterate.c switches to 60fps during animations */
    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");

    icons_stash_renderer(state->rendererData.renderer);
    icons_update_colors(state);

    return SDL_APP_CONTINUE;
}
