#include <stdlib.h>
#include <string.h>

#include "icon.h"
#include "pane.h"
#include "tab.h"
#include "tooltip.h"
#include "../text/buffer_type.h"
#include "theme.h"
#include "../command/keymap.h"
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
    Tab *t = active ? active->content.active_tab : NULL;
    if (t && t->content.buffer.buffer)
        snprintf(buf, sizeof(buf), "%s - sev", t->content.buffer.buffer->name);
    else
        snprintf(buf, sizeof(buf), "sev");
    SDL_SetWindowTitle(window, buf);
}

Tab *tab_alloc(Buffer *buf) {
    Tab *tab = calloc(1, sizeof(Tab));
    if (!tab) return NULL;
    tab->content.type = TAB_BUFFER;
    tab->content.buffer.buffer = buf;
    tab->content.buffer.vline_cache = vline_cache_create();
    return tab;
}

Tab *pane_tab_for_buffer(Pane *pane, Buffer *buf) {
    if (!pane || pane->type != PANE_CONTENT) return NULL;
    for (Tab *t = pane->content.list; t; t = t->next)
        if (t->content.buffer.buffer == buf) return t;
    return NULL;
}

void tab_set_buffer(Tab *tab, Buffer *buf) {
    if (!tab || !buf) return;
    tab->content.buffer.buffer = buf;
    tab->content.buffer.vline_cache.full_rebuild = true;
    tab->content.buffer.vline_cache.scroll_offset = 0.0f;
    tab->content.buffer.vline_cache.target_scroll = 0.0f;
    tab->content.buffer.vline_cache.last_scroll_point = (size_t)-1;
}

void tab_free(Tab *tab) {
    if (!tab) return;
    vline_cache_destroy(&tab->content.buffer.vline_cache);
    jump_list_free(&tab->content.buffer.jump_list);
    free(tab);
}

Tab *display_tab_new(Pane *dp, Buffer *buf) {
    if (!dp || dp->type != PANE_CONTENT) return NULL;
    Tab *tab = tab_alloc(buf);
    if (!tab) return NULL;

    // Append to end of list.
    if (!dp->content.list) {
        dp->content.list = tab;
    } else {
        Tab *last = dp->content.list;
        while (last->next) last = last->next;
        last->next = tab;
        tab->prev = last;
    }
    dp->content.active_tab = tab;
    return tab;
}

bool display_tab_close(Pane *dp) {
    if (!dp || dp->type != PANE_CONTENT || !dp->content.active_tab) return true;
    Tab *t = dp->content.active_tab;

    // Choose next active tab.
    Tab *next_active = t->next ? t->next : t->prev;

    // Unlink from list.
    if (t->prev) t->prev->next = t->next;
    else dp->content.list = t->next;
    if (t->next) t->next->prev = t->prev;

    tab_free(t);
    dp->content.active_tab = next_active;
    return dp->content.list == NULL;
}

bool display_tab_next(Pane *dp) {
    if (!dp || dp->type != PANE_CONTENT || !dp->content.active_tab) return false;
    Tab *cur = dp->content.active_tab;
    if (!cur->next && !cur->prev) return false;
    dp->content.active_tab = cur->next ? cur->next : dp->content.list;
    sync_active_buffer();
    return true;
}

bool display_tab_prev(Pane *dp) {
    if (!dp || dp->type != PANE_CONTENT || !dp->content.active_tab) return false;
    Tab *cur = dp->content.active_tab;
    if (!cur->next && !cur->prev) return false;
    if (cur->prev) {
        dp->content.active_tab = cur->prev;
    } else {
        Tab *last = dp->content.list;
        while (last->next) last = last->next;
        dp->content.active_tab = last;
    }
    sync_active_buffer();
    return true;
}

bool tab_new_with_buffer(const char *buf_name, bool always_create) {
    Buffer *buf = NULL;
    if (!always_create)
        buf = buffer_get_by_name(buf_name);
    if (!buf) {
        buf = buffer_create(buf_name);
        if (!buf) return false;
    }

    Pane *root = pane_get_root();
    if (!root) {
        // No panes yet: create the root PANE_CONTENT.
        Pane *p = pane_content_create(buf);
        if (!p) return false;
        // pane_content_create doesn't set root_pane; we do it via replace_child(NULL,NULL,p).
        pane_replace_child(NULL, NULL, p);
        p->content.active = true;
    } else {
        // If a tab in the active pane already shows this buffer, switch to it.
        Pane *active = pane_get_active();
        if (!active) return false;
        Tab *existing = pane_tab_for_buffer(active, buf);
        if (existing) {
            active->content.active_tab = existing;
            sync_active_buffer();
            G->needs_redraw = true;
            return true;
        }
        Tab *t = display_tab_new(active, buf);
        if (!t) return false;
    }

    sync_active_buffer();
    G->needs_redraw = true;
    return true;
}

