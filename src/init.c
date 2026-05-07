#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <chibi/eval.h>

#include "state.h"
#include "state_io.h"
#include "file_scanner.h"
#include "cursor_flash.h"
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

    if (!SDL_CreateWindowAndRenderer("sev", 800, 600,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
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

    static const struct { int id; const char *rel; int style; } font_specs[] = {
        { FONT_UI_NORMAL,  "resources/IBMPlexSans-Regular.ttf",    TTF_STYLE_NORMAL },
        { FONT_UI_BOLD,    "resources/IBMPlexSans-SemiBold.ttf",   TTF_STYLE_NORMAL },
        { FONT_UI_ITALIC,  "resources/IBMPlexSans-Italic.ttf",     TTF_STYLE_ITALIC },
        { FONT_BUF_NORMAL, "resources/JetBrainsMono-Regular.ttf",  TTF_STYLE_NORMAL },
        { FONT_BUF_BOLD,   "resources/JetBrainsMono-SemiBold.ttf", TTF_STYLE_NORMAL },
        { FONT_BUF_ITALIC, "resources/JetBrainsMono-Italic.ttf",   TTF_STYLE_ITALIC },
    };

#ifdef __EMSCRIPTEN__
    const char *font_prefix = "/";
#else
    const char *font_prefix = SDL_GetBasePath();
#endif

    state->rendererData.font_paths = SDL_calloc(FONT_COUNT, sizeof(const char *));

    for (int i = 0; i < FONT_COUNT; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s%s", font_prefix, font_specs[i].rel);
        state->rendererData.font_paths[font_specs[i].id] = SDL_strdup(path);

        TTF_Font *font = TTF_OpenFont(path, 24);
        if (!font) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font %s = %s", path, SDL_GetError());
            return SDL_APP_FAILURE;
        }
        TTF_SetFontStyle(font, font_specs[i].style);
        TTF_SetFontHinting(font, TTF_HINTING_LIGHT_SUBPIXEL);
        state->rendererData.fonts[font_specs[i].id] = font;
    }

    if (!clay_init(state)) {
        fprintf(stderr, "Failed to initialise Clay UI.");
        return SDL_APP_FAILURE;
    }

    if (!init_input(state)) {
        fprintf(stderr, "Failed to initialise keybinding module.");
        return SDL_APP_FAILURE;
    }

    float dpi = SDL_GetWindowDisplayScale(state->window);
    state->ui.dpi_scale = (dpi > 0.0f) ? dpi : 1.0f;
    state->ui.user_scale = 1.0f;
    state->ui.scale_factor = state->ui.dpi_scale * state->ui.user_scale;
    scheme_init(state);

    if (!state_io_load(state)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "state_io_load failed; starting with empty state");
    }

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

    if (!scanner_init(&state->scanner)) {
        fprintf(stderr, "Failed to initialise file scanner.");
        return SDL_APP_FAILURE;
    }

#ifdef __EMSCRIPTEN__
    if (state->first_launch) {
        // Persist the seeded state so subsequent loads don't re-trigger this.
        state_io_save(state);

        // Open /demo/demo-file.scm, mirroring the open-file Scheme command.
        sexp ctx = state->chibi.ctx;
        sexp env = state->chibi.env;
        sexp result = sexp_eval_string(ctx,
            "(let ((f \"/demo/demo-file.scm\"))"
            "  (when (file-exists? f)"
            "    (when (no-panes?) (%tab-new! f))"
            "    (%buffer-create f)"
            "    (%tab-set-buffer! f)"
            "    (%set-buffer-file-name! f)"
            "    (%buffer-read)"
            "    (set-auto-mode!)"
            "    (%update-recent-project! \"/demo\")))",
            -1, env);
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    }
#endif

    state->animating = false;
    state->last_frame_ns = 0;
    cursor_flash_reset(state);

    /* Start in event-driven mode; iterate.c switches to 60fps during animations */
    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");

    icons_stash_renderer(state->rendererData.renderer);
    icons_update_colors(state);

    return SDL_APP_CONTINUE;
}
