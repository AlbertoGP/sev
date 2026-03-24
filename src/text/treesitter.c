#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "treesitter.h"
#include "buffer.h"
#include "buffer_type.h"
#include "line.h"
#include "../../vendored/tree-sitter-scheme/bindings/c/tree-sitter-scheme.h"
#include <chibi/eval.h>
#include "../command/scheme_internal.h"

// highlights.scm from vendored/tree-sitter-scheme/queries/highlights.scm
// Embedded to avoid runtime file I/O (works on WASM too).
static const char SCHEME_HIGHLIGHTS_SCM[] =
    "[\"(\" \")\" \"[\" \"]\" \"{\" \"}\"] @punctuation.bracket\n"
    "\n"
    "(number) @number\n"
    "(character) @constant.builtin\n"
    "(boolean) @constant.builtin\n"
    "(symbol) @variable\n"
    "\n"
    "(string) @string\n"
    "\n"
    "(escape_sequence) @escape\n"
    "\n"
    "(list\n"
    "  .\n"
    "  (symbol) @function)\n"
    "\n"
    "(list\n"
    "  .\n"
    "  \"[\"\n"
    "  .\n"
    "  (symbol)+ @variable\n"
    "  .\n"
    "  \"]\")\n"
    "\n"
    "((symbol) @operator\n"
    " (#match? @operator \"^(\\\\+|-|\\\\*|/|=|>|<|>=|<=)$\"))\n"
    "\n"
    "(list\n"
    "  .\n"
    "  (symbol) @keyword\n"
    "  (#match? @keyword\n"
    "   \"^(define-syntax|let\\\\*|lambda|\xce\xbb|case|=>|quote-splicing|unquote-splicing|set!|let|letrec|letrec-syntax|let-values|let\\\\*-values|do|else|define|cond|syntax-rules|unquote|begin|quote|let-syntax|and|if|quasiquote|letrec|delay|or|when|unless|identifier-syntax|assert|library|export|import|rename|only|except|prefix)$\"\n"
    "   ))\n"
    "\n"
    "(list\n"
    "  .\n"
    "  (symbol) @function.builtin\n"
    "  (#match? @function.builtin\n"
    "   \"^(caar|cadr|call-with-input-file|call-with-output-file|cdar|cddr|list|open-input-file|open-output-file|with-input-from-file|with-output-to-file|\\\\*|\\\\+|-|/|<|<=|=|>|>=|abs|acos|angle|append|apply|asin|assoc|assq|assv|atan|boolean\\\\?|caaaar|caaadr|caaar|caadar|caaddr|caadr|cadaar|cadadr|cadar|caddar|cadddr|caddr|call-with-current-continuation|call-with-values|car|cdaaar|cdaadr|cdaar|cdadar|cdaddr|cdadr|cddaar|cddadr|cddar|cdddar|cddddr|cdddr|cdr|ceiling|char->integer|char-alphabetic\\\\?|char-ci<=\\\\?|char-ci<\\\\?|char-ci=\\\\?|char-ci>=\\\\?|char-ci>\\\\?|char-downcase|char-lower-case\\\\?|char-numeric\\\\?|char-ready\\\\?|char-upcase|char-upper-case\\\\?|char-whitespace\\\\?|char<=\\\\?|char<\\\\?|char=\\\\?|char>=\\\\?|char>\\\\?|char\\\\?|close-input-port|close-output-port|complex\\\\?|cons|cos|current-error-port|current-input-port|current-output-port|denominator|display|dynamic-wind|eof-object\\\\?|eq\\\\?|equal\\\\?|eqv\\\\?|eval|even\\\\?|exact->inexact|exact\\\\?|exp|expt|floor|flush-output|for-each|force|gcd|imag-part|inexact->exact|inexact\\\\?|input-port\\\\?|integer->char|integer\\\\?|interaction-environment|lcm|length|list->string|list->vector|list-ref|list-tail|list\\\\?|load|log|magnitude|make-polar|make-rectangular|make-string|make-vector|map|max|member|memq|memv|min|modulo|negative\\\\?|newline|not|null-environment|null\\\\?|number->string|number\\\\?|numerator|odd\\\\?|output-port\\\\?|pair\\\\?|peek-char|positive\\\\?|procedure\\\\?|quotient|rational\\\\?|rationalize|read|read-char|real-part|real\\\\?|remainder|reverse|round|scheme-report-environment|set-car!|set-cdr!|sin|sqrt|string|string->list|string->number|string->symbol|string-append|string-ci<=\\\\?|string-ci<\\\\?|string-ci=\\\\?|string-ci>=\\\\?|string-ci>\\\\?|string-copy|string-fill!|string-length|string-ref|string-set!|string<=\\\\?|string<\\\\?|string=\\\\?|string>=\\\\?|string>\\\\?|string\\\\?|substring|symbol->string|symbol\\\\?|tan|transcript-off|transcript-on|truncate|values|vector|vector->list|vector-fill!|vector-length|vector-ref|vector-set!|vector\\\\?|write|write-char|zero\\\\?)$\"\n"
    "   ))\n"
    "\n"
    ";; quote ;;\n"
    "\n"
    ";; hardcoded highlight four levels of nested structure\n"
    "\n"
    "; 'atom\n"
    "(quote\n"
    "  _ @constant)\n"
    "\n"
    "; '()\n"
    "(quote\n"
    "  (_ _* @constant))\n"
    "\n"
    "; '(())\n"
    "(quote\n"
    "  (_ (_ _* @constant)))\n"
    "\n"
    "; '((()))\n"
    "(quote\n"
    "  (_ (_ (_ _* @constant))))\n"
    "\n"
    ";; sexp comment ;;\n"
    "\n"
    ";; hardcoded highlight four levels of nested structure\n"
    "\n"
    "; #;atom\n"
    "(comment\n"
    "  _ @comment)\n"
    "\n"
    "; #;(list)\n"
    "(comment\n"
    "  (_ _* @comment))\n"
    "\n"
    "; #;(list (list))\n"
    "(comment\n"
    "  (_ (_ _* @comment)))\n"
    "\n"
    "; #;(list (list (list)))\n"
    "(comment\n"
    "  (_ (_ (_ _ @comment))))\n"
    "\n"
    "[(comment)\n"
    " (block_comment)\n"
    " (directive)] @comment\n";

