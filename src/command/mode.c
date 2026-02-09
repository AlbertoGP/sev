// Major and minor mode implementation

#include <stdlib.h>
#include <string.h>

#include "mode.h"
#include "../text/buffer.h"

// Global mode registries
static Mode *g_major_modes = NULL;
static Mode *g_minor_modes = NULL;

void modelist_init(ModeList *ml) {
    ml->head = NULL;
    ml->count = 0;
}

void modelist_destroy(ModeList *ml) {
    // Free the list nodes - modes themselves are owned by global registry
    ModeListNode *n = ml->head;
    while (n) {
        ModeListNode *next = n->next;
        free(n);
        n = next;
    }
    ml->head = NULL;
    ml->count = 0;
}

Mode *mode_create(const char *name, ModeType type, Keymap *keymap) {
    Mode *m = malloc(sizeof(Mode));
    if (!m) return NULL;

    m->name = strdup(name);
    if (!m->name) {
        free(m);
        return NULL;
    }

    m->type = type;
    m->keymap = keymap;
    m->next = NULL;

    return m;
}

Mode *mode_register(Mode *mode) {
    if (!mode) return NULL;

    // Check if already registered
    Mode **registry = (mode->type == MODE_MAJOR) ? &g_major_modes : &g_minor_modes;

    for (Mode *m = *registry; m; m = m->next) {
        if (strcmp(m->name, mode->name) == 0) {
            return NULL;  // already registered
        }
    }

    // Prepend to registry
    mode->next = *registry;
    *registry = mode;

    return mode;
}

Mode *mode_lookup(const char *name, ModeType type) {
    Mode *registry = (type == MODE_MAJOR) ? g_major_modes : g_minor_modes;

    for (Mode *m = registry; m; m = m->next) {
        if (strcmp(m->name, name) == 0) {
            return m;
        }
    }

    return NULL;
}

Mode *mode_lookup_any(const char *name) {
    Mode *m = mode_lookup(name, MODE_MAJOR);
    if (m) return m;
    return mode_lookup(name, MODE_MINOR);
}

Mode *mode_get_fundamental(void) {
    Mode *m = mode_lookup("fundamental-mode", MODE_MAJOR);
    if (m) return m;

    // Create fundamental-mode
    m = mode_create("fundamental-mode", MODE_MAJOR, NULL);
    if (!m) return NULL;

    mode_register(m);
    return m;
}

// --- Scheme bindings ---

// (%register-mode name type keymap) -> mode-ptr or #f
sexp scm_register_mode(sexp ctx, sexp self, sexp n,
                       sexp sname, sexp stype, sexp skeymap) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }
    if (!sexp_symbolp(stype)) {
        return sexp_user_exception(ctx, self, "type must be 'major or 'minor", stype);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    const char *type_str = sexp_string_data(sexp_symbol_to_string(ctx, stype));

    ModeType type;
    if (strcmp(type_str, "major") == 0) {
        type = MODE_MAJOR;
    } else if (strcmp(type_str, "minor") == 0) {
        type = MODE_MINOR;
    } else {
        return sexp_user_exception(ctx, self, "type must be 'major or 'minor", stype);
    }

    Keymap *km = NULL;
    if (sexp_cpointerp(skeymap)) {
        km = sexp_cpointer_value(skeymap);
    }

    Mode *mode = mode_create(name, type, km);
    if (!mode) return SEXP_FALSE;

    if (!mode_register(mode)) {
        free((void *)mode->name);
        free(mode);
        return SEXP_FALSE;
    }

    return sexp_make_cpointer(ctx, SEXP_CPOINTER, mode, SEXP_FALSE, 0);
}

// (%set-major-mode name) -> #t or #f
sexp scm_set_major_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MAJOR);
    if (!mode) return SEXP_FALSE;

    buffer_set_major_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%enable-minor-mode name) -> #t or #f
sexp scm_enable_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MINOR);
    if (!mode) return SEXP_FALSE;

    buffer_enable_minor_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%disable-minor-mode name) -> #t or #f
sexp scm_disable_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname)) {
        return sexp_user_exception(ctx, self, "name must be a symbol", sname);
    }

    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    Mode *mode = mode_lookup(name, MODE_MINOR);
    if (!mode) return SEXP_FALSE;

    buffer_disable_minor_mode(buffer_get_current(), mode);
    return SEXP_TRUE;
}

// (%buffer-major-mode) -> symbol or #f
sexp scm_buffer_major_mode(sexp ctx, sexp self, sexp n) {
    Mode *mode = buffer_get_major_mode(buffer_get_current());
    if (!mode) return SEXP_FALSE;

    return sexp_intern(ctx, mode->name, -1);
}

// (%buffer-minor-modes) -> list of symbols
sexp scm_buffer_minor_modes(sexp ctx, sexp self, sexp n) {
    ModeList *minors = buffer_get_minor_modes(buffer_get_current());
    if (!minors) return SEXP_NULL;

    sexp result = SEXP_NULL;
    sexp_gc_var1(sym);
    sexp_gc_preserve1(ctx, sym);

    for (ModeListNode *node = minors->head; node; node = node->next) {
        sym = sexp_intern(ctx, node->mode->name, -1);
        result = sexp_cons(ctx, sym, result);
    }

    sexp reversed = SEXP_NULL;
    while (sexp_pairp(result)) {
        sexp tmp = sexp_cdr(result);
        sexp_cdr(result) = reversed;
        reversed = result;
        result = tmp;
    }

    sexp_gc_release1(ctx);
    return reversed;
}

// (%buffer-has-minor-mode? name) -> #t or #f
sexp scm_buffer_has_minor_mode(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_symbolp(sname))
        return sexp_user_exception(ctx, self, "name must be symbol", sname);
    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sname));
    return buffer_has_minor_mode(buffer_get_current(), name) ? SEXP_TRUE : SEXP_FALSE;
}
