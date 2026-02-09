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
#include "message.h"
#include "var.h"
#include "../command/scheme_internal.h"
#include "../display/scale.h"

extern KeyEvent last_event;

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

    Keymap *local_map;
    VarTable locals;
    ModeList minor_modes;
    Mode *major_mode;

    float scale_factor;

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

static void update_point(Buffer *buf) {
    buf->point.pos = gb_point_get(buf->contents);
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
    modelist_destroy(&buf->minor_modes);
    vartable_destroy(&buf->locals);
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


Buffer *buffer_create(const char *name) {
    Buffer *buf = calloc(1, sizeof(Buffer));
    if (!buf) return NULL;

    buf->next = NULL;

    strncpy(buf->name, name, BUFFER_NAME_MAX - 1);
    buf->name[BUFFER_NAME_MAX - 1] = '\0';

    buf->cur_line = 1;
    buf->num_lines = 1;
    buf->col_saved = buf->col = 1;

    vartable_init(&buf->locals);
    modelist_init(&buf->minor_modes);

    buf->lt = line_table_create();
    if (!buf->lt.lines) {
        free (buf);
        return NULL;
    }

    buf->contents = gb_new(0);
    if (!buf->contents) {
        line_table_destroy(&buf->lt);
        free (buf);
        return NULL;
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

    // Enable evil-normal-mode if registered (no-op before scheme_init)
    Mode *evil = mode_lookup("evil-normal-mode", MODE_MINOR);
    if (evil) {
        buffer_enable_minor_mode(buf, evil);
        sexp ctx = G->chibi.ctx;
        sexp key = sexp_intern(ctx, "mode-name", -1);
        sexp val = sexp_c_string(ctx, "NORMAL", -1);
        sexp_preserve_object(ctx, val);
        vartable_set(&buf->locals, key, val);
    }

    reset_buffer_scale(G, buf);

    return buf;
}

Buffer *buffer_get_by_name(const char *name) {
    Buffer *buf = bl.list;
    while (buf && strcmp(buf->name, name)) {
        buf = buf->next;
    };
    return buf;
}

bool buffer_clear(Buffer *buf) {
    if (!buf) return false;
    GapBuf *new_gb = gb_new(0);
    if (!new_gb) return false;
    LineTable new_lt = line_table_create();
    if (!new_lt.lines) {
        gb_free(new_gb);
        return false;
    }

    if (buf->marks) {
        mark_delete_all(buf->marks);
        buf->marks = NULL;
    }

    gb_free(buf->contents);
    buf->contents = new_gb;

    buf->point.pos = 0;
    buf->cur_line = 1;
    buf->num_lines = 1;
    buf->num_chars = 0;
    buf->col_saved = buf->col = 1;
    if (buf->file_name[0] == '\0') {
        buf->is_modified = true;
    }

    line_table_destroy(&buf->lt);
    buf->lt = new_lt;

    return true;
}

bool buffer_delete(Buffer *buf) {
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

char *buffer_get_name(Buffer *buf) {
    return buf->name;
}

float buffer_get_scale(Buffer *buf) {
    return buf->scale_factor;
}

bool buffer_set_scale(Buffer *buf, float new_value) {
    if (!buf) return false;
    buf->scale_factor = new_value;
    return true;
}

bool point_set(Location loc) {
    if (!bl.current) return false;

    gb_point_set(bl.current->contents, loc.pos);
    bl.current->col = 0;
    update_point(bl.current);
    return true;
}

bool point_move(int count) {
    if (!bl.current) return false;

    size_t point = point_get(bl.current).pos;

    if (!count) return true;
    else if (count > 0) {
        size_t remaining = get_char_count(bl.current) - point;
        if (count > (int)remaining) {
            count = (int)remaining;
        }
        for (int i = 0; i < count; i++) {
            if (char_from_point(i) == '\n') {
                bl.current->cur_line++;
                bl.current->col = 0;
            } else {
                bl.current->col++;
            }
        }
    } else {
        if (point == 0) return true;
        if (count < -(int)point)
            count = -(int)point;
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
    update_point(bl.current);
    bl.current->col_saved = get_column(bl.current);
    return true;
}

size_t buf_get_line(Buffer *buf) {
    return buf->cur_line;
}

bool buf_set_line(Buffer *buf, size_t line) {
    if (!buf) return false;

    const LineTable *lt = buffer_get_line_table(buf);
    Location new_loc = {
        .pos = lt->lines[line + 1].start
    };

    point_set(new_loc);
    get_column(buf);
    set_column(buf->col_saved, false);
    buf->cur_line = line;
    return true;
}

bool point_move_by_line(int count) {
    Buffer *buf = bl.current;
    if (!buf) return false;
    if (!count) return true;
   
    const LineTable *lt = buffer_get_line_table(buf);
    size_t line_index = line_index_at(lt, point_get(buf).pos);

    if (count <= -(int)(line_index)) {
        count = -(int)(line_index);
    } else if (count >= (int)(buf->num_lines - buf->cur_line)) {
        count = (int)(buf->num_lines - buf->cur_line);
    }
    Location new_loc = {
        .pos = lt->lines[line_index + count].start
    };

    point_set(new_loc);
    get_column(buf);
    set_column(buf->col_saved, false);
    buf->cur_line = line_index + count + 1;
    return true;
}

Location point_get(Buffer *buf) {
    return buf->point;
}

size_t point_get_line(Buffer *buf) {
    if (buf->cur_line) {     // line is known
        return buf->cur_line;
    } else {                        // line must be recalculated
        buf->cur_line = 1;
        char *c = gb_text(buf->contents);
        for (size_t i = 0; i < point_get(buf).pos; i++, c++)
            if (*c == '\n') buf->cur_line++;
        return buf->cur_line;
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
    if (point_get(bl.current).pos == get_char_count(bl.current))
        return '\0';
    return char_at_point();
}
void get_string(char *string, size_t count);

size_t get_char_count(Buffer *buf) {
    return buf->num_chars;
}

size_t get_line_count(Buffer *buf) {
    if (buf->num_lines) {
        return buf->num_lines;
    } else {
        buf->num_lines = 1;
        for (char *c = gb_text(buf->contents); *c; c++)
            if (*c == '\n') buf->num_lines++;
        return buf->num_lines;
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
void insert_char(Buffer *buf, char c) {
    if (c == '\n') {
        buf->num_lines++;
        buf->cur_line++;
        buf->col_saved = buf->col = 1;
    } else {
        buf->col_saved = ++(buf->col);
    }
    gb_insert(buf->contents, c);
    buf->num_chars = gb_used(buf->contents);
    line_insert_char(&buf->lt, point_get(buf).pos, c);
    update_point(buf);
}
void insert_string(Buffer *buf, const char *s) {
    while (*s) {
        insert_char(buf, *s++);
    }
}
void replace_char(char c);
void replace_string(char *string);

bool delete_chars(Buffer *buf, int count) {
    if (!buf) return false;

    int point = point_get(buf).pos;

    if (count > 0) {
        if (count > point)
            count = point;
        for (int i = 0; i < count; i++) {
            char c = char_from_point(i - 1);
            line_backspace_char(&buf->lt, point, c);
            if (c == '\n') {
                buf->cur_line--;
                buf->num_lines--;
                buf->col = 0;
            } else {
                buf->col_saved = --(buf->col);
            }
        }
        gb_backspace(buf->contents, count);
    }
    if (count < 0) {
        if (count < -(int)(gb_back(buf->contents)))
            count = -(int)(gb_back(buf->contents));
        for (int i = 0; i > count; i--) {
            char c = char_from_point(i);
            line_delete_char(&buf->lt, point, c);
            if (c == '\n')
                buf->num_lines--;
        }
        gb_delete(buf->contents, -count);
    }
    buf->num_chars = gb_used(buf->contents);
    update_point(buf);
    buf->col_saved = get_column(buf);
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
int get_column(Buffer *buf) {
    if (buf->col) {  // column is known
        return buf->col;
    } else {    // column must be recalculated.
        size_t pos = buf->point.pos;
        size_t line_index = line_index_at(&buf->lt, pos);
        
        return buf->col = (int)(pos + 1 - buf->lt.lines[line_index].start);
    }
}

void set_column(int column, bool round) {
    Buffer *buf = bl.current;
    if (buf->col == column) return;
    if (column < 0) column = 0;

    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    int last_col = (int)(lt.lines[line_index].end - lt.lines[line_index].start);

    if (column > last_col)
        column = last_col;

    int delta = column - buf->col;
    Location loc = {
        .pos = pos + delta
    };
    point_set(loc);
}

void point_to_line_start(Buffer *buf) {
    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    point_set((Location){.pos = buf->lt.lines[line_index].start});
}

void point_to_line_end(Buffer *buf) {
    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    point_set((Location){.pos = buf->lt.lines[line_index].end});
}

char *buffer_text(Buffer *buf) {
    return gb_text(buf->contents);
}

char char_at_point(void) {
    if (point_get(bl.current).pos == get_char_count(bl.current)) return '\0';
    return buf_char_at(bl.current, point_get(bl.current).pos);
}

char char_from_point(int n) {
    int point = point_get(bl.current).pos;
    int remaining = get_char_count(bl.current) - point;
    if (n < -point || n > remaining) return '\0';
    return buf_char_at(bl.current, point + n);
}

int buf_char_at(Buffer *buf, size_t index) {
    return gb_char_at(buf->contents, index);
}

int buf_size(Buffer *buf) {
    return gb_used(buf->contents);
}

const LineTable *buffer_get_line_table(Buffer *buf) {
    if (!buf) return NULL;
    return &buf->lt;
}

void buffer_set_major_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode || mode->type != MODE_MAJOR) return;
    buf->major_mode = mode;
}

Mode *buffer_get_major_mode(Buffer *buf) {
    if (!buf) return NULL;
    return buf->major_mode;
}

void buffer_enable_minor_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode || mode->type != MODE_MINOR) return;

    // Check if already enabled
    for (ModeListNode *n = buf->minor_modes.head; n; n = n->next) {
        if (n->mode == mode) return;
    }

    // Prepend to list (most-recently-enabled first)
    ModeListNode *node = malloc(sizeof(ModeListNode));
    if (!node) return;

    node->mode = mode;
    node->next = buf->minor_modes.head;
    buf->minor_modes.head = node;
    buf->minor_modes.count++;
}

void buffer_disable_minor_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode) return;

    ModeListNode **pp = &buf->minor_modes.head;
    while (*pp) {
        if ((*pp)->mode == mode) {
            ModeListNode *to_free = *pp;
            *pp = (*pp)->next;
            free(to_free);
            buf->minor_modes.count--;
            return;
        }
        pp = &(*pp)->next;
    }
}

bool buffer_has_minor_mode(Buffer *buf, const char *name) {
    if (!buf || !name) return false;

    for (ModeListNode *n = buf->minor_modes.head; n; n = n->next) {
        if (strcmp(n->mode->name, name) == 0) return true;
    }
    return false;
}

ModeList *buffer_get_minor_modes(Buffer *buf) {
    if (!buf) return NULL;
    return &buf->minor_modes;
}

Keymap *buffer_get_local_map(Buffer *buf) {
    if (!buf) return NULL;
    return buf->local_map;
}

void buffer_set_local_map(Buffer *buf, Keymap *km) {
    if (!buf) return;
    buf->local_map = km;
}

VarTable *buffer_get_locals(Buffer *buf) {
    if (!buf) return NULL;
    return &buf->locals;
}

// --- Scheme bindings ---

sexp scm_insert_char(sexp ctx, sexp self, sexp n, sexp ch) {
    G->needs_redraw = true;
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, ch);
    insert_char(buffer_get_current(), sexp_unbox_character(ch));

    message_clear();
    return SEXP_VOID;
}

sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    KeyEvent last = last_event;

    if (last.type != KEYEVENT_CHAR) return SEXP_VOID;
    insert_char(buffer_get_current(), last.codepoint);

    message_clear();
    return SEXP_VOID;
}

