#include "tab.h"
#include "pane.h"
#include <stdlib.h>
#include <string.h>

static TabList tl;
static SDL_Window *window;

static void update_window_title(void) {
    char buf[TAB_NAME_MAX];
    snprintf(buf, TAB_NAME_MAX, "%s - sev", tl.current->name);
    SDL_SetWindowTitle(window, buf);
}

bool tab_list_init(AppState *state) {
    tl.list = NULL;
    tl.current = NULL;

    window = state->window;

    if (!buffer_get_current()) return false;

    if (!tab_create("*scratch*")) {
        return false;
    }
    tl.current->contents = pane_create();
    pane_set_to_buffer(tl.current->contents, buffer_get_by_name("*scratch*"), true);

    #define NAME_1 "untitled-1"
    Buffer *buf = buffer_create(NAME_1);
    tab_create(NAME_1);
    Tab *next = tl.list->next;
    next->contents = pane_create();
    pane_set_to_buffer(next->contents, buf, true);

    #define NAME_2 "untitled-2"
    buf = buffer_create(NAME_2);
    tab_create(NAME_2);
    next = next->next;
    next->contents = pane_create();
    pane_set_to_buffer(next->contents, buf, true);

    update_window_title();

    return true;
}

// Free resources allocated for a tab.
static void tab_destroy(Tab *tab) {
    // Quit if tab is only tab.
    if (!tl.current->next && !tl.current->prev) {
        SDL_Event quit_event;
        quit_event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit_event);
        return;
    }

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

    update_window_title();
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

Tab *tab_get_current(void) {
    return tl.current;
}

bool tab_next(void) {
    if (!tl.current->prev && !tl.current->next) return false;

    tl.current = tl.current->next ? tl.current->next : tl.list;
    sync_active_buffer();
    update_window_title();

    return true;
}

bool tab_prev(void) {
    if (!tl.current->prev && !tl.current->next) return false;

    if (tl.current->prev) {
         tl.current = tl.current->prev;
    } else {
        while (tl.current->next) tl.current = tl.current->next;
    }
    sync_active_buffer();
    update_window_title();

    return true;
}

Pane *tab_get_root_pane(void) {
    return tl.current->contents;
}

void tab_set_root_pane(Pane *pane) {
    tl.current->contents = pane;
}

void tab_close_current(void) {
    tab_destroy(tl.current);
}

static void HandleCloseTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    Tab *t = (Tab *)userData;
    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        tab_destroy(t);
        sync_active_buffer();
    }
}

static bool CloseButton(AppState *state, Tab *t) {
    bool hovered = false;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIXED(16),
                .height = CLAY_SIZING_FIXED(16),
             },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            }
        },
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .backgroundColor = Clay_Hovered() ? state->colors.bar : (Clay_Color){0, 0, 0, 0}
    }) {
        Clay_OnHover(HandleCloseTab, t);
        CLAY_TEXT(CLAY_STRING("×"), CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 20,
            .textColor = tl.current == t ? state->colors.text : state->colors.textFaded,
            .textAlignment = CLAY_TEXT_ALIGN_CENTER,
        }));
        hovered = Clay_Hovered();
    }
    return hovered;
}

static void HandleClickTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    Tab *t = (Tab *)userData;
    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        tl.current = t;
        sync_active_buffer();
        update_window_title();
    }
}

void TabBar(AppState *state) {
    Tab *t = tl.list;
    // TODO: extract to a state variable toggle-able from Scheme.
    // if (!t || !t->next) return;  // Hide tab bar if only one tab.
    if (!t) return;                 // Show tab bar if only one tab.
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
        CLAY(CLAY_IDI("Tab", i), {
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
            bool block = CloseButton(state, t);
            Clay_OnHover(block ? NULL : HandleClickTab, t);
        }
    }
}

static Clay_Sizing layoutExpand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

void TabContent(AppState *state) {
    Tab *t = tl.current;

    if (!t->contents) {
         CLAY_AUTO_ID({ .layout = { .sizing = layoutExpand } });
         return;
    }

    PaneContent(state, t->contents, 0, 0);
}
