// Implementation of editor buffer data structures,
// and the functions that manipulate them.

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buffer.h"
#include "gap.h"
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
    int cur_line;
    int num_chars;
    int num_lines;

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

    buf->contents = gb_new(0);
    if (!buf->contents) {
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

bool buffer_set_current(const char *name) {
    Buffer *buf = buffer_get_by_name(name);
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

    if (!count) return true;
    else if (count > 0) {
        if (count > get_char_count() - point_get().pos)
            count = get_char_count() - point_get().pos;
        for (int i = 0; i < count; i++) {
            if (char_from_point(i) == '\n') {
                bl.current->cur_line++;
                bl.current->col = 0;
            } else {
                bl.current->col++;
            }
        }
    } else {
        if (!point_get().pos) return true;
        if (count < -point_get().pos)
            count = -point_get().pos;
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

    gb_point_set(bl.current->contents, point_get().pos + count);
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

int point_get_line(void) {
    if (bl.current->cur_line) {     // line is known
        return bl.current->cur_line;
    } else {                        // line must be recalculated
        bl.current->cur_line = 1;
        char *c = gb_text(bl.current->contents);
        for (int i = 0; i < bl.current->contents->point; i++, c++)
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
int location_to_count(Location loc) {
    return loc.pos;
}
Location count_to_location(int count) {
    return (Location){.pos = count};
}
char get_char(void) {
    if (point_get().pos == get_char_count())
        return '\0';
    return char_at_point();
}
void get_string(char *string, int count);

int get_char_count(void) {
    return bl.current->num_chars;
}

int get_line_count(void) {
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
    if (!bl.current) return false;

    if (count > 0) {
        if (count > point_get().pos)
            count = point_get().pos;
        for (int i = 0; i < count; i++) {
            if (char_from_point(i - 1) == '\n') {
                bl.current->cur_line--;
                bl.current->num_lines--;
                bl.current->col = 0;
            } else {
                bl.current->col_saved = --(bl.current->col);
            }
        }
        gb_backspace(bl.current->contents, count);
    }
    if (count < 0) {
        if (count < -gb_back(bl.current->contents))
            count = -gb_back(bl.current->contents);
        for (int i = 0; i > count; i--) {
            if (char_from_point(i) == '\n')
                bl.current->num_lines--;
        }
        gb_delete(bl.current->contents, -count);
    }
    bl.current->num_chars = gb_used(bl.current->contents);
    update_point();
    bl.current->col_saved = get_column();
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
        for (int i = 1; i < point_get().pos; i++) {
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


char *buffer_text(void) {
    static char* text;
    if (text) free(text);
    text = gb_text(bl.current->contents);
    return text;
}

char char_at_point(void) {
    return gb_char_at(bl.current->contents, bl.current->point.pos);
}

char char_from_point(int n) {
    if (n < 0) n = 0;
    if (n > bl.current->num_chars) return '\0';
    return gb_char_at(bl.current->contents, bl.current->point.pos + n);
}

int buf_char_at(Buffer *buf, int index) {
    return gb_char_at(buf->contents, index);
}

int buf_size(Buffer *buf) {
    return gb_used(buf->contents);
}
