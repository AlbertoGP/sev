#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <chibi/eval.h>

#include "state.h"
#include "command/keyboard.h"
#include "display/pane.h"
#include "display/vline.h"
#include "text/buffer.h"

// Invoke (mouse-click-cb button buf-pos clicks) on the Scheme side.
static void invoke_mouse_click_cb(AppState *state, int button,
                                  size_t pos, int clicks) {
    sexp cb = state->input.mouse_click_cb;
    if (cb == SEXP_FALSE) return;

    sexp ctx = state->chibi.ctx;
    sexp_gc_var1(args);
    sexp_gc_preserve1(ctx, args);
    args = sexp_list3(ctx,
        sexp_make_fixnum(button),
        sexp_make_fixnum((sexp_sint_t)pos),
        sexp_make_fixnum(clicks));
    sexp result = sexp_apply(ctx, cb, args);
    if (sexp_exceptionp(result))
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    sexp_gc_release1(ctx);
}

// Invoke (mouse-drag-cb current-pos start-pos) on the Scheme side.
static void invoke_mouse_drag_cb(AppState *state, size_t pos, size_t start_pos) {
    sexp cb = state->input.mouse_drag_cb;
    if (cb == SEXP_FALSE) return;

    sexp ctx = state->chibi.ctx;
    sexp_gc_var1(args);
    sexp_gc_preserve1(ctx, args);
    args = sexp_list2(ctx,
        sexp_make_fixnum((sexp_sint_t)pos),
        sexp_make_fixnum((sexp_sint_t)start_pos));
    sexp result = sexp_apply(ctx, cb, args);
    if (sexp_exceptionp(result))
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    sexp_gc_release1(ctx);
}


