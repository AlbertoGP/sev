// Visual line cache implementation.

#include "vline.h"
#include "../subeditor/buffer.h"
#include "../subeditor/line.h"
#include "../clay/renderer.h"
#include <stdlib.h>
#include <string.h>

#define MIN_CACHE_SIZE 16
#define TAB_WIDTH 4

// Measure the width of a substring using the font.
static float measure_text(Clay_SDL3RendererData *renderer,
                          uint16_t font_id, uint16_t font_size,
                          const char *text, size_t len) {
    if (len == 0) return 0.0f;

    TTF_Font *font = renderer->fonts[font_id];
    TTF_SetFontSize(font, font_size);

    int w = 0, h = 0;
    TTF_GetStringSize(font, text, len, &w, &h);
    return (float)w;
}

int vline_get_line_height(Clay_SDL3RendererData *renderer,
                          uint16_t font_id, uint16_t font_size) {
    TTF_Font *font = renderer->fonts[font_id];
    TTF_SetFontSize(font, font_size);
    return TTF_GetFontHeight(font);
}

VLineCache vline_cache_create(void) {
    VLineCache cache = {0};
    cache.lines = calloc(MIN_CACHE_SIZE, sizeof(VisualLine));
    if (cache.lines) {
        cache.cap = MIN_CACHE_SIZE;
    }
    cache.full_rebuild = true;
    return cache;
}

void vline_cache_destroy(VLineCache *cache) {
    if (cache->lines) {
        free(cache->lines);
        cache->lines = NULL;
    }
    cache->count = 0;
    cache->cap = 0;
}

// Grow cache capacity using doubling strategy.
static bool vline_cache_grow(VLineCache *cache, size_t needed) {
    if (cache->cap >= needed) return true;

    size_t new_cap = cache->cap ? cache->cap : MIN_CACHE_SIZE;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    VisualLine *new_lines = realloc(cache->lines, new_cap * sizeof(VisualLine));
    if (!new_lines) return false;

    cache->lines = new_lines;
    cache->cap = new_cap;
    return true;
}

// Append a visual line to the cache.
static bool vline_append(VLineCache *cache, VisualLine vline) {
    if (cache->count >= cache->cap) {
        if (!vline_cache_grow(cache, cache->count + 1)) {
            return false;
        }
    }
    cache->lines[cache->count++] = vline;
    return true;
}

// Check if a character is a word boundary for wrapping.
static bool is_word_boundary(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\0';
}

// Wrap a single logical line into visual lines.
// Returns the number of visual lines produced.
static size_t wrap_logical_line(VLineCache *cache,
                                Clay_SDL3RendererData *renderer,
                                uint16_t font_id, uint16_t font_size,
                                const char *buf_text, const Line *line,
                                float max_width) {
    size_t start = line->start;
    size_t end = line->end;
    uint16_t visual_index = 0;
    size_t lines_added = 0;

    // Handle empty lines
    if (start >= end) {
        VisualLine vline = {
            .line_id = line->line_id,
            .line_version = line->version,
            .byte_start = start,
            .byte_end = end,
            .visual_index = 0,
            .measured_width = 0.0f
        };
        if (vline_append(cache, vline)) {
            lines_added++;
        }
        return lines_added;
    }

    size_t line_start = start;

    while (line_start < end) {
        size_t line_end = line_start;
        size_t last_break = line_start;
        float width = 0.0f;

        // Scan forward, tracking last word boundary
        while (line_end < end) {
            char c = buf_text[line_end];

            // Measure incrementally
            float char_width = measure_text(renderer, font_id, font_size,
                                            &buf_text[line_end], 1);

            // Handle tabs - approximate as spaces to next tab stop
            if (c == '\t') {
                // Calculate current column position
                float space_width = measure_text(renderer, font_id, font_size, " ", 1);
                int col = (int)(width / space_width);
                int next_tab = ((col / TAB_WIDTH) + 1) * TAB_WIDTH;
                char_width = (next_tab - col) * space_width;
            }

            // Would this character overflow?
            if (width + char_width > max_width && line_end > line_start) {
                // Try to break at word boundary
                if (last_break > line_start) {
                    line_end = last_break;
                }
                // Otherwise break at current position (mid-word)
                break;
            }

            width += char_width;
            line_end++;

            // Track word boundaries
            if (is_word_boundary(c)) {
                last_break = line_end;
            }
        }

        // Create visual line
        VisualLine vline = {
            .line_id = line->line_id,
            .line_version = line->version,
            .byte_start = line_start,
            .byte_end = line_end,
            .visual_index = visual_index++,
            .measured_width = width
        };

        if (vline_append(cache, vline)) {
            lines_added++;
        }

        line_start = line_end;
    }

    return lines_added;
}

void vline_rebuild(VLineCache *cache, struct Buffer *buf,
                   Clay_SDL3RendererData *renderer,
                   float pane_width, uint16_t font_id, uint16_t font_size) {
    if (!cache || !buf || !renderer) return;

    // Check if we need a full rebuild
    bool needs_rebuild = cache->full_rebuild ||
                         cache->pane_width != pane_width ||
                         cache->font_id != font_id ||
                         cache->font_size != font_size;

    if (!needs_rebuild) {
        // TODO: incremental rebuild by checking line versions
        // For now, always rebuild
        needs_rebuild = true;
    }

    if (!needs_rebuild) return;

    // Reset cache
    cache->count = 0;
    cache->pane_width = pane_width;
    cache->font_id = font_id;
    cache->font_size = font_size;
    cache->full_rebuild = false;

    // Get buffer text and line table
    const char *text = buffer_text(buf);
    const LineTable *lt = buffer_get_line_table(buf);
    if (!lt || !lt->lines) return;

    // Wrap each logical line
    for (size_t i = 0; i < lt->count; i++) {
        wrap_logical_line(cache, renderer, font_id, font_size,
                          text, &lt->lines[i], pane_width);
    }
}

void vline_scroll_to_cursor(VLineCache *cache, size_t byte_pos, size_t visible_count) {
    if (!cache || cache->count == 0 || visible_count == 0) return;

    size_t cursor_vline = vline_for_byte_pos(cache, byte_pos);
    if (cursor_vline >= cache->count) {
        cursor_vline = cache->count - 1;
    }

    // Scroll up if cursor is above viewport
    if (cursor_vline < cache->top_vline) {
        cache->top_vline = cursor_vline;
    }
    // Scroll down if cursor is below viewport
    else if (cursor_vline >= cache->top_vline + visible_count) {
        cache->top_vline = cursor_vline - visible_count + 1;
    }
}

size_t vline_for_byte_pos(const VLineCache *cache, size_t byte_pos) {
    if (!cache || cache->count == 0) return 0;

    // Binary search for the visual line containing byte_pos
    size_t lo = 0, hi = cache->count;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        const VisualLine *vl = &cache->lines[mid];

        if (byte_pos < vl->byte_start) {
            hi = mid;
        } else if (byte_pos >= vl->byte_end && mid + 1 < cache->count) {
            // Check if we're at line boundary
            if (byte_pos == vl->byte_end && vl->byte_start < vl->byte_end) {
                // Cursor at end of non-empty line belongs to this line
                return mid;
            }
            lo = mid + 1;
        } else {
            return mid;
        }
    }

    // Return last line if beyond end
    return lo < cache->count ? lo : cache->count - 1;
}
