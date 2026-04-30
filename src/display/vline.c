// Visual line cache implementation.

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "vline.h"
#include "../text/buffer.h"
#include "../text/line.h"
#include "../text/utf8.h"
#include "../clay/renderer.h"
#include "text_surface.h"

#define MIN_CACHE_SIZE 16


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
    cache.last_scroll_point = (size_t)-1;  // sentinel: force scroll-to-cursor on first render
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

// Emit a single non-wrapped visual line for a logical line.
// Measures the full line width and records one VisualLine entry.
// Updates cache->max_line_width if this line is wider.
static size_t nowrap_logical_line(VLineCache *cache,
                                  Clay_SDL3RendererData *renderer,
                                  uint16_t font_id, uint16_t font_size,
                                  const char *buf_text, const Line *line,
                                  int tab_width) {
    size_t vline_start = cache->count;
    size_t start = line->start;
    size_t end   = line->end;

    // Measure full line width respecting tabs.
    float width = text_measure_tab_aware(renderer, font_id, font_size, buf_text + start, end - start, tab_width);

    VisualLine vline = {
        .line_id      = line->line_id,
        .line_version = line->version,
        .byte_start   = start,
        .byte_end     = end,
        .visual_index = 0,
        .measured_width = width
    };
    size_t lines_added = 0;
    if (vline_append(cache, vline))
        lines_added = 1;

    if (width > cache->max_line_width)
        cache->max_line_width = width;

    if (index_grow(cache, cache->index_count + 1)) {
        cache->index[cache->index_count++] = (LogicalLineIndex){
            .line_id    = line->line_id,
            .version    = line->version,
            .vline_start = vline_start,
            .vline_count = lines_added
        };
    }
    return lines_added;
}

// Wrap a single logical line into visual lines.
// Appends to cache->lines starting at vline_start_out, returns count added.
// Also records index entry at cache->index[cache->index_count].
static size_t wrap_logical_line(VLineCache *cache,
                                Clay_SDL3RendererData *renderer,
                                uint16_t font_id, uint16_t font_size,
                                const char *buf_text, const Line *line,
                                float max_width, int tab_width) {
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

            // Handle tabs - expand to next tab stop
            if (c == '\t') {
                float space_width = text_measure_space(renderer, font_id, font_size);
                char_width = text_tab_stop_width(width, tab_width, space_width);
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
                   float pane_width, uint16_t font_id, uint16_t font_size,
                   int tab_width, bool wrap_lines, uint32_t render_gen) {
    if (!cache || !buf || !renderer) return;
    if (tab_width <= 0) tab_width = 4;

    // Check if we need a full rebuild (cache key changed)
    bool full_rebuild = cache->full_rebuild ||
                        cache->pane_width != pane_width ||
                        cache->font_id != font_id ||
                        cache->font_size != font_size ||
                        cache->tab_width != tab_width ||
                        cache->wrap_lines != wrap_lines;

    // Suppress a second key-change-triggered full rebuild within the same
    // SDL_AppIterate frame.  When needs_extra_frame causes a second layout pass,
    // Clay's updated bounding boxes can produce a slightly different pane_width,
    // which would otherwise trigger a redundant O(chars) full rebuild.
    // Explicit cache->full_rebuild requests (alloc failure, external flag) bypass
    // this guard so they are never suppressed.
    if (full_rebuild && !cache->full_rebuild &&
        cache->last_full_rebuild_gen == render_gen)
        full_rebuild = false;

    cache->pane_width = pane_width;
    cache->font_id = font_id;
    cache->font_size = font_size;
    cache->tab_width = tab_width;
    cache->wrap_lines = wrap_lines;
    cache->full_rebuild = false;

    const LineTable *lt = buffer_get_line_table(buf);
    if (!lt || !lt->lines) return;

    const char *text = buffer_text_cached(buf);
    if (!text) { cache->full_rebuild = true; return; }

    if (full_rebuild) {
        // Full rebuild: reset and re-wrap everything
        cache->count = 0;
        cache->index_count = 0;
        cache->max_line_width = 0.0f;

        for (size_t i = 0; i < lt->count; i++) {
            if (wrap_lines)
                wrap_logical_line(cache, renderer, font_id, font_size,
                                  text, &lt->lines[i], pane_width, tab_width);
            else
                nowrap_logical_line(cache, renderer, font_id, font_size,
                                    text, &lt->lines[i], tab_width);
        }
        cache->last_full_rebuild_gen = render_gen;
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
            vline_rebuild(cache, buf, renderer, pane_width, font_id, font_size, tab_width, wrap_lines, render_gen);
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
                if (wrap_lines)
                    wrap_logical_line(cache, renderer, font_id, font_size,
                                      text, line, pane_width, tab_width);
                else
                    nowrap_logical_line(cache, renderer, font_id, font_size,
                                        text, line, tab_width);
            }
        }

        // Free old arrays
        free(old_lines);
        free(old_index);

        // Recompute max_line_width from all vlines (incremental path may have copied old widths)
        if (!wrap_lines) {
            cache->max_line_width = 0.0f;
            for (size_t i = 0; i < cache->count; i++) {
                if (cache->lines[i].measured_width > cache->max_line_width)
                    cache->max_line_width = cache->lines[i].measured_width;
            }
        }
    }

    // Clamp horizontal scroll to valid range.
    if (!wrap_lines) {
        float max_scroll_x = cache->max_line_width > pane_width
                             ? cache->max_line_width - pane_width : 0.0f;
        if (cache->scroll_x > max_scroll_x) {
            cache->scroll_x = max_scroll_x;
            cache->target_scroll_x = max_scroll_x;
        }
    }

    // Shrink if underutilized
    vline_cache_shrink(cache);
    index_shrink(cache);
}