// Map a capture name (no leading @) to an HLKind.
// Returns HL_DEFAULT for captures we don't assign a distinct color to.
static HLKind capture_name_to_kind(const char *name, uint32_t len) {
    switch (len) {
    case 6:
        if (memcmp(name, "number", 6) == 0) return HL_NUMBER;
        if (memcmp(name, "string", 6) == 0) return HL_STRING;
        break;
    case 7:
        if (memcmp(name, "comment", 7) == 0) return HL_COMMENT;
        if (memcmp(name, "keyword", 7) == 0) return HL_KEYWORD;
        break;
    case 8:
        if (memcmp(name, "constant", 8) == 0) return HL_CONSTANT;
        if (memcmp(name, "function", 8) == 0) return HL_FUNCTION;
        if (memcmp(name, "operator", 8) == 0) return HL_OPERATOR;
        break;
    case 16:
        if (memcmp(name, "constant.builtin", 16) == 0) return HL_CONSTANT;
        if (memcmp(name, "function.builtin", 16) == 0) return HL_BUILTIN;
        break;
    default:
        break;
    }
    return HL_DEFAULT;
}

// Zero-copy read callback over the gap buffer's two-segment layout.
// Logical byte index maps directly to physical storage in one of the two
// contiguous segments, so we can return a pointer without copying.
static const char *ts_read_gap(void *payload, uint32_t byte_index,
                                TSPoint position, uint32_t *bytes_read) {
    (void)position;
    GapBuf *gb = (GapBuf *)payload;
    size_t used = gb_used(gb);

    if (byte_index >= used) {
        *bytes_read = 0;
        return NULL;
    }

    if (byte_index < gb->point) {
        // Front segment: buffer[0 .. point-1]
        *bytes_read = (uint32_t)(gb->point - byte_index);
        return &gb->buffer[byte_index];
    } else {
        // Back segment: buffer[gap_end .. size-1]
        size_t physical = byte_index + (gb->gap_end - gb->point);
        *bytes_read = (uint32_t)(gb->size - physical);
        return &gb->buffer[physical];
    }
}

TSPoint ts_byte_to_point(const LineTable *lt, uint32_t byte) {
    size_t idx = line_index_at(lt, (size_t)byte);
    return (TSPoint){ .row = (uint32_t)idx, .column = (uint32_t)(byte - lt->lines[idx].start) };
}

