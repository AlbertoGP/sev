#include "pane.h"
#include "status.h"
#include "tab.h"
#include <stdlib.h>

// Recursively free resources allocated for a pane sub-tree.
void pane_destroy(Pane *pane) {
    if (pane->type == PANE_H_SPLIT) {
        pane_destroy(pane->h_split.top);
        pane_destroy(pane->h_split.bottom);
    } else if (pane->type == PANE_V_SPLIT) {
        pane_destroy(pane->v_split.left);
        pane_destroy(pane->v_split.right);
    }
    free(pane);
}

// Get first/second child of a split pane (NULL if content pane).
static Pane *pane_first_child(Pane *pane) {
    if (pane->type == PANE_H_SPLIT) return pane->h_split.top;
    if (pane->type == PANE_V_SPLIT) return pane->v_split.left;
    return NULL;
}

static Pane *pane_second_child(Pane *pane) {
    if (pane->type == PANE_H_SPLIT) return pane->h_split.bottom;
    if (pane->type == PANE_V_SPLIT) return pane->v_split.right;
    return NULL;
}

// Check if pane is the first child (top/left) of its parent.
static bool pane_is_first_child(Pane *pane) {
    Pane *parent = pane->parent;
    if (!parent) return false;
    return pane_first_child(parent) == pane;
}

Pane *pane_get_sibling(Pane *pane) {
    Pane *parent = pane->parent;
    if (!parent) return NULL;
    return pane_is_first_child(pane) ? pane_second_child(parent) : pane_first_child(parent);
}

void pane_replace_child(Pane *parent, Pane *old_child, Pane *new_child) {
    if (!parent) {
        tab_set_root_pane(new_child);
        return;
    }
    if (parent->type == PANE_H_SPLIT) {
        if (parent->h_split.top == old_child)
            parent->h_split.top = new_child;
        else
            parent->h_split.bottom = new_child;
    } else if (parent->type == PANE_V_SPLIT) {
        if (parent->v_split.left == old_child)
            parent->v_split.left = new_child;
        else
            parent->v_split.right = new_child;
    }
}

void sync_active_buffer(void) {
    Pane *active = pane_get_active();
    if (active && active->content.type == CONTENT_TEXT) {
        buffer_set_current(active->content.buffer);
    }
}

Pane *pane_create(void) {
    Pane *pane = calloc(1, sizeof(Pane));
    return pane;
}

static void usurp_parent(Pane *pane, Pane *parent) {
    Pane *grandparent = parent->parent;
    pane_replace_child(grandparent, parent, pane);
    pane->parent = grandparent;
    free(parent);
}

// Descend into pane tree, preferring the side closest to the given direction.
// E.g., DIR_UP prefers top/left children, DIR_DOWN prefers bottom/right.
static void descend_pane_tree(Pane *pane, Direction from_dir) {
    while (pane->type != PANE_CONTENT) {
        pane = pane_first_child(pane);
    }
    pane->content.active = true;
}

void pane_close(void) {
    Pane *pane = pane_get_active();
    if (!pane) return;
    Pane *parent = pane->parent;
    if (!parent) {
        tab_close_current();
        sync_active_buffer();
        return;
    }
    Pane *sibling = pane_get_sibling(pane);
    pane->content.active = false;
    descend_pane_tree(sibling, DIR_DOWN);
    usurp_parent(sibling, parent);
    free(pane);
}

bool pane_set_to_buffer(Pane *pane, Buffer *buf, bool make_active) {
    if (!pane) return false;
    // Don't abandon *or* delete a pane sub-tree, just return a signal.
    if (pane->type == PANE_V_SPLIT && (pane->v_split.left || pane->v_split.right))
        return false;
    if (pane->type == PANE_H_SPLIT && (pane->h_split.top || pane->h_split.bottom))
        return false;

    // Otherwise proceed.
    pane->type = PANE_CONTENT;
    pane->content.type = CONTENT_TEXT;
    pane->content.buffer = buf;
    pane->content.active = make_active;

    return true;
}

static Pane *pane_get_active_from_children(Pane *pane) {
    if (!pane) return NULL;

    if (pane->type == PANE_CONTENT && pane->content.active)
        return pane;

    Pane *found;
    if (pane->type == PANE_V_SPLIT) {
        found = pane_get_active_from_children(pane->v_split.left);
        if (found) return found;
        found = pane_get_active_from_children(pane->v_split.right);
        if (found) return found;
    }
    if (pane->type == PANE_H_SPLIT) {
        found = pane_get_active_from_children(pane->h_split.top);
        if (found) return found;
        found = pane_get_active_from_children(pane->h_split.bottom);
        if (found) return found;
    }

    return NULL;
}

Pane *pane_get_active(void) {
    Pane *pane = tab_get_root_pane();
    return pane_get_active_from_children(pane);
}

