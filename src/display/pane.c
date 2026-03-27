#include <stdlib.h>
#include <string.h>

#include "buf_render.h"
#include "pane.h"
#include "tab.h"
#include "theme.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"
#include "../text/buffer_type.h"

static Pane *root_pane = NULL;

bool pane_init(AppState *state) {
    root_pane = NULL;
    return tab_system_init(state);
}

void pane_quit(void) {
    pane_destroy(root_pane);
    root_pane = NULL;
}

Pane *pane_get_root(void) {
    return root_pane;
}

// --- Tree helpers ---

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
        root_pane = new_child;
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

Pane *pane_at_coords(Pane *root, float x, float y) {
    if (!root) return NULL;
    if (root->type == PANE_V_SPLIT) {
        Pane *found = pane_at_coords(root->v_split.left, x, y);
        if (found) return found;
        return pane_at_coords(root->v_split.right, x, y);
    }
    if (root->type == PANE_H_SPLIT) {
        Pane *found = pane_at_coords(root->h_split.top, x, y);
        if (found) return found;
        return pane_at_coords(root->h_split.bottom, x, y);
    }
    // PANE_CONTENT — use stored geometry (available after first rendered frame).
    ContentPane *d = &root->content;
    if (d->line_height_px <= 0) return NULL;
    float left   = d->text_origin_x - d->gutter_width_px;
    float right  = (d->pane_right > 0) ? d->pane_right : d->text_origin_x + d->text_origin_w;
    float top    = d->text_origin_y;
    float bottom = d->text_origin_y + d->text_origin_h;
    if (x >= left && x <= right && y >= top && y <= bottom)
        return root;
    return NULL;
}

void sync_active_buffer(void) {
    Pane *active = pane_get_active();
    if (active && active->content.active_tab)
        buffer_set_current(active->content.active_tab->content.buffer.buffer);
    if (!G->minibuf.active)
        G->input.current_focus = active ? FOCUS_PANE : FOCUS_WELCOME;
}

Pane *pane_content_create(Buffer *buf) {
    Pane *pane = calloc(1, sizeof(Pane));
    if (!pane) return NULL;
    pane->type = PANE_CONTENT;

    Tab *tab = tab_alloc(buf);
    if (!tab) { free(pane); return NULL; }

    pane->content.list = tab;
    pane->content.active_tab = tab;
    pane->content.active = false;
    return pane;
}

// Recursively free resources for a pane sub-tree.
void pane_destroy(Pane *pane) {
    if (!pane) return;
    if (pane->type == PANE_H_SPLIT) {
        pane_destroy(pane->h_split.top);
        pane_destroy(pane->h_split.bottom);
    } else if (pane->type == PANE_V_SPLIT) {
        pane_destroy(pane->v_split.left);
        pane_destroy(pane->v_split.right);
    } else if (pane->type == PANE_CONTENT) {
        Tab *t = pane->content.list;
        while (t) {
            Tab *next = t->next;
            tab_free(t);
            t = next;
        }
    }
    free(pane);
}

// Descend into pane tree, activating the first PANE_CONTENT leaf found.
static void descend_pane_tree(Pane *pane) {
    while (pane->type != PANE_CONTENT)
        pane = pane_first_child(pane);
    pane->content.active = true;
    sync_active_buffer();
}

static void usurp_parent(Pane *pane, Pane *parent) {
    Pane *grandparent = parent->parent;
    pane_replace_child(grandparent, parent, pane);
    pane->parent = grandparent;
    free(parent);
}

