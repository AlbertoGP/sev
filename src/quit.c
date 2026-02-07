#include <SDL3/SDL.h>

#include "state.h"
#include "display/tab.h"
#include "text/buffer.h"
#include "text/var.h"

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    tab_list_quit();
    buffer_list_quit();

    AppState *state = appstate;

    if (state) {
        vartable_destroy(&state->globals);

        if (state->rendererData.renderer)
            SDL_DestroyRenderer(state->rendererData.renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        if (state->rendererData.fonts) {
            for(size_t i = 0; i < FONT_COUNT; i++) {
                TTF_CloseFont(state->rendererData.fonts[i]);
                state->rendererData.fonts[i] = NULL;
            }

            SDL_free(state->rendererData.fonts);
        }

        if (state->rendererData.textEngine)
            TTF_DestroyRendererTextEngine(state->rendererData.textEngine);
    }

    TTF_Quit();
}
