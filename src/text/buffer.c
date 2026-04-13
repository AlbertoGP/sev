// Buffer lifecycle and list management.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buffer.h"
#include "buffer_type.h"
#include "change.h"
#include "gap.h"
#include "treesitter.h"
#include "var.h"
#include "../command/mode.h"
#include "../command/scheme_internal.h"
#include "../display/scale.h"

typedef struct BufferList {
    Buffer *list;
    Buffer *current;
} BufferList;

static BufferList bl;

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

    ts_buffer_free(buf);
    change_free_all(buf, G ? G->chibi.ctx : NULL);
    free(buf->line_restore_text);
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
        free(buf);
        return NULL;
    }

    buf->contents = gb_new(0);
    if (!buf->contents) {
        line_table_destroy(&buf->lt);
        free(buf);
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

    // Set text-mode as the default major mode (no-op before scheme_init)
    Mode *default_mode = mode_get_default();
    if (default_mode)
        buffer_set_major_mode(buf, default_mode);

    // Enable evil-normal-mode if registered (no-op before scheme_init)
    Mode *evil = mode_lookup("evil-normal-mode", MODE_MINOR);
    if (evil) {
        buffer_enable_minor_mode(buf, evil);
        sexp ctx = G->chibi.ctx;
        sexp key = sexp_intern(ctx, "mode-name", -1);
        sexp val = sexp_c_string(ctx, "Normal", -1);
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

    uint64_t next_id = buf->lt.next_line_id;
    line_table_destroy(&buf->lt);
    buf->lt = new_lt;
    buf->lt.next_line_id = next_id;
    buf->lt.lines[0].line_id = buf->lt.next_line_id++;

    // Force redraw
    if (G) G->needs_redraw = true;

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
    if (!bl.current || !name)
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

void get_file_name(char *file_name, int size) {
    Buffer *buf = buffer_get_current();
    if (!buf || !file_name || size <= 0) return;
    strncpy(file_name, buf->file_name, size - 1);
    file_name[size - 1] = '\0';
}

bool set_file_name(char *file_name) {
    Buffer *buf = buffer_get_current();
    if (!buf || !file_name) return false;
    strncpy(buf->file_name, file_name, FILE_NAME_MAX - 1);
    buf->file_name[FILE_NAME_MAX - 1] = '\0';
    return true;
}

bool buffer_write(void) {
    Buffer *buf = buffer_get_current();
    if (!buf || buf->file_name[0] == '\0') return false;

    FILE *f = fopen(buf->file_name, "wb");
    if (!f) return false;

    char *text = buffer_text(buf);
    if (text) {
        size_t len = strlen(text);
        fwrite(text, 1, len, f);
        free(text);
    }
    fclose(f);

    buf->file_time.mtime.tv_sec = time(NULL);
    buf->file_time.mtime.tv_nsec = 0;
    buf->is_modified = false;
    return true;
}

void set_modified(bool is_modified) {
    Buffer *buf = buffer_get_current();
    if (buf) buf->is_modified = is_modified;
}

bool get_modified(void) {
    Buffer *buf = buffer_get_current();
    return buf ? buf->is_modified : false;
}

bool buffer_insert(char *file_name) {
    Buffer *buf = buffer_get_current();
    if (!buf || !file_name || file_name[0] == '\0') return false;

    FILE *f = fopen(file_name, "rb");
    if (!f) return false;

    char buffer[4096];
    size_t bytes;
    bool inserted = false;

    while ((bytes = fread(buffer, 1, sizeof(buffer) - 1, f)) > 0) {
        buffer[bytes] = '\0';
        insert_string(buf, buffer);
        inserted = true;
    }

    fclose(f);
    if (inserted)
        buf->is_modified = true;
    return true;
}

bool buffer_read(void) {
    Buffer *buf = buffer_get_current();
    if (!buf || buf->file_name[0] == '\0') return false;

    buffer_clear(buf);

    FILE *f = fopen(buf->file_name, "rb");
    if (!f) return false;

    char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer) - 1, f)) > 0) {
        buffer[bytes] = '\0';
        insert_string(buf, buffer);
    }

    fclose(f);

    buf->file_time.mtime.tv_sec = time(NULL);
    buf->file_time.mtime.tv_nsec = 0;
    buf->is_modified = false;

    point_set(count_to_location(0));
    if (buf->ts.parser) ts_buffer_parse(buf);

    return true;
}
