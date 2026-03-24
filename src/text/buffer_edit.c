// Content editing (insert/delete) and related Scheme bindings.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buffer_type.h"
#include "change.h"
#include "gap.h"
#include "line.h"
#include "treesitter.h"
#include "utf8.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"

extern KeyEvent last_event;

static void update_point(Buffer *buf) {
    buf->point.pos = gb_point_get(buf->contents);
}

void insert_char(Buffer *buf, char c) {
    size_t pos_before = point_get(buf).pos;
    uint32_t pos = (uint32_t)pos_before;
    TSPoint start_pt = ts_byte_to_point(&buf->lt, pos);
    TSPoint new_end_pt = (c == '\n')
        ? (TSPoint){ start_pt.row + 1, 0 }
        : (TSPoint){ start_pt.row, start_pt.column + 1 };
    ts_buffer_edit(buf, pos, pos, pos + 1, new_end_pt);
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
    change_record_insert(buf, pos_before, &c, 1);
    ts_buffer_reparse(buf);
}

void insert_codepoint(Buffer *buf, uint32_t cp) {
    if (cp < 0x80) {
        insert_char(buf, (char)cp);
        return;
    }
    char bytes[4];
    int len = utf8_encode(cp, bytes);
    size_t pos_before = point_get(buf).pos;
    uint32_t pos = (uint32_t)pos_before;
    TSPoint start_pt = ts_byte_to_point(&buf->lt, pos);
    TSPoint new_end_pt = { start_pt.row, start_pt.column + (uint32_t)len };
    ts_buffer_edit(buf, pos, pos, pos + (uint32_t)len, new_end_pt);
    for (int i = 0; i < len; i++) {
        gb_insert(buf->contents, bytes[i]);
        line_insert_char(&buf->lt, pos_before + i, bytes[i]);
    }
    buf->col_saved = ++(buf->col);
    buf->num_chars = gb_used(buf->contents);
    update_point(buf);
    change_record_insert(buf, pos_before, bytes, len);
    ts_buffer_reparse(buf);
}

void insert_string(Buffer *buf, const char *s) {
    while (*s) {
        insert_char(buf, *s++);
    }
}

bool delete_chars(Buffer *buf, int count) {
    if (!buf) return false;

    int point = point_get(buf).pos;

    if (count > 0) {
        if (count > point)
            count = point;
        uint32_t del_start = (uint32_t)(point - count);
        TSPoint  del_start_pt = ts_byte_to_point(&buf->lt, del_start);
        ts_buffer_edit(buf, del_start, (uint32_t)point, del_start, del_start_pt);
        // Record deleted chars before removal (backspace: chars before point)
        if (!buf->suppress_recording && buf->current_change) {
            char tmp[1];
            for (int i = 0; i < count; i++) {
                tmp[0] = char_from_point(i - count);
                change_record_delete(buf, point - count, tmp, 1);
            }
        }
        for (int i = 0; i < count; i++) {
            char c = char_from_point(i - 1);
            line_backspace_char(&buf->lt, point, c);
            if (c == '\n') {
                if (buf->line_restore_line == buf->cur_line) {
                    free(buf->line_restore_text);
                    buf->line_restore_text = NULL;
                }
                buf->cur_line--;
                buf->num_lines--;
                buf->col = 0;
            } else {
                buf->col_saved = --(buf->col);
            }
        }
        gb_backspace(buf->contents, count);
        ts_buffer_reparse(buf);
    }
    if (count < 0) {
        if (count < -(int)(gb_back(buf->contents)))
            count = -(int)(gb_back(buf->contents));
        uint32_t del_start = (uint32_t)point;
        uint32_t del_end   = (uint32_t)(point + (size_t)(-count));
        TSPoint  del_start_pt = ts_byte_to_point(&buf->lt, del_start);
        ts_buffer_edit(buf, del_start, del_end, del_start, del_start_pt);
        // Record deleted chars before removal (forward delete: chars after point)
        if (!buf->suppress_recording && buf->current_change) {
            char tmp[1];
            for (int i = 0; i < -count; i++) {
                tmp[0] = char_from_point(i);
                change_record_delete(buf, point, tmp, 1);
            }
        }
        for (int i = 0; i > count; i--) {
            char c = char_from_point(i);
            line_delete_char(&buf->lt, point, c);
            if (c == '\n') {
                if (buf->line_restore_line == buf->cur_line) {
                    free(buf->line_restore_text);
                    buf->line_restore_text = NULL;
                }
                buf->num_lines--;
            }
        }
        gb_delete(buf->contents, -count);
        ts_buffer_reparse(buf);
    }
    buf->num_chars = gb_used(buf->contents);
    update_point(buf);
    buf->col_saved = get_column(buf);
    return true;
}

// --- Scheme bindings ---

sexp scm_insert_char(sexp ctx, sexp self, sexp n, sexp ch) {
    G->needs_redraw = true;
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, ch);
    insert_codepoint(buffer_get_current(), (uint32_t)sexp_unbox_character(ch));

    message_clear();
    return SEXP_VOID;
}

sexp scm_self_insert(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    KeyEvent last = last_event;

    if (last.type != KEYEVENT_CHAR) return SEXP_VOID;

    Buffer *buf = buffer_get_current();
    if (buf->replace_mode) {
        char c = char_at_point();
        if (c != '\0' && c != '\n')
            delete_chars(buf, -utf8_seq_len_fwd(&c));
    }
    insert_codepoint(buf, last.codepoint);

    message_clear();
    return SEXP_VOID;
}

