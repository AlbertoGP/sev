// Logical line data type and functions.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Data structure for storing information on logical lines in a buffer.
typedef struct Line {
    uint64_t line_id;   // uniquely identifies line even if position changes.
    size_t start, end;  // indices into buffer.
    uint64_t version;   // dirty, needs_highlight, etc.
} Line;

// Vector of Line data type. Each buffer has its own LineTable.
typedef struct {
    Line *lines;            // array of lines.
    uint64_t next_line_id;  // unique ID next line will receive.
    size_t count;           // number of lines in vector.
    size_t cap;             // size of vector.
} LineTable;

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
