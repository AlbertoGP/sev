#include "tab.h"
#include <stdlib.h>
#include <string.h>

static TabList tl;

bool tab_list_init(void) {
    tl.list = NULL;
    tl.current = NULL;
    
    if (!buffer_get_current()) return false;

    if (!tab_create("*scratch*")) {
        return false;
    }
    tl.current->contents = pane_create();
    pane_set_to_buffer(tl.current->contents, "*scratch*", true);

    #define NAME_1 "untitled-1"
    buffer_create(NAME_1);
    tab_create(NAME_1);
    Tab *next = tl.list->next;
    next->contents = pane_create();
    pane_set_to_buffer(next->contents, NAME_1, true);

    #define NAME_2 "untitled-2"
    buffer_create(NAME_2);
    tab_create(NAME_2);
    next = next->next;
    next->contents = pane_create();
    pane_set_to_buffer(next->contents, NAME_2, true);

    return true;
}

// Recursively free resources allocated for a pane sub-tree.
static void pane_destroy(Pane *pane) {
    if (pane->type == PANE_H_SPLIT) {
        pane_destroy(pane->h_split.top);
        pane_destroy(pane->h_split.bottom);
    }
    if (pane->type == PANE_V_SPLIT) {
        pane_destroy(pane->v_split.left);
        pane_destroy(pane->v_split.right);
    }

    free(pane);
}

// Free resources allocated for a tab.
static void tab_destroy(Tab *tab) {
    if (tl.current == tab) {
        tl.current = tab->next ? tab->next : tab->prev;
    }

    if (tab->prev)
        tab->prev->next = tab->next;
    else
        tl.list = tab->next;

    if (tab->next)
        tab->next->prev = tab->prev;

    if (tab->contents) {
        pane_destroy(tab->contents);
        tab->contents = NULL;
    }

    free(tab);
}


void tab_list_quit(void) {
    Tab *tab = tl.list;

    if (!tab) return;
    
    while (tab) {
        Tab *next = tab->next;
        tab_destroy(tab);
        tab = next;
    }

    tl.list = NULL;
    tl.current = NULL;
}

bool tab_create(const char *name) {
    Tab *tab = malloc(sizeof(Tab));
    if (!tab) return false;

    strncpy(tab->name, name, TAB_NAME_MAX - 1);
    tab->contents = NULL;
    tab->next = NULL;

    // if tab list is empty, tab becomes list head
    if (!tl.list) {
        tab->prev = NULL;
        tl.list = tab;
        tl.current = tab;
    // otherwise append to the end of the list
    } else {
        Tab *list = tl.list;
        while (list->next) {
            list = list->next;
        }
        list->next = tab;
        tab->prev = list;
    }

    return true;
}

Pane *pane_create(void) {
    Pane *pane = calloc(1, sizeof(Pane));
    return pane;
}

bool pane_set_to_buffer(Pane *pane, const char *buf, bool make_active) {
    if (!pane) return false;
    // Don't abandon *or* delete a pane sub-tree, just return a signal.
    if (pane->type == PANE_V_SPLIT && (pane->v_split.left || pane->v_split.right))
        return false;
    if (pane->type == PANE_H_SPLIT && (pane->h_split.top || pane->h_split.bottom))
        return false;

    // Otherwise proceed.
    pane->type = PANE_CONTENT;
    pane->content.type = CONTENT_TEXT;
    pane->content.buffer = buffer_get_by_name(buf);
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
    Pane *pane = tl.current->contents;

    return pane_get_active_from_children(pane);
}

bool pane_split_horizontal(Pane *pane) {
    Pane *split = malloc(sizeof(Pane));
    if (!split) return false;
    Pane *new = malloc(sizeof(Pane));
    if (!new) {
        free(split);
        return false;
    }

    split->type = PANE_H_SPLIT;
    split->h_split.top_height = 0.5;
    split->h_split.top = pane;
    split->h_split.bottom = new;

    if (!pane->parent) {
        tl.current->contents = split;
    } else {
        Pane *p = pane->parent;
        if (p->type == PANE_V_SPLIT) {
            if (p->v_split.left == pane)
                p->v_split.left = split;
            if (p->v_split.right == pane)
                p->v_split.right = split;
        }
        if (p->type == PANE_H_SPLIT) {
            if (p->h_split.top == pane)
                p->h_split.top = split;
            if (p->h_split.bottom == pane)
                p->h_split.bottom = split;
        }
    }

    split->parent = pane->parent;
    pane->parent = split;

    new->type = PANE_CONTENT;
    new->content.type = CONTENT_TEXT;
    new->content.buffer = pane->content.buffer;
    new->parent = split;

    return true;
}

