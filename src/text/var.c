// Buffer-local variables implementation

#include <stdlib.h>

#include <chibi/eval.h>

#include "buffer.h"
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

// --- Scheme bindings ---

// (%setq-local name val) -> val
sexp scm_set_local(sexp ctx, sexp self, sexp n, sexp key, sexp sval) {
    if (!sexp_symbolp(key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", key);
    }

    VarTable *locals = buffer_get_locals(buffer_get_current());
    if (!locals) return SEXP_FALSE;

    sexp_preserve_object(ctx, sval);
    vartable_set(locals, key, sval);
    return sval;
}

// (%getq-local name default) -> value or default
sexp scm_get_local(sexp ctx, sexp self, sexp n, sexp key, sexp sdefault) {
    if (!sexp_symbolp(key)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", key);
    }

    VarTable *locals = buffer_get_locals(buffer_get_current());
    if (!locals) return sdefault;

    return vartable_get(locals, key, sdefault);
}
