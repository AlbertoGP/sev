#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "buf_render.h"
#include "cursor.h"
#include "pane.h"
#include "tab.h"
#include "theme.h"
#include "vline.h"
#include "../clay/renderer.h"
#include "../text/buffer_type.h"
#include "../text/line.h"

// --- Shared rendering context ---
//
// Populated at the start of BufferContentRender and passed by pointer to all
// helper functions, avoiding large parameter lists.

typedef struct {
    AppState        *state;
    ContentPane     *cp;
    Tab             *tab;
    int32_t          index;
    Buffer          *buf;
    char            *chars;
    size_t           point;
    const LineTable *lt;
    VLineCache      *cache;
    uint16_t         font_id;
    uint16_t         font_size;
    float            padding;
    TTF_Font        *font;
    int              line_num_type;     // 0=off 1=abs 2=rel 3=visual
    float            gutter_width;
    size_t           cursor_logical_line;
    char            *lnum_strs;         // slab for formatted line number strings
    Clay_BoundingBox box;
    float            text_height;
    int              line_height;
    size_t           first_vline;
    size_t           end_vline;
    Clay_ElementId   outer_id;
    Clay_ElementId   track_id;
    Clay_ElementId   content_pane_id;
    int32_t          run_id;            // monotonically increasing Clay segment ID
} BufRenderCtx;

// --- File-local state ---

static Clay_Sizing tab_layout_expand = {
    .width  = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0)
};

static ScissoredRectData sel_pool[SCISSORED_RECT_POOL_SIZE];
static int sel_pool_idx = 0;

#define LNUM_STR_LEN 12

void buf_render_reset(void) { sel_pool_idx = 0; }

// --- Static helpers ---

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

// Emit one syntax-highlighted text segment into the current Clay VLine container.
// Relies on `ctx` being in scope at the call site.
#define EMIT_RUN(from, to, role_expr) do { \
    TextStyle _style = ui_resolve_text_style(ctx->state, (role_expr), \
                                             FONT_BUF_NORMAL, 15); \
    TTF_Font *_sfont = SDL_Clay_GetRenderFont(&ctx->state->rendererData, \
                                              _style.font_id, (float)ctx->font_size); \
    int _wx = 0, _wh = 0; \
    TTF_GetStringSize(_sfont, ctx->chars + (from), (to) - (from), &_wx, &_wh); \
    float _sw = (float)_wx; \
    if (_sw > 0.0f) { \
        Clay_String _t = { .chars = ctx->chars + (from), .length = (to) - (from) }; \
        CLAY(CLAY_IDI_LOCAL("Seg", ctx->run_id++), { \
            .layout = { .sizing = { \
                .width  = CLAY_SIZING_FIXED(_sw), \
                .height = CLAY_SIZING_FIXED(ctx->line_height) \
            }}, \
        }) { \
            CLAY_TEXT(_t, CLAY_TEXT_CONFIG({ \
                .fontId    = _style.font_id, \
                .fontSize  = ctx->font_size, \
                .textColor = _style.color, \
                .wrapMode  = CLAY_TEXT_WRAP_NONE \
            })); \
        } \
    } \
} while(0)

// --- Component functions ---

// Returns the fractional pixel sub-line offset for smooth scrolling.
// Computed before the outer CLAY() container so it can be passed to .clip.
static float BufRender_ComputeSubOffset(BufRenderCtx *ctx) {
    float sub_offset = 0.0f;
    int lh = vline_get_line_height(&ctx->state->rendererData,
                                   ctx->font_id, ctx->font_size);
    VLineCache *vc = ctx->cache;
    if (lh > 0 && vc->count > 0) {
        size_t first = (size_t)(vc->scroll_offset / lh);
        sub_offset = vc->scroll_offset - (float)first * (float)lh;
    }
    return sub_offset;
}

