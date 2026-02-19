// Major/minor modes, local keymap, and local variable accessors.

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "buffer_type.h"
#include "../command/mode.h"

void buffer_set_major_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode || mode->type != MODE_MAJOR) return;
    buf->major_mode = mode;
}

Mode *buffer_get_major_mode(Buffer *buf) {
    if (!buf) return NULL;
    return buf->major_mode;
}

void buffer_enable_minor_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode || mode->type != MODE_MINOR) return;

    // Check if already enabled
    for (ModeListNode *n = buf->minor_modes.head; n; n = n->next) {
        if (n->mode == mode) return;
    }

    // Prepend to list (most-recently-enabled first)
    ModeListNode *node = malloc(sizeof(ModeListNode));
    if (!node) return;

    node->mode = mode;
    node->next = buf->minor_modes.head;
    buf->minor_modes.head = node;
    buf->minor_modes.count++;
}

void buffer_disable_minor_mode(Buffer *buf, Mode *mode) {
    if (!buf || !mode) return;

    ModeListNode **pp = &buf->minor_modes.head;
    while (*pp) {
        if ((*pp)->mode == mode) {
            ModeListNode *to_free = *pp;
            *pp = (*pp)->next;
            free(to_free);
            buf->minor_modes.count--;
            return;
        }
        pp = &(*pp)->next;
    }
}

bool buffer_has_minor_mode(Buffer *buf, const char *name) {
    if (!buf || !name) return false;

    for (ModeListNode *n = buf->minor_modes.head; n; n = n->next) {
        if (strcmp(n->mode->name, name) == 0) return true;
    }
    return false;
}

ModeList *buffer_get_minor_modes(Buffer *buf) {
    if (!buf) return NULL;
    return &buf->minor_modes;
}

Keymap *buffer_get_local_map(Buffer *buf) {
    if (!buf) return NULL;
    return buf->local_map;
}

void buffer_set_local_map(Buffer *buf, Keymap *km) {
    if (!buf) return;
    buf->local_map = km;
}

VarTable *buffer_get_locals(Buffer *buf) {
    if (!buf) return NULL;
    return &buf->locals;
}