void pane_close(void) {
    Pane *pane = pane_get_active();
    if (!pane) return;

    // If this content pane has more than one tab, just close the active tab.
    if (pane->content.list && pane->content.list->next) {
        bool empty = display_tab_close(pane);
        (void)empty;  // cannot be true since we checked list->next above
        sync_active_buffer();
        update_window_title();
        return;
    }

    // Single-tab pane: close the entire pane.
    Pane *parent = pane->parent;
    if (!parent) {
        // Root pane: destroy it, set root to NULL → welcome.
        pane_destroy(pane);
        root_pane = NULL;
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
    update_window_title();
}


static Pane *pane_get_active_from(Pane *pane) {
    if (!pane) return NULL;
    if (pane->type == PANE_CONTENT)
        return pane->content.active ? pane : NULL;
    Pane *found;
    if (pane->type == PANE_V_SPLIT) {
        found = pane_get_active_from(pane->v_split.left);
        if (found) return found;
        found = pane_get_active_from(pane->v_split.right);
        if (found) return found;
    }
    if (pane->type == PANE_H_SPLIT) {
        found = pane_get_active_from(pane->h_split.top);
        if (found) return found;
        found = pane_get_active_from(pane->h_split.bottom);
        if (found) return found;
    }
    return NULL;
}

Pane *pane_get_active(void) {
    return pane_get_active_from(root_pane);
}

bool pane_set_active(Pane *pane) {
    if (!pane) return false;
    Pane *current = pane_get_active();
    if (current) current->content.active = false;
    pane->content.active = true;
    sync_active_buffer();
    return true;
}

Pane *pane_split(Pane *pane, PaneType split_type) {
    if (split_type != PANE_H_SPLIT && split_type != PANE_V_SPLIT) return NULL;
    if (pane->type != PANE_CONTENT) return NULL;

    Pane *split = malloc(sizeof(Pane));
    if (!split) return NULL;

    // New sibling gets one tab showing the same buffer as pane's active tab.
    Buffer *buf = pane->content.active_tab ? pane->content.active_tab->content.buffer.buffer : NULL;
    Pane *new_pane = pane_content_create(buf);
    if (!new_pane) { free(split); return NULL; }

    split->type = split_type;
    if (split_type == PANE_H_SPLIT) {
        split->h_split.top_height = 0.5;
        split->h_split.top = pane;
        split->h_split.bottom = new_pane;
    } else {
        split->v_split.left_width = 0.5;
        split->v_split.left = pane;
        split->v_split.right = new_pane;
    }

    pane_replace_child(pane->parent, pane, split);
    split->parent = pane->parent;
    pane->parent = split;
    new_pane->parent = split;

    return new_pane;
}

// Returns the split type relevant for the given direction.
static PaneType dir_split_type(Direction dir) {
    return (dir == DIR_UP || dir == DIR_DOWN) ? PANE_H_SPLIT : PANE_V_SPLIT;
}

// Returns true if navigating in dir means we need to be the second child.
static bool dir_from_second(Direction dir) {
    return (dir == DIR_UP || dir == DIR_LEFT);
}

bool pane_navigate(Direction dir) {
    Pane *pane = pane_get_active();
    if (!pane) return false;
    Pane *parent = pane->parent;
    PaneType target_split = dir_split_type(dir);
    bool from_second = dir_from_second(dir);

    while (parent) {
        if (parent->type == target_split) {
            bool is_second = !pane_is_first_child(pane);
            if (is_second == from_second) {
                Pane *target = from_second ? pane_first_child(parent) : pane_second_child(parent);
                pane_get_active()->content.active = false;
                descend_pane_tree(target);
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
    while (pane && pane->type != PANE_V_SPLIT) pane = pane->parent;
    if (pane) { pane->v_split.left_width = MIN(0.8, pane->v_split.left_width + 0.05); return true; }
    return false;
}

bool pane_v_split_decrease(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_V_SPLIT) pane = pane->parent;
    if (pane) { pane->v_split.left_width = MAX(0.2, pane->v_split.left_width - 0.05); return true; }
    return false;
}

bool pane_h_split_increase(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_H_SPLIT) pane = pane->parent;
    if (pane) { pane->h_split.top_height = MIN(0.8, pane->h_split.top_height + 0.05); return true; }
    return false;
}

bool pane_h_split_decrease(void) {
    Pane *pane = pane_get_active();
    while (pane && pane->type != PANE_H_SPLIT) pane = pane->parent;
    if (pane) { pane->h_split.top_height = MAX(0.2, pane->h_split.top_height - 0.05); return true; }
    return false;
}

bool pane_has_neighbour(Pane *pane, Direction dir) {
    Pane *parent = pane->parent;
    PaneType target_split = dir_split_type(dir);
    bool from_second = dir_from_second(dir);

    while (parent) {
        if (parent->type == target_split) {
            bool is_second = !pane_is_first_child(pane);
            if (is_second == from_second) return true;
        }
        pane = parent;
        parent = pane->parent;
    }
    return false;
}

// --- Buffer close: walk the pane tree removing any tab showing buf ---

// Returns true if the subtree is entirely dead and should be removed.
static bool pane_tree_close_buffer(Pane *pane, Buffer *buf) {
    if (!pane) return false;

    if (pane->type == PANE_CONTENT) {
        // Remove all tabs in this content pane that show buf.
        Tab *t = pane->content.list;
        while (t) {
            Tab *next = t->next;
            if (t->content.buffer.buffer == buf) {
                if (t->prev) t->prev->next = t->next;
                else pane->content.list = t->next;
                if (t->next) t->next->prev = t->prev;
                if (pane->content.active_tab == t)
                    pane->content.active_tab = t->next ? t->next : t->prev;
                tab_free(t);
            }
            t = next;
        }
        return pane->content.list == NULL;
    }

    Pane *first  = (pane->type == PANE_H_SPLIT) ? pane->h_split.top    : pane->v_split.left;
    Pane *second = (pane->type == PANE_H_SPLIT) ? pane->h_split.bottom : pane->v_split.right;

    bool kill_first  = pane_tree_close_buffer(first,  buf);
    bool kill_second = pane_tree_close_buffer(second, buf);

    if (kill_first && kill_second) {
        pane_destroy(first);
        pane_destroy(second);
        if (pane->type == PANE_H_SPLIT) pane->h_split.top    = pane->h_split.bottom = NULL;
        else                            pane->v_split.left   = pane->v_split.right   = NULL;
        return true;
    }

    if (kill_first || kill_second) {
        Pane *dead     = kill_first  ? first  : second;
        Pane *survivor = kill_first  ? second : first;

        if (pane->parent) pane_replace_child(pane->parent, pane, survivor);
        else              root_pane = survivor;
        survivor->parent = pane->parent;

        pane_destroy(dead);
        if (pane->type == PANE_H_SPLIT) pane->h_split.top  = pane->h_split.bottom = NULL;
        else                            pane->v_split.left = pane->v_split.right   = NULL;
        free(pane);
    }

    return false;
}

// Ensure at least one PANE_CONTENT is active (activate first found if none is).
static bool ensure_active_display_from(Pane *pane) {
    if (!pane) return false;
    if (pane->type == PANE_CONTENT) {
        if (!pane->content.active) pane->content.active = true;
        return true;
    }
    Pane *first  = pane_first_child(pane);
    Pane *second = pane_second_child(pane);
    if (ensure_active_display_from(first))  return true;
    if (ensure_active_display_from(second)) return true;
    return false;
}

static bool any_active_from(Pane *pane) {
    if (!pane) return false;
    if (pane->type == PANE_CONTENT) return pane->content.active;
    return any_active_from(pane_first_child(pane)) ||
           any_active_from(pane_second_child(pane));
}

static void ensure_active_pane(void) {
    if (!root_pane) return;
    if (!any_active_from(root_pane))
        ensure_active_display_from(root_pane);
}

sexp scm_buffer_close(sexp ctx, sexp self, sexp n, sexp sname) {
    if (!sexp_stringp(sname))
        return sexp_user_exception(ctx, self, "buffer name must be a string", sname);
    Buffer *buf = buffer_get_by_name(sexp_string_data(sname));
    if (!buf) return SEXP_FALSE;

    bool remove_root = pane_tree_close_buffer(root_pane, buf);
    if (remove_root) {
        pane_destroy(root_pane);
        root_pane = NULL;
    } else {
        ensure_active_pane();
    }

    sync_active_buffer();
    buffer_delete(buf);
    G->needs_redraw = true;
    return SEXP_TRUE;
}

// --- Rendering ---

void pane_free_strings(void) {
    // String cleanup moved to tab_free_strings() in tab.c.
}

static void LeafPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    CLAY(CLAY_IDI_LOCAL("ContentPane", index), {
        .layout = {
            .sizing = {
                .width  = width  ? CLAY_SIZING_PERCENT(width)  : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg)
    }) {
        TabBar(state, pane, index);
        BufferContentRender(state, &pane->content, pane->content.active_tab, index);
    }
}

static void VSplitPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    CLAY(CLAY_IDI_LOCAL("VSplitPane", index), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width  = width  ? CLAY_SIZING_PERCENT(width)  : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
        },
        .border = {
            .width = { .betweenChildren = 1 },
            .color = ui_resolve_color(state, state->ui.roles.border_inactive)
        }
    }) {
        PaneContent(state, pane->v_split.left,  1, pane->v_split.left_width,       0);
        PaneContent(state, pane->v_split.right, 2, 1 - pane->v_split.left_width,   0);
    }
}

