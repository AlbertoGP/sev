#pragma once

#include <tree_sitter/api.h>
#include "gap.h"
#include "line.h"

typedef enum {
    HL_DEFAULT = 0,
    HL_KEYWORD,
    HL_STRING,
    HL_COMMENT,
    HL_NUMBER,
    HL_CONSTANT,   // booleans, chars, quoted forms
    HL_FUNCTION,   // first symbol in a list
    HL_BUILTIN,    // built-in functions
    HL_OPERATOR,
    HL_COUNT       // HL_VARIABLE / HL_PUNCTUATION / HL_ESCAPE → HL_DEFAULT
} HLKind;

typedef struct {
    uint32_t start_byte;
    uint32_t end_byte;
    HLKind   kind;
} HLSpan;

typedef struct {
    TSParser         *parser;
    TSTree           *tree;
    const TSLanguage *language;
    TSRange          *changed_ranges;        // malloc'd by tree-sitter, freed by us
    uint32_t          changed_ranges_count;
    TSQuery          *hl_query;
    HLSpan           *spans;
    uint32_t          span_count;
    uint32_t          span_cap;
} TSState;

typedef struct Buffer Buffer;

// Convert a logical byte offset to a TSPoint using the current line table.
TSPoint ts_byte_to_point(const LineTable *lt, uint32_t byte);

// Notify tree-sitter of a pending edit. Call BEFORE any buffer mutation
// while the line table still reflects the old text.
// new_end_point must be supplied by the caller (depends on inserted content).
void ts_buffer_edit(Buffer *buf,
                    uint32_t start_byte, uint32_t old_end_byte, uint32_t new_end_byte,
                    TSPoint new_end_point);

// Incrementally reparse after a mutation. Call AFTER the buffer has been mutated.
// Stores changed ranges in buf->ts.{changed_ranges, changed_ranges_count}.
void ts_buffer_reparse(Buffer *buf);

void ts_buffer_init(Buffer *buf);
void ts_buffer_parse(Buffer *buf);
void ts_buffer_highlight(Buffer *buf);
void ts_buffer_free(Buffer *buf);
