
#include <string.h>
#include <stdlib.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "cursor.h"
#include "theme.h"
#include "vline.h"
#include "../clay/renderer.h"
#include "../text/buffer.h"
#include "../state.h"

void MinibufPalette(AppState *state) {
    if (!state->minibuf.active) return;

    int win_w = 0, win_h = 0;
    SDL_GetWindowSizeInPixels(state->window, &win_w, &win_h);

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

    // Refresh provider items based on current input.
    if (state->minibuf.provider)
        state->minibuf.provider(state, minibuf_text);

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

    float pad    = 5.0f * scale;
    int   line_h = vline_get_line_height(&state->rendererData, FONT_UI_NORMAL, font_size);

    Clay_Color text_color = placeholder
        ? ui_resolve_color(state, state->ui.roles.text_faded)
        : ui_resolve_color(state, state->ui.roles.text_primary);

    CLAY(CLAY_ID("MinibufBackdrop"), {
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                .parent  = CLAY_ATTACH_POINT_LEFT_TOP
            },
            .offset = { 0, 0 },
            .zIndex = 149
        },
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_FIXED((float)win_w),
                .height = CLAY_SIZING_FIXED((float)win_h)
            }
        },
        .backgroundColor = { 0, 0, 0, 0 }
    }) {}

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
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.ui_bg),
        .border = {
            .color = ui_resolve_color(state, state->ui.roles.border_active),
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1 }
        },
        .cornerRadius = CLAY_CORNER_RADIUS(6.0f * scale)
    }) {
        CLAY(CLAY_ID("MinibufPaletteContent"), {
            .layout = {
                .sizing = {
                    .width  = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIT(0)
                },
                .padding = { 
                    .left = 2 * pad,
                    .right = 2 * pad, 
                    .top = 8.0 * scale,
                    .bottom = 8.0 * scale
                },
            },
        }) {
            CLAY_TEXT(display_str, CLAY_TEXT_CONFIG({
                .fontId    = FONT_UI_NORMAL,
                .fontSize  = font_size,
                .textColor = text_color,
                .wrapMode  = CLAY_TEXT_WRAP_NONE
            }));
            if (state->cursor_visible)
                Cursor(state, 0, cursor_x + 2.0f * pad, 8.0f * scale, line_h,
                       0.0f, 0.0f, 65535.0f, 65535.0f,
                       FONT_UI_NORMAL, font_size, 151);
        }

        // Item list (command palette results)
        if (state->minibuf.provider && state->minibuf.item_count > 0) {
            int visible = state->minibuf.item_count < MINIBUF_VISIBLE_ITEMS
                        ? state->minibuf.item_count : MINIBUF_VISIBLE_ITEMS;
            CLAY(CLAY_ID_LOCAL("MinibufDivider"), {
                .layout = {
                    .sizing = { .width = CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(pad)    ,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .border = {
                    .color = ui_resolve_color(state, state->ui.roles.border_active),
                    .width = { .top = 1 }
                }
            }) {
                Clay_Color sel = ui_resolve_color(state, state->ui.roles.selection);
                int scroll = state->minibuf.item_scroll;
                for (int i = 0; i < visible; i++) {
                    int abs_idx = scroll + i;
                    bool is_selected = (abs_idx == state->minibuf.selected);
                    Clay_String label = {
                        .chars  = state->minibuf.items[abs_idx].label,
                        .length = (int32_t)strlen(state->minibuf.items[abs_idx].label)
                    };
                    bool has_kb = state->minibuf.items[abs_idx].keybinding[0] != '\0';
                    CLAY(CLAY_IDI_LOCAL("MinibufItem", i), {
                        .layout = {
                            .sizing         = { .width = CLAY_SIZING_GROW(0) },
                            .padding        = CLAY_PADDING_ALL(pad),
                            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                            .childGap       = (uint16_t)(pad * 2)
                        },
                        .backgroundColor = Clay_Hovered()
                            ? (Clay_Color){sel.r, sel.g, sel.b, sel.a / 2}
                            : (is_selected
                                ? sel
                                : (Clay_Color){0, 0, 0, 0}),
                        .cornerRadius = CLAY_CORNER_RADIUS(4.0f * scale)
                    }) {
                    if (Clay_Hovered()) state->input.desired_cursor = SDL_SYSTEM_CURSOR_POINTER;
                        CLAY(CLAY_IDI_LOCAL("MinibufItemLabel", i), {
                            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
                        }) {
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                                .fontId    = FONT_UI_NORMAL,
                                .fontSize  = font_size,
                                .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
                                .wrapMode  = CLAY_TEXT_WRAP_NONE
                            }));
                        }
                        if (has_kb) {
                            Clay_String kb = {
                                .chars  = state->minibuf.items[scroll + i].keybinding,
                                .length = (int32_t)strlen(state->minibuf.items[scroll + i].keybinding)
                            };
                            CLAY_TEXT(kb, CLAY_TEXT_CONFIG({
                                .fontId    = FONT_UI_NORMAL,
                                .fontSize  = font_size,
                                .textColor = ui_resolve_color(state, state->ui.roles.text_key),
                                .wrapMode  = CLAY_TEXT_WRAP_NONE
                            }));
                        }
                    }
                }
            }
        }
    }

    int visible_count = (state->minibuf.provider && state->minibuf.item_count > 0)
        ? (state->minibuf.item_count < MINIBUF_VISIBLE_ITEMS
           ? state->minibuf.item_count : MINIBUF_VISIBLE_ITEMS)
        : 0;
    // MinibufPaletteContent: top=8*scale + line_h + bottom=8*scale
    float input_h = 16.0f * scale + (float)line_h;
    // MinibufDivider: 2*pad wrapper + visible items each (pad + line_h + pad)
    float items_h = visible_count > 0
        ? 2.0f * pad + (float)visible_count * (2.0f * pad + (float)line_h)
        : 0.0f;
    state->minibuf.palette_w = 520.0f * scale;
    state->minibuf.palette_h = input_h + items_h;
    state->minibuf.palette_x = ((float)win_w - state->minibuf.palette_w) / 2.0f;
    state->minibuf.palette_y = 64.0f * scale;
    // Item geometry for click hit-testing in event.c
    float item_h = 2.0f * pad + (float)line_h;
    state->minibuf.palette_item_h  = visible_count > 0 ? item_h : 0.0f;
    state->minibuf.palette_items_y = visible_count > 0
        ? state->minibuf.palette_y + input_h + pad
        : 0.0f;

    buffer_set_current(saved);
}