sexp scm_delete_char(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;

    int count_unboxed = sexp_unbox_fixnum(count);
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());

    message_clear();
    if (count_unboxed > (int)point)
        message_send("Beginning of buffer");
    if (count_unboxed < 0 && -count_unboxed > (int)(chars - point))
        message_send("End of buffer");

    delete_chars(buffer_get_current(), sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

sexp scm_point_move(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;

    int count_unboxed = sexp_unbox_fixnum(count);
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());

    message_clear();
    if (count_unboxed < 0 && -count_unboxed > (int)point)
        message_send("Beginning of buffer");
    if (count_unboxed > (int)(chars - point))
        message_send("End of buffer");

    point_move(sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

sexp scm_point_move_by_line(sexp ctx, sexp self, sexp n, sexp count) {
    G->needs_redraw = true;
    point_move_by_line(sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

SCM_CMD(scm_next_line,           point_move_by_line(1))
SCM_CMD(scm_prev_line,           point_move_by_line(-1))

sexp scm_forward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());
    if (point == chars)
        message_send("End of buffer");
    point_move(1);
    return SEXP_VOID;
}

sexp scm_backward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    if (point == 0)
        message_send("Beginning of buffer");
    point_move(-1);
    return SEXP_VOID;
}

SCM_CMD(scm_newline,             (insert_char(buffer_get_current(), '\n'), message_clear()))
SCM_CMD(scm_insert_tab,          (insert_char(buffer_get_current(), '\t'), message_clear()))

SCM_CMD(scm_point_to_line_start, point_to_line_start(buffer_get_current()))
SCM_CMD(scm_point_to_line_end,   point_to_line_end(buffer_get_current()))

sexp scm_delete_backward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    size_t point = point_get(buffer_get_current()).pos;
    message_clear();
    if (point == 0)
        message_send("Beginning of buffer");
    delete_chars(buffer_get_current(), 1);
    return SEXP_VOID;
}

sexp scm_delete_forward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());
    message_clear();
    if (point >= chars)
        message_send("End of buffer");
    delete_chars(buffer_get_current(), -1);
    return SEXP_VOID;
}

sexp scm_skip_whitespace(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    while (char_at_point() == ' ' || char_at_point() == '\t')
        point_move(1);
    return SEXP_VOID;
}

sexp scm_set_column(sexp ctx, sexp self, sexp n, sexp column) {
    G->needs_redraw = true;
    set_column(sexp_unbox_fixnum(column), false);

    return SEXP_VOID;
}

sexp scm_char_at_point(sexp ctx, sexp self, sexp n) {
    printf("point: %c\n", char_at_point());

    return SEXP_VOID;
}
