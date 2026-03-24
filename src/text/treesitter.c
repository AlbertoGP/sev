#include "treesitter.h"
#include "buffer.h"
#include "buffer_type.h"
#include "../../vendored/tree-sitter-scheme/bindings/c/tree-sitter-scheme.h"
#include <chibi/eval.h>
#include "../command/scheme_internal.h"

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

void ts_buffer_init(Buffer *buf) {
    buf->ts.language = tree_sitter_scheme();
    buf->ts.tree     = NULL;
    buf->ts.parser   = ts_parser_new();
    if (!buf->ts.parser) return;
    ts_parser_set_language(buf->ts.parser, buf->ts.language);
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
    if (buf->ts.tree)   { ts_tree_delete(buf->ts.tree);     buf->ts.tree   = NULL; }
    if (buf->ts.parser) { ts_parser_delete(buf->ts.parser); buf->ts.parser = NULL; }
}
