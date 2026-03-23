#include <stdlib.h>
#include <string.h>

#include "icon.h"
#include "pane.h"
#include "tab.h"
#include "../text/buffer_type.h"
#include "theme.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"

static SDL_Window *window;

bool tab_system_init(AppState *state) {
    window = state->window;
    return true;
}

void update_window_title(void) {
    char buf[BUFFER_NAME_MAX + 8];
    Pane *active = pane_get_active();
    Tab *t = active ? active->display.active_tab : NULL;
    if (t && t->buffer)
        snprintf(buf, sizeof(buf), "%s - sev", t->buffer->name);
    else
        snprintf(buf, sizeof(buf), "sev");
    SDL_SetWindowTitle(window, buf);
}

Tab *tab_alloc(Buffer *buf) {
    Tab *tab = calloc(1, sizeof(Tab));
    if (!tab) return NULL;
    tab->buffer = buf;
    tab->vline_cache = vline_cache_create();
    return tab;
}

void tab_set_buffer(Tab *tab, Buffer *buf) {
    if (!tab || !buf) return;
    tab->buffer = buf;
    tab->vline_cache.full_rebuild = true;
}

void tab_free(Tab *tab) {
    if (!tab) return;
    vline_cache_destroy(&tab->vline_cache);
    jump_list_free(&tab->jump_list);
    free(tab);
}

Tab *display_tab_new(Pane *dp, Buffer *buf) {
    if (!dp || dp->type != PANE_DISPLAY) return NULL;
    Tab *tab = tab_alloc(buf);
    if (!tab) return NULL;

    // Append to end of list.
    if (!dp->display.list) {
        dp->display.list = tab;
    } else {
        Tab *last = dp->display.list;
        while (last->next) last = last->next;
        last->next = tab;
        tab->prev = last;
    }
    dp->display.active_tab = tab;
    return tab;
}

bool display_tab_close(Pane *dp) {
    if (!dp || dp->type != PANE_DISPLAY || !dp->display.active_tab) return true;
    Tab *t = dp->display.active_tab;

    // Choose next active tab.
    Tab *next_active = t->next ? t->next : t->prev;

    // Unlink from list.
    if (t->prev) t->prev->next = t->next;
    else dp->display.list = t->next;
    if (t->next) t->next->prev = t->prev;

    tab_free(t);
    dp->display.active_tab = next_active;
    return dp->display.list == NULL;
}

bool display_tab_next(Pane *dp) {
    if (!dp || dp->type != PANE_DISPLAY || !dp->display.active_tab) return false;
    Tab *cur = dp->display.active_tab;
    if (!cur->next && !cur->prev) return false;
    dp->display.active_tab = cur->next ? cur->next : dp->display.list;
    sync_active_buffer();
    update_window_title();
    return true;
}

bool display_tab_prev(Pane *dp) {
    if (!dp || dp->type != PANE_DISPLAY || !dp->display.active_tab) return false;
    Tab *cur = dp->display.active_tab;
    if (!cur->next && !cur->prev) return false;
    if (cur->prev) {
        dp->display.active_tab = cur->prev;
    } else {
        Tab *last = dp->display.list;
        while (last->next) last = last->next;
        dp->display.active_tab = last;
    }
    sync_active_buffer();
    update_window_title();
    return true;
}

bool tab_new_with_buffer(const char *buf_name) {
    Buffer *buf = buffer_get_by_name(buf_name);
    if (!buf) {
        buf = buffer_create(buf_name);
        if (!buf) return false;
    }

    Pane *root = pane_get_root();
    if (!root) {
        // No panes yet: create the root PANE_DISPLAY.
        Pane *p = pane_display_create(buf);
        if (!p) return false;
        // pane_display_create doesn't set root_pane; we do it via replace_child(NULL,NULL,p).
        pane_replace_child(NULL, NULL, p);
        p->display.active = true;
    } else {
        // Add tab to active display pane.
        Pane *active = pane_get_active();
        if (!active) return false;
        Tab *t = display_tab_new(active, buf);
        if (!t) return false;
    }

    sync_active_buffer();
    update_window_title();
    G->needs_redraw = true;
    return true;
}

// --- TabBar Clay rendering ---

static void HandleCloseTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if (pointerInfo.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *dp = ((void **)userData)[0];
    Tab  *t  = ((void **)userData)[1];
    if (!dp || !t) return;

    // Switch active if closing the current tab.
    if (dp->display.active_tab == t) {
        Tab *next = t->next ? t->next : t->prev;
        dp->display.active_tab = next;
    }
    // Unlink.
    if (t->prev) t->prev->next = t->next;
    else dp->display.list = t->next;
    if (t->next) t->next->prev = t->prev;
    tab_free(t);

    if (!dp->display.list) {
        // No tabs left: close the pane entirely.
        // Temporarily mark it active so pane_close() finds it.
        // First clear the real active pane's flag to avoid pane_get_active()
        // finding the wrong pane (e.g. the left sibling) via depth-first search.
        bool was_active = dp->display.active;
        if (!was_active) {
            Pane *current = pane_get_active();
            if (current) current->display.active = false;
        }
        dp->display.active = true;
        pane_close();
    } else {
        sync_active_buffer();
        update_window_title();
    }
}

static void HandleClickTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if (pointerInfo.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *dp = ((void **)userData)[0];
    Tab  *t  = ((void **)userData)[1];
    if (!dp || !t) return;
    dp->display.active_tab = t;
    if (!dp->display.active) pane_set_active(dp);
    else sync_active_buffer();
    update_window_title();
}

