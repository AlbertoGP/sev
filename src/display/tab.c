#include <stdlib.h>
#include <string.h>

#include "icon.h"
#include "pane.h"
#include "tab.h"
#include "theme.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"

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
    pane_set_buffer(tl.current->contents, buffer_get_by_name("*scratch*"));
    tl.current->contents->content.active = true;
    insert_string(buffer_get_current(),
"'Twas brillig, and the slithy toves\n"
"    Did gyre and gimble in the wabe:\n"
"All mimsy were the borogoves,\n"
"    And the mome raths outgrabe.\n"
"\n"
"\"Beware the Jabberwock, my son!\n"
"    The jaws that bite, the claws that catch!\n"
"Beware the Jubjub bird, and shun\n"
"    The frumious Bandersnatch!\"\n"
"\n"
"He took his vorpal sword in hand;\n"
"    Long time the manxome foe he sought—\n"
"So rested he by the Tumtum tree\n"
"    And stood awhile in thought.\n"
"\n"
"And, as in uffish thought he stood,\n"
"    The Jabberwock, with eyes of flame,\n"
"Came whiffling through the tulgey wood,\n"
"    And burbled as it came!\n"
"\n"
"One, two! One, two! And through and through\n"
"    The vorpal blade went snicker-snack!\n"
"He left it dead, and with its head\n"
"    He went galumphing back.\n"
"\n"
"\"And hast thou slain the Jabberwock?\n"
"    Come to my arms, my beamish boy!\n"
"O frabjous day! Callooh! Callay!\"\n"
"    He chortled in his joy.\n"
"\n"
"'Twas brillig, and the slithy toves\n"
"    Did gyre and gimble in the wabe:\n"
"All mimsy were the borogoves,\n"
"    And the mome raths outgrabe.");
    point_set((Location){.pos = 0});

    #define NAME_1 "untitled-1"
    Buffer *buf = buffer_create(NAME_1);
    tab_create(NAME_1);
    Tab *next = tl.list->next;
    next->contents = pane_create();
    pane_set_buffer(next->contents, buf);
    next->contents->content.active = true;

    #define NAME_2 "untitled-2"
    buf = buffer_create(NAME_2);
    tab_create(NAME_2);
    next = next->next;
    next->contents = pane_create();
    pane_set_buffer(next->contents, buf);
    next->contents->content.active = true;

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
                .width = CLAY_SIZING_FIXED(16 * state->ui.scale_factor),
                .height = CLAY_SIZING_FIXED(16 * state->ui.scale_factor),
             },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            }
        },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .attachPoints = {
                 .element = CLAY_ATTACH_POINT_RIGHT_CENTER,
                 .parent = CLAY_ATTACH_POINT_RIGHT_CENTER
            },
            .offset = { .x = -5 * state->ui.scale_factor },
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        },
        .cornerRadius = CLAY_CORNER_RADIUS(8 * state->ui.scale_factor),
        .backgroundColor = Clay_Hovered()
            ? ui_resolve_color(state, state->ui.roles.bar_bg)
            : (Clay_Color){0}
    }) {
        SDL_Texture *icon = tl.current == t
            ? icon_get("tab-close-active", state, 8, 8)
            : icon_get("tab-close-inactive", state, 8, 8);
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = 8.0 * state->ui.scale_factor,
                    .height = 8.0 * state->ui.scale_factor
                }
            },
            .image = {
                .imageData = icon
            },
        }){}
        Clay_OnHover(HandleCloseTab, t);
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

static void OpenTabs(AppState *state);

void TabBar(AppState *state) {
    CLAY(CLAY_ID("Tab Bar"), {
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(5 * state->ui.scale_factor),
            .childGap = 5.0 * state->ui.scale_factor,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .clip = { .horizontal = true },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.tab_bar),
    }) {
        SDL_Texture *app_tex = icon_get("tab-icon", state, 24, 24);
        CLAY(CLAY_ID("App Icon"), {
            .layout = {
                .sizing = {
                    .width = 24.0 * state->ui.scale_factor,
                    .height = 24.0 * state->ui.scale_factor
                },
            },
            .image = app_tex,
        }) {}
        OpenTabs(state);
    }
}

