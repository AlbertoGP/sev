#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "buf_render.h"
#include "icon.h"
#include "pane.h"
#include "tab.h"
#include "text_surface.h"
#include "theme.h"
#include "tooltip.h"
#include "../command/keyevent.h"
#include "../command/keymap.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"
#include "../text/buffer.h"
#include "../text/buffer_type.h"

extern KeyEvent last_event;  // defined in command/keymap.c

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

void pane_set_root(Pane *p) {
    root_pane = p;
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

#define SPLIT_GRAB_ZONE 4.0f

// Geometry helpers: walk to edge leaves using stored content geometry.
// Return -1 if geometry is not yet available (first frame).

static float pane_leftmost_x(Pane *p) {
    if (!p) return -1.0f;
    if (p->type == PANE_CONTENT)
        return p->content.line_height_px > 0 ? p->content.pane_left : -1.0f;
    if (p->type == PANE_V_SPLIT) return pane_leftmost_x(p->v_split.left);
    return pane_leftmost_x(p->h_split.top);
}

static float pane_rightmost_x(Pane *p) {
    if (!p) return -1.0f;
    if (p->type == PANE_CONTENT)
        return p->content.line_height_px > 0 ? p->content.pane_right : -1.0f;
    if (p->type == PANE_V_SPLIT) return pane_rightmost_x(p->v_split.right);
    return pane_rightmost_x(p->h_split.top);
}

static float pane_topmost_y(Pane *p) {
    if (!p) return -1.0f;
    if (p->type == PANE_CONTENT)
        return p->content.line_height_px > 0 ? p->content.pane_top : -1.0f;
    if (p->type == PANE_H_SPLIT) return pane_topmost_y(p->h_split.top);
    return pane_topmost_y(p->v_split.left);
}

static float pane_bottommost_y(Pane *p) {
    if (!p) return -1.0f;
    if (p->type == PANE_CONTENT)
        return p->content.line_height_px > 0 ? p->content.pane_bottom : -1.0f;
    if (p->type == PANE_H_SPLIT) return pane_bottommost_y(p->h_split.bottom);
    return pane_bottommost_y(p->v_split.left);
}

Pane *pane_split_at_coords(Pane *root, float x, float y) {
    if (!root) return NULL;
    if (root->type == PANE_V_SPLIT) {
        float lx = pane_leftmost_x(root->v_split.left);
        float rx = pane_rightmost_x(root->v_split.right);
        if (lx >= 0.0f && rx > lx) {
            float div_x = lx + (rx - lx) * root->v_split.left_width;
            if (fabsf(x - div_x) <= SPLIT_GRAB_ZONE) return root;
        }
        Pane *found = pane_split_at_coords(root->v_split.left, x, y);
        return found ? found : pane_split_at_coords(root->v_split.right, x, y);
    }
    if (root->type == PANE_H_SPLIT) {
        float ty = pane_topmost_y(root->h_split.top);
        float by = pane_bottommost_y(root->h_split.bottom);
        if (ty >= 0.0f && by > ty) {
            float div_y = ty + (by - ty) * root->h_split.top_height;
            if (fabsf(y - div_y) <= SPLIT_GRAB_ZONE) return root;
        }
        Pane *found = pane_split_at_coords(root->h_split.top, x, y);
        return found ? found : pane_split_at_coords(root->h_split.bottom, x, y);
    }
    return NULL;
}

void pane_split_drag_update(Pane *split, float x, float y) {
    if (!split) return;
    if (split->type == PANE_V_SPLIT) {
        float lx = pane_leftmost_x(split->v_split.left);
        float rx = pane_rightmost_x(split->v_split.right);
        if (lx >= 0.0f && rx > lx) {
            float r = (x - lx) / (rx - lx);
            if (r < 0.2f) r = 0.2f;
            if (r > 0.8f) r = 0.8f;
            split->v_split.left_width = r;
        }
    } else if (split->type == PANE_H_SPLIT) {
        float ty = pane_topmost_y(split->h_split.top);
        float by = pane_bottommost_y(split->h_split.bottom);
        if (ty >= 0.0f && by > ty) {
            float r = (y - ty) / (by - ty);
            if (r < 0.2f) r = 0.2f;
            if (r > 0.8f) r = 0.8f;
            split->h_split.top_height = r;
        }
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
    // Extend bottom to include horizontal scrollbar strip when present.
    float bottom = (d->has_hscrollbar && d->pane_bottom > d->text_origin_y)
                   ? d->pane_bottom
                   : d->text_origin_y + d->text_origin_h;
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
    update_window_title();
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
        search_session_free(&pane->content.search);
    }
    free(pane);
}

// Descend into pane tree, activating the first PANE_CONTENT leaf found.
void descend_pane_tree(Pane *pane) {
    while (pane->type != PANE_CONTENT)
        pane = pane_first_child(pane);
    pane->content.active = true;
    sync_active_buffer();
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
    return SEXP_TRUE;
}

// --- Rendering ---

void pane_free_strings(void) {
    // String cleanup moved to tab_free_strings() in tab.c.
}

static void search_jump_to_active(Pane *pane);

static void HandleSearchPrev(Clay_ElementId id, Clay_PointerData ptr, void *ud) {
    if (ptr.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *pane = (Pane *)ud;
    if (!pane) return;
    SearchSession *s = &pane->content.search;
    if (!s->active || s->match_count == 0) return;
    search_session_prev_match(s);
    search_jump_to_active(pane);
}

static void HandleSearchNext(Clay_ElementId id, Clay_PointerData ptr, void *ud) {
    if (ptr.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *pane = (Pane *)ud;
    if (!pane) return;
    SearchSession *s = &pane->content.search;
    if (!s->active || s->match_count == 0) return;
    search_session_next_match(s);
    search_jump_to_active(pane);
}

static void HandleCloseSearch(Clay_ElementId id, Clay_PointerData ptr, void *ud) {
    if (ptr.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    Pane *pane = (Pane *)ud;
    if (!pane) return;
    SearchSession *s = &pane->content.search;
    s->bar_open    = false;
    s->active      = false;
    s->match_count = 0;
    s->query_len   = 0;
    s->query[0]    = '\0';
    G->input.current_focus = FOCUS_PANE;
}

static void SearchBar(AppState *state, Pane *pane, int32_t index) {
    SearchSession *s = &pane->content.search;
    if (!s->bar_open) return;

    float sf = state->ui.scale_factor;
    CLAY(CLAY_IDI_LOCAL("SearchBar", index), {
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .padding = {
                .left   = (uint16_t)(8 * sf),
                .right  = (uint16_t)(8 * sf),
                .top    = (uint16_t)(8 * sf),
                .bottom = (uint16_t)(8 * sf),
            },
            .childGap = (uint16_t)(6 * sf),
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg),
        .border = {
            .width = { .bottom = 2 },
            .color = ui_resolve_color(state, state->ui.roles.border_active),
        },
    }) {
        uint16_t font_sz = (uint16_t)(12 * sf);
        float cursor_x = s->query_len > 0
            ? text_measure_tab_aware(&state->rendererData, FONT_BUF_NORMAL, font_sz,
                                     s->query, s->query_len, 4)
            : 0.0f;

        Clay_String qstr = {
            .chars               = s->query,
            .length              = (int32_t)s->query_len,
            .isStaticallyAllocated = true,
        };
        CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_GROW(0) },
                        .padding = { .top = 4 * sf, .bottom = 4 * sf, .left = 8 * sf, .right = 8 * sf }
                    },
                    .border = {
                        .color = ui_resolve_color(state, state->ui.roles.border_inactive),
                        .width = CLAY_BORDER_OUTSIDE(2)
                    },
                    .cornerRadius = CLAY_CORNER_RADIUS(5*sf)
                }) {
            CLAY_TEXT(qstr.length ? qstr : CLAY_STRING("Search..."), CLAY_TEXT_CONFIG({
                .fontId    = FONT_BUF_NORMAL,
                .fontSize  = font_sz,
                .textColor = !qstr.length
                ? ui_resolve_color(state, state->ui.roles.text_faded)
                : (s->match_count == 0
                    ? ui_resolve_color(state, state->ui.roles.search_no_match)
                    : ui_resolve_color(state, state->ui.roles.text_primary)),
            }));
            if (state->input.current_focus == FOCUS_SEARCH && state->cursor_visible) {
                float cw = sf >= 2.0f ? 2.0f * sf : 1.0f;
                CLAY_AUTO_ID({
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .offset   = { .x = cursor_x + 8 * sf, .y = 4.0f * sf },
                        .zIndex   = 10,
                    },
                    .layout = {
                        .sizing = {
                            .width  = CLAY_SIZING_FIXED(cw),
                            .height = CLAY_SIZING_FIXED(font_sz),
                        }
                    },
                    .backgroundColor = ui_get_cursor_color(state),
                }) {}
            }
        }

        bool nav_disabled = !s->query_len || !s->match_count;

        bool search_prev_hovered = false;
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { .width = CLAY_SIZING_FIXED(15 * sf), .height = CLAY_SIZING_FIXED(15 * sf) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .cornerRadius    = CLAY_CORNER_RADIUS(8 * sf),
            .backgroundColor = (!nav_disabled && Clay_Hovered())
                ? ui_resolve_color(state, state->ui.roles.tab_close)
                : (Clay_Color){0}
        }) {
            SDL_Texture *prev_icon = icon_get(
                nav_disabled ? "caret-left-faded" : "caret-left", state, 10, 10);
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = 10.0f * sf, .height = 10.0f * sf } },
                .image  = { .imageData = prev_icon },
            }) {}
            if (!nav_disabled) Clay_OnHover(HandleSearchPrev, pane);
            search_prev_hovered = Clay_Hovered();
            if (search_prev_hovered)
                state->input.desired_cursor = nav_disabled
                    ? SDL_SYSTEM_CURSOR_NOT_ALLOWED
                    : SDL_SYSTEM_CURSOR_POINTER;
        }
        char prev_binding[64] = {0};
        keymap_where_is_first(state, "vim-search-prev", prev_binding, sizeof(prev_binding));
        TextTooltipWithBinding(state, search_prev_hovered, index + 1025,
                               "Select Previous Match", prev_binding[0] ? prev_binding : NULL);

        bool search_next_hovered = false;
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { .width = CLAY_SIZING_FIXED(15 * sf), .height = CLAY_SIZING_FIXED(15 * sf) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .cornerRadius    = CLAY_CORNER_RADIUS(8 * sf),
            .backgroundColor = (!nav_disabled && Clay_Hovered())
                ? ui_resolve_color(state, state->ui.roles.tab_close)
                : (Clay_Color){0}
        }) {
            SDL_Texture *next_icon = icon_get(
                nav_disabled ? "caret-right-faded" : "caret-right", state, 10, 10);
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = 10.0f * sf, .height = 10.0f * sf } },
                .image  = { .imageData = next_icon },
            }) {}
            if (!nav_disabled) Clay_OnHover(HandleSearchNext, pane);
            search_next_hovered = Clay_Hovered();
            if (search_next_hovered)
                state->input.desired_cursor = nav_disabled
                    ? SDL_SYSTEM_CURSOR_NOT_ALLOWED
                    : SDL_SYSTEM_CURSOR_POINTER;
        }
        char next_binding[64] = {0};
        keymap_where_is_first(state, "vim-search-next", next_binding, sizeof(next_binding));
        TextTooltipWithBinding(state, search_next_hovered, index + 1026,
                               "Select Next Match", next_binding[0] ? next_binding : NULL);

        // count_str is pre-formatted and stored in the SearchSession (stable per-pane memory).
        const char *count_chars = s->query_len == 0 ? "0/0" : s->count_str;
        Clay_String cstr = {
            .chars               = count_chars,
            .length              = (int32_t)strlen(count_chars),
            .isStaticallyAllocated = true,
        };
        CLAY_TEXT(cstr, CLAY_TEXT_CONFIG({
            .fontId    = FONT_UI_NORMAL,
            .fontSize  = (uint16_t)(10 * sf),
            .textColor = s->query_len && s->match_count
              ? ui_resolve_color(state, state->ui.roles.text_primary)
              : ui_resolve_color(state, state->ui.roles.text_faded),
        }));

        bool search_close_hovered = false;
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { .width = CLAY_SIZING_FIXED(15 * sf), .height = CLAY_SIZING_FIXED(15 * sf) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .cornerRadius    = CLAY_CORNER_RADIUS(8 * sf),
            .backgroundColor = Clay_Hovered()
                ? ui_resolve_color(state, state->ui.roles.tab_close)
                : (Clay_Color){0}
        }) {
            SDL_Texture *icon = icon_get("tab-close", state, 7, 7);
            CLAY_AUTO_ID({
                .layout = { .sizing = { .width = 7.0f * sf, .height = 7.0f * sf } },
                .image  = { .imageData = icon },
            }) {}
            Clay_OnHover(HandleCloseSearch, pane);
            search_close_hovered = Clay_Hovered();
        }
        char binding[64] = {0};
        keymap_where_is_first(state, "search-cancel", binding, sizeof(binding));
        TextTooltipWithBinding(state, search_close_hovered, index + 1024,
                               "Close Search Bar", binding[0] ? binding : NULL);
    }
}

