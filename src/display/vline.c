// Visual line cache implementation.

#include <stdlib.h>
#include <string.h>

#include "vline.h"
#include "../text/buffer.h"
#include "../text/line.h"
#include "../text/utf8.h"
#include "../clay/renderer.h"

#define MIN_CACHE_SIZE 16
#define TAB_WIDTH 4

// Measure the width of a substring using the font.
static float measure_text(Clay_SDL3RendererData *renderer,
                          uint16_t font_id, uint16_t font_size,
                          const char *text, size_t len) {
    if (len == 0) return 0.0f;
    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    int w = 0, h = 0;
    TTF_GetStringSize(font, text, len, &w, &h);
    return (float)w;
}

int vline_get_line_height(Clay_SDL3RendererData *renderer,
                          uint16_t font_id, uint16_t font_size) {
    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    return TTF_GetFontHeight(font);
}

VLineCache vline_cache_create(void) {
    VLineCache cache = {0};
    cache.lines = calloc(MIN_CACHE_SIZE, sizeof(VisualLine));
    if (cache.lines) {
        cache.cap = MIN_CACHE_SIZE;
    }
    cache.index = calloc(MIN_CACHE_SIZE, sizeof(LogicalLineIndex));
    if (cache.index) {
        cache.index_cap = MIN_CACHE_SIZE;
    }
    cache.full_rebuild = true;
    return cache;
}

