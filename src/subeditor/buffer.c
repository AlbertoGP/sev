// Implementation of editor buffer data structures,
// and the functions that manipulate them.

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buffer.h"
#include "gap.h"
#include "line.h"
#include "mark.h"
#include "mode.h"

typedef struct {
    struct timespec mtime;
} Time;

#define BUFFER_NAME_MAX 256
#define FILE_NAME_MAX 256

// Stores the data for a text buffer.
// Part of a doubly-linked list of all buffers.
typedef struct Buffer {
    struct Buffer *next;
    struct Buffer *prev;
    char name[BUFFER_NAME_MAX];

    Location point;
    size_t cur_line;
    size_t num_chars;
    size_t num_lines;

    LineTable lt;

    int col;
    int col_saved;

    Mark *marks;
    Mode *modes;

    GapBuf *contents;
    char file_name[FILE_NAME_MAX];
    Time file_time;
    bool is_modified;
} Buffer;

typedef struct BufferList {
    Buffer *list;
    Buffer *current;
} BufferList;

static BufferList bl;

static void update_point(void) {
    bl.current->point.pos = gb_point_get(bl.current->contents);
}

bool buffer_list_init(void) {
    bl.list = NULL;
    bl.current = NULL;
    
    if (!buffer_create("*scratch*")) {
        return false;
    }
    
    return true;
}

static void buffer_destroy(Buffer *buf) {
    if (bl.current == buf) {
        bl.current = buf->next ? buf->next : buf->prev;
    }

    if (buf->prev)
        buf->prev->next = buf->next;
    else
        bl.list = buf->next;

    if (buf->next)
        buf->next->prev = buf->prev;

    if (buf->marks) {
        mark_delete_all(buf->marks);
        buf->marks = NULL;
    }
    if (buf->modes) {
        mode_delete_all(buf->modes);
        buf->modes = NULL;
    }
    if (buf->contents) {
        gb_free(buf->contents);
        buf->contents = NULL;
    }
    line_table_destroy(&buf->lt);

    free(buf);
}


void buffer_list_quit(void) {
    Buffer *buf = bl.list;

    if (!buf) return;
    
    while (buf) {
        Buffer *next = buf->next;
        buffer_destroy(buf);
        buf = next;
    }

    bl.list = NULL;
    bl.current = NULL;
}


bool buffer_create(const char *name) {
    Buffer *buf = calloc(1, sizeof(Buffer));
    if (!buf) return false;

    buf->next = NULL;

    strncpy(buf->name, name, BUFFER_NAME_MAX - 1);
    buf->name[BUFFER_NAME_MAX - 1] = '\0';

    buf->cur_line = 1;
    buf->num_lines = 1;
    buf->col_saved = buf->col = 1;

    buf->lt = line_table_create();
    if (!buf->lt.lines) {
        free (buf);
        return false;
    }

    buf->contents = gb_new(0);
    if (!buf->contents) {
        line_table_destroy(&buf->lt);
        free (buf);
        return false;
    }

    // if buffer list is empty, buffer becomes list head
    if (!bl.list) {
        buf->prev = NULL;
        buf->next = NULL;
        bl.list = buf;
        bl.current = buf;
    // otherwise append to the end of the list
    } else {
        Buffer *list = bl.list;
        while (list->next) {
            list = list->next;
        }
        list->next = buf;
        buf->prev = list;
    }

    return true;
}

Buffer *buffer_get_by_name(const char *name) {
    Buffer *buf = bl.list;
    while (buf && strcmp(buf->name, name)) {
        buf = buf->next;
    };
    return buf;
}

bool buffer_set_by_name(const char *name) {
    Buffer *buf = buffer_get_by_name(name);
    if (!buf) return false;

    bl.current = buf;
    return true;
}

bool buffer_clear(const char *name) {
    Buffer *buf = buffer_get_by_name(name);
    if (!buf) return false;

    if (buf->marks) {
        mark_delete_all(buf->marks);
        buf->marks = NULL;
    }

    gb_free(buf->contents);
    buf->contents = gb_new(0);
    if (!buf->contents) return false;

    buf->point.pos = 0;
    buf->cur_line = 1;
    buf->num_lines = 1;
    buf->num_chars = 0;
    buf->col_saved = buf->col = 1;
    if (buf->file_name[0] == '\0') {
        buf->is_modified = true;
    }

    return true;
}

bool buffer_delete(const char *name) {
    Buffer *buf = buffer_get_by_name(name);
    if (!buf) return false;

    buffer_destroy(buf);
    // In the event that this was the only open buffer
    if (!bl.list) {
        buffer_create("*scratch*");
    }

    return true;
}

