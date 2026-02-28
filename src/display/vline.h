// Visual line cache for word-wrapped text rendering.
// Caches the mapping from logical lines to visual (display) lines.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../clay/renderer.h"

// A single visual line (one row of wrapped text on screen).
typedef struct VisualLine {
    uint64_t line_id;       // from Line.line_id (stable across edits)
    uint64_t line_version;  // from Line.version (invalidation check)
    size_t byte_start;      // byte offset into buffer
    size_t byte_end;        // exclusive
    uint16_t visual_index;  // 0,1,2... for wrapped portions of same logical line
    float measured_width;   // cached width in pixels
} VisualLine;

// Index entry mapping logical line to its visual line range.
typedef struct LogicalLineIndex {
    uint64_t line_id;       // from Line.line_id
    uint64_t version;       // from Line.version
    size_t vline_start;     // first vline index for this logical line
    size_t vline_count;     // how many vlines this logical line produces
} LogicalLineIndex;

// Cache of visual lines for a pane.
typedef struct VLineCache {
    VisualLine *lines;      // array of visual lines
    size_t count;           // number of visual lines
    size_t cap;             // allocated capacity

    // Index mapping logical lines to visual line ranges
    LogicalLineIndex *index;
    size_t index_count;
    size_t index_cap;

    // Cache key (full invalidation if changed)
    float pane_width;
    uint16_t font_id;
    uint16_t font_size;

    // Scroll state
    size_t top_vline;       // first visible visual line index
    bool full_rebuild;      // flag to force full rebuild
} VLineCache;

// Forward declarations
struct Buffer;

// Create a new empty VLineCache.
VLineCache vline_cache_create(void);

// Free resources allocated for a VLineCache.
void vline_cache_destroy(VLineCache *cache);

// Rebuild the visual line cache from the buffer's logical lines.
// Parameters:
//   cache      - the cache to rebuild
//   buf        - the buffer containing text
//   renderer   - renderer data for font metrics
//   pane_width - available width for text in pixels
//   font_id    - font to use for measurement
//   font_size  - font size for measurement
void vline_rebuild(VLineCache *cache, struct Buffer *buf,
                   Clay_SDL3RendererData *renderer,
                   float pane_width, uint16_t font_id, uint16_t font_size);

// Adjust top_vline to ensure the cursor (at byte_pos) is visible.
// visible_count is the number of visual lines that fit in the viewport.
void vline_scroll_to_cursor(VLineCache *cache, size_t byte_pos, size_t visible_count);

// Find the visual line index containing a given byte position.
// Returns the index, or cache->count if not found.
size_t vline_for_byte_pos(const VLineCache *cache, size_t byte_pos);

// Get line height for the given font.
int vline_get_line_height(Clay_SDL3RendererData *renderer,
                          uint16_t font_id, uint16_t font_size);

// Given pixel coordinates relative to the top-left of the text area (after
// padding and gutter), return the buffer byte offset of the character
// nearest to (rel_x, rel_y).  Clamps to line boundaries.
size_t vline_byte_pos_at_xy(const VLineCache *cache, const char *buf_text,
                             float rel_x, float rel_y, int line_height,
                             Clay_SDL3RendererData *renderer,
                             uint16_t font_id, uint16_t font_size);
