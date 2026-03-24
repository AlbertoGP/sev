// Logical line data type and functions.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    HL_DEFAULT = 0,
    HL_KEYWORD, HL_STRING, HL_COMMENT, HL_NUMBER,
    HL_CONSTANT, HL_FUNCTION, HL_BUILTIN, HL_OPERATOR,
    HL_COUNT
} HLKind;

typedef struct {
    uint32_t start_byte;
    uint32_t end_byte;
    uint16_t style;    // HLKind cast to uint16_t; 0 = HL_DEFAULT
} HLSpan;

// Data structure for storing information on logical lines in a buffer.
typedef struct Line {
    uint64_t line_id;   // uniquely identifies line even if position changes.
    size_t start, end;  // indices into buffer.
    uint64_t version;   // dirty, needs_highlight, etc.
    HLSpan  *hl_spans;
    uint32_t hl_span_count;
    uint32_t hl_span_cap;
    bool     hl_dirty;  // true = spans need rebuild (Stage 6)
} Line;

// Vector of Line data type. Each buffer has its own LineTable.
typedef struct {
    Line *lines;            // array of lines.
    uint64_t next_line_id;  // unique ID next line will receive.
    size_t count;           // number of lines in vector.
    size_t cap;             // size of vector.
} LineTable;

// Free and zero-out a line's hl_spans array. Safe to call on a zeroed Line.
void line_free_hl(Line *line);

// Create a new LineTable with a single Line.
LineTable line_table_create(void);
// Free up all memory allocated to a given LineTable.
void line_table_destroy(LineTable *lt);
// Locate the line number of a given buffer position via binary search.
size_t line_index_at(const LineTable *lt, size_t pos);
// Update LineTable after the event of character insertion into a buffer.
bool line_insert_char(LineTable *lt, size_t pos, char ch);
// Update LineTable after the event of character backspaced from a buffer.
void line_backspace_char(LineTable *lt, size_t pos, char ch);
// Update LineTable after the event of character deleted from a buffer.
void line_delete_char(LineTable *lt, size_t pos, char ch);