bool pane_split_vertical(Pane *pane) {
    Pane *split = malloc(sizeof(Pane));
    if (!split) return false;
    Pane *new = malloc(sizeof(Pane));
    if (!new) {
        free(split);
        return false;
    }

    split->type = PANE_V_SPLIT;
    split->v_split.left_width = 0.5;
    split->v_split.left = pane;
    split->v_split.right = new;

    if (!pane->parent) {
        tl.current->contents = split;
    } else {
        Pane *p = pane->parent;
        if (p->type == PANE_V_SPLIT) {
            if (p->v_split.left == pane)
                p->v_split.left = split;
            if (p->v_split.right == pane)
                p->v_split.right = split;
        }
        if (p->type == PANE_H_SPLIT) {
            if (p->h_split.top == pane)
                p->h_split.top = split;
            if (p->h_split.bottom == pane)
                p->h_split.bottom = split;
        }
    }

    split->parent = pane->parent;
    pane->parent = split;

    new->type = PANE_CONTENT;
    new->content.type = CONTENT_TEXT;
    new->content.buffer = pane->content.buffer;
    new->parent = split;

    return true;
}

void HandleCloseTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    Tab *t = (Tab *)userData;
    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        tab_destroy(t);
    }
}

static void CloseButton(AppState *state, Tab *t) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIXED(20),
                .height = CLAY_SIZING_FIXED(20),
             },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            }
        },
        .cornerRadius = CLAY_CORNER_RADIUS(5),
        .backgroundColor = Clay_Hovered() ? state->colors.bar : (Clay_Color){0, 0, 0, 0}
    }) {
        Clay_OnHover(HandleCloseTab, t);
        CLAY_TEXT(CLAY_STRING("×"), CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 21,
            .textColor = tl.current == t ? state->colors.text : state->colors.textFaded
        }));
    }
}

void HandleClickTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    Tab *t = (Tab *)userData;
    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        tl.current = t;
        Pane *active = pane_get_active();
        if (active->content.type == CONTENT_TEXT) {
            buffer_set_current(active->content.buffer);
        }
    }
}

void TabBar(AppState *state) {
    Tab *t = tl.list;
    CLAY(CLAY_ID("Tab Bar"), {
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(5),
            .childGap = 5
        },
        .backgroundColor = state->colors.foreground,
    }) for (int i = 1; t != NULL; t = t->next, i++) {
        Clay_String tab_name = {
            .chars = t->name,
            .length = strlen(t->name),
            .isStaticallyAllocated = true
        };
        CLAY(CLAY_IDI("Tab-", i), {
            .layout = {
                .padding = { .left = 10, .right = 5 },
                .childAlignment = {
                    .y = CLAY_ALIGN_Y_CENTER
                },
            },
            .cornerRadius = CLAY_CORNER_RADIUS(5),
            .backgroundColor = tl.current == t
                ? state-> colors.background
                : Clay_Hovered()
                    ? state->colors.background
                    : state->colors.foreground,
        }) {
            Clay_OnHover(HandleClickTab, t);
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(120),
                        .height = CLAY_SIZING_FIXED(30)
                    },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                }
            }) {
                CLAY_TEXT(tab_name, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14,
                    .textColor = tl.current == t ? state->colors.text : state->colors.textFaded
                }));
            }
            CloseButton(state, t);
        }
    }
}

static Clay_Sizing layoutExpand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

void BufferPane(AppState *state, Pane *pane, int width, int height) {
    int length = get_char_count();
    char *chars = buffer_text(pane->content.buffer);
    int point = point_get().pos;
    Clay_String head = {
        .chars = chars,
        .length = point,
        .isStaticallyAllocated = true
    };
    Clay_String tail = {
        .chars = chars + point,
        .length = length - point,
        .isStaticallyAllocated = true
    };

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .padding = CLAY_PADDING_ALL(24)
        },
        .border = {
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1 },
            .color = pane->content.active ? state->colors.text : state->colors.textFaded
        },
        // .cornerRadius = CLAY_CORNER_RADIUS(3)
    }) {
        CLAY_TEXT(head, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 18,
            .textColor = state->colors.text,
        }));
        CLAY_TEXT(tail, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 18,
            .textColor = state->colors.textFaded,
        }));
    }
}

void VSplitPane(AppState *state, Pane *pane, int width, int height) {
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
        HandlePane(state, pane->v_split.left,
                   pane->v_split.left_width, 1 - pane->v_split.left_width);
        HandlePane(state, pane->v_split.right, 0, 0);
    }
}

void HSplitPane(AppState *state, Pane *pane, int width, int height) {
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
        HandlePane(state, pane->h_split.top, 1 - pane->h_split.top_height,
                   pane->h_split.top_height);
        HandlePane(state, pane->h_split.bottom, 0, 0);
    }
}

void HandlePane(AppState *state, Pane *pane, int width, int height) {
    if (pane->type == PANE_V_SPLIT) VSplitPane(state, pane, width, height);
    if (pane->type == PANE_H_SPLIT) HSplitPane(state, pane, width, height);
    if (pane->type == PANE_CONTENT) BufferPane(state, pane, width, height);
}

void TabContent(AppState *state) {
    Tab *t = tl.current;

    if (!t->contents) {
         CLAY_AUTO_ID({ .layout = { .sizing = layoutExpand } });
         return;
    }

    HandlePane(state, t->contents, 0, 0);
}