static void LeafPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    Clay_ElementId cpane_id = CLAY_IDI_LOCAL("ContentPane", index);
    CLAY(cpane_id, {
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
        SearchBar(state, pane, index);
        BufferContentRender(state, &pane->content, pane->content.active_tab, index, cpane_id);
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

// --- Search ---

static void search_recompute_current(Pane *pane) {
    if (!pane || !pane->content.active_tab) return;
    Buffer *buf = pane->content.active_tab->content.buffer.buffer;
    if (!buf) return;
    const char *text = buffer_text_cached(buf);
    search_session_recompute(&pane->content.search, text, strlen(text));
}

static void search_jump_to_active(Pane *pane) {
    if (!pane) return;
    SearchSession *s = &pane->content.search;
    if (s->match_count == 0) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    Location loc = { .pos = s->matches[s->active_match_index].start };
    point_set(loc);
    update_line(buf);
    save_current_column(buf);
}

void search_handle_key(AppState *state, const KeyEvent *ev) {
    Pane *pane = pane_get_active();
    if (!pane) { state->input.current_focus = FOCUS_PANE; return; }
    SearchSession *s = &pane->content.search;

    if (ev->type == KEYEVENT_SPECIAL) {
        switch (ev->keycode) {
        case KEY_BACKSPACE:
            while (s->query_len > 0 && (s->query[s->query_len - 1] & 0xC0) == 0x80)
                s->query_len--;
            if (s->query_len > 0) s->query_len--;
            s->query[s->query_len] = '\0';
            search_recompute_current(pane);
            break;
        case KEY_RETURN:
            state->input.current_focus = FOCUS_PANE;
            search_jump_to_active(pane);
            break;
        case KEY_ESC:
            s->bar_open    = false;
            s->active      = false;
            s->match_count = 0;
            s->query_len   = 0;
            s->query[0]    = '\0';
            state->input.current_focus = FOCUS_PANE;
            break;
        default:
            break;
        }
        return;
    }

    if (ev->type == KEYEVENT_CHAR && ev->mods == MOD_NONE) {
        uint32_t cp = ev->codepoint;
        char utf8[5] = {0};
        int len;
        if      (cp < 0x80)    { utf8[0] = (char)cp;                                                             len = 1; }
        else if (cp < 0x800)   { utf8[0] = (char)(0xC0|(cp>>6));  utf8[1] = (char)(0x80|(cp&0x3F));             len = 2; }
        else if (cp < 0x10000) { utf8[0] = (char)(0xE0|(cp>>12)); utf8[1] = (char)(0x80|((cp>>6)&0x3F));  utf8[2] = (char)(0x80|(cp&0x3F));            len = 3; }
        else                   { utf8[0] = (char)(0xF0|(cp>>18)); utf8[1] = (char)(0x80|((cp>>12)&0x3F)); utf8[2] = (char)(0x80|((cp>>6)&0x3F)); utf8[3] = (char)(0x80|(cp&0x3F)); len = 4; }

        if (s->query_len + (size_t)len < SEARCH_QUERY_MAX) {
            memcpy(s->query + s->query_len, utf8, (size_t)len);
            s->query_len += (size_t)len;
            s->query[s->query_len] = '\0';
            search_recompute_current(pane);
        }
    }
}

// --- Scheme bindings ---

sexp scm_pane_navigate_up(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_navigate_up()) message_send("pane-navigate-up");
    return SEXP_VOID;
}

sexp scm_pane_navigate_down(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_navigate_down()) message_send("pane-navigate-down");
    return SEXP_VOID;
}

