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

// Cache of visual lines for a pane.
typedef struct VLineCache {
    VisualLine *lines;      // array of visual lines
    size_t count;           // number of visual lines
    size_t cap;             // allocated capacity

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