sexp scm_set_replace_mode(sexp ctx, sexp self, sexp n, sexp val) {
    Buffer *buf = buffer_get_current();
    if (buf) buf->replace_mode = sexp_truep(val);
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

SCM_CMD(scm_newline,    (insert_char(buffer_get_current(), '\n'), message_clear()))
SCM_CMD(scm_insert_tab, (insert_char(buffer_get_current(), '\t'), message_clear()))

sexp scm_delete_backward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    size_t point = point_get(buf).pos;
    message_clear();
    if (point == 0) {
        message_send("Beginning of buffer");
        return SEXP_VOID;
    }
    // Walk back over continuation bytes to find the full UTF-8 sequence length.
    int seq_len = 0;
    char c;
    do {
        seq_len++;
        c = (char)gb_char_at(buf->contents, point - seq_len);
    } while (seq_len < 4 && seq_len < (int)point && (c & 0xC0) == 0x80);
    delete_chars(buf, seq_len);
    return SEXP_VOID;
}

sexp scm_delete_forward_char(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    size_t point = point_get(buf).pos;
    size_t chars = get_char_count(buf);
    message_clear();
    if (point >= chars) {
        message_send("End of buffer");
        return SEXP_VOID;
    }
    char c = char_at_point();
    delete_chars(buf, -utf8_seq_len_fwd(&c));
    return SEXP_VOID;
}

sexp scm_delete_range(sexp ctx, sexp self, sexp n, sexp sstart, sexp send) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    int start = sexp_unbox_fixnum(sstart);
    int end = sexp_unbox_fixnum(send);
    if (start > end) { int t = start; start = end; end = t; }
    int num_chars = (int)get_char_count(buf);
    if (start < 0) start = 0;
    if (end > num_chars) end = num_chars;
    int count = end - start;
    if (count <= 0) return SEXP_VOID;
    // Move point to start, then delete forward one char at a time.
    // Single-char delete_chars(-1) correctly updates all metadata (line table,
    // cur_line, num_lines) on each iteration, avoiding the bulk-delete bug
    // where char_from_point reads stale positions.
    point_set((Location){.pos = (size_t)start});
    update_line(buf);
    for (int i = 0; i < count; i++) {
        delete_chars(buf, -1);
    }
    return SEXP_VOID;
}

sexp scm_insert_string(sexp ctx, sexp self, sexp n, sexp stext) {
    G->needs_redraw = true;
    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, stext);
    insert_string(buffer_get_current(), sexp_string_data(stext));
    return SEXP_VOID;
}

// --- Change / undo bindings ---

sexp scm_begin_change(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (buf) change_begin(buf);
    return SEXP_VOID;
}

sexp scm_end_change(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (buf) change_end(buf);
    return SEXP_VOID;
}

sexp scm_undo(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->undo_head) {
        message_send("No further undo information");
        return SEXP_VOID;
    }
    change_undo(buf, ctx);
    return SEXP_VOID;
}

sexp scm_redo(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->redo_head) {
        message_send("No further redo information");
        return SEXP_VOID;
    }
    change_redo(buf, ctx);
    return SEXP_VOID;
}

sexp scm_change_active(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_FALSE;
    return buf->current_change ? SEXP_TRUE : SEXP_FALSE;
}

sexp scm_change_set_repeat_info(sexp ctx, sexp self, sexp n, sexp info) {
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->current_change) return SEXP_VOID;
    if (buf->current_change->repeat_info != SEXP_FALSE) {
        sexp_release_object(ctx, buf->current_change->repeat_info);
    }
    sexp_preserve_object(ctx, info);
    buf->current_change->repeat_info = info;
    return SEXP_VOID;
}

sexp scm_change_last_repeat_info(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->undo_head) return SEXP_FALSE;
    return buf->undo_head->repeat_info;
}

sexp scm_change_current_inserts(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->current_change) return sexp_c_string(ctx, "", 0);

    // Calculate total length of INSERT ops
    size_t total = 0;
    for (EditOp *op = buf->current_change->ops; op; op = op->next) {
        if (op->type == EDIT_INSERT) total += op->len;
    }

    if (total == 0) return sexp_c_string(ctx, "", 0);

    char *combined = malloc(total);
    if (!combined) return sexp_c_string(ctx, "", 0);

    size_t pos = 0;
    for (EditOp *op = buf->current_change->ops; op; op = op->next) {
        if (op->type == EDIT_INSERT) {
            memcpy(combined + pos, op->text, op->len);
            pos += op->len;
        }
    }

    sexp result = sexp_c_string(ctx, combined, (int)total);
    free(combined);
    return result;
}

sexp scm_line_restore(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->line_restore_text) {
        message_send("No line snapshot");
        return SEXP_VOID;
    }
    if (buf->line_restore_line != buf->cur_line) {
        message_send("Line snapshot is for a different line");
        return SEXP_VOID;
    }

    const LineTable *lt = buffer_get_line_table(buf);
    size_t li = line_index_at(lt, point_get(buf).pos);
    size_t start = lt->lines[li].start;
    size_t end = lt->lines[li].end;

    // Delete current line content
    point_set((Location){.pos = start});
    update_line(buf);
    size_t count = end - start;
    for (size_t i = 0; i < count; i++) {
        delete_chars(buf, -1);
    }

    // Insert snapshot text
    for (size_t i = 0; i < buf->line_restore_len; i++) {
        insert_char(buf, buf->line_restore_text[i]);
    }

    // Move to line start
    point_to_line_start(buf);
    return SEXP_VOID;
}
