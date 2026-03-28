#include <string.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "cursor.h"
#include "message.h"
#include "theme.h"
#include "vline.h"
#include "../clay/renderer.h"
#include "../text/buffer.h"
#include "../state.h"

extern Clay_String message_string;

void MinibufArea(AppState *state) {
    Buffer *buf = state->minibuf.buf;

    // Temporarily make the minibuf the current buffer so that char_at_point()
    // inside Cursor() returns the right character.
    Buffer *saved = buffer_get_current();
    buffer_set_current(buf);

    char *raw = buffer_text(buf);
    const char *text = raw ? raw : "";
    size_t text_len = strlen(text);
    size_t point_pos = point_get(buf).pos;
    if (point_pos > text_len) point_pos = text_len;

    size_t prompt_len = strlen(state->minibuf.prompt);

    // Build display string: prompt + full text (no cursor marker)
    static char minibuf_display[4096 + MINIBUF_PROMPT_MAX + 1];
    char *dst = minibuf_display;
    memcpy(dst, state->minibuf.prompt, prompt_len);
    dst += prompt_len;
    memcpy(dst, text, text_len);
    dst += text_len;
    *dst = '\0';

    free(raw);

    Clay_String display_str = {
        .chars  = minibuf_display,
        .length = (int32_t)(prompt_len + text_len)
    };

    // Measure x offset for the cursor: width of (prompt + text[0..point_pos))
    uint16_t font_size = (uint16_t)(14.0f * state->ui.scale_factor);
    TTF_Font *font = SDL_Clay_GetRenderFont(&state->rendererData, FONT_BUF_NORMAL, (float)font_size);

    float cursor_x = 0.0f;
    size_t prefix_len = prompt_len + point_pos;
    if (prefix_len > 0) {
        int w = 0, h = 0;
        TTF_GetStringSize(font, minibuf_display, prefix_len, &w, &h);
        cursor_x = (float)w;
    }

    float left_pad  = 10.0f * state->ui.scale_factor;
    int   line_h    = vline_get_line_height(&state->rendererData,
                                            FONT_BUF_NORMAL, font_size);

    CLAY(CLAY_ID("Minibuf Area"), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(16.0 * state->ui.scale_factor + 8)
            },
            .padding = {
                .left  = left_pad,
                .right = left_pad
            }
        },
    }) {
        CLAY_TEXT(display_str, CLAY_TEXT_CONFIG({
            .fontId    = FONT_BUF_NORMAL,
            .fontSize  = font_size,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
        if (state->cursor_visible)
            Cursor(state, 0, cursor_x + left_pad, line_h,
                   0.0f, 0.0f, 65535.0f, 65535.0f,
                   FONT_BUF_NORMAL, font_size);
    }

    buffer_set_current(saved);
}

void MessageArea(AppState *state) {
    CLAY(CLAY_ID("Echo Area"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(16.0 * state->ui.scale_factor + 8)
            },
            .padding = {
                .left = 10.0 * state->ui.scale_factor,
                .right = 10.0 * state->ui.scale_factor
            }
        },
        .clip = CLAY_CLIP_TO_NONE
    }){
        CLAY_TEXT(message_string, CLAY_TEXT_CONFIG({
            .fontId = FONT_BUF_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}