void OpenTabs(AppState *state) {
    Tab *t = tl.list;
    // TODO: extract to a state variable toggle-able from Scheme.
    // if (!t || !t->next) return;  // Hide tab bar if only one tab.
    if (!t) return;                 // Show tab bar if only one tab.
    float cr = 5 * state->ui.scale_factor;
    Clay_Color active_color = ui_resolve_color(state, state->ui.roles.tab_active) ;

    for (int i = 1; t != NULL; t = t->next, i++) {
        Clay_String tab_name = {
            .chars = t->name,
            .length = strlen(t->name),
            .isStaticallyAllocated = true
        };
        CLAY(CLAY_IDI("Tab", i), {
            .layout = {
                .padding = {
                    .left = 2 * cr,
                    .right = cr,
                    .top = 0, .bottom = 0
                },
                .childAlignment = {
                    .y = CLAY_ALIGN_Y_CENTER
                },
            },
            .cornerRadius = CLAY_CORNER_RADIUS(cr),
            .backgroundColor = tl.current == t
                ? active_color
                : Clay_Hovered()
                    ? ui_resolve_color(state, state->ui.roles.tab_hover)
                    : ui_resolve_color(state, state->ui.roles.tab_inactive),
        }) {
            // floating filler to join tab to content area
            if (tl.current == t) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_PERCENT(1),
                            .height = CLAY_SIZING_FIXED(2 * cr)
                        },
                    },
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .attachPoints = {
                             .parent = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                             .element = CLAY_ATTACH_POINT_CENTER_TOP
                        },
                        .offset = {
                            .y = -cr
                        }
                    },
                    .backgroundColor = active_color,
                }) {
                    Clay_Color tab_color = active_color;
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {
                                .width = CLAY_SIZING_FIXED(2 * cr),
                                .height = CLAY_SIZING_FIXED(2 * cr)
                            },
                        },
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .attachPoints = {
                                .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                                .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM
                            },
                        },
                        .backgroundColor = tab_color,
                        .custom = { .customData = (void *)(uintptr_t)CUSTOM_RENDER_CONCAVE_LEFT },
                    }) {}
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {
                                .width = CLAY_SIZING_FIXED(2 * cr),
                                .height = CLAY_SIZING_FIXED(2 * cr)
                            },
                        },
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .attachPoints = {
                                .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                                .element = CLAY_ATTACH_POINT_LEFT_BOTTOM
                            },
                        },
                        .backgroundColor = tab_color,
                        .custom = { .customData = (void *)(uintptr_t)CUSTOM_RENDER_CONCAVE_RIGHT },
                    }) {}
                }
            }
            Clay_Color c = tl.current == t
                ? ui_resolve_color(state, state->ui.roles.text_primary)
                : ui_resolve_color(state, state->ui.roles.text_faded);
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(136 * state->ui.scale_factor),
                        .height = CLAY_SIZING_FIXED(25 * state->ui.scale_factor)
                    },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                }
            }) {
                CLAY_TEXT(tab_name, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14 * state->ui.scale_factor,
                    .textColor = c
                }));
            }
            bool block_click = CloseButton(state, t);
            Clay_OnHover(block_click ? NULL : HandleClickTab, t);
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

    PaneContent(state, t->contents, 1, 0, 0);
}

bool tab_new_with_buffer(const char *buf_name) {
    Buffer *buf = buffer_get_by_name(buf_name);
    if (!buf) {
        buf = buffer_create(buf_name);
        if (!buf) return false;
    }

    if (!tab_create(buf_name)) return false;

    // tab_create appends to the end; find and switch to it.
    Tab *t = tl.list;
    while (t->next) t = t->next;
    tl.current = t;

    t->contents = pane_create();
    if (!t->contents) return false;
    pane_set_buffer(t->contents, buf);
    t->contents->content.active = true;

    sync_active_buffer();
    update_window_title();
    G->needs_redraw = true;
    return true;
}

// --- Scheme bindings ---

sexp scm_tab_next(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (tab_next()) message_send("tab-next");
    return SEXP_VOID;
}

sexp scm_tab_prev(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (tab_prev()) message_send("tab-prev");
    return SEXP_VOID;
}

sexp scm_tab_new(sexp ctx, sexp self, sexp n, sexp sbuf_name) {
    if (!sexp_stringp(sbuf_name))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sbuf_name);
    bool ok = tab_new_with_buffer(sexp_string_data(sbuf_name));
    return ok ? SEXP_TRUE : SEXP_FALSE;
}
