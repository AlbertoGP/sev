// Buffer-local variables (VarTable)
// Stores sexp values keyed by string names.

#pragma once

#include <chibi/sexp.h>

typedef struct VarEntry {
    sexp key;
    sexp value;
    struct VarEntry *next;
} VarEntry;

typedef struct VarTable {
    VarEntry *entries;
} VarTable;

// Initialize a VarTable (zeroes entries pointer)
void vartable_init(VarTable *vt);

// Free all entries in a VarTable
void vartable_destroy(VarTable *vt);

// Get a variable's value, or return default_val if not found
sexp vartable_get(VarTable *vt, sexp key, sexp default_val);

// Set a variable's value (creates entry if needed)
// The name string is copied internally
void vartable_set(VarTable *vt, sexp key, sexp value);

// Get string value from sexp (symbol or string), returns default if not string-like
const char *sexp_to_cstring(sexp ctx, sexp val, const char *default_val);