Buffer *buffer_get_current(void) {
    return bl.current;
}

bool buffer_set_current(Buffer *buf) {
    if (!buf) return false;

    bl.current = buf;
    return true;
}

char *buffer_set_next(void) {
    if (bl.current->next)
        bl.current = bl.current->next;
    else bl.current = bl.list;

    return bl.current->name;
}

char *buffer_set_prev(void) {
    if (bl.current->prev)
        bl.current = bl.current->prev;
    else {
        while (bl.current->next)
            bl.current = bl.current->next;
    }

    return bl.current->name;
}

bool buffer_set_name(const char *name) {
    if (!bl.current  || !name)
        return false;

    if (strlen(name) >= BUFFER_NAME_MAX)
        return false;

    if (buffer_get_by_name(name))
        return false;

    strcpy(bl.current->name, name);
    return true;
}

char *buffer_get_name(void) {
    return bl.current->name;
}

bool point_set(Location loc) {
    if (!bl.current) return false;

    gb_point_set(bl.current->contents, loc.pos);
    bl.current->col = 0;
    update_point();
    return true;
}

bool point_move(int count) {
    if (!bl.current) return false;

    size_t point = point_get().pos;

    if (!count) return true;
    else if (count > 0) {
        size_t remaining = get_char_count() - point;
        if (count > remaining)
            count = remaining;
        for (size_t i = 0; i < count; i++) {
            if (char_from_point(i) == '\n') {
                bl.current->cur_line++;
                bl.current->col = 0;
            } else {
                bl.current->col++;
            }
        }
    } else {
        if (point == 0) return true;
        if (count < -point)
            count = -point;
        for (int i = 0; i > count; i--) {
            if (char_from_point(i - 1) == '\n') {
                bl.current->cur_line--;
            }
            if (char_from_point(i) == '\n') {
                bl.current->col = 0;
            } else if (bl.current->col) {
                bl.current->col--;
            }
        }
    }

    gb_point_set(bl.current->contents, point + count);
    update_point();
    bl.current->col_saved = get_column();
    return true;
}

bool point_move_by_line(int count) {
    if (!bl.current) return false;

    if (!count) return true;
    if (count <= 1 - bl.current->cur_line) {
        point_set(buffer_start());
        get_column();
        set_column(bl.current->col_saved, false);
        bl.current->cur_line = 1;
        return true;
    }
    if (count >= bl.current->num_lines - bl.current->cur_line) {
        point_set(buffer_end());
        get_column();
        set_column(bl.current->col_saved, false);
        bl.current->cur_line = bl.current->num_lines;
        return true;
    }
    if (count > 0) {
        int lines = 0;
        for (int i = point_get().pos; i < get_char_count(); i++) {
            if (gb_char_at(bl.current->contents, i) == '\n') {
                lines++;
                if (lines == count) {
                    point_set((Location){.pos = i + 1});
                    get_column();
                    set_column(bl.current->col_saved, false);
                    bl.current->cur_line = 0;
                    point_get_line();
                    return true;
                }
            }
        }
    }
    if (count < 0) {
        int lines = 0;
        for (int i = point_get().pos; i > 0; i--) {
            if (gb_char_at(bl.current->contents, i) == '\n') {
                lines--;
                if (lines == count) {
                    point_set((Location){.pos = i});
                    get_column();
                    set_column(bl.current->col_saved, false);
                    bl.current->cur_line = 0;
                    point_get_line();
                    return true;
                }
            }
        }
    }
    return false;
}

Location point_get(void) {
    return bl.current->point;
}

size_t point_get_line(void) {
    if (bl.current->cur_line) {     // line is known
        return bl.current->cur_line;
    } else {                        // line must be recalculated
        bl.current->cur_line = 1;
        char *c = gb_text(bl.current->contents);
        for (size_t i = 0; i < point_get().pos; i++, c++)
            if (*c == '\n') bl.current->cur_line++;
        return bl.current->cur_line;
    }
}
Location buffer_start(void) {
    return (Location){.pos = 0};
}
Location buffer_end(void) {
    return (Location){.pos = gb_used(bl.current->contents)};
}
int compare_locations(Location loc1, Location loc2) {
    int cmp = loc1.pos - loc2.pos;
    if (cmp > 0) return 1;
    if (cmp < 0) return -1;
    return 0;
}
size_t location_to_count(Location loc) {
    return loc.pos;
}
Location count_to_location(size_t count) {
    return (Location){.pos = count};
}
char get_char(void) {
    if (point_get().pos == get_char_count())
        return '\0';
    return char_at_point();
}
void get_string(char *string, size_t count);

size_t get_char_count(void) {
    return bl.current->num_chars;
}

