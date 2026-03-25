#include <chibi/sexp.h>
#include <string.h>
#include <stdio.h>

#include "which_key.h"
#include "theme.h"
#include "../command/keymap.h"
#include "../command/scheme_internal.h"

#define WK_MAX 64
#define WK_KEY_SZ 64

static char wk_key_strs[WK_MAX][WK_KEY_SZ];
static char wk_label_strs[WK_MAX][64];
static bool wk_is_prefix[WK_MAX];
static int  wk_count;

static void key_event_to_str(char *buf, size_t sz, const KeyEvent *ev) {
    char tmp[32];
    size_t ti = 0;

    if (ev->mods & MOD_CTRL)  { tmp[ti++] = 'C'; tmp[ti++] = '-'; }
    if (ev->mods & MOD_META)  { tmp[ti++] = 'M'; tmp[ti++] = '-'; }
    if (ev->mods & MOD_SHIFT) { tmp[ti++] = 'S'; tmp[ti++] = '-'; }

    if (ev->type == KEYEVENT_SPECIAL) {
        const char *name;
        switch (ev->keycode) {
        case KEY_ESC:       name = "ESC";   break;
        case KEY_RETURN:    name = "RET";   break;
        case KEY_TAB:       name = "TAB";   break;
        case KEY_BACKSPACE: name = "BSP";   break;
        case KEY_DELETE:    name = "DEL";   break;
        case KEY_UP:        name = "UP";    break;
        case KEY_DOWN:      name = "DOWN";  break;
        case KEY_LEFT:      name = "LEFT";  break;
        case KEY_RIGHT:     name = "RIGHT"; break;
        default:            name = "?";     break;
        }
        snprintf(tmp + ti, sizeof(tmp) - ti, "%s", name);
    } else {
        if (ev->codepoint == (uint32_t)' ') {
            snprintf(tmp + ti, sizeof(tmp) - ti, "SPC");
        } else {
            tmp[ti++] = (char)ev->codepoint;
            tmp[ti]   = '\0';
        }
    }

    snprintf(buf, sz, "%s", tmp);
}

static void collect_entries(sexp ctx, Keymap *km, const char *prefix, int depth) {
    if (!km || wk_count >= WK_MAX || depth > 4) return;

    for (size_t i = 0; i < km->count && wk_count < WK_MAX; i++) {
        KeymapEntry *e = &km->entries[i];

        char key_str[32];
        key_event_to_str(key_str, sizeof(key_str), &e->key);

        char full_path[WK_KEY_SZ];
        if (prefix[0])
            snprintf(full_path, sizeof(full_path), "%s %s", prefix, key_str);
        else
            snprintf(full_path, sizeof(full_path), "%s", key_str);

        if (e->binding.type == BINDING_COMMAND) {
            snprintf(wk_key_strs[wk_count], sizeof(wk_key_strs[0]),
                     "%s", full_path);
            sexp name_sexp = sexp_symbol_to_string(ctx, e->binding.command_sym);
            const char *sym_name = sexp_string_data(name_sexp);
            snprintf(wk_label_strs[wk_count], sizeof(wk_label_strs[0]),
                     "%s", sym_name ? sym_name : "?");
            wk_is_prefix[wk_count] = false;
            wk_count++;
        } else if (e->binding.type == BINDING_KEYMAP) {
            Keymap *child = e->binding.keymap;
            if (child && child->name) {
                // Named prefix: show as collapsed entry
                snprintf(wk_key_strs[wk_count], sizeof(wk_key_strs[0]),
                         "%s", full_path);
                snprintf(wk_label_strs[wk_count], sizeof(wk_label_strs[0]),
                         "+%s", child->name);
                wk_is_prefix[wk_count] = true;
                wk_count++;
            } else {
                // Anonymous prefix: recurse as before
                collect_entries(ctx, child, full_path, depth + 1);
            }
        }
    }
}

void WhichKey(AppState *state) {
    wk_count = 0;
    collect_entries(state->chibi.ctx, state->which_key.keymap, "", 0);
    if (wk_count == 0) return;

    float scale = state->ui.scale_factor;
    uint16_t font_size = (uint16_t)(14.0f * scale);
    float pad = 8.0f * scale;

    CachedRoles roles = state->ui.roles;
    Clay_Color bg         = ui_resolve_color(state, roles.bar_bg);
    Clay_Color fg_header  = ui_resolve_color(state, roles.text_primary);
    Clay_Color fg_key     = ui_resolve_color(state, roles.text_key);
    Clay_Color fg_label   = ui_resolve_color(state, roles.text_command);
    Clay_Color fg_prefix  = ui_resolve_color(state, roles.text_prefix);
    Clay_Color border_col = ui_resolve_color(state, roles.border_inactive);

    CLAY(CLAY_ID("WhichKey"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                .parent  = CLAY_ATTACH_POINT_RIGHT_BOTTOM
            },
            .offset = { -16.0f * scale, -60.0f * scale },
            .zIndex = 100
        },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .left = pad, .right = pad, .top = pad, .bottom = pad },
            .childGap = (uint16_t)(2.0f * scale)
        },
        .backgroundColor = bg,
        .border = {
            .color = border_col,
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1 }
        }
    }) {
        // Header: show the accumulated prefix
        Clay_String header_cs = {
            .chars  = state->which_key.prefix_str,
            .length = (int32_t)strlen(state->which_key.prefix_str)
        };
        CLAY_TEXT(header_cs, CLAY_TEXT_CONFIG({
            .fontId    = FONT_BUF_NORMAL,
            .fontSize  = font_size,
            .textColor = fg_header,
            .wrapMode  = CLAY_TEXT_WRAP_NONE
        }));

        for (int i = 0; i < wk_count; i++) {
            Clay_String key_cs = {
                .chars  = wk_key_strs[i],
                .length = (int32_t)strlen(wk_key_strs[i])
            };
            Clay_String label_cs = {
                .chars  = wk_label_strs[i],
                .length = (int32_t)strlen(wk_label_strs[i])
            };

            CLAY(CLAY_IDI_LOCAL("WK_Row", i), {
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = (uint16_t)(8.0f * scale)
                }
            }) {
                CLAY(CLAY_IDI_LOCAL("WK_Key", i), {
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(60.0f * scale)
                        }
                    }
                }) {
                    CLAY_TEXT(key_cs, CLAY_TEXT_CONFIG({
                        .fontId    = FONT_BUF_NORMAL,
                        .fontSize  = font_size,
                        .textColor = fg_key,
                        .wrapMode  = CLAY_TEXT_WRAP_NONE
                    }));
                }
                CLAY_TEXT(label_cs, CLAY_TEXT_CONFIG({
                    .fontId    = FONT_BUF_NORMAL,
                    .fontSize  = font_size,
                    .textColor = wk_is_prefix[i] ? fg_prefix : fg_label,
                    .wrapMode  = CLAY_TEXT_WRAP_NONE
                }));
            }
        }
    }
}

sexp scm_which_key_toggle(sexp ctx, sexp self, sexp n) {
    G->which_key.enabled = !G->which_key.enabled;
    if (!G->which_key.enabled)
        G->which_key.active = false;
    G->needs_redraw = true;
    return SEXP_VOID;
}