static void HSplitPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    CLAY(CLAY_IDI_LOCAL("HSplitPane", index), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width  = width  ? CLAY_SIZING_PERCENT(width)  : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
        },
        .border = {
            .width = { .betweenChildren = 1 },
            .color = ui_resolve_color(state, state->ui.roles.border_inactive)
        }
    }) {
        PaneContent(state, pane->h_split.top,    1, 0, pane->h_split.top_height);
        PaneContent(state, pane->h_split.bottom, 2, 0, 1 - pane->h_split.top_height);
    }
}

void PaneContent(AppState *state, Pane *pane, int32_t index, float width, float height) {
    if (pane->type == PANE_V_SPLIT) VSplitPane(state, pane, index, width, height);
    if (pane->type == PANE_H_SPLIT) HSplitPane(state, pane, index, width, height);
    if (pane->type == PANE_CONTENT) LeafPane(state, pane, index, width, height);
}

// --- Scheme bindings ---

sexp scm_pane_navigate_up(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; message_clear();
    if (pane_navigate_up()) message_send("pane-navigate-up");
    return SEXP_VOID;
}

sexp scm_pane_navigate_down(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; message_clear();
    if (pane_navigate_down()) message_send("pane-navigate-down");
    return SEXP_VOID;
}

sexp scm_pane_navigate_left(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; message_clear();
    if (pane_navigate_left()) message_send("pane-navigate-left");
    return SEXP_VOID;
}