void vline_cache_destroy(VLineCache *cache) {
    if (cache->lines) {
        free(cache->lines);
        cache->lines = NULL;
    }
    if (cache->index) {
        free(cache->index);
        cache->index = NULL;
    }
    cache->count = 0;
    cache->cap = 0;
    cache->index_count = 0;
    cache->index_cap = 0;
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

// Shrink cache if underutilized.
static void vline_cache_shrink(VLineCache *cache) {
    if (cache->count >= cache->cap / 4) return;

    size_t new_cap = cache->cap / 2;
    if (new_cap < MIN_CACHE_SIZE) new_cap = MIN_CACHE_SIZE;
    if (new_cap >= cache->cap || new_cap < cache->count) return;

    VisualLine *new_lines = realloc(cache->lines, new_cap * sizeof(VisualLine));
    if (!new_lines) return;
    cache->lines = new_lines;
    cache->cap = new_cap;
}

// Grow index capacity.
static bool index_grow(VLineCache *cache, size_t needed) {
    if (cache->index_cap >= needed) return true;

    size_t new_cap = cache->index_cap ? cache->index_cap : MIN_CACHE_SIZE;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    LogicalLineIndex *new_index = realloc(cache->index, new_cap * sizeof(LogicalLineIndex));
    if (!new_index) return false;

    cache->index = new_index;
    cache->index_cap = new_cap;
    return true;
}

// Shrink index if underutilized.
static void index_shrink(VLineCache *cache) {
    if (cache->index_count >= cache->index_cap / 4) return;

    size_t new_cap = cache->index_cap / 2;
    if (new_cap < MIN_CACHE_SIZE) new_cap = MIN_CACHE_SIZE;
    if (new_cap >= cache->index_cap || new_cap < cache->index_count) return;

    LogicalLineIndex *new_index = realloc(cache->index, new_cap * sizeof(LogicalLineIndex));
    if (!new_index) return;
    cache->index = new_index;
    cache->index_cap = new_cap;
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
// Appends to cache->lines starting at vline_start_out, returns count added.
// Also records index entry at cache->index[cache->index_count].
static size_t wrap_logical_line(VLineCache *cache,
                                Clay_SDL3RendererData *renderer,
                                uint16_t font_id, uint16_t font_size,
                                const char *buf_text, const Line *line,
                                float max_width) {
    size_t vline_start = cache->count;
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
        goto record_index;
    }

    size_t line_start = start;

    while (line_start < end) {
        size_t line_end = line_start;
        size_t last_break = line_start;
        float width = 0.0f;

        // Scan forward, tracking last word boundary
        while (line_end < end) {
            char c = buf_text[line_end];
            int seq_len = utf8_seq_len_fwd(&buf_text[line_end]);

            // Measure incrementally
            float char_width = measure_text(renderer, font_id, font_size,
                                            &buf_text[line_end], seq_len);

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
            line_end += seq_len;

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

record_index:
    // Record index entry
    if (index_grow(cache, cache->index_count + 1)) {
        cache->index[cache->index_count++] = (LogicalLineIndex){
            .line_id = line->line_id,
            .version = line->version,
            .vline_start = vline_start,
            .vline_count = lines_added
        };
    }

    return lines_added;
}

// Find old index entry by line_id. Returns NULL if not found.
// Uses hint as starting position for scan.
static const LogicalLineIndex *find_old_index(const LogicalLineIndex *old_index,
                                               size_t old_count, uint64_t line_id,
                                               size_t hint) {
    if (!old_index || old_count == 0) return NULL;

    // Try hint position first
    if (hint < old_count && old_index[hint].line_id == line_id) {
        return &old_index[hint];
    }

    if (hint >= old_count) hint = old_count - 1;

    // Scan from hint forward, then backward
    for (size_t off = 1; off < old_count; off++) {
        if (hint + off < old_count && old_index[hint + off].line_id == line_id) {
            return &old_index[hint + off];
        }
        if (off <= hint && hint - off < old_count && old_index[hint - off].line_id == line_id) {
            return &old_index[hint - off];
        }
    }
    return NULL;
}

void vline_rebuild(VLineCache *cache, struct Buffer *buf,
                   Clay_SDL3RendererData *renderer,
                   float pane_width, uint16_t font_id, uint16_t font_size) {
    if (!cache || !buf || !renderer) return;

    // Check if we need a full rebuild (cache key changed)
    bool full_rebuild = cache->full_rebuild ||
                        cache->pane_width != pane_width ||
                        cache->font_id != font_id ||
                        cache->font_size != font_size;

    cache->pane_width = pane_width;
    cache->font_id = font_id;
    cache->font_size = font_size;
    cache->full_rebuild = false;

    const LineTable *lt = buffer_get_line_table(buf);
    if (!lt || !lt->lines) return;

    const char *text = buffer_text(buf);
    if (!text) { cache->full_rebuild = true; return; }

    if (full_rebuild) {
        // Full rebuild: reset and re-wrap everything
        cache->count = 0;
        cache->index_count = 0;

        for (size_t i = 0; i < lt->count; i++) {
            wrap_logical_line(cache, renderer, font_id, font_size,
                              text, &lt->lines[i], pane_width);
        }
    } else {
        // Incremental rebuild: check line versions, copy valid entries
        // Save old arrays
        VisualLine *old_lines = cache->lines;
        size_t old_cap = cache->cap;
        LogicalLineIndex *old_index = cache->index;
        size_t old_index_count = cache->index_count;
        size_t old_index_cap = cache->index_cap;

        // Allocate new arrays
        cache->lines = calloc(MIN_CACHE_SIZE, sizeof(VisualLine));
        cache->cap = cache->lines ? MIN_CACHE_SIZE : 0;
        cache->count = 0;
        cache->index = calloc(MIN_CACHE_SIZE, sizeof(LogicalLineIndex));
        cache->index_cap = cache->index ? MIN_CACHE_SIZE : 0;
        cache->index_count = 0;

        if (!cache->lines || !cache->index) {
            // Allocation failed, restore old and do full rebuild
            free(cache->lines);
            free(cache->index);
            cache->lines = old_lines;
            cache->cap = old_cap;
            cache->index = old_index;
            cache->index_cap = old_index_cap;
            cache->full_rebuild = true;
            free((char*)text);
            vline_rebuild(cache, buf, renderer, pane_width, font_id, font_size);
            return;
        }

        for (size_t i = 0; i < lt->count; i++) {
            const Line *line = &lt->lines[i];
            const LogicalLineIndex *old_entry = find_old_index(old_index, old_index_count,
                                                                line->line_id, i);

            if (old_entry && old_entry->version == line->version) {
                // Version matches - copy visual lines from old cache
                // Need to adjust byte_start/byte_end since line positions may have shifted
                size_t copy_count = old_entry->vline_count;
                if (vline_cache_grow(cache, cache->count + copy_count) &&
                    index_grow(cache, cache->index_count + 1)) {

                    size_t new_vline_start = cache->count;
                    // Calculate offset between old and new line positions
                    VisualLine *first_old = &old_lines[old_entry->vline_start];
                    ptrdiff_t offset = (ptrdiff_t)line->start - (ptrdiff_t)first_old->byte_start;

                    for (size_t j = 0; j < copy_count; j++) {
                        VisualLine vl = old_lines[old_entry->vline_start + j];
                        vl.byte_start = (size_t)((ptrdiff_t)vl.byte_start + offset);
                        vl.byte_end = (size_t)((ptrdiff_t)vl.byte_end + offset);
                        cache->lines[cache->count++] = vl;
                    }
                    cache->index[cache->index_count++] = (LogicalLineIndex){
                        .line_id = line->line_id,
                        .version = line->version,
                        .vline_start = new_vline_start,
                        .vline_count = copy_count
                    };
                }
            } else {
                // Version mismatch or not found - re-wrap
                wrap_logical_line(cache, renderer, font_id, font_size,
                                  text, line, pane_width);
            }
        }

        // Free old arrays
        free(old_lines);
        free(old_index);
    }

    free((char*)text);

    // Shrink if underutilized
    vline_cache_shrink(cache);
    index_shrink(cache);
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

    // If entire buffer fits in the viewport, always show from top.
    if (cache->count <= visible_count)
        cache->top_vline = 0;
}

size_t vline_clamp_byte_pos_to_viewport(const VLineCache *cache,
                                        size_t byte_pos,
                                        size_t visible_count) {
    if (!cache || cache->count == 0 || visible_count == 0) return byte_pos;

    size_t cursor_vline = vline_for_byte_pos(cache, byte_pos);
    if (cursor_vline >= cache->count)
        cursor_vline = cache->count - 1;

    if (cursor_vline < cache->top_vline)
        return cache->lines[cache->top_vline].byte_start;

    size_t last_visible = cache->top_vline + visible_count - 1;
    if (last_visible >= cache->count) last_visible = cache->count - 1;
    if (cursor_vline > last_visible)
        return cache->lines[last_visible].byte_start;

    return byte_pos;
}

size_t vline_byte_pos_at_xy(const VLineCache *cache, const char *buf_text,
                             float rel_x, float rel_y, int line_height,
                             Clay_SDL3RendererData *renderer,
                             uint16_t font_id, uint16_t font_size) {
    if (!cache || cache->count == 0 || line_height <= 0) return 0;

    // Map rel_y to a visual line index.
    size_t vline_idx = cache->top_vline;
    if (rel_y > 0.0f)
        vline_idx = cache->top_vline + (size_t)(rel_y / line_height);
    if (vline_idx >= cache->count)
        vline_idx = cache->count - 1;

    const VisualLine *vl = &cache->lines[vline_idx];
    const char *line_text = buf_text + vl->byte_start;
    size_t len = vl->byte_end - vl->byte_start;

    // Click left of text area → clamp to line start.
    if (rel_x <= 0.0f) return vl->byte_start;
    // Click past end of line → clamp to line end.
    if (len == 0 || rel_x >= vl->measured_width) return vl->byte_end;

    // Linear scan: find which character was clicked.
    // With a block cursor the cursor covers the character it sits on, so
    // clicking anywhere within a character's pixel extent places the cursor
    // before that character.  Only advance past a character when the click
    // is strictly beyond its right edge.
    float x_accum = 0.0f;
    size_t byte_offset = 0;
    while (byte_offset < len) {
        int seq = utf8_seq_len_fwd(line_text + byte_offset);
        float char_w = measure_text(renderer, font_id, font_size,
                                    line_text + byte_offset, seq);
        if (rel_x < x_accum + char_w)
            return vl->byte_start + byte_offset;
        x_accum += char_w;
        byte_offset += seq;
    }
    return vl->byte_end;
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
            if (byte_pos == vl->byte_end) {
                // Cursor exactly at line's end belongs to this line only if it's the last visual segment
                // AND the next line doesn't start exactly at this point.
                bool is_last = (cache->lines[mid+1].line_id != vl->line_id);
                if (is_last && cache->lines[mid+1].byte_start != byte_pos) {
                    return mid;
                }
            }
            lo = mid + 1;
        } else {
            return mid;
        }
    }

    // Return last line if beyond end
    return lo < cache->count ? lo : cache->count - 1;
}
