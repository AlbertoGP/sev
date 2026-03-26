#include "cursor.h"
#include "theme.h"
#include "../clay/renderer.h"
#include "../text/buffer.h"
#include "../text/utf8.h"

#include <SDL3_ttf/SDL_ttf.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

static CursorRenderData cursor_pool[CURSOR_POOL_SIZE];
static int cursor_pool_idx = 0;

void cursor_cb_reset(void) { cursor_pool_idx = 0; }

void Cursor(AppState *state, int32_t index,
            float offset, int line_height,
            float clip_x, float clip_y, float clip_w, float clip_h,
            FontID font_id, uint16_t font_size) {

    if (cursor_pool_idx >= CURSOR_POOL_SIZE) return;
    CursorRenderData *d = &cursor_pool[cursor_pool_idx++];

    d->type   = CUSTOM_TYPE_CURSOR;
    d->clip_x = clip_x;
    d->clip_y = clip_y;
    d->clip_w = clip_w;
    d->clip_h = clip_h;

    CursorType cursor = get_cursor_type();
    d->cursor_type  = (int)cursor;
    d->bg_color     = ui_get_cursor_color(state);
    d->border_color = ui_get_cursor_color(state);
    d->border_width = MAX(1, state->ui.scale_factor);
    d->text_color   = ui_resolve_color(state, state->ui.roles.ui_bg);
    d->font_id      = font_id;
    d->font_size    = font_size;
    d->height       = (float)line_height;

    // Read char under cursor for width measurement and SOLID overlay
    char char_buf[5] = " ";
    int  char_len    = 1;
    char first = char_at_point();
    if (first) {
        int seq_len = utf8_seq_len_fwd(&first);
        Buffer *buf = buffer_get_current();
        size_t pos  = point_get(buf).pos;
        for (int i = 0; i < seq_len; i++)
            char_buf[i] = (char)buf_char_at(buf, pos + i);
        char_buf[seq_len] = '\0';
        char_len = seq_len;
    }

    // Width: 2px for THIN, measured char width for all others
    if (cursor == CURSOR_THIN) {
        d->width = MAX(1, 2 * state->ui.scale_factor);
        d->char_len = 0;
    } else {
        TTF_Font *font = SDL_Clay_GetRenderFont(&state->rendererData,
                                                font_id, (float)font_size);
        int w = 0, h = 0;
        if (font) TTF_GetStringSize(font, char_buf, char_len, &w, &h);
        d->width = w > 0 ? (float)w : MAX(1, 2 * state->ui.scale_factor);

        // Store char for SOLID overlay only
        if (cursor == CURSOR_SOLID) {
            for (int i = 0; i < char_len; i++) d->char_buf[i] = char_buf[i];
            d->char_buf[char_len] = '\0';
            d->char_len = char_len;
        } else {
            d->char_len = 0;
        }
    }

    float y_off = 0.0f;
    if (cursor == CURSOR_UNDER) {
        y_off     = (float)line_height - 2.0f;
        d->height = MAX(1, 2 * state->ui.scale_factor);
    } else if (cursor == CURSOR_HOLLOW) {
        d->bg_color = (Clay_Color){0};
    }

    CLAY(CLAY_IDI_LOCAL("Cursor", index), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .offset = { .x = offset, .y = y_off }
        },
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_FIT(d->width),
                .height = CLAY_SIZING_FIXED(d->height)
            }
        },
        .custom = { .customData = d }
    }) {}
}
