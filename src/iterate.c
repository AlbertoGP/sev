#include <SDL3/SDL.h>

#include "state.h"
#include "display/layout.h"
#include "display/pane.h"
#include "display/status.h"
#include "display/tab.h"
#include "display/welcome.h"

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
    state->input.welcome_row_hovered = false;
    state->render_gen++;

    Uint64 now = SDL_GetTicksNS();

    /* Idle: nothing to do */
    if (!state->needs_redraw && !state->needs_extra_frame && !state->animating) {
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

    /* Draw to the screen */
    SDL_SetRenderDrawColor(state->rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->rendererData.renderer);
    Clay_RenderCommandArray render_commands = create_app_layout(state);
    tab_flush_pending_close();
    welcome_flush_pending();
    SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);
    tab_free_strings();
    pane_free_strings();
    bar_free_strings();
    if (state->needs_extra_frame) {
        state->input.welcome_row_hovered = false;
        Clay_RenderCommandArray render_commands = create_app_layout(state);
        SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);
        pane_free_strings();
        bar_free_strings();
    }

    /* Update system cursor based on final layout's hover state */
    {
        static SDL_Cursor *cursor_ew      = NULL;
        static SDL_Cursor *cursor_ns      = NULL;
        static SDL_Cursor *cursor_default = NULL;
        static SDL_Cursor *cursor_hand    = NULL;
        if (!cursor_ew)      cursor_ew      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
        if (!cursor_ns)      cursor_ns      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
        if (!cursor_default) cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
        if (!cursor_hand)    cursor_hand    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
        Pane *split_hover = pane_split_at_coords(pane_get_root(),
                                state->input.mouse_x, state->input.mouse_y);
        if (split_hover && split_hover->type == PANE_V_SPLIT)
            SDL_SetCursor(cursor_ew);
        else if (split_hover && split_hover->type == PANE_H_SPLIT)
            SDL_SetCursor(cursor_ns);
        else if (state->input.welcome_row_hovered)
            SDL_SetCursor(cursor_hand);
        else
            SDL_SetCursor(cursor_default);
    }

    SDL_RenderPresent(state->rendererData.renderer);

    /* Reset dirty flag */
    state->needs_redraw = false;
    state->needs_extra_frame = false;

    return SDL_APP_CONTINUE;
}
