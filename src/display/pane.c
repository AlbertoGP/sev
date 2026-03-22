#include <stdlib.h>
#include <string.h>

#include "cursor.h"
#include "pane.h"
#include "tab.h"
#include "theme.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"
#include "../text/buffer_type.h"
#include "../text/line.h"

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
    // PANE_DISPLAY — use stored geometry (available after first rendered frame).
    DisplayContent *d = &root->display;
    if (d->line_height_px <= 0) return NULL;
    float left   = d->text_origin_x - d->gutter_width_px;
    float right  = d->text_origin_x + d->text_origin_w;
    float top    = d->text_origin_y;
    float bottom = d->text_origin_y + d->text_origin_h;
    if (x >= left && x <= right && y >= top && y <= bottom)
        return root;
    return NULL;
}

void sync_active_buffer(void) {
    Pane *active = pane_get_active();
    if (active && active->display.active_tab)
        buffer_set_current(active->display.active_tab->buffer);
    if (!G->minibuf.active)
        G->input.current_focus = active ? FOCUS_PANE : FOCUS_SPLASH;
}

Pane *pane_display_create(Buffer *buf) {
    Pane *pane = calloc(1, sizeof(Pane));
    if (!pane) return NULL;
    pane->type = PANE_DISPLAY;

    Tab *tab = tab_alloc(buf);
    if (!tab) { free(pane); return NULL; }

    pane->display.list = tab;
    pane->display.active_tab = tab;
    pane->display.active = false;
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
    } else if (pane->type == PANE_DISPLAY) {
        Tab *t = pane->display.list;
        while (t) {
            Tab *next = t->next;
            tab_free(t);
            t = next;
        }
    }
    free(pane);
}

// Descend into pane tree, activating the first PANE_DISPLAY leaf found.
static void descend_pane_tree(Pane *pane) {
    while (pane->type != PANE_DISPLAY)
        pane = pane_first_child(pane);
    pane->display.active = true;
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

    // If this display pane has more than one tab, just close the active tab.
    if (pane->display.list && pane->display.list->next) {
        bool empty = display_tab_close(pane);
        (void)empty;  // cannot be true since we checked list->next above
        sync_active_buffer();
        update_window_title();
        return;
    }

    // Single-tab pane: close the entire pane.
    Pane *parent = pane->parent;
    if (!parent) {
        // Root pane: destroy it, set root to NULL → splash.
        pane_destroy(pane);
        root_pane = NULL;
        G->input.current_focus = FOCUS_SPLASH;
        buffer_set_current(NULL);
        update_window_title();
        return;
    }

    Pane *sibling = pane_get_sibling(pane);
    pane->display.active = false;
    descend_pane_tree(sibling);
    usurp_parent(sibling, parent);
    // Destroy this pane's single tab then the pane node.
    tab_free(pane->display.list);
    free(pane);
    update_window_title();
}


static Pane *pane_get_active_from(Pane *pane) {
    if (!pane) return NULL;
    if (pane->type == PANE_DISPLAY)
        return pane->display.active ? pane : NULL;
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
    if (current) current->display.active = false;
    pane->display.active = true;
    sync_active_buffer();
    return true;
}

