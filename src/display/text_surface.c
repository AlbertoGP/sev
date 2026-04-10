#include "text_surface.h"
#include "../clay/renderer.h"
#include <SDL3_ttf/SDL_ttf.h>

float text_measure_space(Clay_SDL3RendererData *renderer, uint16_t font_id, uint16_t font_size) {
    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    int sw = 0, sh = 0;
    TTF_GetStringSize(font, " ", 1, &sw, &sh);
    return (float)sw;
}

float text_tab_stop_width(float x_accum, int tab_width, float space_w) {
    if (tab_width <= 0) tab_width = 4;
    int col = (int)(x_accum / space_w);
    return (float)(((col / tab_width) + 1) * tab_width - col) * space_w;
}

float text_measure_tab_aware(Clay_SDL3RendererData *renderer, uint16_t font_id, uint16_t font_size,
                             const char *text, size_t len, int tab_width) {
    if (len == 0) return 0.0f;
    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    float space_w = text_measure_space(renderer, font_id, font_size);
    if (tab_width <= 0) tab_width = 4;

    float x = 0.0f;
    size_t i = 0;
    while (i < len) {
        if (text[i] == '\t') {
            x += text_tab_stop_width(x, tab_width, space_w);
            i++;
        } else {
            size_t j = i;
            while (j < len && text[j] != '\t') j++;
            int w = 0, h = 0;
            TTF_GetStringSize(font, text + i, j - i, &w, &h);
            x += (float)w;
            i = j;
        }
    }
    return x;
}

void text_emit_tab_aware(Clay_SDL3RendererData *renderer, int32_t *run_counter, float *x_accum,
                         uint16_t font_id, uint16_t font_size, int tab_width, int line_height,
                         Clay_Color text_color, Clay_Color bg_color,
                         const char *text, size_t len) {
    if (len == 0) return;

    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    float space_w = text_measure_space(renderer, font_id, font_size);
    if (tab_width <= 0) tab_width = 4;

    size_t p = 0;
    while (p < len) {
        size_t t = p;
        while (t < len && text[t] != '\t') t++;

        if (t > p) {
            // Emit run
            int w = 0, h = 0;
            TTF_GetStringSize(font, text + p, t - p, &w, &h);
            float run_w = (float)w;

            if (x_accum) *x_accum += run_w;

            if (run_w > 0.0f) {
                Clay_String cs = { .chars = text + p, .length = (int32_t)(t - p) };
                CLAY(CLAY_IDI_LOCAL("Seg", (*run_counter)++), {
                    .layout = { .sizing = {
                        .width  = CLAY_SIZING_FIXED(run_w),
                        .height = CLAY_SIZING_FIXED((float)line_height)
                    }},
                    .backgroundColor = bg_color,
                }) {
                    CLAY_TEXT(cs, CLAY_TEXT_CONFIG({
                        .fontId    = font_id,
                        .fontSize  = font_size,
                        .textColor = text_color,
                        .wrapMode  = CLAY_TEXT_WRAP_NONE
                    }));
                }
            }
        }

        if (t < len) { // text[t] == '\t'
            // Emit tab
            float tab_px = text_tab_stop_width(x_accum ? *x_accum : 0.0f, tab_width, space_w);
            if (x_accum) *x_accum += tab_px;

            if (tab_px > 0.0f) {
                CLAY(CLAY_IDI_LOCAL("Tab", (*run_counter)++), {
                    .layout = { .sizing = {
                        .width  = CLAY_SIZING_FIXED(tab_px),
                        .height = CLAY_SIZING_FIXED((float)line_height)
                    }},
                }) {}
            }
            t++;
        }
        p = t;
    }
}

