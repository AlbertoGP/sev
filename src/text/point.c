// Point movement, position queries, and navigation Scheme bindings.

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buffer_type.h"
#include "gap.h"
#include "line.h"
#include "utf8.h"
#include "var.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"

extern KeyEvent last_event;

static void update_point(Buffer *buf) {
    buf->point.pos = gb_point_get(buf->contents);
}

static int buffer_tab_width(Buffer *buf) {
    sexp ctx = G->chibi.ctx;
    sexp tw_sym = sexp_intern(ctx, "tab-width", -1);
    sexp tw_val = vartable_get(buffer_get_locals(buf), tw_sym, SEXP_FALSE);
    return (sexp_fixnump(tw_val) && sexp_unbox_fixnum(tw_val) > 0)
           ? (int)sexp_unbox_fixnum(tw_val) : 4;
}

// Convert a byte offset within a line to a 1-indexed display column.
// Advances one UTF-8 sequence per column; tabs jump to the next tab stop.
static int line_byte_to_col(Buffer *buf, size_t line_start, size_t byte_pos, int tab_width) {
    if (tab_width <= 0) tab_width = 4;
    int col = 0;
    size_t p = line_start;
    while (p < byte_pos) {
        char c = (char)buf_char_at(buf, p);
        if (c == '\t') {
            col += tab_width - (col % tab_width);
            p += 1;
        } else {
            int n = utf8_seq_len_fwd(&c);
            if (p + n > byte_pos) n = (int)(byte_pos - p);
            col += 1;
            p += n;
        }
    }
    return col + 1;
}

// Find the byte position within [line_start, line_end) at which the display
// column first reaches target_col_1based. When the target lands inside a tab,
// `round` selects whether to stop before (false) or after (true) the tab.
static size_t line_col_to_byte(Buffer *buf, size_t line_start, size_t line_end,
                               int target_col_1based, int tab_width, bool round) {
    if (tab_width <= 0) tab_width = 4;
    if (target_col_1based < 1) target_col_1based = 1;
    int target0 = target_col_1based - 1;
    int col = 0;
    size_t p = line_start;
    while (p < line_end) {
        if (col >= target0) break;
        char c = (char)buf_char_at(buf, p);
        if (c == '\t') {
            int next = col + (tab_width - (col % tab_width));
            if (next > target0 && !round) break;
            col = next;
            p += 1;
        } else {
            int n = utf8_seq_len_fwd(&c);
            if (p + (size_t)n > line_end) n = (int)(line_end - p);
            col += 1;
            p += n;
        }
    }
    return p;
}

static void snapshot_line(Buffer *buf) {
    const LineTable *lt = buffer_get_line_table(buf);
    size_t li = line_index_at(lt, point_get(buf).pos);
    size_t start = lt->lines[li].start;
    size_t end = lt->lines[li].end;
    size_t len = end - start;

    free(buf->line_restore_text);
    buf->line_restore_text = malloc(len);
    if (buf->line_restore_text) {
        char *text = gb_text(buf->contents);
        memcpy(buf->line_restore_text, text + start, len);
        buf->line_restore_len = len;
    } else {
        buf->line_restore_len = 0;
    }
    buf->line_restore_line = buf->cur_line;
}

bool point_set(Location loc) {
    Buffer *buf = buffer_get_current();
    if (!buf) return false;

    gb_point_set(buf->contents, loc.pos);
    buf->col = 0;
    update_point(buf);
    update_line(buf);
    return true;
}

void update_line(Buffer *buf) {
    const LineTable *lt = buffer_get_line_table(buf);
    size_t line_index = line_index_at(lt, point_get(buf).pos);
    size_t new_line = line_index + 1;

    if (new_line != buf->cur_line && !buf->suppress_recording) {
        buf->cur_line = new_line;
        snapshot_line(buf);
    } else {
        buf->cur_line = new_line;
    }
}

void save_current_column(Buffer *buf) {
    buf->col_saved = get_column(buf);
}

