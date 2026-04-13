// Line-based text diffing.
//
// Wraps a divide-and-conquer LCS algorithm (fossil/FreeBSD lineage) behind
// an API that speaks in byte spans rather than nul-terminated C strings, and
// supports building one side of the diff directly from a LineTable so the
// current buffer doesn't need its text rescanned for line boundaries.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "line.h"

typedef struct Buffer Buffer;

// Result of a raw diff: an array of COPY/DELETE/INSERT triples terminated
// by a {0,0,0} triple, plus the line counts of both sides.
//
// `edits` is NULL if either side was rejected as binary (any line longer
// than 8192 bytes). In that case `n_from`/`n_to` are 0 and there is
// nothing to free.
typedef struct {
    int *edits;
    int  n_from;
    int  n_to;
} DiffResult;

// Raw-text vs raw-text. Neither side needs to be nul-terminated.
DiffResult diff_text(const char *from, size_t from_len,
                     const char *to,   size_t to_len);

// Same as diff_text, but the "to" side reuses an existing LineTable.
// `to_text` must be the buffer text the line table indexes into; the DLine
// array built for the "to" side points directly into `to_text`, so it has
// to outlive the call.
DiffResult diff_text_vs_line_table(const char *from,    size_t from_len,
                                   const char *to_text, size_t to_len,
                                   const LineTable *to_lt);

// Free the edits array owned by a DiffResult.
void diff_result_free(DiffResult *r);

// Per-line classification of the "to" side. Flags combine: e.g. a modified
// line whose following line was deleted is LINE_CHANGE_MODIFIED | DEL_BELOW.
typedef enum {
    LINE_CHANGE_NONE      = 0,
    LINE_CHANGE_ADDED     = 1 << 0,  // line is new in "to"
    LINE_CHANGE_MODIFIED  = 1 << 1,  // line replaces a "from"-side line
    LINE_CHANGE_DEL_ABOVE = 1 << 2,  // deletion run immediately before this line
    LINE_CHANGE_DEL_BELOW = 1 << 3,  // deletion run immediately after this line
} LineChangeKind;

// Collapse the edit triple stream into per-line change flags for the
// "to" side. Returns a malloc'd uint8_t array of length `result->n_to`
// (or NULL if the diff was rejected or n_to is 0). Caller frees with free().
//
// Rules:
//   copy n                      -> advance, no marks
//   delete d, insert i:
//     first min(d,i) inserted   -> |= MODIFIED
//     remaining i-d inserted    -> |= ADDED         (when i > d)
//     pure deletion (d>0, i==0) -> DEL_BELOW on cursor-1 (if >0)
//                                  DEL_ABOVE on cursor   (if <n_to)
// Mixed triples with d > i > 0 do not add deletion squares: the modified
// stripes already indicate the change in that span.
uint8_t *diff_line_markers(const DiffResult *result);

// Convenience: compute per-line markers for `buf` against its last-saved
// snapshot. Returns NULL if the buffer has no snapshot, is unmodified, or
// the diff was rejected. Caller frees with free().
// `out_n_lines` (optional) receives the length of the returned array.
uint8_t *buffer_get_line_markers(Buffer *buf, int *out_n_lines);