Pane *pane_split(Pane *pane, PaneType split_type) {
    if (split_type != PANE_H_SPLIT && split_type != PANE_V_SPLIT) return NULL;
    if (pane->type != PANE_DISPLAY) return NULL;

    Pane *split = malloc(sizeof(Pane));
    if (!split) return NULL;

    // New sibling gets one tab showing the same buffer as pane's active tab.
    Buffer *buf = pane->display.active_tab ? pane->display.active_tab->buffer : NULL;
    Pane *new_pane = pane_display_create(buf);
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
                pane_get_active()->display.active = false;
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

    if (pane->type == PANE_DISPLAY) {
        // Remove all tabs in this display pane that show buf.
        Tab *t = pane->display.list;
        while (t) {
            Tab *next = t->next;
            if (t->buffer == buf) {
                if (t->prev) t->prev->next = t->next;
                else pane->display.list = t->next;
                if (t->next) t->next->prev = t->prev;
                if (pane->display.active_tab == t)
                    pane->display.active_tab = t->next ? t->next : t->prev;
                tab_free(t);
            }
            t = next;
        }
        return pane->display.list == NULL;
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

// Ensure at least one PANE_DISPLAY is active (activate first found if none is).
static bool ensure_active_display_from(Pane *pane) {
    if (!pane) return false;
    if (pane->type == PANE_DISPLAY) {
        if (!pane->display.active) pane->display.active = true;
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
    if (pane->type == PANE_DISPLAY) return pane->display.active;
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

static Clay_Sizing layoutExpand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

#define PANE_STRINGS_MAX 32
static char *pane_strings[PANE_STRINGS_MAX];
static int pane_strings_count = 0;

void pane_free_strings(void) {
    for (int i = 0; i < pane_strings_count; i++)
        free(pane_strings[i]);
    pane_strings_count = 0;
}

static void BufferPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    Tab *tab = pane->display.active_tab;
    if (!tab) return;
    Buffer *buf = tab->buffer;
    if (!buf) return;

    char *chars = buffer_text(buf);
    if (pane_strings_count < PANE_STRINGS_MAX)
        pane_strings[pane_strings_count++] = chars;
    size_t point = point_get(buf).pos;

    const uint16_t font_id   = FONT_NORMAL;
    const uint16_t font_size = 15 * state->ui.scale_factor * buffer_get_scale(buf);
    const float padding      = 24.0f * state->ui.scale_factor;

    CLAY(CLAY_IDI_LOCAL("BufferPane", index), {
        .layout = {
            .sizing = {
                .width  = width  ? CLAY_SIZING_PERCENT(width)  : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg)
    }) {
        // Per-pane tab bar.
        TabBar(state, pane, index);

        Clay_ElementId id = CLAY_IDI_LOCAL("Buffer Text", index);
        CLAY(id, {
            .layout = {
                .sizing = layoutExpand,
                .padding = { .left = padding, .right = padding, .top = 2, .bottom = 2 },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .clip = { .vertical = true }
        }) {
            Clay_ElementData data = Clay_GetElementData(id);
            if (data.found) {
                Clay_BoundingBox box = data.boundingBox;
                float text_width  = box.width - (2 * padding);
                float text_height = box.height;

                TTF_Font *font = state->rendererData.fonts[font_id];
                TTF_SetFontSize(font, font_size);

                sexp lnum_sym = sexp_intern(state->chibi.ctx, "display-line-numbers-type", -1);
                sexp lnum_val = vartable_get(buffer_get_locals(buf), lnum_sym, SEXP_FALSE);
                int line_num_type = 0;
                if (lnum_val == SEXP_TRUE) line_num_type = 1;
                else if (sexp_symbolp(lnum_val)) {
                    sexp ctx_l = state->chibi.ctx;
                    if      (lnum_val == sexp_intern(ctx_l, "relative", -1)) line_num_type = 2;
                    else if (lnum_val == sexp_intern(ctx_l, "visual",   -1)) line_num_type = 3;
                }

                const LineTable *lt = buffer_get_line_table(buf);
                float gutter_width = 0;
                if (line_num_type) {
                    char num_buf[20];
                    int ndigits = snprintf(num_buf, sizeof(num_buf), "%zu", lt->count);
                    if (ndigits < 3) ndigits = 3;
                    memset(num_buf, '0', ndigits);
                    num_buf[ndigits] = '\0';
                    int w = 0, h = 0;
                    TTF_GetStringSize(font, num_buf, ndigits, &w, &h);
                    gutter_width = (float)w + 16.0f * state->ui.scale_factor;
                }

                VLineCache *cache = &tab->vline_cache;
                float wrap_width = text_width - gutter_width;
                vline_rebuild(cache, buf, &state->rendererData, wrap_width, font_id, font_size);

                int line_height = vline_get_line_height(&state->rendererData, font_id, font_size);

                // Store geometry in the pane (not the tab) for mouse hit-testing.
                pane->display.text_origin_x   = box.x + padding + gutter_width;
                pane->display.text_origin_y   = box.y + 2;
                pane->display.text_origin_w   = text_width - gutter_width;
                pane->display.text_origin_h   = text_height;
                pane->display.gutter_width_px = gutter_width;
                pane->display.line_height_px  = line_height;
                pane->display.render_font_id   = font_id;
                pane->display.render_font_size = font_size;

                size_t visible_count = line_height > 0 ? (size_t)(text_height / line_height) : 0;
                if (visible_count > 0 && pane->display.active)
                    vline_scroll_to_cursor(cache, point, visible_count);
                if (visible_count == 0) visible_count = 1;

                size_t end_vline = cache->top_vline + visible_count;
                if (end_vline > cache->count) end_vline = cache->count;

                size_t cursor_logical_line = 0;
                if (line_num_type)
                    cursor_logical_line = line_index_at(lt, point);

                #define LNUM_STR_LEN 12
                char *lnum_strs = NULL;
                if (line_num_type) {
                    size_t num_visible = end_vline - cache->top_vline;
                    lnum_strs = malloc(num_visible * LNUM_STR_LEN);
                    if (pane_strings_count < PANE_STRINGS_MAX)
                        pane_strings[pane_strings_count++] = lnum_strs;
                }

                for (size_t i = cache->top_vline; i < end_vline; i++) {
                    VisualLine *vl = &cache->lines[i];
                    size_t line_start = vl->byte_start;
                    size_t line_end   = vl->byte_end;
                    size_t line_len   = line_end - line_start;

                    bool cursor_on_line = (point >= line_start) && (point < line_end)
                                          && !state->minibuf.active;
                    bool is_last = (i + 1 == cache->count) ||
                                   (cache->lines[i+1].line_id != vl->line_id);
                    if (!cursor_on_line && point == line_end && is_last) {
                        if (i + 1 == cache->count || cache->lines[i+1].byte_start != point)
                            cursor_on_line = true;
                    }

                    float cursor_offset = 0;
                    if (cursor_on_line) {
                        size_t head_len = point - line_start;
                        if (head_len > 0) {
                            int w = 0, h = 0;
                            TTF_GetStringSize(font, chars + line_start, head_len, &w, &h);
                            cursor_offset = (float)w;
                        }
                    }

                    CLAY(CLAY_IDI_LOCAL("VLine", (int32_t)i), {
                        .layout = {
                            .sizing = {
                                .width  = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(line_height)
                            },
                            .layoutDirection = line_num_type ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
                        }
                    }) {
                        if (line_num_type && lnum_strs) {
                            size_t str_idx = i - cache->top_vline;
                            char *str = lnum_strs + str_idx * LNUM_STR_LEN;
                            size_t slen = 0;
                            bool show_number = true;

                            if (line_num_type == 3) {
                                slen = snprintf(str, LNUM_STR_LEN, "%zu", i - cache->top_vline + 1);
                            } else if (vl->visual_index > 0) {
                                show_number = false;
                            } else {
                                size_t logical = line_index_at(lt, vl->byte_start);
                                if (line_num_type == 1) {
                                    slen = snprintf(str, LNUM_STR_LEN, "%zu", logical + 1);
                                } else {
                                    size_t rel = (logical > cursor_logical_line)
                                        ? logical - cursor_logical_line
                                        : cursor_logical_line - logical;
                                    slen = snprintf(str, LNUM_STR_LEN, "%zu",
                                                    rel == 0 ? logical + 1 : rel);
                                }
                            }

                            float gutter_pad = 8.0f * state->ui.scale_factor;
                            CLAY(CLAY_IDI_LOCAL("LineNum", (int32_t)i), {
                                .layout = {
                                    .sizing = {
                                        .width  = CLAY_SIZING_FIXED(gutter_width),
                                        .height = CLAY_SIZING_FIXED(line_height)
                                    },
                                    .padding = { .left = gutter_pad, .right = gutter_pad },
                                    .childAlignment = { .x = CLAY_ALIGN_X_RIGHT },
                                },
                            }) {
                                if (show_number && slen > 0) {
                                    bool is_current = (line_num_type != 3) &&
                                        (line_index_at(lt, vl->byte_start) == cursor_logical_line);
                                    Clay_String numStr = { .chars = str, .length = slen };
                                    CLAY_TEXT(numStr, CLAY_TEXT_CONFIG({
                                        .fontId = font_id,
                                        .fontSize = font_size,
                                        .textColor = ui_resolve_color(state,
                                            is_current && pane->display.active
                                                ? state->ui.roles.text_primary
                                                : state->ui.roles.text_faded),
                                    }));
                                }
                            }
                        }

                        if (cursor_on_line && pane->display.active) {
                            Cursor(state, (int32_t)i, cursor_offset + gutter_width,
                                   line_height, font_id, font_size);
                        }

                        if (buf->select_mode != SELECT_NONE && pane->display.active) {
                            size_t sel_a   = buf->select_start.pos;
                            size_t sel_b   = point;
                            size_t sel_min = sel_a < sel_b ? sel_a : sel_b;
                            size_t sel_max = sel_a > sel_b ? sel_a + 1 : sel_b + 1;

                            size_t hl_start = 0, hl_end = 0;
                            bool do_highlight = false;

                            switch (buf->select_mode) {
                            case SELECT_REGULAR: {
                                if (line_start < sel_max && line_end >= sel_min) {
                                    hl_start = sel_min > line_start ? sel_min : line_start;
                                    hl_end   = sel_max < line_end   ? sel_max : line_end;
                                    do_highlight = true;
                                }
                                break;
                            }
                            case SELECT_LINE: {
                                size_t line_min = line_index_at(lt, sel_min);
                                size_t line_max = line_index_at(lt, sel_max > 0 ? sel_max - 1 : 0);
                                size_t vl_logical = line_index_at(lt, vl->byte_start);
                                if (vl_logical >= line_min && vl_logical <= line_max) {
                                    hl_start = line_start;
                                    hl_end   = line_end;
                                    do_highlight = true;
                                }
                                break;
                            }
                            case SELECT_RECTANGLE_RAGGED:
                            case SELECT_RECTANGLE: {
                                size_t line_a = line_index_at(lt, sel_a);
                                size_t line_b = line_index_at(lt, sel_b);
                                size_t row_min = line_a < line_b ? line_a : line_b;
                                size_t row_max = line_a > line_b ? line_a : line_b;
                                size_t col_a = sel_a - lt->lines[line_a].start;
                                size_t col_b = sel_b - lt->lines[line_b].start;
                                size_t col_min = col_a < col_b ? col_a : col_b;
                                size_t col_max = col_a > col_b ? col_a : col_b;

                                size_t vl_logical = line_index_at(lt, vl->byte_start);
                                if (vl_logical >= row_min && vl_logical <= row_max) {
                                    size_t log_start = lt->lines[vl_logical].start;
                                    size_t log_end   = lt->lines[vl_logical].end;
                                    size_t hs = log_start + col_min;
                                    size_t he;
                                    if (buf->select_mode == SELECT_RECTANGLE_RAGGED) {
                                        he = log_end;
                                        if (he > log_start && chars[he - 1] == '\n') he--;
                                    } else {
                                        he = log_start + col_max + 1;
                                    }
                                    if (he > log_end) he = log_end;
                                    if (hs > log_end) hs = log_end;
                                    if (hs < line_start) hs = line_start;
                                    if (he > line_end)   he = line_end;
                                    if (he > hs) {
                                        hl_start = hs;
                                        hl_end   = he;
                                        do_highlight = true;
                                    }
                                }
                                break;
                            }
                            default: break;
                            }

                            if (do_highlight && hl_end > hl_start) {
                                float sel_x = 0;
                                if (hl_start > line_start) {
                                    int w = 0, h = 0;
                                    TTF_GetStringSize(font, chars + line_start,
                                                      hl_start - line_start, &w, &h);
                                    sel_x = (float)w;
                                }
                                int sw = 0, sh = 0;
                                TTF_GetStringSize(font, chars + hl_start,
                                                  hl_end - hl_start, &sw, &sh);
                                float sel_w = (float)sw;
                                if (sel_w > 0) {
                                    CLAY(CLAY_IDI_LOCAL("Sel", (int32_t)i), {
                                        .floating = {
                                            .attachTo = CLAY_ATTACH_TO_PARENT,
                                            .offset = { .x = sel_x + gutter_width, .y = 0 }
                                        },
                                        .layout = {
                                            .sizing = {
                                                .width  = CLAY_SIZING_FIXED(sel_w),
                                                .height = CLAY_SIZING_FIXED(line_height)
                                            }
                                        },
                                        .backgroundColor = ui_resolve_color(state,
                                            state->ui.roles.selection),
                                    }) {}
                                }
                            }
                        }

                        if (line_len > 0) {
                            Clay_String text = {
                                .chars = chars + line_start,
                                .length = line_len,
                                .isStaticallyAllocated = false
                            };
                            CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                                .fontId = font_id,
                                .fontSize = font_size,
                                .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
                            }));
                        }
                    }
                }
            } else {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = layoutExpand,
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_CENTER,
                            .y = CLAY_ALIGN_Y_CENTER
                        }
                    }
                }) {
                    CLAY_TEXT(CLAY_STRING("Loading..."),
                        CLAY_TEXT_CONFIG({
                            .fontId = FONT_NORMAL,
                            .fontSize = 16,
                            .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
                            .textAlignment = CLAY_TEXT_ALIGN_CENTER
                        }));
                }
            }
        }
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
    if (pane->type == PANE_DISPLAY) BufferPane(state, pane, index, width, height);
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
    if (!pane || pane->type != PANE_DISPLAY || !pane->display.active_tab) return SEXP_VOID;
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_VOID;
    Jump j = { .buf_name = strdup(buffer_get_name(buf)), .point = point_get(buf), .filename = NULL };
    jump_list_push(&pane->display.active_tab->jump_list, j);
    return SEXP_VOID;
}

void pane_push_jump(void) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_DISPLAY || !pane->display.active_tab) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    Jump j = { .buf_name = strdup(buffer_get_name(buf)), .point = point_get(buf), .filename = NULL };
    jump_list_push(&pane->display.active_tab->jump_list, j);
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
        if (!pane || !pane->display.active_tab) return;
        tab_set_buffer(pane->display.active_tab, target);
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
    if (!pane || pane->type != PANE_DISPLAY || !pane->display.active_tab) return SEXP_VOID;
    JumpList *jl = &pane->display.active_tab->jump_list;
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
    if (!pane || pane->type != PANE_DISPLAY || !pane->display.active_tab) return SEXP_VOID;
    if (jump_list_forward(&pane->display.active_tab->jump_list))
        jump_apply(&pane->display.active_tab->jump_list);
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
