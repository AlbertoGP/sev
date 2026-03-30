
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

void MinibufPalette(AppState *state) {
    if (!state->minibuf.active) return;

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

    // Copy text into static buffer before freeing raw, so display_str stays valid.
    static char minibuf_text[4096 + 1];
    if (text_len > sizeof(minibuf_text) - 1) text_len = sizeof(minibuf_text) - 1;
    memcpy(minibuf_text, text, text_len);
    minibuf_text[text_len] = '\0';
    free(raw);

    // When empty, show the prompt as faded placeholder text.
    // When non-empty, show only the user's input in the normal color.
    bool placeholder = (text_len == 0);

    Clay_String display_str = placeholder
        ? (Clay_String){ .chars = state->minibuf.prompt, .length = (int32_t)prompt_len }
        : (Clay_String){ .chars = minibuf_text,          .length = (int32_t)text_len   };

    float scale = state->ui.scale_factor;
    uint16_t font_size = (uint16_t)(12.0f * scale);
    TTF_Font *font = SDL_Clay_GetRenderFont(&state->rendererData, FONT_UI_NORMAL, (float)font_size);

    // Cursor x: measure width of text[0..point_pos) (no prompt prefix).
    float cursor_x = 0.0f;
    if (!placeholder && point_pos > 0) {
        int w = 0, h = 0;
        TTF_GetStringSize(font, minibuf_text, point_pos, &w, &h);
        cursor_x = (float)w;
    }

    float pad    = 12.0f * scale;
    int   line_h = vline_get_line_height(&state->rendererData, FONT_UI_NORMAL, font_size);

    Clay_Color text_color = placeholder
        ? ui_resolve_color(state, state->ui.roles.text_faded)
        : ui_resolve_color(state, state->ui.roles.text_primary);

    CLAY(CLAY_ID("MinibufPalette"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_CENTER_TOP,
                .parent  = CLAY_ATTACH_POINT_CENTER_TOP
            },
            .offset = { 0, 64.0f * scale },
            .zIndex = 150
        },
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_FIXED(520.0f * scale),
                .height = CLAY_SIZING_FIT(0)
            },
            .padding = { .left = pad, .right = pad, .top = pad, .bottom = pad }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
        .border = {
            .color = ui_resolve_color(state, state->ui.roles.border_active),
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1 }
        },
        .cornerRadius = CLAY_CORNER_RADIUS(6.0f * scale)
    }) {
        // Inner wrapper with no padding so Cursor()'s CLAY_ATTACH_TO_PARENT
        // origin aligns with the text baseline (not the outer padding box).
        CLAY(CLAY_ID("MinibufPaletteContent"), {
            .layout = {
                .sizing = {
                    .width  = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIT(0)
                }
            }
        }) {
            CLAY_TEXT(display_str, CLAY_TEXT_CONFIG({
                .fontId    = FONT_UI_NORMAL,
                .fontSize  = font_size,
                .textColor = text_color,
                .wrapMode  = CLAY_TEXT_WRAP_NONE
            }));
            if (state->cursor_visible)
                Cursor(state, 0, cursor_x, line_h,
                       0.0f, 0.0f, 65535.0f, 65535.0f,
                       FONT_UI_NORMAL, font_size, 151);
        }
    }

    buffer_set_current(saved);
}
