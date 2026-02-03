// Buffer-local variables implementation

#include <stdlib.h>
#include <string.h>
#include "var.h"

void vartable_init(VarTable *vt) {
    vt->entries = NULL;
}

void vartable_destroy(VarTable *vt) {
    VarEntry *e = vt->entries;
    while (e) {
        VarEntry *next = e->next;
        free((void *)e->name);
        free(e);
        e = next;
    }
    vt->entries = NULL;
}

sexp vartable_get(VarTable *vt, const char *name, sexp default_val) {
    for (VarEntry *e = vt->entries; e; e = e->next) {
        if (strcmp(e->name, name) == 0) {
            return e->value;
        }
    }
    return default_val;
}

void vartable_set(VarTable *vt, const char *name, sexp value) {
    // Check if already exists
    for (VarEntry *e = vt->entries; e; e = e->next) {
        if (strcmp(e->name, name) == 0) {
            e->value = value;
            return;
        }
    }

    // Create new entry
    VarEntry *e = malloc(sizeof(VarEntry));
    if (!e) return;

    e->name = strdup(name);
    e->value = value;
    e->next = vt->entries;
    vt->entries = e;
}
