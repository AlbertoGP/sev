// Buffer-local variables implementation

#include <stdlib.h>
#include <chibi/eval.h>
#include "var.h"

void vartable_init(VarTable *vt) {
    vt->entries = NULL;
}

void vartable_destroy(VarTable *vt) {
    VarEntry *e = vt->entries;
    while (e) {
        VarEntry *next = e->next;
        free(e);
        e = next;
    }
    vt->entries = NULL;
}

sexp vartable_get(VarTable *vt, sexp key, sexp default_val) {
    for (VarEntry *e = vt->entries; e; e = e->next) {
        if (e->key == key) {
            return e->value;
        }
    }
    return default_val;
}

void vartable_set(VarTable *vt, sexp key, sexp value) {
    for (VarEntry *e = vt->entries; e; e = e->next) {
        if (e->key == key) {
            e->value = value;
            return;
        }
    }

    VarEntry *e = malloc(sizeof(VarEntry));
    if (!e) return;

    e->key = key;
    e->value = value;
    e->next = vt->entries;
    vt->entries = e;
}

const char *sexp_to_cstring(sexp ctx, sexp val, const char *default_val) {
    if (sexp_stringp(val))
        return sexp_string_data(val);
    if (sexp_symbolp(val))
        return sexp_string_data(sexp_symbol_to_string(ctx, val));
    return default_val;
}
