// Major and minor mode implementation

#include <stdlib.h>
#include <string.h>

#include "mode.h"

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