// Static storage for close-button callback data.
// We need (Pane*, Tab*) pairs — one per tab rendered per frame.
#define MAX_TAB_CB_DATA 64
static void *tab_cb_data[MAX_TAB_CB_DATA][2];
static int   tab_cb_count = 0;

void tab_cb_reset(void) { tab_cb_count = 0; }

static void **tab_cb_alloc(Pane *dp, Tab *t) {
    if (tab_cb_count >= MAX_TAB_CB_DATA) return NULL;
    void **slot = tab_cb_data[tab_cb_count++];
    slot[0] = dp;
    slot[1] = t;
    return slot;
}

static bool CloseButton(AppState *state, Pane *dp, Tab *t) {
    void **cb = tab_cb_alloc(dp, t);
    bool hovered = false;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_FIXED(15 * state->ui.scale_factor),
                .height = CLAY_SIZING_FIXED(15 * state->ui.scale_factor),
            },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .cornerRadius = CLAY_CORNER_RADIUS(8 * state->ui.scale_factor),
        .backgroundColor = Clay_Hovered()
            ? ui_resolve_color(state, state->ui.roles.bar_bg)
            : (Clay_Color){0}
    }) {
        SDL_Texture *icon = icon_get("tab-close",   state, 7, 7);
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width  = 7.0 * state->ui.scale_factor,
                    .height = 7.0 * state->ui.scale_factor
                }
            },
            .image = { .imageData = icon },
        }) {}
        if (cb) Clay_OnHover(HandleCloseTab, cb);
        hovered = Clay_Hovered();
    }
    return hovered;
}

void TabBar(AppState *state, Pane *dp, int32_t index) {
    if (!dp || dp->type != PANE_DISPLAY) return;

    Clay_Color active_color = ui_resolve_color(state, state->ui.roles.tab_active);

    CLAY(CLAY_IDI_LOCAL("Tab Bar", index), {
        .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } },
        .clip = { .horizontal = true },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.tab_bar),
        .border = {
            .width = { .betweenChildren = 1 },
            .color = ui_resolve_color(state, state->ui.roles.border_inactive)
        }
    }) {
        Tab *t = dp->display.list;
        if (!t) return;

        for (int i = 1; t != NULL; t = t->next, i++) {
            bool is_active = (t == dp->display.active_tab);
            Clay_String tab_name = {
                .chars = t->buffer->name,
                .length = strlen(t->buffer->name),
                .isStaticallyAllocated = true
            };
            CLAY(CLAY_IDI_LOCAL("Tab", i), {
                .layout = {
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = is_active
                    ? active_color
                    : ui_resolve_color(state, state->ui.roles.tab_inactive),
                .border = {
                    .width = { .bottom = is_active ? 0 : 2 },
                    .color = ui_resolve_color(state, state->ui.roles.border_inactive)
            }
            }) {
                Clay_Color c = ui_resolve_color(state, state->ui.roles.text_primary);
                float TAB_HEIGHT = 26 * state->ui.scale_factor;
                float TAB_PADDING_X = 20 * state->ui.scale_factor;
                CLAY(CLAY_IDI_LOCAL("Modified Indicator", i), {
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_FIXED(TAB_PADDING_X) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
                    },
                }) {
                    float radius = 3 * state->ui.scale_factor;
                    if (t->buffer->is_modified) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = { .height = CLAY_SIZING_FIXED(2 * radius), .width = CLAY_SIZING_FIXED(2 * radius) },
                            },
                            .cornerRadius = CLAY_CORNER_RADIUS(radius),
                            .backgroundColor = ui_resolve_color(state, state->ui.roles.border_active)
                        }) {}
                    }
                }
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {
                            .width  = CLAY_SIZING_FIT(0),
                            .height = CLAY_SIZING_FIXED(TAB_HEIGHT)
                        },
                        .padding = { .bottom = 4 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                    }
                }) {
                    CLAY_TEXT(tab_name, CLAY_TEXT_CONFIG({
                        .fontId   = FONT_UI_NORMAL,
                        .fontSize = 12 * state->ui.scale_factor,
                        .textColor = c
                    }));
                }
                void **cb = tab_cb_alloc(dp, t);
                bool show_cross = Clay_Hovered();
                bool block_click = false;
                CLAY(CLAY_IDI_LOCAL("Close Button", i), {
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_FIXED(TAB_PADDING_X) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
                    },
                }) {
                    if (show_cross) {
                        block_click = CloseButton(state, dp, t);
                    }
                }
                if (cb) Clay_OnHover(block_click ? NULL : HandleClickTab, cb);
            }
        }
        CLAY(CLAY_ID_LOCAL("Tab bar space"), {
            .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0)
                }
            },
            .border = {
                .width = { .bottom = 2 },
                .color = ui_resolve_color(state, state->ui.roles.border_inactive)
            }
        }) {}
    }
}

// --- Scheme bindings ---

sexp scm_tab_next(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    Pane *active = pane_get_active();
    if (active && display_tab_next(active)) message_send("tab-next");
    return SEXP_VOID;
}

sexp scm_tab_prev(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    Pane *active = pane_get_active();
    if (active && display_tab_prev(active)) message_send("tab-prev");
    return SEXP_VOID;
}

sexp scm_tab_new(sexp ctx, sexp self, sexp n, sexp sbuf_name) {
    if (!sexp_stringp(sbuf_name))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sbuf_name);
    bool ok = tab_new_with_buffer(sexp_string_data(sbuf_name));
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

sexp scm_no_panes_p(sexp ctx, sexp self, sexp n) {
    return pane_get_root() == NULL ? SEXP_TRUE : SEXP_FALSE;
}