// Populates all geometry fields of ctx from Clay element data.
// Returns false (and sets needs_extra_frame) if layout data isn't ready yet.
static bool BufRender_SetupGeometry(BufRenderCtx *ctx, Clay_ElementId id) {
    Clay_ElementData data = Clay_GetElementData(id);
    if (!data.found || data.boundingBox.width <= 0) {
        ctx->state->needs_extra_frame = true;
        return false;
    }
    Clay_BoundingBox box = data.boundingBox;
    float text_width  = box.width - (2 * ctx->padding);
    float text_height = box.height;
    ctx->box         = box;
    ctx->text_height = text_height;

    TTF_Font *font = SDL_Clay_GetRenderFont(&ctx->state->rendererData,
                                            ctx->font_id, (float)ctx->font_size);
    ctx->font = font;

    // Determine line-number display mode from buffer-local variable.
    sexp lnum_sym = sexp_intern(ctx->state->chibi.ctx, "display-line-numbers-type", -1);
    sexp lnum_val = vartable_get(buffer_get_locals(ctx->buf), lnum_sym, SEXP_FALSE);
    int line_num_type = 0;
    if (lnum_val == SEXP_TRUE) {
        line_num_type = 1;
    } else if (sexp_symbolp(lnum_val)) {
        sexp c = ctx->state->chibi.ctx;
        if      (lnum_val == sexp_intern(c, "relative", -1)) line_num_type = 2;
        else if (lnum_val == sexp_intern(c, "visual",   -1)) line_num_type = 3;
    }
    ctx->line_num_type = line_num_type;

    // Measure gutter width for line numbers.
    float gutter_width = 0;
    if (line_num_type) {
        char num_buf[20];
        int ndigits = snprintf(num_buf, sizeof(num_buf), "%zu", ctx->lt->count);
        if (ndigits < 3) ndigits = 3;
        memset(num_buf, '0', ndigits);
        num_buf[ndigits] = '\0';
        int w = 0, h = 0;
        TTF_GetStringSize(font, num_buf, ndigits, &w, &h);
        gutter_width = (float)w + 16.0f * ctx->state->ui.scale_factor;
    }
    ctx->gutter_width = gutter_width;

    VLineCache *cache = ctx->cache;
    vline_rebuild(cache, ctx->buf, &ctx->state->rendererData,
                  text_width - gutter_width, ctx->font_id, ctx->font_size);

    int line_height  = vline_get_line_height(&ctx->state->rendererData,
                                             ctx->font_id, ctx->font_size);
    ctx->line_height = line_height;

    // Store geometry in ContentPane for mouse hit-testing.
    ContentPane *cp      = ctx->cp;
    cp->text_origin_x    = box.x + ctx->padding + gutter_width;
    cp->text_origin_y    = box.y + 2;
    cp->text_origin_w    = text_width - gutter_width;
    cp->text_origin_h    = text_height;
    cp->gutter_width_px  = gutter_width;
    cp->line_height_px   = line_height;
    cp->render_font_id   = ctx->font_id;
    cp->render_font_size = ctx->font_size;

    Clay_ElementData outer_data = Clay_GetElementData(ctx->outer_id);
    if (outer_data.found) {
        cp->pane_left  = outer_data.boundingBox.x;
        cp->pane_right = outer_data.boundingBox.x + outer_data.boundingBox.width;
    }
    Clay_ElementData cpane_data = Clay_GetElementData(ctx->content_pane_id);
    if (cpane_data.found) {
        cp->pane_top    = cpane_data.boundingBox.y;
        cp->pane_bottom = cpane_data.boundingBox.y + cpane_data.boundingBox.height;
    }
    Clay_ElementData track_data = Clay_GetElementData(ctx->track_id);
    if (track_data.found)
        cp->scrollbar_x = track_data.boundingBox.x;
    cp->scrollbar_y       = box.y;
    cp->scrollbar_track_h = text_height;

    // Scroll to keep cursor visible when this pane is active.
    size_t point = ctx->point;
    if (cp->active && point != cache->last_scroll_point) {
        cache->last_scroll_point = point;
        if (line_height > 0)
            vline_scroll_to_cursor_pixels(cache, point, text_height, line_height, 3);
    }

    float max_scroll = (cache->count > 1)
        ? (float)(cache->count - 1) * line_height : 0.0f;
    cache->scroll_offset = fmaxf(0.0f, fminf(cache->scroll_offset, max_scroll));

    size_t first_vline = 0;
    if (line_height > 0)
        first_vline = (size_t)(cache->scroll_offset / line_height);
    ctx->first_vline = first_vline;

    size_t visible_count = line_height > 0
        ? (size_t)(text_height / line_height) + 2 : 1;
    size_t end_vline = first_vline + visible_count;
    if (end_vline > cache->count) end_vline = cache->count;
    ctx->end_vline = end_vline;

    ctx->cursor_logical_line = 0;
    if (line_num_type)
        ctx->cursor_logical_line = line_index_at(ctx->lt, point);

    ctx->lnum_strs = NULL;
    if (line_num_type) {
        ctx->lnum_strs = malloc((end_vline - first_vline) * LNUM_STR_LEN);
        tab_register_string(ctx->lnum_strs);
    }

    return true;
}

