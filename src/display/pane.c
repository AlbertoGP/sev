#include <stdlib.h>
#include <string.h>

#include "cursor.h"
#include "pane.h"
#include "status.h"
#include "tab.h"
#include "theme.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"
#include "../text/buffer_type.h"
#include "../text/line.h"

// Recursively free resources allocated for a pane sub-tree.
void pane_destroy(Pane *pane) {
    if (!pane) return;
    if (pane->type == PANE_H_SPLIT) {
        pane_destroy(pane->h_split.top);
        pane_destroy(pane->h_split.bottom);
    } else if (pane->type == PANE_V_SPLIT) {
        pane_destroy(pane->v_split.left);
        pane_destroy(pane->v_split.right);
    } else if (pane->type == PANE_CONTENT) {
        vline_cache_destroy(&pane->content.vline_cache);
        jump_list_free(&pane->content.jump_list);
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
    // PANE_CONTENT — use stored geometry (available after first rendered frame)
    Content *c = &root->content;
    if (c->line_height_px <= 0) return NULL;
    float left   = c->text_origin_x - c->gutter_width_px;
    float right  = c->text_origin_x + c->text_origin_w;
    float top    = c->text_origin_y;
    float bottom = c->text_origin_y + c->text_origin_h;
    if (x >= left && x <= right && y >= top && y <= bottom)
        return root;
    return NULL;
}

void sync_active_buffer(void) {
    Pane *active = pane_get_active();
    if (active && active->content.type == CONTENT_TEXT) {
        buffer_set_current(active->content.buffer);
    }
}

Pane *pane_create(void) {
    Pane *pane = calloc(1, sizeof(Pane));
    if (pane) {
        pane->content.vline_cache = vline_cache_create();
    }
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
static void descend_pane_tree(Pane *pane) {
    while (pane->type != PANE_CONTENT) {
        pane = pane_first_child(pane);
    }
    pane->content.active = true;
    sync_active_buffer();
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
    descend_pane_tree(sibling);
    usurp_parent(sibling, parent);
    free(pane);
}

bool pane_set_buffer(Pane *pane, Buffer *buf) {
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
    pane->content.vline_cache.full_rebuild = true;

    if (pane->content.active) {
        sync_active_buffer();
    }

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
    if (!pane) return NULL;
    return pane_get_active_from_children(pane);
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
    new->content.vline_cache = vline_cache_create();
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

#define PANE_STRINGS_MAX 32
static char *pane_strings[PANE_STRINGS_MAX];
static int pane_strings_count = 0;

void pane_free_strings(void) {
    for (int i = 0; i < pane_strings_count; i++) {
        free(pane_strings[i]);
    }
    pane_strings_count = 0;
}

static void BufferPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    Buffer *buf = pane->content.buffer;
    char *chars = buffer_text(buf);
    if (pane_strings_count < PANE_STRINGS_MAX) {
        pane_strings[pane_strings_count++] = chars;
    }
    size_t point = point_get(buf).pos;

    const uint16_t font_id = FONT_NORMAL;
    const uint16_t font_size = 15 * state->ui.scale_factor * buffer_get_scale(buf);
    const float padding = 24.0f * state->ui.scale_factor;

    CLAY(CLAY_IDI_LOCAL("BufferPane", index), {
        .layout = {
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
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
                float text_width = box.width - (2 * padding);
                float text_height = box.height;

                // Get font early (needed for gutter width measurement)
                TTF_Font *font = state->rendererData.fonts[font_id];
                TTF_SetFontSize(font, font_size);

                // Check display-line-numbers-type buffer-local variable
                sexp lnum_sym = sexp_intern(state->chibi.ctx, "display-line-numbers-type", -1);
                sexp lnum_val = vartable_get(buffer_get_locals(buf), lnum_sym, SEXP_FALSE);
                // 0=off, 1=absolute, 2=relative, 3=visual
                int line_num_type = 0;
                if (lnum_val == SEXP_TRUE) line_num_type = 1;
                else if (sexp_symbolp(lnum_val)) {
                    sexp ctx_l = state->chibi.ctx;
                    if      (lnum_val == sexp_intern(ctx_l, "relative", -1)) line_num_type = 2;
                    else if (lnum_val == sexp_intern(ctx_l, "visual",   -1)) line_num_type = 3;
                }

                // Calculate gutter width for line numbers
                const LineTable *lt = buffer_get_line_table(buf);
                float gutter_width = 0;
                if (line_num_type) {
                    char num_buf[20];
                    int ndigits = snprintf(num_buf, sizeof(num_buf), "%zu", lt->count);
                    if (ndigits < 3) ndigits = 3; // minimum 3 chars wide
                    memset(num_buf, '0', ndigits);
                    num_buf[ndigits] = '\0';
                    int w = 0, h = 0;
                    TTF_GetStringSize(font, num_buf, ndigits, &w, &h);
                    gutter_width = (float)w + 16.0f * state->ui.scale_factor;
                }

                // Rebuild visual line cache
                VLineCache *cache = &pane->content.vline_cache;
                float wrap_width = text_width - gutter_width;
                vline_rebuild(cache, buf, &state->rendererData,
                              wrap_width, font_id, font_size);

                // Calculate visible lines
                int line_height = vline_get_line_height(&state->rendererData,
                                                        font_id, font_size);

                // Store geometry for mouse hit-testing (used next frame).
                pane->content.text_origin_x   = box.x + padding + gutter_width;
                pane->content.text_origin_y   = box.y + 2;
                pane->content.text_origin_w   = text_width - gutter_width;
                pane->content.text_origin_h   = text_height;
                pane->content.gutter_width_px = gutter_width;
                pane->content.line_height_px  = line_height;
                pane->content.render_font_id   = font_id;
                pane->content.render_font_size = font_size;

                size_t visible_count = line_height > 0 ? (size_t)(text_height / line_height) : 0;
                if (visible_count > 0 && pane->content.active)
                    vline_scroll_to_cursor(cache, point, visible_count);
                if (visible_count == 0) visible_count = 1;

                // Render visible visual lines
                size_t end_vline = cache->top_vline + visible_count;
                if (end_vline > cache->count) end_vline = cache->count;

                // Cursor's logical line (for relative mode + current line highlight)
                size_t cursor_logical_line = 0;
                if (line_num_type)
                    cursor_logical_line = line_index_at(lt, point);

                // Allocate line number string buffer
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
                    size_t line_end = vl->byte_end;
                    size_t line_len = line_end - line_start;

                    // Determine if cursor is on this line
                    bool cursor_on_line = (point >= line_start && point < line_end);
                    bool is_last = (i + 1 == cache->count) || (cache->lines[i+1].line_id != vl->line_id);
                    if (!cursor_on_line && point == line_end && is_last) {
                        // Only claim it if the next line doesn't start exactly at this point
                        if (i + 1 == cache->count || cache->lines[i+1].byte_start != point) {
                            cursor_on_line = true;
                        }
                    }

                    // Calculate cursor x-offset if on this line
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
                                .width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(line_height)
                            },
                            .layoutDirection = line_num_type ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM,
                        }
                    }) {
                        // Render line number gutter
                        if (line_num_type && lnum_strs) {
                            size_t str_idx = i - cache->top_vline;
                            char *str = lnum_strs + str_idx * LNUM_STR_LEN;
                            size_t slen = 0;
                            bool show_number = true;

                            if (line_num_type == 3) { // visual
                                slen = snprintf(str, LNUM_STR_LEN, "%zu", i - cache->top_vline + 1);
                            } else if (vl->visual_index > 0) {
                                show_number = false; // wrapped continuation
                            } else {
                                size_t logical = line_index_at(lt, vl->byte_start);
                                if (line_num_type == 1) { // absolute
                                    slen = snprintf(str, LNUM_STR_LEN, "%zu", logical + 1);
                                } else { // relative
                                    size_t rel = (logical > cursor_logical_line)
                                        ? logical - cursor_logical_line
                                        : cursor_logical_line - logical;
                                    slen = snprintf(str, LNUM_STR_LEN, "%zu", rel == 0 ? logical + 1 : rel);
                                }
                            }

                            float gutter_pad = 8.0f * state->ui.scale_factor;
                            CLAY(CLAY_IDI_LOCAL("LineNum", (int32_t)i), {
                                .layout = {
                                    .sizing = {
                                        .width = CLAY_SIZING_FIXED(gutter_width),
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
                                            is_current && pane->content.active
                                                ? state->ui.roles.text_primary
                                                : state->ui.roles.text_faded),
                                    }));
                                }
                            }
                        }

                        // Render cursor as floating element (doesn't affect text layout)
                        if (cursor_on_line && pane->content.active) {
                            Cursor(state, (int32_t)i, cursor_offset + gutter_width,
                                   line_height, font_id, font_size);
                        }

                        // Render selection highlight as floating element
                        if (buf->select_mode != SELECT_NONE && pane->content.active) {
                            size_t sel_a = buf->select_start.pos;
                            size_t sel_b = point;
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
                                    size_t he = log_start + col_max + 1;
                                    if (he > log_end) he = log_end;
                                    if (hs > log_end) hs = log_end;
                                    // Clamp to visual line bounds
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
                                                .width = CLAY_SIZING_FIXED(sel_w),
                                                .height = CLAY_SIZING_FIXED(line_height)
                                            }
                                        },
                                        .backgroundColor = ui_resolve_color(state,
                                            state->ui.roles.selection),
                                    }) {}
                                }
                            }
                        }

                        // Render entire line as single text element
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
                // Dimensions not yet available, render placeholder
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
        StatusBar(state, pane);
    }
}

