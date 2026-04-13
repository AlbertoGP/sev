#include <SDL3/SDL.h>
#include <stdint.h>

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
    state->input.desired_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
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
    uint64_t new_time = SDL_GetTicksNS();
    float delta_time = ((float) SDL_GetTicksNS() - state->last_frame_ns) / 1e9;
    state->last_frame_ns = new_time;

    /* Draw to the screen */
    SDL_SetRenderDrawColor(state->rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->rendererData.renderer);
    Clay_RenderCommandArray render_commands = create_app_layout(state, delta_time);
    tab_flush_pending_close();
    welcome_flush_pending();
    SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);
    tab_free_strings();
    pane_free_strings();
    bar_free_strings();
    if (state->needs_extra_frame) {
        state->input.desired_cursor = SDL_SYSTEM_CURSOR_DEFAULT;
        Clay_RenderCommandArray render_commands = create_app_layout(state, delta_time);
        SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);
        pane_free_strings();
        bar_free_strings();
    }

    /* Update system cursor: geometric resize overrides layout's desired_cursor */
    {
        static SDL_Cursor *cursors[SDL_SYSTEM_CURSOR_COUNT] = {0};
        Pane *split_hover = pane_split_at_coords(pane_get_root(),
                                state->input.mouse_x, state->input.mouse_y);
        if (split_hover && split_hover->type == PANE_V_SPLIT)
            state->input.desired_cursor = SDL_SYSTEM_CURSOR_EW_RESIZE;
        else if (split_hover && split_hover->type == PANE_H_SPLIT)
            state->input.desired_cursor = SDL_SYSTEM_CURSOR_NS_RESIZE;
        SDL_SystemCursor id = state->input.desired_cursor;
        if (!cursors[id]) cursors[id] = SDL_CreateSystemCursor(id);
        SDL_SetCursor(cursors[id]);
    }

    SDL_RenderPresent(state->rendererData.renderer);

    /* Reset dirty flags; if any render pass requested more work, carry it forward
     * and push a wakeup event so SDL breaks out of waitevent sleep. */
    state->needs_redraw = state->needs_extra_frame;
    state->needs_extra_frame = false;
    if (state->needs_redraw) {
        SDL_Event ev = {0};
        ev.type = SDL_EVENT_USER;
        ev.user.code = 0; /* unrecognised → no-op in SDL_AppEvent, just wakes SDL */
        SDL_PushEvent(&ev);
    }

    return SDL_APP_CONTINUE;
}
