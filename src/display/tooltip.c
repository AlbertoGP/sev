#include "tooltip.h"
#include "theme.h"

#include <SDL3/SDL.h>
#include <string.h>

#define TOOLTIP_DELAY_MS 500

static Uint32 tooltip_timer_cb(void *ud, SDL_TimerID id, Uint32 interval) {
    (void)id; (void)interval;
    SDL_Event ev = {0};
    ev.type = SDL_EVENT_USER;
    ev.user.code = TOOLTIP_SHOW_EVENT;
    SDL_PushEvent(&ev);
    return 0; // one-shot
}

void tooltip_handle_show(AppState *state) {
    TooltipState *ts = &state->tooltip;
    if (ts->tracked_id != 0) {
        ts->visible = true;
        ts->timer = 0;
        state->needs_redraw = true;
    }
}

void Tooltip(AppState *state, bool is_hovered, int unique_id,
             TooltipRenderFn render_fn, void *user_data) {
    TooltipState *ts = &state->tooltip;
    uint32_t this_id = (uint32_t)(unique_id + 1);

    if (is_hovered) {
        if (ts->tracked_id != this_id) {
            if (ts->timer) { SDL_RemoveTimer(ts->timer); ts->timer = 0; }
            ts->tracked_id = this_id;
            ts->visible = false;
            ts->timer = SDL_AddTimer(TOOLTIP_DELAY_MS, tooltip_timer_cb, state);
        }
    } else if (ts->tracked_id == this_id) {
        if (ts->timer) { SDL_RemoveTimer(ts->timer); ts->timer = 0; }
        ts->tracked_id = 0;
        if (ts->visible) {
            ts->visible = false;
            state->needs_redraw = true;
        }
    }

    if (!ts->visible || ts->tracked_id != this_id) return;

    float scale = state->ui.scale_factor;
    uint16_t pad_x = (uint16_t)(6 * scale);
    uint16_t pad_y = (uint16_t)(4 * scale);

    CLAY(CLAY_IDI_LOCAL("Tooltip", unique_id), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                .parent  = CLAY_ATTACH_POINT_LEFT_BOTTOM
            },
            .offset = { 0, 4.0f * scale },
            .zIndex = 200
        },
        .layout = {
            .padding = { .left = pad_x, .right = pad_x, .top = pad_y, .bottom = pad_y }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg),
        .cornerRadius = CLAY_CORNER_RADIUS(3 * scale),
        .border = {
            .color = ui_resolve_color(state, state->ui.roles.border_active),
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1 }
        }
    }) {
        render_fn(user_data);
    }
}

typedef struct { AppState *state; const char *text; } TextTooltipData;

static void text_tooltip_render(void *user_data) {
    TextTooltipData *d = (TextTooltipData *)user_data;
    float scale = d->state->ui.scale_factor;
    Clay_String cs = { .chars = d->text, .length = (int32_t)strlen(d->text) };
    CLAY_TEXT(cs, CLAY_TEXT_CONFIG({
        .fontId    = FONT_UI_NORMAL,
        .fontSize  = (uint16_t)(12 * scale),
        .textColor = ui_resolve_color(d->state, d->state->ui.roles.text_primary)
    }));
}

void TextTooltip(AppState *state, bool is_hovered, int unique_id, const char *text) {
    TextTooltipData d = { state, text };
    Tooltip(state, is_hovered, unique_id, text_tooltip_render, &d);
}