size_t get_line_count(void) {
    if (bl.current->num_lines) {
        return bl.current->num_lines;
    } else {
        bl.current->num_lines = 1;
        for (char *c = gb_text(bl.current->contents); *c; c++)
            if (*c == '\n') bl.current->num_lines++;
        return bl.current->num_lines;
    }
}
void get_file_name(char *file_name, int size);
bool set_file_name(char *file_name);
bool buffer_write(void);
bool buffer_read(void);
bool buffer_insert(char *file_name);
bool is_file_changed(void);
void set_modified(bool is_modified);
bool get_modified(void);
void insert_char(char c) {
    if (c == '\n') {
        bl.current->num_lines++;
        bl.current->cur_line++;
        bl.current->col_saved = bl.current->col = 0;
    } else {
        bl.current->col_saved = ++(bl.current->col);
    }
    gb_insert(bl.current->contents, c);
    bl.current->num_chars = gb_used(bl.current->contents);
    line_insert_char(&bl.current->lt, point_get().pos, c);
    update_point();
}
void insert_string(char *s) {
    while (*s) {
        insert_char(*s++);
    }
}
void replace_char(char c);
void replace_string(char *string);

bool delete_chars(int count) {
    Buffer *b = bl.current;
    if (!b) return false;

    int point = point_get().pos;

    if (count > 0) {
        if (count > point)
            count = point;
        for (int i = 0; i < count; i++) {
            char c = char_from_point(i - 1);
            line_backspace_char(&b->lt, point, c);
            if (c == '\n') {
                b->cur_line--;
                b->num_lines--;
                b->col = 0;
            } else {
                b->col_saved = --(b->col);
            }
        }
        gb_backspace(b->contents, count);
    }
    if (count < 0) {
        if (count < -(int)(gb_back(b->contents)))
            count = -(int)(gb_back(b->contents));
        for (int i = 0; i > count; i--) {
            char c = char_from_point(i);
            line_delete_char(&b->lt, point, c);
            if (c == '\n')
                b->num_lines--;
        }
        gb_delete(b->contents, -count);
    }
    b->num_chars = gb_used(b->contents);
    update_point();
    b->col_saved = get_column();
    return true;
}

bool delete_region(Mark *mark);
bool copy_region(char *buffer_name, Mark *mark);
bool search_forward(char *string);
bool search_backward(char *string);
bool is_a_match(char *string);
bool find_first_in_forward(char *string);
bool find_first_in_backward(char *string);
bool find_first_not_in_forward(char *string);
bool find_first_not_in_backward(char *string);
int get_column(void) {
    if (bl.current->col) {  // column is known
        return bl.current->col;
    } else {    // column must be recalculated.
        for (size_t i = 1; i < point_get().pos; i++) {
            if (gb_char_at(bl.current->contents, point_get().pos - i) == '\n') {
                return bl.current->col = i;
            }
        }
        return bl.current->col = point_get().pos + 1;
    }
}
void set_column(int column, bool round) {
    if (bl.current->col == column) return;
    if (column < 0) column = 0;

    int delta = column - bl.current->col;
    if (delta < 0) {
        point_move(delta);
        return;
    }
    if (delta > gb_back(bl.current->contents))
        delta = gb_back(bl.current->contents);
    for (int i = 0; i < delta; i++) {
        if (char_at_point() == '\n') {
            delta = i;
            break;
        }
    }
    point_move(delta);
}


char *buffer_text(Buffer *buf) {
    static char* text;
    if (text) free(text);
    text = gb_text(buf->contents);
    return text;
}

char char_at_point(void) {
    return gb_char_at(bl.current->contents, point_get().pos);
}

char char_from_point(int n) {
    int point = point_get().pos;
    int remaining = get_char_count() - point;
    if (n < -point || n > remaining) return '\0';
    return gb_char_at(bl.current->contents, point + n);
}

int buf_char_at(Buffer *buf, size_t index) {
    return gb_char_at(buf->contents, index);
}

int buf_size(Buffer *buf) {
    return gb_used(buf->contents);
}

// Diagnostic function, delete once logical lines are confirmed working properly.
void line_table_print(void) {
    LineTable lt = bl.current->lt;
    printf("\nLine: %4zu  Count: %4zu  Cap: %4zu\n\n", line_index_at(&lt, point_get().pos), lt.count, lt.cap);
    for (size_t i = 0; i < lt.count; i++) {
        printf("L: %4zu  ID: %4lu  Loc: %2zu - %2zu  Ver: %lu\n", i, lt.lines[i].line_id, lt.lines[i].start, lt.lines[i].end, lt.lines[i].version);
    }
}
