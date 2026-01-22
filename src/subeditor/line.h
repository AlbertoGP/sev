// Logical line data type and functions.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Data structure for storing information on logical lines in a buffer.
typedef struct Line {
    size_t start;      // buffer index of first char
    size_t end;        // buffer index *after* newline (or buffer end)
    uint32_t flags;    // dirty, needs_highlight, etc
} Line;

typedef enum {
    LINE_DIRTY_TEXT   = 1 << 0,
    LINE_DIRTY_WRAP   = 1 << 1,
    LINE_DIRTY_STYLE  = 1 << 2,
} LineFlags;

typedef struct {
    Line *lines;
    size_t count;
    size_t cap;
} LineTable;

LineTable line_table_create(void);
void line_table_destroy(LineTable *lt);
// Update LineTable after the event of character insertion into a buffer.
bool line_insert_char(LineTable *lt, size_t pos, char ch);
// Update LineTable after the event of character backspaced from a buffer.
void line_backspace_char(LineTable *lt, size_t pos, char ch);
// Update LineTable after the event of character deleted from a buffer.
void line_delete_char(LineTable *lt, size_t pos, char ch);
size_t line_index_at(const LineTable *lt, size_t pos);