// Centered "Loading..." placeholder shown while Clay layout data isn't ready.
static void BufRender_LoadingPlaceholder(BufRenderCtx *ctx) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = tab_layout_expand,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        }
    }) {
        CLAY_TEXT(CLAY_STRING("Loading..."),
            CLAY_TEXT_CONFIG({
                .fontId        = FONT_BUF_NORMAL,
                .fontSize      = 16,
                .textColor     = ui_resolve_color(ctx->state,
                                     ctx->state->ui.roles.text_faded),
                .textAlignment = CLAY_TEXT_ALIGN_CENTER
            }));
    }
}

// Renders the line-number gutter cell for visual line i.
static void BufRender_GutterCell(BufRenderCtx *ctx, size_t i, VisualLine *vl) {
    if (!ctx->line_num_type || !ctx->lnum_strs) return;

    size_t str_idx    = i - ctx->first_vline;
    char  *str        = ctx->lnum_strs + str_idx * LNUM_STR_LEN;
    size_t slen       = 0;
    bool   show_number = true;

    if (ctx->line_num_type == 3) {
        slen = snprintf(str, LNUM_STR_LEN, "%zu", i - ctx->first_vline + 1);
    } else if (vl->visual_index > 0) {
        show_number = false;
    } else {
        size_t logical = line_index_at(ctx->lt, vl->byte_start);
        if (ctx->line_num_type == 1) {
            slen = snprintf(str, LNUM_STR_LEN, "%zu", logical + 1);
        } else {
            size_t rel = (logical > ctx->cursor_logical_line)
                ? logical - ctx->cursor_logical_line
                : ctx->cursor_logical_line - logical;
            slen = snprintf(str, LNUM_STR_LEN, "%zu",
                            rel == 0 ? logical + 1 : rel);
        }
    }

    float gutter_pad = 8.0f * ctx->state->ui.scale_factor;
    CLAY(CLAY_IDI_LOCAL("LineNum", (int32_t)i), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_FIXED(ctx->gutter_width),
                .height = CLAY_SIZING_FIXED(ctx->line_height)
            },
            .padding        = { .left = gutter_pad, .right = gutter_pad },
            .childAlignment = { .x = CLAY_ALIGN_X_RIGHT },
        },
    }) {
        if (show_number && slen > 0) {
            bool is_current = (ctx->line_num_type != 3) &&
                (line_index_at(ctx->lt, vl->byte_start) == ctx->cursor_logical_line);
            Clay_String numStr = { .chars = str, .length = slen };
            CLAY_TEXT(numStr, CLAY_TEXT_CONFIG({
                .fontId    = ctx->font_id,
                .fontSize  = ctx->font_size,
                .textColor = ui_resolve_color(ctx->state,
                    is_current && ctx->cp->active
                        ? ctx->state->ui.roles.text_primary
                        : ctx->state->ui.roles.text_faded),
            }));
        }
    }
}

