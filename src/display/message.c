#include <string.h>
#include <stdlib.h>

#include "message.h"
#include "theme.h"
#include "../text/buffer.h"
#include "../state.h"

extern Clay_String message_string;

void MinibufArea(AppState *state) {
    Buffer *buf = state->minibuf.buf;
    char *raw = buffer_text(buf);
    const char *text = raw ? raw : "";
    size_t text_len = strlen(text);
    size_t point_pos = point_get(buf).pos;
    if (point_pos > text_len) point_pos = text_len;

    size_t prompt_len = strlen(state->minibuf.prompt);
    static char minibuf_display[4096 + MINIBUF_PROMPT_MAX + 2];

    char *dst = minibuf_display;
    memcpy(dst, state->minibuf.prompt, prompt_len);
    dst += prompt_len;
    memcpy(dst, text, point_pos);
    dst += point_pos;
    *dst++ = '|';
    size_t after_len = text_len - point_pos;
    memcpy(dst, text + point_pos, after_len);
    dst += after_len;
    *dst = '\0';

    free(raw);

    Clay_String display_str = {
        .chars  = minibuf_display,
        .length = (int32_t)(dst - minibuf_display)
    };

    CLAY(CLAY_ID("Minibuf Area"), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(16.0 * state->ui.scale_factor + 8)
            },
            .padding = {
                .left  = 10.0 * state->ui.scale_factor,
                .right = 10.0 * state->ui.scale_factor
            }
        },
    }) {
        CLAY_TEXT(display_str, CLAY_TEXT_CONFIG({
            .fontId    = FONT_NORMAL,
            .fontSize  = 14.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
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
            .fontId = FONT_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
    }
}