void vline_scroll_x_to_cursor_pixels(VLineCache *cache,
                                      float cursor_x_in_line,
                                      float viewport_width) {
    if (!cache || viewport_width <= 0.0f) return;
    float margin = 40.0f;
    float scroll = cache->scroll_x;
    if (cursor_x_in_line < scroll + margin)
        scroll = fmaxf(0.0f, cursor_x_in_line - margin);
    else if (cursor_x_in_line > scroll + viewport_width - margin)
        scroll = cursor_x_in_line - viewport_width + margin;
    float max_scroll = cache->max_line_width > viewport_width
                       ? cache->max_line_width - viewport_width : 0.0f;
    cache->scroll_x = fmaxf(0.0f, fminf(scroll, max_scroll));
    cache->target_scroll_x = cache->scroll_x;
}

void vline_scroll_to_cursor_pixels(VLineCache *cache, size_t byte_pos,
                                    float viewport_height, int line_height,
                                    int margin_lines) {
    if (!cache || cache->count == 0 || line_height <= 0) return;

    size_t cursor_vline = vline_for_byte_pos(cache, byte_pos);
    if (cursor_vline >= cache->count)
        cursor_vline = cache->count - 1;

    float lh         = (float)line_height;
    float cursor_top = (float)cursor_vline * lh;
    float cursor_bot = cursor_top + lh;
    float scroll     = cache->scroll_offset;
    float max_scroll = (cache->count > 1) ? (float)(cache->count - 1) * lh : 0.0f;

    if (cursor_top < scroll) {
        float gap = scroll - cursor_top;
        scroll = (gap <= lh)
            ? cursor_top
            : fmaxf(0.0f, cursor_top - (float)margin_lines * lh);
    } else if (cursor_bot > scroll + viewport_height) {
        float gap = cursor_bot - (scroll + viewport_height);
        scroll = (gap <= lh)
            ? cursor_bot - viewport_height
            : cursor_bot + (float)margin_lines * lh - viewport_height;
    }

    if (scroll < 0.0f) scroll = 0.0f;
    if (scroll > max_scroll) scroll = max_scroll;
    cache->scroll_offset = scroll;
    cache->target_scroll = scroll;
}

size_t vline_byte_pos_at_xy(const VLineCache *cache, const char *buf_text,
                             float rel_x, float rel_y, int line_height,
                             Clay_SDL3RendererData *renderer,
                             uint16_t font_id, uint16_t font_size) {
    if (!cache || cache->count == 0 || line_height <= 0) return 0;

    // Map rel_y to a visual line index.
    size_t vline_idx = (size_t)((cache->scroll_offset + rel_y) / line_height);
    if (vline_idx >= cache->count)
        vline_idx = cache->count - 1;

    const VisualLine *vl = &cache->lines[vline_idx];
    const char *line_text = buf_text + vl->byte_start;
    size_t len = vl->byte_end - vl->byte_start;

    // In nowrap mode, clicks are offset by the horizontal scroll.
    float effective_x = rel_x + cache->scroll_x;

    // Click left of text area → clamp to line start.
    if (effective_x <= 0.0f) return vl->byte_start;

    if (len == 0) return vl->byte_start;

    // Walk the line in tab-free runs. Within a run, use TTF_MeasureString to
    // find the click position in one shot — summing per-glyph TTF_GetStringSize
    // widths drifts against the actual rendered advance widths (especially for
    // narrow glyphs like U+2500 "─"), which is why per-char scanning used to
    // mis-report the clicked column on long "─" runs.
    //
    // With a block cursor, a click anywhere within a character's pixel extent
    // places the cursor before that character; we only advance past a char
    // when the click is strictly beyond its right edge.
    float space_w = text_measure_space(renderer, font_id, font_size);
    TTF_Font *font = SDL_Clay_GetRenderFont(renderer, font_id, (float)font_size);
    float x_accum = 0.0f;
    size_t byte_offset = 0;
    size_t last_byte_offset = 0;
    while (byte_offset < len) {
        if (line_text[byte_offset] == '\t') {
            float tab_w = text_tab_stop_width(x_accum, cache->tab_width, space_w);
            if (effective_x < x_accum + tab_w)
                return vl->byte_start + byte_offset;
            last_byte_offset = byte_offset;
            x_accum += tab_w;
            byte_offset += 1;
            continue;
        }

        // Find the end of the tab-free run.
        size_t run_end = byte_offset;
        while (run_end < len && line_text[run_end] != '\t') run_end++;
        size_t run_len = run_end - byte_offset;

        float budget = effective_x - x_accum;
        int budget_px = budget > 0 ? (int)(budget + 0.5f) : 0;

        int mw = 0;
        size_t ml = 0;
        if (font) {
            TTF_MeasureString(font, line_text + byte_offset, run_len,
                              budget_px, &mw, &ml);
        }

        if (ml < run_len) {
            // Click lands within this run — cursor goes before the char at
            // byte_offset + ml (the first char whose extent crosses budget_px).
            return vl->byte_start + byte_offset + ml;
        }

        // Whole run fits to the left of the click — advance past it.
        float run_w = measure_text(renderer, font_id, font_size,
                                   line_text + byte_offset, run_len);
        // Track the byte offset of the last character in the run for the
        // "click past end of line" fallback below.
        if (run_len > 0) {
            last_byte_offset = run_end - utf8_seq_len_back(line_text + run_end);
        }
        x_accum += run_w;
        byte_offset = run_end;
    }
    // Click past all characters: place cursor on the last character of this
    // visual line rather than byte_end (which would land on the next line).
    return vl->byte_start + last_byte_offset;
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