Pane *pane_split(Pane *pane, PaneType split_type) {
    if (split_type != PANE_H_SPLIT && split_type != PANE_V_SPLIT)
        return NULL;

    Pane *split = malloc(sizeof(Pane));
    if (!split) return NULL;
    Pane *new = malloc(sizeof(Pane));
    if (!new) {
        free(split);
        return NULL;
    }

    split->type = split_type;
    if (split_type == PANE_H_SPLIT) {
        split->h_split.top_height = 0.5;
        split->h_split.top = pane;
        split->h_split.bottom = new;
    } else {
        split->v_split.left_width = 0.5;
        split->v_split.left = pane;
        split->v_split.right = new;
    }

    pane_replace_child(pane->parent, pane, split);
    split->parent = pane->parent;
    pane->parent = split;

    new->type = PANE_CONTENT;
    new->content.type = CONTENT_TEXT;
    new->content.buffer = pane->content.buffer;
    new->content.active = false;
    new->parent = split;

    return new;
}

// Returns the split type relevant for the given direction.
static PaneType dir_split_type(Direction dir) {
    return (dir == DIR_UP || dir == DIR_DOWN) ? PANE_H_SPLIT : PANE_V_SPLIT;
}

// Returns true if navigating in `dir` means we need to be the second child.
static bool dir_from_second(Direction dir) {
    return (dir == DIR_UP || dir == DIR_LEFT);
}

bool pane_navigate(Direction dir) {
    Pane *pane = pane_get_active();
    Pane *parent = pane->parent;
    PaneType target_split = dir_split_type(dir);
    bool from_second = dir_from_second(dir);

    while (parent) {
        if (parent->type == target_split) {
            bool is_second = !pane_is_first_child(pane);
            if (is_second == from_second) {
                Pane *target = from_second ? pane_first_child(parent) : pane_second_child(parent);
                pane_get_active()->content.active = false;
                descend_pane_tree(target, dir);
                sync_active_buffer();
                return true;
            }
        }
        pane = parent;
        parent = pane->parent;
    }
    return false;
}

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

bool pane_v_split_increase(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_V_SPLIT) {
        pane = pane->parent;
    }
    if (pane) {
        pane->v_split.left_width = MIN(0.8, pane->v_split.left_width + 0.05);
        return true;
    }
    return false;
}

bool pane_v_split_decrease(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_V_SPLIT) {
        pane = pane->parent;
    }
    if (pane) {
        pane->v_split.left_width = MAX(0.2, pane->v_split.left_width - 0.05);
        return true;
    }
    return false;
}

bool pane_h_split_increase(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_H_SPLIT) {
        pane = pane->parent;
    }
    if (pane) {
        pane->h_split.top_height = MIN(0.8, pane->h_split.top_height + 0.05);
        return true;
    }
    return false;
}

bool pane_h_split_decrease(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_H_SPLIT) {
        pane = pane->parent;
    }
    if (pane) {
        pane->h_split.top_height = MAX(0.2, pane->h_split.top_height - 0.05);
        return true;
    }
    return false;
}

bool pane_has_neighbour(Pane *pane, Direction dir) {
    Pane *parent = pane->parent;
    PaneType target_split = dir_split_type(dir);
    bool from_second = dir_from_second(dir);

    while (parent) {
        if (parent->type == target_split) {
            bool is_second = !pane_is_first_child(pane);
            if (is_second == from_second)
                return true;
        }
        pane = parent;
        parent = pane->parent;
    }
    return false;
}

static Clay_Sizing layoutExpand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

static void BufferPane(AppState *state, Pane *pane, float width, float height) {
    int length = get_char_count(pane->content.buffer);
    static char *chars;
    if (chars) free (chars);
    chars = buffer_text(pane->content.buffer);
    int point = point_get(pane->content.buffer).pos;
    Clay_String head = {
        .chars = chars,
        .length = point,
        .isStaticallyAllocated = false
    };
    Clay_String tail = {
        .chars = chars + point,
        .length = length - point,
        .isStaticallyAllocated = false
    };

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = layoutExpand,
                .padding = CLAY_PADDING_ALL(24)
            }
        }) {
            CLAY_TEXT(head, CLAY_TEXT_CONFIG({
                .fontId = FONT_NORMAL,
                .fontSize = 16,
                .textColor = state->colors.text,
            }));
            CLAY_TEXT(tail, CLAY_TEXT_CONFIG({
                .fontId = FONT_NORMAL,
                .fontSize = 16,
                .textColor = state->colors.textFaded,
            }));
        }
        StatusBar(state, pane->content.active);
    }
}

static void VSplitPane(AppState *state, Pane *pane, float width, float height) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .childGap = 2
        },
    }) {
        PaneContent(state, pane->v_split.left, pane->v_split.left_width, 0);
        PaneContent(state, pane->v_split.right, 1 - pane->v_split.left_width, 0);
    }
}

static void HSplitPane(AppState *state, Pane *pane, float width, float height) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .childGap = 2
        }
    }) {
        PaneContent(state, pane->h_split.top, 0, pane->h_split.top_height);
        PaneContent(state, pane->h_split.bottom, 0, 1 - pane->h_split.top_height);
    }
}

void PaneContent(AppState *state, Pane *pane, float width, float height) {
    if (pane->type == PANE_V_SPLIT) VSplitPane(state, pane, width, height);
    if (pane->type == PANE_H_SPLIT) HSplitPane(state, pane, width, height);
    if (pane->type == PANE_CONTENT) BufferPane(state, pane, width, height);
}
