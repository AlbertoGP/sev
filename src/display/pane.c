#include <stdlib.h>

#include "cursor.h"
#include "pane.h"
#include "status.h"
#include "tab.h"
#include "theme.h"
#include "../command/scheme_internal.h"
#include "../text/message.h"

// Recursively free resources allocated for a pane sub-tree.
void pane_destroy(Pane *pane) {
    if (pane->type == PANE_H_SPLIT) {
        pane_destroy(pane->h_split.top);
        pane_destroy(pane->h_split.bottom);
    } else if (pane->type == PANE_V_SPLIT) {
        pane_destroy(pane->v_split.left);
        pane_destroy(pane->v_split.right);
    } else if (pane->type == PANE_CONTENT) {
        vline_cache_destroy(&pane->content.vline_cache);
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
                .padding = { .left = padding, .right = padding },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .clip = { .vertical = true }
        }) {
            Clay_ElementData data = Clay_GetElementData(id);
            if (data.found) {
                Clay_BoundingBox box = data.boundingBox;
                float text_width = box.width - (2 * padding);
                float text_height = box.height;

                // Rebuild visual line cache
                VLineCache *cache = &pane->content.vline_cache;
                vline_rebuild(cache, buf, &state->rendererData,
                              text_width, font_id, font_size);

                // Calculate visible lines
                int line_height = vline_get_line_height(&state->rendererData,
                                                        font_id, font_size);
                size_t visible_count = line_height > 0 ? (size_t)(text_height / line_height) : 1;
                if (visible_count == 0) visible_count = 1;

                // Scroll to keep cursor visible
                vline_scroll_to_cursor(cache, point, visible_count);

                // Render visible visual lines
                size_t end_vline = cache->top_vline + visible_count;
                if (end_vline > cache->count) end_vline = cache->count;

                // Get font for cursor offset measurement
                TTF_Font *font = state->rendererData.fonts[font_id];
                TTF_SetFontSize(font, font_size);

                for (size_t i = cache->top_vline; i < end_vline; i++) {
                    VisualLine *vl = &cache->lines[i];
                    size_t line_start = vl->byte_start;
                    size_t line_end = vl->byte_end;
                    size_t line_len = line_end - line_start;

                    // Determine if cursor is on this line
                    bool cursor_on_line = (point >= line_start && point <= line_end);

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
                        }
                    }) {
                        // Render cursor as floating element (doesn't affect text layout)
                        if (cursor_on_line && pane->content.active) {
                            Cursor(state, (int32_t)i, cursor_offset, line_height,
                                   font_id, font_size);
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
    pane_split_vertical(pane_get_active());

    message_send("split-vertical");
    return SEXP_VOID;
}

sexp scm_split_horizontal(sexp ctx, sexp self, sexp n) {
    G->needs_redraw = true;
    G->needs_extra_frame = true;
    pane_split_horizontal(pane_get_active());

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
