#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "line.h"

#define MIN_TABLE_SIZE 8
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

LineTable line_table_create(void) {
    LineTable lt;

    lt.lines = calloc(MIN_TABLE_SIZE, sizeof(Line));

    lt.next_line_id = 1;

    lt.lines[0].line_id = lt.next_line_id++;
    lt.lines[0].start = 0;
    lt.lines[0].end = 0;
    lt.lines[0].version = 0;

    lt.count = 1;
    lt.cap = MIN_TABLE_SIZE;

    return lt;
}

void line_free_hl(Line *line) {
    free(line->hl_spans);
    line->hl_spans      = NULL;
    line->hl_span_count = 0;
    line->hl_span_cap   = 0;
    line->hl_dirty      = true;
}

void line_table_destroy(LineTable *lt) {
    if (lt->lines) {
        for (size_t i = 0; i < lt->count; i++)
            line_free_hl(&lt->lines[i]);
        free(lt->lines);
    }
}

size_t line_index_at(const LineTable *lt, size_t pos) {
    size_t lo = 0, hi = lt->count;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (pos < lt->lines[mid].start)
            hi = mid;
        else
            lo = mid + 1;
    }

    size_t idx = lo - 1;
    if (idx >= lt->count) idx = lt->count - 1;
    return idx;
}


// Grow LineTable to new_size. Operation can fail.
static bool line_table_grow(LineTable *lt, size_t new_size) {
    new_size = MAX(new_size, MIN_TABLE_SIZE);
    if (lt->cap >= new_size) {
        return true;
    }
    Line *new_lines = realloc(lt->lines, new_size * sizeof(Line)); // allocate a larger table
    if (!new_lines) return false;

    lt->lines  = new_lines;
    lt->cap    = new_size;

    return true;
}

// Shrink LineTable to new_size - never fails.
// No return value; i.e. no bool as in line_table_grow(),
// because it does not matter if we cannot shrink.
static void line_table_shrink(LineTable *lt, size_t new_size) {
    new_size = MAX(new_size, MIN_TABLE_SIZE);
    if (new_size < lt->count) {
        return; // we do not resize if we'd lose data!
    }

    Line *new_lines = realloc(lt->lines, new_size * sizeof(Line)); // allocate a smaller buffer
    if (!new_lines) return;

    lt->lines = new_lines;
    lt->cap    = new_size;
}

static void shift_line_spans(Line *line, int32_t delta) {
    for (uint32_t si = 0; si < line->hl_span_count; si++) {
        line->hl_spans[si].start_byte = (uint32_t)((int32_t)line->hl_spans[si].start_byte + delta);
        line->hl_spans[si].end_byte   = (uint32_t)((int32_t)line->hl_spans[si].end_byte   + delta);
    }
}

#define capped_dbl_size(s) (((s) < SIZE_MAX / 2) ? (2 * (s)) : SIZE_MAX)

static bool line_table_insert(LineTable *lt, size_t index, Line line) {
    if (lt->count == lt->cap) {
        size_t new_cap = capped_dbl_size(lt->cap);
        if (!line_table_grow(lt, new_cap)) {
            return false;
        }
    }

    memmove(&lt->lines[index + 1],
            &lt->lines[index],
            (lt->count - index) * sizeof(Line));

    lt->lines[index] = line;
    lt->count++;

    return true;
}


bool line_insert_char(LineTable *lt, size_t pos, char ch) {
    size_t line_number = line_index_at(lt, pos);
    Line *current_line = &lt->lines[line_number];

    if (ch != '\n') {
        // Simple case: structure unchanged, line end moves forward by 1
        current_line->end += 1;
        current_line->version++;

        // Update start/end indices for all subsequent lines
        for (size_t i = line_number + 1; i < lt->count; i++) {
            lt->lines[i].start++;
            lt->lines[i].end++;
            shift_line_spans(&lt->lines[i], +1);
        }

        return true;
    }

    // Newline insertion: split the line
    Line new_line;

    new_line.line_id      = lt->next_line_id++;
    new_line.start        = pos + 1;
    new_line.end          = current_line->end + 1; // buffer already grew
    new_line.version      = 0;
    new_line.hl_spans     = NULL;
    new_line.hl_span_count = 0;
    new_line.hl_span_cap  = 0;
    new_line.hl_dirty     = true;

    if (!line_table_insert(lt, line_number + 1, new_line))
        return false;

    // line_table_insert might have reallocated lt->lines, so we must refresh the pointer
    current_line = &lt->lines[line_number];

    current_line->end = pos + 1;
    current_line->version++;
    current_line->hl_dirty = true;

    // Update start/end indices for all subsequent lines
    for (size_t i = line_number + 2; i < lt->count; i++) {
        lt->lines[i].start++;
        lt->lines[i].end++;
        lt->lines[i].version++;
        shift_line_spans(&lt->lines[i], +1);
    }

    return true;
}

void line_backspace_char(LineTable *lt, size_t pos, char ch) {
    size_t line_number = line_index_at(lt, pos);
    Line *current_line = &lt->lines[line_number];

    if (ch != '\n') {
        // Simple case: structure unchanged, line shrinks
        current_line->end -= 1;
        current_line->version++;

        // Update the start/end indices of all subsequent lines
        for (size_t i = line_number + 1; i < lt->count; i++) {
            lt->lines[i].start--;
            lt->lines[i].end--;
            shift_line_spans(&lt->lines[i], -1);
        }

        return;
    }

    // Newline deletion: merge line i with line i-1
    Line *prev_line = &lt->lines[line_number - 1];

    prev_line->end = current_line->end - 1; // buffer already shrank
    prev_line->version++;
    prev_line->hl_dirty = true;

    line_free_hl(current_line);
    // remove current line
    memmove(current_line,
            current_line + 1,
            (lt->count - (line_number + 1)) * sizeof(Line));

    lt->count--;

    if (lt->count < lt->cap / 4) {
        line_table_shrink(lt, lt->cap / 2);
    }

    // Update the start/end indices of all subsequent lines
    for (size_t i = line_number; i < lt->count; i++) {
        lt->lines[i].start--;
        lt->lines[i].end--;
        lt->lines[i].version++;
        shift_line_spans(&lt->lines[i], -1);
    }
}

void line_delete_char(LineTable *lt, size_t pos, char ch) {
    size_t line_number = line_index_at(lt, pos);
    Line *current_line = &lt->lines[line_number];

    if (ch != '\n') {
        // Simple case: structure unchanged, line shrinks
        current_line->end -= 1;
        current_line->version++;

        // Update the start/end indices of all subsequent lines
        for (size_t i = line_number + 1; i < lt->count; i++) {
            lt->lines[i].start--;
            lt->lines[i].end--;
            shift_line_spans(&lt->lines[i], -1);
        }

        return;
    }

    // Newline deletion: merge line i with line i+1
    Line *next_line = &lt->lines[line_number + 1];

    current_line->end = next_line->end - 1; // buffer already shrank
    current_line->version++;
    current_line->hl_dirty = true;

    line_free_hl(next_line);
    // remove next line
    memmove(next_line,
            next_line + 1,
            (lt->count - (line_number + 2)) * sizeof(Line));

    lt->count--;

    if (lt->count < lt->cap / 4) {
        line_table_shrink(lt, lt->cap / 2);
    }

    // Update the start/end indices of all subsequent lines
    for (size_t i = line_number + 1; i < lt->count; i++) {
        lt->lines[i].start--;
        lt->lines[i].end--;
        lt->lines[i].version++;
        shift_line_spans(&lt->lines[i], -1);
    }
}
