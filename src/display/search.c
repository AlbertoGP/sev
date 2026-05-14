#include "search.h"

#include <string.h>

#include "icon.h"
#include "pane.h"
#include "text_surface.h"
#include "theme.h"
#include "tooltip.h"
#include "../command/keymap.h"
#include "../command/scheme_internal.h"
#include "../text/buffer.h"

extern KeyEvent last_event;  // defined in command/keymap.c

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

void SearchBar(AppState *state, Pane *pane, int32_t index) {
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