static void VSplitPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    CLAY(CLAY_IDI_LOCAL("VSplitPane", index), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .childGap = 2
        },
    }) {
        PaneContent(state, pane->v_split.left, 1, pane->v_split.left_width, 0);
        PaneContent(state, pane->v_split.right, 2, 1 - pane->v_split.left_width, 0);
    }
}

static void HSplitPane(AppState *state, Pane *pane, int32_t index, float width, float height) {
    CLAY(CLAY_IDI_LOCAL("HSplitPane", index), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = width ? CLAY_SIZING_PERCENT(width) : CLAY_SIZING_GROW(0),
                .height = height ? CLAY_SIZING_PERCENT(height) : CLAY_SIZING_GROW(0),
            },
            .childGap = 2
        }
    }) {
        PaneContent(state, pane->h_split.top, 1, 0, pane->h_split.top_height);
        PaneContent(state, pane->h_split.bottom, 2, 0, 1 - pane->h_split.top_height);
    }
}

void PaneContent(AppState *state, Pane *pane, int32_t index, float width, float height) {
    if (pane->type == PANE_V_SPLIT) VSplitPane(state, pane, index, width, height);
    if (pane->type == PANE_H_SPLIT) HSplitPane(state, pane, index, width, height);
    if (pane->type == PANE_CONTENT) BufferPane(state, pane, index, width, height);
}