bool point_move(int count) {
    Buffer *buf = buffer_get_current();
    if (!buf) return false;

    size_t point = point_get(buf).pos;

    if (!count) return true;
    else if (count > 0) {
        size_t remaining = get_char_count(buf) - point;
        if (count > (int)remaining) {
            count = (int)remaining;
        }
        size_t byte_pos = point;
        for (int i = 0; i < count; i++) {
            char c = (char)gb_char_at(buf->contents, byte_pos);
            if (c == '\n') {
                buf->cur_line++;
            }
            byte_pos += utf8_seq_len_fwd(&c);
            if (byte_pos > get_char_count(buf)) {
                byte_pos = get_char_count(buf);
                break;
            }
        }
        buf->col = 0;
        gb_point_set(buf->contents, byte_pos);
    } else {
        if (point == 0) return true;
        if (count < -(int)point)
            count = -(int)point;
        size_t byte_pos = point;
        for (int i = 0; i > count; i--) {
            // Walk back over continuation bytes to find the leading byte.
            int back_len = 0;
            char c;
            do {
                back_len++;
                c = (char)gb_char_at(buf->contents, byte_pos - back_len);
            } while (back_len < 4 && back_len < (int)byte_pos && (c & 0xC0) == 0x80);
            // c is the leading byte of the sequence we're stepping over.
            if (c == '\n') {
                buf->cur_line--;
            }
            byte_pos -= back_len;
        }
        buf->col = 0;
        gb_point_set(buf->contents, byte_pos);
    }
    update_point(buf);
    buf->col_saved = get_column(buf);
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
    Buffer *buf = buffer_get_current();
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
    } else {                 // line must be recalculated
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
    return (Location){.pos = gb_used(buffer_get_current()->contents)};
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
    Buffer *buf = buffer_get_current();
    if (point_get(buf).pos == get_char_count(buf))
        return '\0';
    return char_at_point();
}

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

int get_column(Buffer *buf) {
    if (buf->col) {  // column is known
        return buf->col;
    } else {    // column must be recalculated.
        size_t pos = buf->point.pos;
        size_t line_index = line_index_at(&buf->lt, pos);
        size_t line_start = buf->lt.lines[line_index].start;
        return buf->col = line_byte_to_col(buf, line_start, pos, buffer_tab_width(buf));
    }
}

void set_column(int column, bool round) {
    Buffer *buf = buffer_get_current();
    if (buf->col == column) return;
    if (column < 1) column = 1;

    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    size_t line_start = lt.lines[line_index].start;
    size_t line_end   = lt.lines[line_index].end;
    if (line_end > line_start && buf_char_at(buf, line_end - 1) == '\n') {
        line_end--;
    }

    int tab_width = buffer_tab_width(buf);
    int max_col = line_byte_to_col(buf, line_start, line_end, tab_width);
    if (column > max_col) column = max_col;

    size_t new_pos = line_col_to_byte(buf, line_start, line_end, column, tab_width, round);
    point_set((Location){ .pos = new_pos });
    buf->col = line_byte_to_col(buf, line_start, new_pos, tab_width);
}

void point_to_line_start(Buffer *buf) {
    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    point_set((Location){.pos = buf->lt.lines[line_index].start});
    update_line(buf);
    save_current_column(buf);
}

void point_to_line_end(Buffer *buf) {
    size_t pos = point_get(buf).pos;
    LineTable lt = buf->lt;
    size_t line_index = line_index_at(&lt, pos);
    size_t end = buf->lt.lines[line_index].end;
    if (end > buf->lt.lines[line_index].start && buf_char_at(buf, end - 1) == '\n') {
        end--;
    }
    point_set((Location){.pos = end});
    update_line(buf);
    save_current_column(buf);
}

char *buffer_text(Buffer *buf) {
    return gb_text(buf->contents);
}

const char *buffer_text_cached(Buffer *buf) {
    if (buf->text_cache && buf->text_cache_seq == buf->edit_sequence)
        return buf->text_cache;
    free(buf->text_cache);
    buf->text_cache = gb_text(buf->contents);
    buf->text_cache_seq = buf->edit_sequence;
    return buf->text_cache;
}

char char_at_point(void) {
    Buffer *buf = buffer_get_current();
    if (point_get(buf).pos == get_char_count(buf)) return '\0';
    return buf_char_at(buf, point_get(buf).pos);
}