sexp scm_pane_navigate_left(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_navigate_left()) message_send("pane-navigate-left");
    return SEXP_VOID;
}

sexp scm_pane_navigate_right(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_navigate_right()) message_send("pane-navigate-right");
    return SEXP_VOID;
}

sexp scm_pane_v_split_increase(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_v_split_increase()) message_send("pane-v-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_v_split_decrease(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_v_split_decrease()) message_send("pane-v-split-decrease");
    return SEXP_VOID;
}

sexp scm_pane_h_split_increase(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_h_split_increase()) message_send("pane-h-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_h_split_decrease(sexp ctx, sexp self, sexp n) {
    message_clear();
    if (pane_h_split_decrease()) message_send("pane-h-split-decrease");
    return SEXP_VOID;
}

sexp scm_split_vertical(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (pane) pane_split_vertical(pane);
    message_send("split-vertical");
    return SEXP_VOID;
}

sexp scm_split_horizontal(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (pane) pane_split_horizontal(pane);
    message_send("split-horizontal");
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

sexp scm_search_open(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    Buffer *buf = buffer_get_current();
    s->point     = buf ? point_get(buf).pos : 0;
    s->query_len = 0;
    s->query[0]  = '\0';
    s->match_count = 0;
    s->active_match_index = 0;
    s->active   = true;
    s->bar_open = true;
    G->input.current_focus = FOCUS_SEARCH;
    return SEXP_VOID;
}

sexp scm_search_self_insert(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    if (last_event.type != KEYEVENT_CHAR) return SEXP_VOID;
    uint32_t cp = last_event.codepoint;
    char utf8[5] = {0};
    int len;
    if      (cp < 0x80)    { utf8[0] = (char)cp;                                                             len = 1; }
    else if (cp < 0x800)   { utf8[0] = (char)(0xC0|(cp>>6));  utf8[1] = (char)(0x80|(cp&0x3F));             len = 2; }
    else if (cp < 0x10000) { utf8[0] = (char)(0xE0|(cp>>12)); utf8[1] = (char)(0x80|((cp>>6)&0x3F));  utf8[2] = (char)(0x80|(cp&0x3F));            len = 3; }
    else                   { utf8[0] = (char)(0xF0|(cp>>18)); utf8[1] = (char)(0x80|((cp>>12)&0x3F)); utf8[2] = (char)(0x80|((cp>>6)&0x3F)); utf8[3] = (char)(0x80|(cp&0x3F)); len = 4; }
    if (s->query_len + (size_t)len < SEARCH_QUERY_MAX) {
        memcpy(s->query + s->query_len, utf8, (size_t)len);
        s->query_len += (size_t)len;
        s->query[s->query_len] = '\0';
        search_recompute_current(pane);
    }
    return SEXP_VOID;
}

sexp scm_search_backspace(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    while (s->query_len > 0 && (s->query[s->query_len - 1] & 0xC0) == 0x80)
        s->query_len--;
    if (s->query_len > 0) s->query_len--;
    s->query[s->query_len] = '\0';
    search_recompute_current(pane);
    return SEXP_VOID;
}

sexp scm_search_confirm(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    // Keep bar_open so the counter stays visible; only close on escape.
    G->input.current_focus = FOCUS_PANE;
    search_jump_to_active(pane);
    return SEXP_VOID;
}

sexp scm_search_cancel(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    s->bar_open    = false;
    s->active      = false;
    s->match_count = 0;
    s->query_len   = 0;
    s->query[0]    = '\0';
    G->input.current_focus = FOCUS_PANE;
    return SEXP_VOID;
}

sexp scm_search_next(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    if (!s->active || s->match_count == 0) return SEXP_VOID;
    search_session_next_match(s);
    search_jump_to_active(pane);
    return SEXP_VOID;
}

sexp scm_search_prev(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_VOID;
    SearchSession *s = &pane->content.search;
    if (!s->active || s->match_count == 0) return SEXP_VOID;
    search_session_prev_match(s);
    search_jump_to_active(pane);
    return SEXP_VOID;
}

sexp scm_search_bar_open_p(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane) return SEXP_FALSE;
    return pane->content.search.bar_open ? SEXP_TRUE : SEXP_FALSE;
}