void ts_buffer_edit(Buffer *buf,
                    uint32_t start_byte, uint32_t old_end_byte, uint32_t new_end_byte,
                    TSPoint new_end_point) {
    buf->ts.last_edit_byte = start_byte;
    if (!buf->ts.tree) return;
    TSInputEdit edit = {
        .start_byte    = start_byte,
        .old_end_byte  = old_end_byte,
        .new_end_byte  = new_end_byte,
        .start_point   = ts_byte_to_point(&buf->lt, start_byte),
        .old_end_point = ts_byte_to_point(&buf->lt, old_end_byte),
        .new_end_point = new_end_point,
    };
    ts_tree_edit(buf->ts.tree, &edit);
}

void ts_buffer_reparse(Buffer *buf) {
    if (!buf->ts.parser || !buf->ts.tree) return;

    TSInput input = {
        .payload  = buf->contents,
        .read     = ts_read_gap,
        .encoding = TSInputEncodingUTF8,
    };

    TSTree *old_tree = buf->ts.tree;
    TSTree *new_tree = ts_parser_parse(buf->ts.parser, old_tree, input);

    free(buf->ts.changed_ranges);
    buf->ts.changed_ranges = ts_tree_get_changed_ranges(old_tree, new_tree,
                                                         &buf->ts.changed_ranges_count);
    ts_tree_delete(old_tree);
    buf->ts.tree = new_tree;

    // Mark logical lines that overlap changed ranges as hl_dirty.
    // Stage 6 will use this to skip re-highlighting clean lines.
    LineTable *lt = (LineTable *)buffer_get_line_table(buf);
    for (uint32_t ri = 0; ri < buf->ts.changed_ranges_count; ri++) {
        uint32_t r_start = buf->ts.changed_ranges[ri].start_byte;
        uint32_t r_end   = buf->ts.changed_ranges[ri].end_byte;
        size_t first = line_index_at(lt, r_start);
        size_t last  = r_end > 0 ? line_index_at(lt, r_end - 1) : first;
        for (size_t li = first; li <= last && li < lt->count; li++)
            lt->lines[li].hl_dirty = true;
    }
    // Always mark the line containing the edit dirty. When typing within an
    // existing token (e.g., extending a comment or string), tree-sitter reports
    // no structural changed_ranges even though the visual span grew, so without
    // this the highlight would only update on the next structural change.
    lt->lines[line_index_at(lt, buf->ts.last_edit_byte)].hl_dirty = true;

    ts_buffer_highlight(buf);
}

void ts_buffer_init(Buffer *buf) {
    buf->ts.language = tree_sitter_scheme();
    buf->ts.tree     = NULL;
    buf->ts.parser   = ts_parser_new();
    if (!buf->ts.parser) return;
    ts_parser_set_language(buf->ts.parser, buf->ts.language);

    uint32_t err_offset;
    TSQueryError err_type;
    buf->ts.hl_query = ts_query_new(
        buf->ts.language,
        SCHEME_HIGHLIGHTS_SCM,
        (uint32_t)strlen(SCHEME_HIGHLIGHTS_SCM),
        &err_offset, &err_type);
    if (!buf->ts.hl_query)
        fprintf(stderr, "ts: highlight query failed at offset %u (error %d)\n",
                err_offset, (int)err_type);
}

void ts_buffer_parse(Buffer *buf) {
    if (!buf->ts.parser) return;

    TSInput input = {
        .payload  = buf->contents,
        .read     = ts_read_gap,
        .encoding = TSInputEncodingUTF8,
    };

    TSTree *new_tree = ts_parser_parse(buf->ts.parser, NULL, input);
    if (buf->ts.tree) ts_tree_delete(buf->ts.tree);
    buf->ts.tree = new_tree;

    // Fresh parse — mark all lines dirty so ts_buffer_highlight rebuilds everything.
    LineTable *lt = (LineTable *)buffer_get_line_table(buf);
    for (size_t i = 0; i < lt->count; i++)
        lt->lines[i].hl_dirty = true;
    ts_buffer_highlight(buf);
}