char char_from_point(int n) {
    Buffer *buf = buffer_get_current();
    int point = point_get(buf).pos;
    int remaining = get_char_count(buf) - point;
    if (n < -point || n > remaining) return '\0';
    return buf_char_at(buf, point + n);
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

// --- Scheme bindings ---

sexp scm_point_move(sexp ctx, sexp self, sexp n, sexp count) {

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
    point_move_by_line(sexp_unbox_fixnum(count));
    return SEXP_VOID;
}

SCM_CMD(scm_next_line, point_move_by_line(1))
SCM_CMD(scm_prev_line, point_move_by_line(-1))

sexp scm_forward_char(sexp ctx, sexp self, sexp n) {
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    size_t chars = get_char_count(buffer_get_current());
    if (point == chars)
        message_send("End of buffer");
    point_move(1);
    return SEXP_VOID;
}

sexp scm_backward_char(sexp ctx, sexp self, sexp n) {
    message_clear();
    size_t point = point_get(buffer_get_current()).pos;
    if (point == 0)
        message_send("Beginning of buffer");
    point_move(-1);
    return SEXP_VOID;
}

SCM_CMD(scm_point_to_line_start, point_to_line_start(buffer_get_current()))
SCM_CMD(scm_point_to_line_end,   point_to_line_end(buffer_get_current()))

sexp scm_skip_whitespace(sexp ctx, sexp self, sexp n) {
    while (char_at_point() == ' ' || char_at_point() == '\t')
        point_move(1);
    return SEXP_VOID;
}

sexp scm_set_column(sexp ctx, sexp self, sexp n, sexp column) {
    set_column(sexp_unbox_fixnum(column), false);
    return SEXP_VOID;
}

sexp scm_char_at_point(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (point_get(buf).pos >= get_char_count(buf))
        return sexp_make_character(0);
    return sexp_make_character(char_at_point());
}

sexp scm_point_get(sexp ctx, sexp self, sexp n) {
    return sexp_make_fixnum(point_get(buffer_get_current()).pos);
}

sexp scm_point_set_to(sexp ctx, sexp self, sexp n, sexp pos) {
    Buffer *buf = buffer_get_current();
    int p = sexp_unbox_fixnum(pos);
    int num_chars = (int)get_char_count(buf);
    if (p < 0) p = 0;
    if (p > num_chars) p = num_chars;
    point_set((Location){.pos = (size_t)p});
    update_line(buf);
    save_current_column(buf);
    return SEXP_VOID;
}

sexp scm_buffer_length(sexp ctx, sexp self, sexp n) {
    return sexp_make_fixnum(get_char_count(buffer_get_current()));
}

sexp scm_char_at(sexp ctx, sexp self, sexp n, sexp pos) {
    Buffer *buf = buffer_get_current();
    int p = sexp_unbox_fixnum(pos);
    int num_chars = (int)get_char_count(buf);
    if (p < 0 || p >= num_chars)
        return sexp_make_character(0);
    return sexp_make_character((char)buf_char_at(buf, (size_t)p));
}

sexp scm_last_key_char(sexp ctx, sexp self, sexp n) {
    if (last_event.type != KEYEVENT_CHAR)
        return SEXP_FALSE;
    return sexp_make_character((char)last_event.codepoint);
}

sexp scm_goto_line(sexp ctx, sexp self, sexp n, sexp line_num) {
    Buffer *buf = buffer_get_current();
    int line = sexp_unbox_fixnum(line_num);
    int total = (int)get_line_count(buf);
    if (line < 1) line = 1;
    if (line > total) line = total;
    const LineTable *lt = buffer_get_line_table(buf);
    point_set((Location){.pos = lt->lines[line - 1].start});
    update_line(buf);
    save_current_column(buf);
    return SEXP_VOID;
}

sexp scm_line_count(sexp ctx, sexp self, sexp n) {
    return sexp_make_fixnum(get_line_count(buffer_get_current()));
}

sexp scm_position_line(sexp ctx, sexp self, sexp n, sexp spos) {
    Buffer *buf = buffer_get_current();
    size_t pos = sexp_unbox_fixnum(spos);
    const LineTable *lt = buffer_get_line_table(buf);
    return sexp_make_fixnum((int)(line_index_at(lt, pos) + 1));
}

sexp scm_line_start_position(sexp ctx, sexp self, sexp n, sexp sline) {
    Buffer *buf = buffer_get_current();
    const LineTable *lt = buffer_get_line_table(buf);
    size_t idx = sexp_unbox_fixnum(sline) - 1;
    if (idx >= lt->count) idx = lt->count - 1;
    return sexp_make_fixnum((int)lt->lines[idx].start);
}

sexp scm_line_end_position(sexp ctx, sexp self, sexp n, sexp sline) {
    Buffer *buf = buffer_get_current();
    const LineTable *lt = buffer_get_line_table(buf);
    size_t idx = sexp_unbox_fixnum(sline) - 1;
    if (idx >= lt->count) idx = lt->count - 1;
    return sexp_make_fixnum((int)lt->lines[idx].end);
}

sexp scm_buffer_substring(sexp ctx, sexp self, sexp n, sexp sstart, sexp send) {
    Buffer *buf = buffer_get_current();
    int start = sexp_unbox_fixnum(sstart);
    int end   = sexp_unbox_fixnum(send);
    int num_chars = (int)get_char_count(buf);
    if (start < 0) start = 0;
    if (end > num_chars) end = num_chars;
    if (start > end) start = end;
    char *text = buffer_text(buf);
    sexp result = sexp_c_string(ctx, text + start, end - start);
    free(text);
    return result;
}

// --- Bracket matching ---

static const char BRACKET_OPEN[]  = "([{";
static const char BRACKET_CLOSE[] = ")]}";

static bool is_in_ignored_span(const LineTable *lt, size_t li, size_t pos) {
    const Line *line = &lt->lines[li];
    uint32_t byte_in_line = (uint32_t)(pos - line->start);
    for (uint32_t i = 0; i < line->hl_span_count; i++) {
        const HLSpan *s = &line->hl_spans[i];
        if (s->style != HL_STRING && s->style != HL_COMMENT) continue;
        if (byte_in_line >= s->start_byte && byte_in_line < s->end_byte)
            return true;
    }
    return false;
}

size_t buffer_find_matching_bracket(Buffer *buf, size_t start) {
    if (!buf) return (size_t)-1;
    const LineTable *lt = buffer_get_line_table(buf);
    size_t buf_len = get_char_count(buf);
    if (start >= buf_len) return (size_t)-1;

    char ch = (char)buf_char_at(buf, start);

    int bracket_idx = -1;
    bool forward = false;
    for (int i = 0; i < 3; i++) {
        if (ch == BRACKET_OPEN[i])  { bracket_idx = i; forward = true;  break; }
        if (ch == BRACKET_CLOSE[i]) { bracket_idx = i; forward = false; break; }
    }
    if (bracket_idx < 0) return (size_t)-1;

    char open  = BRACKET_OPEN[bracket_idx];
    char close = BRACKET_CLOSE[bracket_idx];
    size_t li = line_index_at(lt, start);

    if (forward) {
        int depth = 1;
        size_t pos = start + 1;
        while (pos < buf_len) {
            while (li + 1 < lt->count && pos >= lt->lines[li + 1].start)
                li++;
            if (!is_in_ignored_span(lt, li, pos)) {
                char c = (char)buf_char_at(buf, pos);
                if (c == open)  depth++;
                if (c == close) { depth--; if (depth == 0) return pos; }
            }
            pos++;
        }
        return (size_t)-1;
    } else {
        int depth = 1;
        if (start == 0) return (size_t)-1;
        size_t pos = start - 1;
        for (;;) {
            while (li > 0 && pos < lt->lines[li].start)
                li--;
            if (!is_in_ignored_span(lt, li, pos)) {
                char c = (char)buf_char_at(buf, pos);
                if (c == close) depth++;
                if (c == open)  { depth--; if (depth == 0) return pos; }
            }
            if (pos == 0) return (size_t)-1;
            pos--;
        }
    }
}

bool buffer_jump_to_matching_bracket(Buffer *buf) {
    if (!buf) return false;
    size_t match = buffer_find_matching_bracket(buf, buf->point.pos);
    if (match == (size_t)-1) return false;
    point_set((Location){.pos = match});
    update_line(buf);
    save_current_column(buf);
    return true;
}

sexp scm_jump_to_matching_bracket(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    return buffer_jump_to_matching_bracket(buf) ? SEXP_TRUE : SEXP_FALSE;
}