// Hit-test the screen point (x, y) against the pane tree, then compute the
// buffer byte position.  Returns true and fills *pos_out on success.
static bool pane_hit_to_buf_pos(AppState *state, Pane *pane,
                                 float x, float y, size_t *pos_out) {
    if (!pane || pane->type != PANE_CONTENT || !pane->content.active_tab) return false;
    Buffer *buf = pane->content.active_tab->content.buffer.buffer;
    if (!buf) return false;

    float rel_x = x - pane->content.text_origin_x;
    float rel_y = y - pane->content.text_origin_y;
    // Clamp rel_y so dragging slightly outside still lands on nearest line.
    if (rel_y < 0.0f) rel_y = 0.0f;
    if (rel_y > pane->content.text_origin_h)
        rel_y = pane->content.text_origin_h;

    char *chars = buffer_text(buf);
    *pos_out = vline_byte_pos_at_xy(
        &pane->content.active_tab->content.buffer.vline_cache, chars,
        rel_x, rel_y,
        pane->content.line_height_px,
        &state->rendererData,
        pane->content.render_font_id,
        pane->content.render_font_size);
    free(chars);
    return true;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *state = (AppState*) appstate;

    switch (event->type) {
    case SDL_EVENT_TEXT_INPUT:
        handle_text_input(state, &event->text);
        break;

    case SDL_EVENT_KEY_DOWN:
        handle_key_down(state, &event->key);
        break;

    case SDL_EVENT_MOUSE_MOTION: {
        state->needs_redraw = true;
        float x = event->motion.x;
        float y = event->motion.y;
        bool button_held = (event->motion.state & SDL_BUTTON_LMASK) != 0;
        Clay_SetPointerState((Clay_Vector2){x, y}, button_held);

        if (state->input.scrollbar_drag_pane) {
            Pane *sp = state->input.scrollbar_drag_pane;
            VLineCache *sc = &sp->content.active_tab->content.buffer.vline_cache;
            int slh = sp->content.line_height_px;
            float track_top = sp->content.scrollbar_y;
            float thumb_h   = sp->content.scrollbar_thumb_h;
            float travel    = sp->content.scrollbar_track_h - thumb_h;
            float max_sc    = (sc->count > 1) ? (float)(sc->count - 1) * slh : 0.0f;
            if (travel > 0 && max_sc > 0) {
                float frac = (y - state->input.scrollbar_drag_offset - track_top) / travel;
                if (frac < 0) frac = 0;
                if (frac > 1) frac = 1;
                sc->scroll_offset = frac * max_sc;
                sc->target_scroll = sc->scroll_offset;
            }
            state->needs_redraw = true;
            break;
        }

        if (state->input.mouse_button_down && state->input.mouse_down_pane) {
            float dx = x - state->input.mouse_down_x;
            float dy = y - state->input.mouse_down_y;
            if (!state->input.mouse_drag_active && (dx*dx + dy*dy) > 9.0f)
                state->input.mouse_drag_active = true;

            if (state->input.mouse_drag_active) {
                Pane *p = state->input.mouse_down_pane;
                size_t pos = 0;
                if (pane_hit_to_buf_pos(state, p, x, y, &pos))
                    invoke_mouse_drag_cb(state, pos, state->input.mouse_down_buf_pos);
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        state->needs_redraw = true;
        float x = event->button.x;
        float y = event->button.y;
        Clay_SetPointerState((Clay_Vector2){x, y}, true);

        state->input.mouse_button_down = true;
        state->input.mouse_down_x = x;
        state->input.mouse_down_y = y;
        state->input.mouse_drag_active = false;
        if (event->button.button == SDL_BUTTON_MIDDLE)
            state->input.middle_pressed_this_frame = true;

        Pane *hit = pane_at_coords(pane_get_root(), x, y);
        state->input.mouse_down_pane = hit;

        // Check scrollbar hit before buffer hit.
        if (hit && hit->type == PANE_CONTENT && hit->content.active_tab
            && hit->content.has_scrollbar
            && x >= hit->content.scrollbar_x
            && y >= hit->content.scrollbar_y
            && y <= hit->content.scrollbar_y + hit->content.scrollbar_track_h) {

            float thumb_y = hit->content.scrollbar_thumb_y;
            float thumb_h = hit->content.scrollbar_thumb_h;
            bool  on_thumb = (y >= thumb_y && y < thumb_y + thumb_h);
            state->input.scrollbar_drag_offset = on_thumb
                ? (y - thumb_y)
                : (thumb_h / 2.0f);

            VLineCache *sc = &hit->content.active_tab->content.buffer.vline_cache;
            int slh = hit->content.line_height_px;
            float travel = hit->content.scrollbar_track_h - thumb_h;
            float max_sc  = (sc->count > 1) ? (float)(sc->count - 1) * slh : 0.0f;
            if (travel > 0 && max_sc > 0) {
                float frac = (y - state->input.scrollbar_drag_offset - hit->content.scrollbar_y) / travel;
                if (frac < 0) frac = 0;
                if (frac > 1) frac = 1;
                sc->scroll_offset = frac * max_sc;
                sc->target_scroll = sc->scroll_offset;
            }

            state->input.scrollbar_drag_pane = hit;
            state->needs_redraw = true;
            break;
        }

        if (hit && hit->type == PANE_CONTENT && hit->content.active_tab) {
            // Switch active pane so that Scheme point-set! targets the right buffer.
            if (!hit->content.active)
                pane_set_active(hit);

            size_t pos = 0;
            if (pane_hit_to_buf_pos(state, hit, x, y, &pos)) {
                state->input.mouse_down_buf_pos = pos;
                invoke_mouse_click_cb(state, event->button.button, pos,
                                      event->button.clicks);
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
        state->needs_redraw = true;
        Clay_SetPointerState((Clay_Vector2){event->button.x, event->button.y}, false);
        state->input.mouse_button_down = false;
        state->input.mouse_drag_active = false;
        state->input.mouse_down_pane   = NULL;
        state->input.scrollbar_drag_pane = NULL;
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        state->needs_redraw = true;
        float deltaTime = ((float) SDL_GetTicksNS() - state->last_frame_ns) / 1e9;
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { event->wheel.x, event->wheel.y }, deltaTime);

        // Scroll the buffer pane under the mouse cursor.
        Pane *scroll_hit = pane_at_coords(pane_get_root(),
                                          event->wheel.mouse_x, event->wheel.mouse_y);
        if (scroll_hit && scroll_hit->type == PANE_CONTENT && scroll_hit->content.active_tab) {
            VLineCache *cache = &scroll_hit->content.active_tab->content.buffer.vline_cache;
            if (cache->count > 0) {
                float px_per_line = (float)scroll_hit->content.line_height_px;
                if (px_per_line <= 0) px_per_line = 20.0f;
                float delta_px = event->wheel.y * px_per_line * 3.0f;
                if (event->wheel.direction == SDL_MOUSEWHEEL_NORMAL)
                    delta_px = -delta_px;

                cache->scroll_offset += delta_px;
                float max_scroll = (cache->count > 1)
                    ? (float)(cache->count - 1) * px_per_line : 0.0f;
                if (cache->scroll_offset < 0.0f) cache->scroll_offset = 0.0f;
                if (cache->scroll_offset > max_scroll) cache->scroll_offset = max_scroll;
                cache->target_scroll = cache->scroll_offset;
            }
        }
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        state->needs_redraw = true;
        int width, height;
        SDL_GetWindowSize(state->window, &width, &height);
        Clay_SetLayoutDimensions((Clay_Dimensions) {(float) width, (float) height});
        break;

    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}
