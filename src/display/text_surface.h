#pragma once

#include "../clay/clay.h"
#include "../clay/renderer.h"
#include <stddef.h>
#include <stdint.h>

// Returns pixel width of a space for the given font
float text_measure_space(Clay_SDL3RendererData *renderer, uint16_t font_id,
                         uint16_t font_size);

// Returns width to reach the next tab stop starting at x_accum
float text_tab_stop_width(float x_accum, int tab_width, float space_w);

// Measures total generic width of text containing tabs
float text_measure_tab_aware(Clay_SDL3RendererData *renderer, uint16_t font_id,
                             uint16_t font_size, const char *text, size_t len,
                             int tab_width);

// Emits CLAY segments for tab-aware text, updating *x_accum pointer.
// Handles breaking string at tabs and emitting text runs and tab spacers.
void text_emit_tab_aware(Clay_SDL3RendererData *renderer, int32_t *run_counter,
                         float *x_accum, uint16_t font_id, uint16_t font_size,
                         int tab_width, int line_height, Clay_Color text_color,
                         Clay_Color bg_color, const char *text, size_t len);
