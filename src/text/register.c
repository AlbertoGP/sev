#include <stdlib.h>
#include <string.h>

#include <chibi/eval.h>

#include "register.h"
#include "../command/scheme_internal.h"

static int reg_index(char name) {
    if (name >= 'a' && name <= 'z') return name - 'a';
    if (name >= 'A' && name <= 'Z') return name - 'A';
    if (name == '"') return REGISTER_UNNAMED;
    return -1;
}

void register_write(Register regs[], char name, const char *data, size_t len) {
    int idx = reg_index(name);
    if (idx < 0) return;
    free(regs[idx].data.data);
    regs[idx].data.data = malloc(len);
    if (regs[idx].data.data) {
        memcpy(regs[idx].data.data, data, len);
        regs[idx].data.len = len;
    } else {
        regs[idx].data.len = 0;
    }
    regs[idx].shape = SHAPE_CHARWISE;
    regs[idx].block_width = 0;
}

void register_append(Register regs[], char name, const char *data, size_t len) {
    int idx = reg_index(name);
    if (idx < 0) return;
    size_t old_len = regs[idx].data.len;
    char *new_data = realloc(regs[idx].data.data, old_len + len);
    if (!new_data) return;
    memcpy(new_data + old_len, data, len);
    regs[idx].data.data = new_data;
    regs[idx].data.len = old_len + len;
    // preserve existing shape on append
}

const ByteString *register_read(Register regs[], char name) {
    int idx = reg_index(name);
    if (idx < 0) return NULL;
    return &regs[idx].data;
}

void register_set_shape(Register regs[], char name, SelectionShape shape) {
    int idx = reg_index(name);
    if (idx < 0) return;
    regs[idx].shape = shape;
}

SelectionShape register_get_shape(Register regs[], char name) {
    int idx = reg_index(name);
    if (idx < 0) return SHAPE_CHARWISE;
    return regs[idx].shape;
}

void register_set_block_width(Register regs[], char name, size_t width) {
    int idx = reg_index(name);
    if (idx < 0) return;
    regs[idx].block_width = width;
}

size_t register_get_block_width(Register regs[], char name) {
    int idx = reg_index(name);
    if (idx < 0) return 0;
    return regs[idx].block_width;
}

void register_free_all(Register regs[]) {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        free(regs[i].data.data);
        regs[i].data.data = NULL;
        regs[i].data.len = 0;
    }
}

// --- Scheme bindings ---

sexp scm_register_set(sexp ctx, sexp self, sexp n, sexp sname, sexp stext) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, stext);
    char name = (char)sexp_unbox_character(sname);
    const char *data = sexp_string_data(stext);
    size_t len = sexp_string_size(stext);
    register_write(G->registers, name, data, len);
    return SEXP_VOID;
}

sexp scm_register_append(sexp ctx, sexp self, sexp n, sexp sname, sexp stext) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, stext);
    char name = (char)sexp_unbox_character(sname);
    const char *data = sexp_string_data(stext);
    size_t len = sexp_string_size(stext);
    register_append(G->registers, name, data, len);
    return SEXP_VOID;
}

sexp scm_register_get(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char name = (char)sexp_unbox_character(sname);
    const ByteString *bs = register_read(G->registers, name);
    if (!bs || !bs->data || bs->len == 0) return SEXP_FALSE;
    return sexp_c_string(ctx, bs->data, (sexp_sint_t)bs->len);
}

sexp scm_register_set_block_width(sexp ctx, sexp self, sexp n, sexp sname, sexp swidth) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char name = (char)sexp_unbox_character(sname);
    size_t width = (size_t)sexp_unbox_fixnum(swidth);
    register_set_block_width(G->registers, name, width);
    return SEXP_VOID;
}

sexp scm_register_get_block_width(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char name = (char)sexp_unbox_character(sname);
    size_t width = register_get_block_width(G->registers, name);
    return sexp_make_fixnum((sexp_sint_t)width);
}

sexp scm_register_set_shape(sexp ctx, sexp self, sexp n, sexp sname, sexp sshape) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char name = (char)sexp_unbox_character(sname);
    SelectionShape shape = SHAPE_CHARWISE;
    if (sexp_symbolp(sshape)) {
        if (sshape == sexp_intern(ctx, "linewise", -1))
            shape = SHAPE_LINEWISE;
        else if (sshape == sexp_intern(ctx, "blockwise", -1))
            shape = SHAPE_BLOCKWISE;
    }
    register_set_shape(G->registers, name, shape);
    return SEXP_VOID;
}

sexp scm_register_get_shape(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char name = (char)sexp_unbox_character(sname);
    SelectionShape shape = register_get_shape(G->registers, name);
    switch (shape) {
        case SHAPE_LINEWISE:  return sexp_intern(ctx, "linewise",  -1);
        case SHAPE_BLOCKWISE: return sexp_intern(ctx, "blockwise", -1);
        default:              return sexp_intern(ctx, "charwise",  -1);
    }
}
