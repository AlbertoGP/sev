#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cursor.h"
#include "icon.h"
#include "pane.h"
#include "tab.h"
#include "../text/buffer_type.h"
#include "../text/line.h"
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
    update_window_title();
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
        // No panes yet: create the root PANE_CONTENT.
        Pane *p = pane_content_create(buf);
        if (!p) return false;
        // pane_content_create doesn't set root_pane; we do it via replace_child(NULL,NULL,p).
        pane_replace_child(NULL, NULL, p);
        p->content.active = true;
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
        // Temporarily mark it active so pane_close() finds it.
        // First clear the real active pane's flag to avoid pane_get_active()
        // finding the wrong pane (e.g. the left sibling) via depth-first search.
        bool was_active = dp->content.active;
        if (!was_active) {
            Pane *current = pane_get_active();
            if (current) current->content.active = false;
        }
        dp->content.active = true;
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
    dp->content.active_tab = t;
    if (!dp->content.active) pane_set_active(dp);
    else sync_active_buffer();
    update_window_title();
}

// Static storage for close-button callback data.
// We need (Pane*, Tab*) pairs — one per tab rendered per frame.
#define MAX_TAB_CB_DATA 64
static void *tab_cb_data[MAX_TAB_CB_DATA][2];
static int   tab_cb_count = 0;

static ScissoredRectData sel_pool[SCISSORED_RECT_POOL_SIZE];
static int sel_pool_idx = 0;

void tab_cb_reset(void) { tab_cb_count = 0; sel_pool_idx = 0; }

// String pool for buffer text and line-number allocations made during rendering.
#define TAB_STRINGS_MAX 32
static char *tab_strings[TAB_STRINGS_MAX];
static int   tab_strings_count = 0;

void tab_free_strings(void) {
    for (int i = 0; i < tab_strings_count; i++)
        free(tab_strings[i]);
    tab_strings_count = 0;
}

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

// --- BufferContent rendering ---

static sexp hl_kind_to_role(AppState *state, uint16_t kind) {
    switch ((HLKind)kind) {
    case HL_KEYWORD:  return state->ui.roles.hl_keyword;
    case HL_STRING:   return state->ui.roles.hl_string;
    case HL_COMMENT:  return state->ui.roles.hl_comment;
    case HL_NUMBER:   return state->ui.roles.hl_number;
    case HL_CONSTANT: return state->ui.roles.hl_constant;
    case HL_FUNCTION: return state->ui.roles.hl_function;
    case HL_BUILTIN:  return state->ui.roles.hl_builtin;
    case HL_OPERATOR: return state->ui.roles.hl_operator;
    case HL_BRACKET:  return state->ui.roles.hl_bracket;
    default:          return state->ui.roles.text_primary;
    }
}

static Clay_Sizing tab_layout_expand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

