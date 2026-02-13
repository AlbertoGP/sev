#include <time.h>

#include "gap.h"
#include "line.h"
#include "location.h"
#include "mark.h"
#include "var.h"
#include "../command/mode.h"

typedef struct {
    struct timespec mtime;
} Time;

#define BUFFER_NAME_MAX 256
#define FILE_NAME_MAX   256
#define JUMP_LIST_SIZE  100

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

    Mark named_marks[26];
    Mark last_cursor;
    Mark last_insert;
    Mark last_change;
    Mark change_list[JUMP_LIST_SIZE];
    Mark select_start;
    Mark select_end;
    SelectMode select_mode;
    bool replace_mode;

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