static void line_append_hl_span(Line *line, uint32_t s, uint32_t e, uint16_t style) {
    if (line->hl_span_count == line->hl_span_cap) {
        uint32_t new_cap = line->hl_span_cap ? line->hl_span_cap * 2 : 4;
        HLSpan *new_spans = realloc(line->hl_spans, new_cap * sizeof(HLSpan));
        if (!new_spans) return;
        line->hl_spans    = new_spans;
        line->hl_span_cap = new_cap;
    }
    line->hl_spans[line->hl_span_count++] = (HLSpan){ s, e, style };
}

void ts_buffer_highlight(Buffer *buf) {
    // SAFETY: ts_buffer_highlight is the one place that legitimately writes
    // highlight spans into the line table; cast away const here.
    LineTable *lt = (LineTable *)buffer_get_line_table(buf);

    // Find the byte range covering all dirty lines; clear only their spans.
    uint32_t range_start = UINT32_MAX, range_end = 0;
    bool any_dirty = false;
    for (size_t i = 0; i < lt->count; i++) {
        if (!lt->lines[i].hl_dirty) continue;
        any_dirty = true;
        if ((uint32_t)lt->lines[i].start < range_start)
            range_start = (uint32_t)lt->lines[i].start;
        if ((uint32_t)lt->lines[i].end > range_end)
            range_end = (uint32_t)lt->lines[i].end;
        lt->lines[i].hl_span_count = 0;
    }

    if (!any_dirty || !buf->ts.hl_query || !buf->ts.tree) return;

    TSNode root = ts_tree_root_node(buf->ts.tree);
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_set_byte_range(cursor, range_start, range_end);
    ts_query_cursor_exec(cursor, buf->ts.hl_query, root);

    TSQueryMatch match;
    uint32_t capture_idx;
    while (ts_query_cursor_next_capture(cursor, &match, &capture_idx)) {
        TSQueryCapture cap = match.captures[capture_idx];

        uint32_t name_len;
        const char *name = ts_query_capture_name_for_id(
            buf->ts.hl_query, cap.index, &name_len);

        HLKind kind = capture_name_to_kind(name, name_len);
        if (kind == HL_DEFAULT) continue;

        uint32_t s = ts_node_start_byte(cap.node);
        uint32_t e = ts_node_end_byte(cap.node);
        if (e <= s) continue;

        // Clamp to dirty range and split across dirty lines only.
        if (s < range_start) s = range_start;
        if (e > range_end)   e = range_end;
        if (s >= e) continue;

        size_t line_idx = line_index_at(lt, s);
        while (s < e && line_idx < lt->count) {
            Line *line = &lt->lines[line_idx];
            uint32_t seg_end = ((uint32_t)line->end < e) ? (uint32_t)line->end : e;
            if (s < seg_end && line->hl_dirty) {
                // First-match wins within each line
                if (line->hl_span_count == 0 ||
                    s >= line->hl_spans[line->hl_span_count - 1].end_byte) {
                    line_append_hl_span(line, s, seg_end, (uint16_t)kind);
                }
            }
            s = seg_end;
            line_idx++;
        }
    }

    ts_query_cursor_delete(cursor);

    // Clear dirty flags on processed lines.
    for (size_t i = 0; i < lt->count; i++)
        lt->lines[i].hl_dirty = false;
}

sexp scm_ts_tree_string(sexp ctx, sexp self, sexp n) {
    Buffer *buf = buffer_get_current();
    if (!buf || !buf->ts.tree)
        return sexp_c_string(ctx, "(no tree)", -1);
    TSNode root = ts_tree_root_node(buf->ts.tree);
    char *s = ts_node_string(root);
    sexp result = sexp_c_string(ctx, s, -1);
    free(s);
    return result;
}

void ts_buffer_free(Buffer *buf) {
    free(buf->ts.changed_ranges);
    buf->ts.changed_ranges       = NULL;
    buf->ts.changed_ranges_count = 0;
    if (buf->ts.hl_query) { ts_query_delete(buf->ts.hl_query); buf->ts.hl_query = NULL; }
    if (buf->ts.tree)     { ts_tree_delete(buf->ts.tree);      buf->ts.tree     = NULL; }
    if (buf->ts.parser)   { ts_parser_delete(buf->ts.parser);  buf->ts.parser   = NULL; }
    // Per-line hl_spans are freed by line_table_destroy() in buffer_free().
}