// --- Tab close helpers ---

static void usurp_parent(Pane *pane, Pane *parent) {
    Pane *grandparent = parent->parent;
    pane_replace_child(grandparent, parent, pane);
    pane->parent = grandparent;
    free(parent);
}

void tab_close(void) {
    Pane *pane = pane_get_active();
    if (!pane) return;

    // If this content pane has more than one tab, just close the active tab.
    if (pane->content.list && pane->content.list->next) {
        bool empty = display_tab_close(pane);
        (void)empty;  // cannot be true since we checked list->next above
        sync_active_buffer();
        return;
    }

    // Single-tab pane: close the entire pane.
    Pane *parent = pane->parent;
    if (!parent) {
        // Root pane: destroy it, set root to NULL → welcome.
        pane_destroy(pane);
        pane_set_root(NULL);
        G->input.current_focus = FOCUS_WELCOME;
        buffer_set_current(NULL);
        update_window_title();
        return;
    }

    Pane *sibling = pane_get_sibling(pane);
    pane->content.active = false;
    descend_pane_tree(sibling);
    usurp_parent(sibling, parent);
    // Destroy this pane's single tab then the pane node.
    tab_free(pane->content.list);
    free(pane);
}

sexp scm_tab_close(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true;
    tab_close();
    message_send("tab-close");
    return SEXP_VOID;
}

// --- TabBar Clay rendering ---

// Close a specific tab in a specific pane (handles last-tab pane collapse).
static void close_tab(Pane *dp, Tab *t) {
    // Switch active if closing the current tab.
    if (dp->content.active_tab == t) {
        Tab *next = t->next ? t->next : t->prev;
        dp->content.active_tab = next;
    }
    // Unlink.
    if (t->prev) t->prev->next = t->next;
    else dp->content.list = t->next;
    if (t->next) t->next->prev = t->prev;
    tab_free(t);

    if (!dp->content.list) {
        // No tabs left: close the pane entirely.
        // Temporarily mark it active so tab_close() finds it.
        // First clear the real active pane's flag to avoid pane_get_active()
        // finding the wrong pane (e.g. the left sibling) via depth-first search.
        bool was_active = dp->content.active;
        if (!was_active) {
            Pane *current = pane_get_active();
            if (current) current->content.active = false;
        }
        dp->content.active = true;
        tab_close();
    } else {
        sync_active_buffer();
    }
}

static void HandleCloseTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if (pointerInfo.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *dp = ((void **)userData)[0];
    Tab  *t  = ((void **)userData)[1];
    if (!dp || !t) return;
    close_tab(dp, t);
}