// Renders the cursor overlay for the current line.
// Determines the correct font variant from hl_spans before calling Cursor().
static void BufRender_CursorCell(BufRenderCtx *ctx, size_t i, float cursor_offset) {
    uint16_t cursor_font_id = ctx->font_id;
    size_t pt_line_idx      = line_index_at(ctx->lt, ctx->point);
    const Line *pt_line     = &ctx->lt->lines[pt_line_idx];
    for (uint32_t si = 0; si < pt_line->hl_span_count; si++) {
        const HLSpan *sp = &pt_line->hl_spans[si];
        if ((uint32_t)ctx->point >= sp->start_byte &&
            (uint32_t)ctx->point <  sp->end_byte) {
            TextStyle ts = ui_resolve_text_style(
                ctx->state, hl_kind_to_role(ctx->state, sp->style),
                ctx->font_id, ctx->font_size);
            cursor_font_id = ts.font_id;
            break;
        }
    }
    Cursor(ctx->state, (int32_t)i, cursor_offset + ctx->gutter_width,
           ctx->line_height,
           ctx->box.x, ctx->box.y, ctx->box.width, ctx->text_height,
           cursor_font_id, ctx->font_size);
}

// Renders the selection highlight overlay for the current line.
// Handles SELECT_REGULAR, SELECT_LINE, and SELECT_RECTANGLE modes.
static void BufRender_SelectionCell(BufRenderCtx *ctx, size_t i, VisualLine *vl) {
    if (ctx->buf->select_mode == SELECT_NONE || !ctx->cp->active) return;

    size_t line_start = vl->byte_start;
    size_t line_end   = vl->byte_end;
    size_t sel_a      = ctx->buf->select_start.pos;
    size_t sel_b      = ctx->point;
    size_t sel_min    = sel_a < sel_b ? sel_a : sel_b;
    size_t sel_max    = sel_a > sel_b ? sel_a + 1 : sel_b + 1;

    size_t hl_start    = 0, hl_end = 0;
    bool   do_highlight = false;

    switch (ctx->buf->select_mode) {
    case SELECT_REGULAR: {
        if (line_start < sel_max && line_end >= sel_min) {
            hl_start     = sel_min > line_start ? sel_min : line_start;
            hl_end       = sel_max < line_end   ? sel_max : line_end;
            do_highlight = true;
        }
        break;
    }
    case SELECT_LINE: {
        size_t line_min   = line_index_at(ctx->lt, sel_min);
        size_t line_max   = line_index_at(ctx->lt, sel_max > 0 ? sel_max - 1 : 0);
        size_t vl_logical = line_index_at(ctx->lt, vl->byte_start);
        if (vl_logical >= line_min && vl_logical <= line_max) {
            hl_start     = line_start;
            hl_end       = line_end;
            do_highlight = true;
        }
        break;
    }
    case SELECT_RECTANGLE_RAGGED:
    case SELECT_RECTANGLE: {
        size_t line_a  = line_index_at(ctx->lt, sel_a);
        size_t line_b  = line_index_at(ctx->lt, sel_b);
        size_t row_min = line_a < line_b ? line_a : line_b;
        size_t row_max = line_a > line_b ? line_a : line_b;
        size_t col_a   = sel_a - ctx->lt->lines[line_a].start;
        size_t col_b   = sel_b - ctx->lt->lines[line_b].start;
        size_t col_min = col_a < col_b ? col_a : col_b;
        size_t col_max = col_a > col_b ? col_a : col_b;

        size_t vl_logical = line_index_at(ctx->lt, vl->byte_start);
        if (vl_logical >= row_min && vl_logical <= row_max) {
            size_t log_start = ctx->lt->lines[vl_logical].start;
            size_t log_end   = ctx->lt->lines[vl_logical].end;
            size_t hs        = log_start + col_min;
            size_t he;
            if (ctx->buf->select_mode == SELECT_RECTANGLE_RAGGED) {
                he = log_end;
                if (he > log_start && ctx->chars[he - 1] == '\n') he--;
            } else {
                he = log_start + col_max + 1;
            }
            if (he > log_end)    he = log_end;
            if (hs > log_end)    hs = log_end;
            if (hs < line_start) hs = line_start;
            if (he > line_end)   he = line_end;
            if (he > hs) {
                hl_start     = hs;
                hl_end       = he;
                do_highlight = true;
            }
        }
        break;
    }
    default: break;
    }

    if (!do_highlight || hl_end <= hl_start) return;

    float sel_x = 0;
    if (hl_start > line_start) {
        int w = 0, h = 0;
        TTF_GetStringSize(ctx->font, ctx->chars + line_start,
                          hl_start - line_start, &w, &h);
        sel_x = (float)w;
    }
    int sw = 0, sh = 0;
    TTF_GetStringSize(ctx->font, ctx->chars + hl_start,
                      hl_end - hl_start, &sw, &sh);
    float sel_w = (float)sw;
    if (sel_w > 0 && sel_pool_idx < SCISSORED_RECT_POOL_SIZE) {
        ScissoredRectData *s = &sel_pool[sel_pool_idx++];
        s->type   = CUSTOM_TYPE_SCISSORED_RECT;
        s->clip_x = ctx->box.x;
        s->clip_y = ctx->box.y;
        s->clip_w = ctx->box.width;
        s->clip_h = ctx->text_height;
        s->color  = ui_resolve_color(ctx->state, ctx->state->ui.roles.selection);
        CLAY(CLAY_IDI_LOCAL("Sel", (int32_t)i), {
            .floating = {
                .attachTo = CLAY_ATTACH_TO_PARENT,
                .offset   = { .x = sel_x + ctx->gutter_width, .y = 0 }
            },
            .layout = {
                .sizing = {
                    .width  = CLAY_SIZING_FIXED(sel_w),
                    .height = CLAY_SIZING_FIXED(ctx->line_height)
                }
            },
            .custom = { .customData = s },
        }) {}
    }
}