// --- Scheme bindings ---

sexp scm_pane_navigate_up(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (pane_navigate_up()) message_send("pane-navigate-up");
    return SEXP_VOID;
}

sexp scm_pane_navigate_down(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (pane_navigate_down()) message_send("pane-navigate-down");
    return SEXP_VOID;
}

sexp scm_pane_navigate_left(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (pane_navigate_left()) message_send("pane-navigate-left");
    return SEXP_VOID;
}

sexp scm_pane_navigate_right(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    message_clear();
    if (pane_navigate_right()) message_send("pane-navigate-right");
    return SEXP_VOID;
}

sexp scm_pane_v_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    message_clear();
    if (pane_v_split_increase()) message_send("pane-v-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_v_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    message_clear();
    if (pane_v_split_decrease()) message_send("pane-v-split-decrease");
    return SEXP_VOID;
}

sexp scm_pane_h_split_increase(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    message_clear();
    if (pane_h_split_increase()) message_send("pane-h-split-increase");
    return SEXP_VOID;
}

sexp scm_pane_h_split_decrease(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    message_clear();
    if (pane_h_split_decrease()) message_send("pane-h-split-decrease");
    return SEXP_VOID;
}

sexp scm_split_vertical(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    Pane *pane = pane_get_active();
    if (pane)
        pane_split_vertical(pane);

    message_send("split-vertical");
    return SEXP_VOID;
}