void BufferContentRender(AppState *state, ContentPane *cp, Tab *tab, int32_t index) {
    if (!tab) return;
    Buffer *buf = tab->content.buffer.buffer;
    if (!buf) return;

    char *chars = buffer_text(buf);
    if (tab_strings_count < TAB_STRINGS_MAX)
        tab_strings[tab_strings_count++] = chars;
    size_t point = point_get(buf).pos;

    const uint16_t font_id   = FONT_BUF_NORMAL;
    const uint16_t font_size = 15 * state->ui.scale_factor * buffer_get_scale(buf);
    const float padding      = 24.0f * state->ui.scale_factor;
    float sub_offset         = 0.0f;

    Clay_ElementId outer_id = CLAY_IDI_LOCAL("BufWrap", index);
    Clay_ElementId track_id = CLAY_IDI_LOCAL("ScrollTrack", index);
    float bar_w = 12.0f * state->ui.scale_factor;

    // Compute sub-line pixel offset early (before CLAY() config is evaluated).
    // vline_get_line_height is a cheap cached lookup.
    {
        int _lh = vline_get_line_height(&state->rendererData, font_id, font_size);
        VLineCache *_vc = &tab->content.buffer.vline_cache;
        if (_lh > 0 && _vc->count > 0) {
            size_t _first = (size_t)(_vc->scroll_offset / _lh);
            sub_offset = _vc->scroll_offset - (float)_first * (float)_lh;
        }
    }

    CLAY(outer_id, {
        .layout = { .sizing = tab_layout_expand, .layoutDirection = CLAY_LEFT_TO_RIGHT }
    }) {

    Clay_ElementId id = CLAY_IDI_LOCAL("Buffer Text", index);
    CLAY(id, {
        .layout = {
            .sizing = tab_layout_expand,
            .padding = { .left = padding, .right = padding, .top = 2, .bottom = 2 },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .clip = { .vertical = true, .childOffset = { .x = 0, .y = -sub_offset } }
    }) {
        Clay_ElementData data = Clay_GetElementData(id);
        bool data_valid = data.found && data.boundingBox.width > 0;
        if (!data_valid) state->needs_extra_frame = true;
        if (data_valid) {
            Clay_BoundingBox box = data.boundingBox;
            float text_width  = box.width - (2 * padding);
            float text_height = box.height;

            TTF_Font *font = SDL_Clay_GetRenderFont(&state->rendererData, font_id, (float)font_size);

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

            VLineCache *cache = &tab->content.buffer.vline_cache;
            float wrap_width = text_width - gutter_width;
            vline_rebuild(cache, buf, &state->rendererData, wrap_width, font_id, font_size);

            int line_height = vline_get_line_height(&state->rendererData, font_id, font_size);

            // Store geometry in the content pane for mouse hit-testing.
            cp->text_origin_x   = box.x + padding + gutter_width;
            cp->text_origin_y   = box.y + 2;
            cp->text_origin_w   = text_width - gutter_width;
            cp->text_origin_h   = text_height;
            cp->gutter_width_px = gutter_width;
            cp->line_height_px  = line_height;
            cp->render_font_id   = font_id;
            cp->render_font_size = font_size;

            Clay_ElementData outer_data = Clay_GetElementData(outer_id);
            if (outer_data.found)
                cp->pane_right = outer_data.boundingBox.x + outer_data.boundingBox.width;
            Clay_ElementData track_data = Clay_GetElementData(track_id);
            if (track_data.found)
                cp->scrollbar_x = track_data.boundingBox.x;
            cp->scrollbar_y     = box.y;
            cp->scrollbar_track_h = text_height;

            if (cp->active && point != cache->last_scroll_point) {
                cache->last_scroll_point = point;
                if (line_height > 0)
                    vline_scroll_to_cursor_pixels(cache, point, text_height, line_height, 3);
            }

            float max_scroll = (cache->count > 1)
                ? (float)(cache->count - 1) * line_height : 0.0f;
            cache->scroll_offset = fmaxf(0.0f, fminf(cache->scroll_offset, max_scroll));

            size_t first_vline = 0;
            float sub_offset = 0.0f;
            if (line_height > 0) {
                first_vline = (size_t)(cache->scroll_offset / line_height);
                sub_offset  = cache->scroll_offset - (float)first_vline * (float)line_height;
            }

            size_t visible_count = line_height > 0 ? (size_t)(text_height / line_height) + 2 : 1;
            size_t end_vline = first_vline + visible_count;
            if (end_vline > cache->count) end_vline = cache->count;

            size_t cursor_logical_line = 0;
            if (line_num_type)
                cursor_logical_line = line_index_at(lt, point);

            #define LNUM_STR_LEN 12
            char *lnum_strs = NULL;
            if (line_num_type) {
                size_t num_visible = end_vline - first_vline;
                lnum_strs = malloc(num_visible * LNUM_STR_LEN);
                if (tab_strings_count < TAB_STRINGS_MAX)
                    tab_strings[tab_strings_count++] = lnum_strs;
            }

            int32_t run_id = 0;
            for (size_t i = first_vline; i < end_vline; i++) {
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
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    }
                }) {
                    if (line_num_type && lnum_strs) {
                        size_t str_idx = i - first_vline;
                        char *str = lnum_strs + str_idx * LNUM_STR_LEN;
                        size_t slen = 0;
                        bool show_number = true;

                        if (line_num_type == 3) {
                            slen = snprintf(str, LNUM_STR_LEN, "%zu", i - first_vline + 1);
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
                                        is_current && cp->active
                                            ? state->ui.roles.text_primary
                                            : state->ui.roles.text_faded),
                                }));
                            }
                        }
                    }

                    if (cursor_on_line && cp->active) {
                        // Find the font for the character under the cursor by
                        // looking up which hl_span covers point.
                        uint16_t cursor_font_id = font_id;
                        size_t pt_line_idx = line_index_at(lt, point);
                        const Line *pt_line = &lt->lines[pt_line_idx];
                        for (uint32_t si = 0; si < pt_line->hl_span_count; si++) {
                            const HLSpan *sp = &pt_line->hl_spans[si];
                            if ((uint32_t)point >= sp->start_byte &&
                                (uint32_t)point <  sp->end_byte) {
                                TextStyle ts = ui_resolve_text_style(
                                    state, hl_kind_to_role(state, sp->style),
                                    font_id, font_size);
                                cursor_font_id = ts.font_id;
                                break;
                            }
                        }
                        Cursor(state, (int32_t)i, cursor_offset + gutter_width,
                               line_height,
                               box.x, box.y, box.width, text_height,
                               cursor_font_id, font_size);
                    }

                    if (buf->select_mode != SELECT_NONE && cp->active) {
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
                            if (sel_w > 0 && sel_pool_idx < SCISSORED_RECT_POOL_SIZE) {
                                ScissoredRectData *s = &sel_pool[sel_pool_idx++];
                                s->type  = CUSTOM_TYPE_SCISSORED_RECT;
                                s->clip_x = box.x;
                                s->clip_y = box.y;
                                s->clip_w = box.width;
                                s->clip_h = text_height;
                                s->color = ui_resolve_color(state, state->ui.roles.selection);
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
                                    .custom = { .customData = s },
                                }) {}
                            }
                        }
                    }

                    if (line_len > 0) {
                        size_t log_idx = line_index_at(lt, vl->byte_start);
                        const Line *log_line = &lt->lines[log_idx];
                        size_t pos = line_start;

                        // Measure each segment with its own font so that the
                        // Clay container width matches what is actually rendered.
                        // Using the base (regular) font for all segments was wrong:
                        // italic glyphs (e.g. 'f', '/') are wider, causing Clay to
                        // word-wrap the overflow text onto a second row.
                        #define EMIT_RUN(from, to, role_expr) do { \
                            TextStyle style = ui_resolve_text_style(state, (role_expr), FONT_BUF_NORMAL, 15); \
                            TTF_Font *_sfont = SDL_Clay_GetRenderFont(&state->rendererData, style.font_id, (float)font_size); \
                            int _wx = 0, _wh = 0; \
                            TTF_GetStringSize(_sfont, chars + (from), (to) - (from), &_wx, &_wh); \
                            float _sw = (float)_wx; \
                            if (_sw > 0.0f) { \
                                Clay_String _t = { .chars = chars + (from), \
                                                   .length = (to) - (from) }; \
                                CLAY(CLAY_IDI_LOCAL("Seg", run_id++), { \
                                    .layout = { .sizing = { \
                                        .width  = CLAY_SIZING_FIXED(_sw), \
                                        .height = CLAY_SIZING_FIXED(line_height) \
                                    }}, \
                                }) { \
                                    CLAY_TEXT(_t, CLAY_TEXT_CONFIG({ \
                                        .fontId    = style.font_id, \
                                        .fontSize  = font_size, \
                                        .textColor = style.color, \
                                        .wrapMode  = CLAY_TEXT_WRAP_NONE \
                                    })); \
                                } \
                            } \
                        } while(0)

                        for (uint32_t si = 0; si < log_line->hl_span_count; si++) {
                            const HLSpan *span = &log_line->hl_spans[si];
                            if ((size_t)span->end_byte   <= line_start) continue;
                            if ((size_t)span->start_byte >= line_end)   break;
                            size_t seg_s = (size_t)span->start_byte > line_start
                                         ? (size_t)span->start_byte : line_start;
                            size_t seg_e = (size_t)span->end_byte   < line_end
                                         ? (size_t)span->end_byte   : line_end;

                            if (pos < seg_s)
                                EMIT_RUN(pos, seg_s, state->ui.roles.text_primary);
                            EMIT_RUN(seg_s, seg_e,
                                     hl_kind_to_role(state, span->style));
                            pos = seg_e;
                        }

                        if (pos < line_end)
                            EMIT_RUN(pos, line_end, state->ui.roles.text_primary);

                        #undef EMIT_RUN
                    }
                }
            }

        } else {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = tab_layout_expand,
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER
                    }
                }
            }) {
                CLAY_TEXT(CLAY_STRING("Loading..."),
                    CLAY_TEXT_CONFIG({
                        .fontId = FONT_BUF_NORMAL,
                        .fontSize = 16,
                        .textColor = ui_resolve_color(state, state->ui.roles.text_faded),
                        .textAlignment = CLAY_TEXT_ALIGN_CENTER
                    }));
            }
        }
    }

    CLAY(track_id, {
        .layout = {
            .sizing = { .width = CLAY_SIZING_FIXED(bar_w), .height = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg)
    }) {
        cp->has_scrollbar = (cp->line_height_px > 0 &&
                             tab->content.buffer.vline_cache.count > 1);
        if (cp->has_scrollbar) {
            VLineCache *tc = &tab->content.buffer.vline_cache;
            float th      = cp->text_origin_h;
            float ms      = (float)(tc->count - 1) * cp->line_height_px;
            float total   = th + ms;
            float min_th  = fminf(20.0f * state->ui.scale_factor, th);
            float thumb_h = fmaxf((th / total) * th, min_th);
            float spacer_h = (ms > 0)
                ? (tc->scroll_offset / ms) * (th - thumb_h)
                : 0.0f;
            cp->scrollbar_thumb_h = thumb_h;
            cp->scrollbar_thumb_y = cp->scrollbar_y + spacer_h;
            CLAY_AUTO_ID({
                .layout = { .sizing = {
                    .width  = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIXED(spacer_h) } }
            }) {}
            CLAY_AUTO_ID({
                .layout = { .sizing = {
                    .width  = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_FIXED(thumb_h) } },
                .backgroundColor = Clay_Hovered() && !state->input.mouse_button_down
                        ? ui_resolve_color(state, state->ui.roles.scrollbar_hover)
                        : ui_resolve_color(state, state->ui.roles.scrollbar)
            }) {}
        }
    }

    } // close outer BufWrap
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