// Renders syntax-highlighted text segments for one visual line.
static void BufRender_TextCell(BufRenderCtx *ctx, VisualLine *vl) {
    size_t line_start = vl->byte_start;
    size_t line_end   = vl->byte_end;
    if (line_end == line_start) return;

    size_t log_idx       = line_index_at(ctx->lt, vl->byte_start);
    const Line *log_line = &ctx->lt->lines[log_idx];
    size_t pos           = line_start;

    for (uint32_t si = 0; si < log_line->hl_span_count; si++) {
        const HLSpan *span = &log_line->hl_spans[si];
        if ((size_t)span->end_byte   <= line_start) continue;
        if ((size_t)span->start_byte >= line_end)   break;
        size_t seg_s = (size_t)span->start_byte > line_start
                     ? (size_t)span->start_byte : line_start;
        size_t seg_e = (size_t)span->end_byte < line_end
                     ? (size_t)span->end_byte  : line_end;
        if (pos < seg_s)
            EMIT_RUN(pos, seg_s, ctx->state->ui.roles.text_primary);
        EMIT_RUN(seg_s, seg_e, hl_kind_to_role(ctx->state, span->style));
        pos = seg_e;
    }
    if (pos < line_end)
        EMIT_RUN(pos, line_end, ctx->state->ui.roles.text_primary);
}

