#include <SDL3/SDL.h>
#include <chibi/eval.h>

#include "state.h"
#include "clay/renderer.h"
#include "display/pane.h"
#include "text/buffer.h"
#include "text/register.h"

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    pane_quit();
    buffer_list_quit();

    AppState *state = appstate;

    if (state) {
        if (state->minibuf.buf) {
            if (state->minibuf.on_submit != SEXP_FALSE)
                sexp_release_object(state->chibi.ctx, state->minibuf.on_submit);
            if (state->minibuf.on_cancel != SEXP_FALSE)
                sexp_release_object(state->chibi.ctx, state->minibuf.on_cancel);
        }

        register_free_all(state->registers);
        SDL_free(state->macro_buf);
        if (state->rendererData.renderer)
            SDL_DestroyRenderer(state->rendererData.renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        SDL_Clay_DestroyTextCache();

        if (state->rendererData.fonts) {
            for(size_t i = 0; i < FONT_COUNT; i++) {
                TTF_CloseFont(state->rendererData.fonts[i]);
                state->rendererData.fonts[i] = NULL;
            }

            SDL_free(state->rendererData.fonts);
        }

        if (state->rendererData.font_paths) {
            for (size_t i = 0; i < FONT_COUNT; i++)
                SDL_free((void *)state->rendererData.font_paths[i]);
            SDL_free(state->rendererData.font_paths);
        }

        if (state->rendererData.textEngine)
            TTF_DestroyRendererTextEngine(state->rendererData.textEngine);
    }

    TTF_Quit();
}
