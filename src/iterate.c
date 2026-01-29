#include <SDL3/SDL.h>

#include "layout/layout.h"
#include "layout/pane.h"
#include "layout/status.h"
#include "state.h"
#include "theme.h"

/* Track callback rate mode to avoid redundant hint changes */
static bool callback_rate_animating = false;

static void set_callback_rate(bool animating) {
    if (animating != callback_rate_animating) {
        callback_rate_animating = animating;
        SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, animating ? "60" : "waitevent");
    }
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *state = (AppState*) appstate;

    Uint64 now = SDL_GetTicksNS();

    /* Idle: nothing to do */
    if (!state->needs_redraw && !state->animating) {
        set_callback_rate(false);
#ifndef __EMSCRIPTEN__
        SDL_Delay(16);
#endif
        return SDL_APP_CONTINUE;
    }

    set_callback_rate(state->animating);

#ifndef __EMSCRIPTEN__
    /* Animation FPS cap (desktop only - browser uses requestAnimationFrame) */
    static const Uint64 FRAME_NS = 1000000000ULL / 60;
    if (state->animating) {
        if (state->last_frame_ns != 0) {
            Uint64 elapsed = now - state->last_frame_ns;
            if (elapsed < FRAME_NS) {
                SDL_DelayNS(FRAME_NS - elapsed);
                return SDL_APP_CONTINUE;
            }
        }
    }
#endif
    state->last_frame_ns = SDL_GetTicksNS();

    if (state->color_frames) {
        add_color_delta(state);
    }

    /* Draw to the screen */
    SDL_SetRenderDrawColor(state->rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->rendererData.renderer);
    Clay_RenderCommandArray render_commands = create_app_layout(state);
    SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);
    pane_free_strings();
    bar_free_strings();
    SDL_RenderPresent(state->rendererData.renderer);

    /* Reset dirty flag */
    state->needs_redraw = false;

    return SDL_APP_CONTINUE;
}