sexp scm_pane_navigate_right(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; message_clear();
    if (pane_navigate_right()) message_send("pane-navigate-right");
    return SEXP_VOID;
}

sexp scm_pane_v_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true; message_clear();
    if (pane_v_split_increase()) message_send("pane-v-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_v_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true; message_clear();
    if (pane_v_split_decrease()) message_send("pane-v-split-decrease");
    return SEXP_VOID;
}

sexp scm_pane_h_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true; message_clear();
    if (pane_h_split_increase()) message_send("pane-h-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_h_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true; message_clear();
    if (pane_h_split_decrease()) message_send("pane-h-split-decrease");
    return SEXP_VOID;
}

sexp scm_split_vertical(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true;
    Pane *pane = pane_get_active();
    if (pane) pane_split_vertical(pane);
    message_send("split-vertical");
    return SEXP_VOID;
}

sexp scm_split_horizontal(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true;
    Pane *pane = pane_get_active();
    if (pane) pane_split_horizontal(pane);
    message_send("split-horizontal");
    return SEXP_VOID;
}

sexp scm_pane_close(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true; G->needs_extra_frame = true;
    pane_close();
    message_send("pane-close");
    return SEXP_VOID;
}

sexp scm_jump_push(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT || !pane->content.active_tab) return SEXP_VOID;
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_VOID;
    Jump j = { .buf_name = strdup(buffer_get_name(buf)), .point = point_get(buf), .filename = NULL };
    jump_list_push(&pane->content.active_tab->content.buffer.jump_list, j);
    return SEXP_VOID;
}

void pane_push_jump(void) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT || !pane->content.active_tab) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    Jump j = { .buf_name = strdup(buffer_get_name(buf)), .point = point_get(buf), .filename = NULL };
    jump_list_push(&pane->content.active_tab->content.buffer.jump_list, j);
}

static void jump_apply(JumpList *jl) {
    Jump *j = &jl->list[jl->current_index];
    if (!j->buf_name) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    if (strcmp(j->buf_name, buffer_get_name(buf)) != 0) {
        Buffer *target = buffer_get_by_name(j->buf_name);
        if (!target) return;
        Pane *pane = pane_get_active();
        if (!pane || !pane->content.active_tab) return;
        tab_set_buffer(pane->content.active_tab, target);
        sync_active_buffer();
        buf = buffer_get_current();
    }
    point_set(j->point);
    update_line(buf);
    save_current_column(buf);
    G->needs_redraw = true;
}

sexp scm_jump_backward(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT || !pane->content.active_tab) return SEXP_VOID;
    JumpList *jl = &pane->content.active_tab->content.buffer.jump_list;
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_VOID;
    if (jump_list_at_front(jl)) {
        Jump j = { .buf_name = strdup(buffer_get_name(buf)), .point = point_get(buf), .filename = NULL };
        jump_list_push(jl, j);
        jump_list_backward(jl);
    }
    if (jump_list_backward(jl)) jump_apply(jl);
    return SEXP_VOID;
}

sexp scm_jump_forward(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT || !pane->content.active_tab) return SEXP_VOID;
    if (jump_list_forward(&pane->content.active_tab->content.buffer.jump_list))
        jump_apply(&pane->content.active_tab->content.buffer.jump_list);
    return SEXP_VOID;
}

sexp scm_set_mouse_click_handler(sexp ctx, sexp self, sexp n, sexp cb) {
    if (G->input.mouse_click_cb != SEXP_FALSE)
        sexp_release_object(ctx, G->input.mouse_click_cb);
    G->input.mouse_click_cb = cb;
    if (cb != SEXP_FALSE) sexp_preserve_object(ctx, cb);
    return SEXP_VOID;
}

sexp scm_set_mouse_drag_handler(sexp ctx, sexp self, sexp n, sexp cb) {
    if (G->input.mouse_drag_cb != SEXP_FALSE)
        sexp_release_object(ctx, G->input.mouse_drag_cb);
    G->input.mouse_drag_cb = cb;
    if (cb != SEXP_FALSE) sexp_preserve_object(ctx, cb);
    return SEXP_VOID;
}
