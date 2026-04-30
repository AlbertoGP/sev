#include "tooltip.h"
#include "keybinding.h"
#include "theme.h"
#include "../clay/renderer.h"

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
        ts->timer   = 0;
        ts->spawn_x = state->input.mouse_x;
        ts->spawn_y = state->input.mouse_y;
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
        }
    }

    if (!ts->visible || ts->tracked_id != this_id) return;

    float scale = state->ui.scale_factor;
    uint16_t pad_x = (uint16_t)(6 * scale);
    uint16_t pad_y = (uint16_t)(4 * scale);

    int win_w = 0, win_h = 0;
    SDL_GetWindowSize(state->window, &win_w, &win_h);
    bool right_half  = ts->spawn_x > (float)win_w * 0.5f;
    bool bottom_half = ts->spawn_y > (float)win_h * 0.5f;

    float offset_x = right_half  ? ts->spawn_x - 8.0f * scale
                                  : ts->spawn_x + 8.0f * scale;
    float offset_y = bottom_half ? ts->spawn_y - 4.0f * scale
                                  : ts->spawn_y + 20.0f * scale;

    Clay_FloatingAttachPointType elem_attach;
    if      (!right_half && !bottom_half) elem_attach = CLAY_ATTACH_POINT_LEFT_TOP;
    else if ( right_half && !bottom_half) elem_attach = CLAY_ATTACH_POINT_RIGHT_TOP;
    else if (!right_half &&  bottom_half) elem_attach = CLAY_ATTACH_POINT_LEFT_BOTTOM;
    else                                  elem_attach = CLAY_ATTACH_POINT_RIGHT_BOTTOM;

    CLAY(CLAY_IDI_LOCAL("Tooltip", unique_id), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = elem_attach,
                .parent  = CLAY_ATTACH_POINT_LEFT_TOP
            },
            .offset = { offset_x, offset_y },
            .zIndex = 200
        },
        .layout = {
            .padding = { .left = pad_x, .right = pad_x, .top = pad_y, .bottom = pad_y }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg),
        .cornerRadius = CLAY_CORNER_RADIUS(4 * scale),
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

typedef struct {
    AppState   *state;
    const char *label;
    const char *binding; // may be NULL
} TextBindingTooltipData;

// Static buffers that outlive the layout pass so Clay_String.chars remain valid
// through render command generation. Only written inside the render callback,
// which is called at most once per frame (for the single visible tooltip).
static char s_label_buf[128];
static char s_binding_buf[64];

static void text_binding_tooltip_render(void *user_data) {
    TextBindingTooltipData *d = user_data;
    strncpy(s_label_buf, d->label, sizeof(s_label_buf) - 1);
    s_label_buf[sizeof(s_label_buf) - 1] = '\0';
    if (d->binding) {
        strncpy(s_binding_buf, d->binding, sizeof(s_binding_buf) - 1);
        s_binding_buf[sizeof(s_binding_buf) - 1] = '\0';
    } else {
        s_binding_buf[0] = '\0';
    }

    float    scale = d->state->ui.scale_factor;
    uint16_t fsz   = (uint16_t)(12.0f * scale);
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap        = 12.0f * scale,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER }
        }
    }) {
        Clay_String label_cs = { .chars = s_label_buf, .length = (int32_t)strlen(s_label_buf) };
        CLAY_TEXT(label_cs, CLAY_TEXT_CONFIG({
            .fontId    = FONT_UI_NORMAL,
            .fontSize  = fsz,
            .textColor = ui_resolve_color(d->state, d->state->ui.roles.text_primary)
        }));
        if (s_binding_buf[0]) {
            Clay_String kb = { .chars = s_binding_buf, .length = (int32_t)strlen(s_binding_buf) };
            Keybinding(d->state, kb, fsz);
        }
    }
}

void TextTooltipWithBinding(AppState *state, bool is_hovered, int unique_id,
                            const char *label, const char *binding) {
    TextBindingTooltipData d = { state, label, binding };
    Tooltip(state, is_hovered, unique_id, text_binding_tooltip_render, &d);
}