// Renders a single visual line: gutter cell, cursor overlay, selection overlay,
// and syntax-highlighted text.
static void BufRender_VLine(BufRenderCtx *ctx, size_t i) {
    VisualLine *vl    = &ctx->cache->lines[i];
    size_t line_start = vl->byte_start;
    size_t line_end   = vl->byte_end;

    bool cursor_on_line = (ctx->point >= line_start) && (ctx->point < line_end)
                          && !ctx->state->minibuf.active;
    bool is_last = (i + 1 == ctx->cache->count) ||
                   (ctx->cache->lines[i + 1].line_id != vl->line_id);
    if (!cursor_on_line && ctx->point == line_end && is_last) {
        if (i + 1 == ctx->cache->count || ctx->cache->lines[i + 1].byte_start != ctx->point)
            cursor_on_line = true;
    }

    float cursor_offset = 0;
    if (cursor_on_line) {
        size_t head_len = ctx->point - line_start;
        if (head_len > 0) {
            int w = 0, h = 0;
            TTF_GetStringSize(ctx->font, ctx->chars + line_start, head_len, &w, &h);
            cursor_offset = (float)w;
        }
    }

    CLAY(CLAY_IDI_LOCAL("VLine", (int32_t)i), {
        .layout = {
            .sizing = {
                .width  = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED(ctx->line_height)
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        }
    }) {
        BufRender_GutterCell(ctx, i, vl);
        if (cursor_on_line && ctx->cp->active)
            BufRender_CursorCell(ctx, i, cursor_offset);
        BufRender_SelectionCell(ctx, i, vl);
        BufRender_TextCell(ctx, vl);
    }
}

// Renders the scrollbar track and thumb alongside the buffer text area.
static void BufRender_Scrollbar(BufRenderCtx *ctx) {
    float bar_w     = 12.0f * ctx->state->ui.scale_factor;
    ContentPane *cp = ctx->cp;
    CLAY(ctx->track_id, {
        .layout = {
            .sizing = { .width = CLAY_SIZING_FIXED(bar_w), .height = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ui_resolve_color(ctx->state, ctx->state->ui.roles.bar_bg)
    }) {
        cp->has_scrollbar = (cp->line_height_px > 0 &&
                             ctx->tab->content.buffer.vline_cache.count > 1);
        if (cp->has_scrollbar) {
            VLineCache *tc = &ctx->tab->content.buffer.vline_cache;
            float th       = cp->text_origin_h;
            float ms       = (float)(tc->count - 1) * cp->line_height_px;
            float total    = th + ms;
            float min_th   = fminf(20.0f * ctx->state->ui.scale_factor, th);
            float thumb_h  = fmaxf((th / total) * th, min_th);
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
                .backgroundColor = Clay_Hovered() && !ctx->state->input.mouse_button_down
                    ? ui_resolve_color(ctx->state, ctx->state->ui.roles.scrollbar_hover)
                    : ui_resolve_color(ctx->state, ctx->state->ui.roles.scrollbar)
            }) {}
        }
    }
}

// --- Public entry point ---

void BufferContentRender(AppState *state, ContentPane *cp, Tab *tab, int32_t index,
                         Clay_ElementId container_id) {
    if (!tab) return;
    Buffer *buf = tab->content.buffer.buffer;
    if (!buf) return;

    char *chars = buffer_text(buf);
    tab_register_string(chars);

    BufRenderCtx ctx = {
        .state     = state,
        .cp        = cp,
        .tab       = tab,
        .index     = index,
        .buf       = buf,
        .chars     = chars,
        .point     = point_get(buf).pos,
        .lt        = buffer_get_line_table(buf),
        .cache     = &tab->content.buffer.vline_cache,
        .font_id   = FONT_BUF_NORMAL,
        .font_size = (uint16_t)(15 * state->ui.scale_factor * buffer_get_scale(buf)),
        .padding   = 24.0f * state->ui.scale_factor,
    };

    Clay_ElementId outer_id = CLAY_IDI_LOCAL("BufWrap", index);
    Clay_ElementId track_id = CLAY_IDI_LOCAL("ScrollTrack", index);
    ctx.outer_id        = outer_id;
    ctx.track_id        = track_id;
    ctx.content_pane_id = container_id;

    float sub_offset = BufRender_ComputeSubOffset(&ctx);

    CLAY(outer_id, {
        .layout = { .sizing = tab_layout_expand, .layoutDirection = CLAY_LEFT_TO_RIGHT }
    }) {
        Clay_ElementId id = CLAY_IDI_LOCAL("Buffer Text", index);
        CLAY(id, {
            .layout = {
                .sizing           = tab_layout_expand,
                .padding          = { .left = ctx.padding, .right = ctx.padding,
                                      .top = 2, .bottom = 2 },
                .layoutDirection  = CLAY_TOP_TO_BOTTOM,
            },
            .clip = { .vertical = true, .childOffset = { .x = 0, .y = -sub_offset } }
        }) {
            if (BufRender_SetupGeometry(&ctx, id)) {
                for (size_t i = ctx.first_vline; i < ctx.end_vline; i++)
                    BufRender_VLine(&ctx, i);
            } else {
                BufRender_LoadingPlaceholder(&ctx);
            }
        }

        BufRender_Scrollbar(&ctx);
    }
}