static void HandleClickTab(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    if (pointerInfo.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *dp = ((void **)userData)[0];
    Tab  *t  = ((void **)userData)[1];
    if (!dp || !t) return;
    dp->content.active_tab = t;
    if (!dp->content.active) pane_set_active(dp);
    else sync_active_buffer();
}

// Static storage for close-button callback data.
// We need (Pane*, Tab*) pairs — one per tab rendered per frame.
#define MAX_TAB_CB_DATA 64
static void *tab_cb_data[MAX_TAB_CB_DATA][2];
static int   tab_cb_count = 0;

static Pane *pending_close_pane = NULL;
static Tab  *pending_close_tab  = NULL;
static Pane *pending_new_tab_pane = NULL;

void tab_cb_reset(void) {
    tab_cb_count = 0;
}

void tab_flush_pending_close(void) {
    if (pending_close_pane && pending_close_tab) {
        close_tab(pending_close_pane, pending_close_tab);
        pending_close_pane = NULL;
        pending_close_tab  = NULL;
    }
    if (pending_new_tab_pane) {
        if (!pending_new_tab_pane->content.active) pane_set_active(pending_new_tab_pane);
        sexp ctx = G->chibi.ctx;
        sexp sym = sexp_intern(ctx, "tab-new", -1);
        sexp result = sexp_apply(ctx, G->chibi.call_interactively, sexp_list1(ctx, sym));
        if (sexp_exceptionp(result))
            sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
        pending_new_tab_pane = NULL;
    }
    G->input.middle_pressed_this_frame = false;
    G->input.left_double_click_this_frame = false;
}

// String pool for buffer text and line-number allocations made during rendering.
#define TAB_STRINGS_MAX 32
static char *tab_strings[TAB_STRINGS_MAX];
static int   tab_strings_count = 0;

void tab_free_strings(void) {
    for (int i = 0; i < tab_strings_count; i++)
        free(tab_strings[i]);
    tab_strings_count = 0;
}

void tab_register_string(char *s) {
    if (s && tab_strings_count < TAB_STRINGS_MAX)
        tab_strings[tab_strings_count++] = s;
}

static void **tab_cb_alloc(Pane *dp, Tab *t) {
    if (tab_cb_count >= MAX_TAB_CB_DATA) return NULL;
    void **slot = tab_cb_data[tab_cb_count++];
    slot[0] = dp;
    slot[1] = t;
    return slot;
}

static bool CloseButton(AppState *state, Pane *dp, Tab *t,
                        bool is_active, int pane_index, int tab_index) {
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
            ? ui_resolve_color(state, state->ui.roles.tab_close)
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
    char binding[64] = {0};
    if (is_active)
        keymap_where_is_first(state, "tab-close", binding, sizeof(binding));
    TextTooltipWithBinding(state, hovered, pane_index * 512 + tab_index + 256,
                           "Close Tab", is_active ? binding : NULL);
    return hovered;
}

void TabBar(AppState *state, Pane *dp, int32_t index) {
    if (!dp || dp->type != PANE_CONTENT) return;

    Clay_Color active_color = ui_resolve_color(state, state->ui.roles.tab_active);

    CLAY(CLAY_IDI_LOCAL("Tab Bar", index), {
        .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } },
        .clip = { .horizontal = true },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
        .border = {
            .width = { .betweenChildren = 1 },
            .color = ui_resolve_color(state, state->ui.roles.border_inactive)
        }
    }) {
        Tab *t = dp->content.list;
        if (!t) return;

        for (int i = 1; t != NULL; t = t->next, i++) {
            bool is_active = (t == dp->content.active_tab);
            Clay_String tab_name = {
                .chars = t->content.buffer.buffer->name,
                .length = strlen(t->content.buffer.buffer->name),
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
                    if (t->content.buffer.buffer->is_modified) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = { .height = CLAY_SIZING_FIXED(2 * radius), .width = CLAY_SIZING_FIXED(2 * radius) },
                            },
                            .cornerRadius = CLAY_CORNER_RADIUS(radius),
                            .backgroundColor = ui_resolve_color(state, state->ui.roles.border_active)
                        }) {}
                    }
                }
                CLAY(CLAY_IDI_LOCAL("Tab Name", i), {
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
                    const char *tip = t->content.buffer.buffer->file_name;
                    if (!tip[0]) tip = t->content.buffer.buffer->name;
                    TextTooltip(state, Clay_Hovered(), index * 256 + i, tip);
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
                        block_click = CloseButton(state, dp, t, is_active, index, i);
                    }
                }
                if (show_cross && G->input.middle_pressed_this_frame) {
                    pending_close_pane = dp;
                    pending_close_tab  = t;
                } else if (cb) {
                    Clay_OnHover(block_click ? NULL : HandleClickTab, cb);
                }
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
        }) {
            if (Clay_Hovered() && G->input.left_double_click_this_frame)
                pending_new_tab_pane = dp;
        }
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
    bool ok = tab_new_with_buffer(sexp_string_data(sbuf_name), false);
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

sexp scm_tab_new_fresh(sexp ctx, sexp self, sexp n, sexp sbuf_name) {
    if (!sexp_stringp(sbuf_name))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sbuf_name);
    bool ok = tab_new_with_buffer(sexp_string_data(sbuf_name), true);
    return ok ? SEXP_TRUE : SEXP_FALSE;
}

sexp scm_no_panes_p(sexp ctx, sexp self, sexp n) {
    return pane_get_root() == NULL ? SEXP_TRUE : SEXP_FALSE;
}