sexp scm_split_horizontal(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;

    Pane *pane = pane_get_active();
    if (pane)
        pane_split_horizontal(pane);

    message_send("split-horizontal");
    return SEXP_VOID;
}

sexp scm_pane_close(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    pane_close();

    message_send("pane-close");
    return SEXP_VOID;
}

sexp scm_jump_push(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT) return SEXP_VOID;
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_VOID;
    Jump j = {
        .buf_name = strdup(buffer_get_name(buf)),
        .point    = point_get(buf),
        .filename = NULL,
    };
    jump_list_push(&pane->content.jump_list, j);
    return SEXP_VOID;
}

void pane_push_jump(void) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT) return;
    Buffer *buf = buffer_get_current();
    if (!buf) return;
    Jump j = {
        .buf_name = strdup(buffer_get_name(buf)),
        .point    = point_get(buf),
        .filename = NULL,
    };
    jump_list_push(&pane->content.jump_list, j);
}

static void jump_apply(JumpList *jl) {
    Jump *j = &jl->list[jl->current_index];
    if (!j->buf_name) return;

    Buffer *buf = buffer_get_current();
    if (!buf) return;

    if (strcmp(j->buf_name, buffer_get_name(buf)) != 0) {
        Buffer *target = buffer_get_by_name(j->buf_name);
        if (!target) return;  // buffer was closed
        Pane *pane = pane_get_active();
        if (!pane || !pane_set_buffer(pane, target)) return;
        buf = buffer_get_current();  // refreshed after sync_active_buffer()
    }

    point_set(j->point);
    update_line(buf);
    save_current_column(buf);
    G->needs_redraw = true;
}

sexp scm_jump_backward(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT) return SEXP_VOID;
    JumpList *jl = &pane->content.jump_list;
    Buffer *buf = buffer_get_current();
    if (!buf) return SEXP_VOID;

    if (jump_list_at_front(jl)) {
        // Save current position so C-i can return here.
        Jump j = {
            .buf_name = strdup(buffer_get_name(buf)),
            .point    = point_get(buf),
            .filename = NULL,
        };
        jump_list_push(jl, j);
        // Skip backward past the entry we just pushed (it's our current spot).
        jump_list_backward(jl);
    }

    if (jump_list_backward(jl))
        jump_apply(jl);
    return SEXP_VOID;
}

sexp scm_jump_forward(sexp ctx, sexp self, sexp n) {
    Pane *pane = pane_get_active();
    if (!pane || pane->type != PANE_CONTENT) return SEXP_VOID;
    if (jump_list_forward(&pane->content.jump_list))
        jump_apply(&pane->content.jump_list);
    return SEXP_VOID;
}

sexp scm_set_mouse_click_handler(sexp ctx, sexp self, sexp n, sexp cb) {
    if (G->input.mouse_click_cb != SEXP_FALSE)
        sexp_release_object(ctx, G->input.mouse_click_cb);
    G->input.mouse_click_cb = cb;
    if (cb != SEXP_FALSE)
        sexp_preserve_object(ctx, cb);
    return SEXP_VOID;
}

sexp scm_set_mouse_drag_handler(sexp ctx, sexp self, sexp n, sexp cb) {
    if (G->input.mouse_drag_cb != SEXP_FALSE)
        sexp_release_object(ctx, G->input.mouse_drag_cb);
    G->input.mouse_drag_cb = cb;
    if (cb != SEXP_FALSE)
        sexp_preserve_object(ctx, cb);
    return SEXP_VOID;
}
