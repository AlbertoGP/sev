#include <chibi/sexp.h>
#include <string.h>
#include <stdio.h>

#include "which_key.h"
#include "keybinding.h"
#include "theme.h"
#include "../command/keymap.h"
#include "../command/scheme_internal.h"
#include "../command/message.h"

#define WK_MAX 64
#define WK_KEY_SZ 64

static char wk_key_strs[WK_MAX][WK_KEY_SZ];
static char wk_label_strs[WK_MAX][64];
static bool wk_is_prefix[WK_MAX];
static int  wk_count;

static void key_event_to_str(char *buf, size_t sz, const KeyEvent *ev) {
    char tmp[48];
    size_t ti = 0;

    if (ev->mods & MOD_CTRL)  { memcpy(tmp + ti, "ctrl-",  5); ti += 5; }
    if (ev->mods & MOD_META)  { memcpy(tmp + ti, "alt-",   4); ti += 4; }
    if (ev->mods & MOD_SHIFT) { memcpy(tmp + ti, "shift-", 6); ti += 6; }

    if (ev->type == KEYEVENT_SPECIAL) {
        const char *name;
        switch (ev->keycode) {
        case KEY_ESC:        name = "escape";    break;
        case KEY_RETURN:     name = "return";    break;
        case KEY_TAB:        name = "tab";       break;
        case KEY_BACKSPACE:  name = "backspace"; break;
        case KEY_DELETE:     name = "delete";    break;
        case KEY_UP:         name = "up";        break;
        case KEY_DOWN:       name = "down";      break;
        case KEY_LEFT:       name = "left";      break;
        case KEY_RIGHT:      name = "right";     break;
        case KEY_HOME:       name = "home";      break;
        case KEY_END:        name = "end";       break;
        case KEY_PAGE_UP:    name = "pageup";    break;
        case KEY_PAGE_DOWN:  name = "pagedown";  break;
        case KEY_INSERT:     name = "insert";    break;
        case KEY_MENU:       name = "menu";      break;
        case KEY_PRINT:      name = "print";     break;
        case KEY_PAUSE:      name = "pause";     break;
        case KEY_F1:  name = "f1";  break;
        case KEY_F2:  name = "f2";  break;
        case KEY_F3:  name = "f3";  break;
        case KEY_F4:  name = "f4";  break;
        case KEY_F5:  name = "f5";  break;
        case KEY_F6:  name = "f6";  break;
        case KEY_F7:  name = "f7";  break;
        case KEY_F8:  name = "f8";  break;
        case KEY_F9:  name = "f9";  break;
        case KEY_F10: name = "f10"; break;
        case KEY_F11: name = "f11"; break;
        case KEY_F12: name = "f12"; break;
        case KEY_F13: name = "f13"; break;
        case KEY_F14: name = "f14"; break;
        case KEY_F15: name = "f15"; break;
        case KEY_F16: name = "f16"; break;
        case KEY_F17: name = "f17"; break;
        case KEY_F18: name = "f18"; break;
        case KEY_F19: name = "f19"; break;
        case KEY_F20: name = "f20"; break;
        case KEY_F21: name = "f21"; break;
        case KEY_F22: name = "f22"; break;
        case KEY_F23: name = "f23"; break;
        case KEY_F24: name = "f24"; break;
        default:             name = "?";         break;
        }
        snprintf(tmp + ti, sizeof(tmp) - ti, "%s", name);
    } else {
        if (ev->codepoint == (uint32_t)' ') {
            snprintf(tmp + ti, sizeof(tmp) - ti, "space");
        } else {
            tmp[ti++] = (char)ev->codepoint;
            tmp[ti]   = '\0';
        }
    }

    snprintf(buf, sz, "%s", tmp);
}

static bool key_seen(KeyEvent *seen, int nseen, const KeyEvent *ev) {
    for (int i = 0; i < nseen; i++) {
        if (seen[i].type == ev->type && seen[i].mods == ev->mods &&
            seen[i].codepoint == ev->codepoint)
            return true;
    }
    return false;
}

static void collect_entries(sexp ctx, sexp summary_fn, Keymap *km, const char *prefix,
                             int depth, KeyEvent *seen, int *nseen) {
    if (!km || wk_count >= WK_MAX || depth > 4) return;

    for (size_t i = 0; i < km->count && wk_count < WK_MAX; i++) {
        KeymapEntry *e = &km->entries[i];

        // Skip keys already claimed by a more-specific (child) map
        if (key_seen(seen, *nseen, &e->key)) continue;
        if (*nseen < 64) seen[(*nseen)++] = e->key;

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
            const char *label;
            sexp label_sexp = (summary_fn != SEXP_FALSE)
                ? sexp_apply(ctx, summary_fn, sexp_list1(ctx, e->binding.command_sym))
                : SEXP_FALSE;
            if (sexp_stringp(label_sexp)) {
                label = sexp_string_data(label_sexp);
            } else {
                sexp name_sexp = sexp_symbol_to_string(ctx, e->binding.command_sym);
                label = sexp_string_data(name_sexp);
            }
            snprintf(wk_label_strs[wk_count], sizeof(wk_label_strs[0]),
                     "%s", label ? label : "?");
            wk_is_prefix[wk_count] = false;
            wk_count++;
        } else if (e->binding.type == BINDING_KEYMAP) {
            Keymap *child = e->binding.keymap;
            if (child && child->name) {
                // Named prefix: show as collapsed entry
                snprintf(wk_key_strs[wk_count], sizeof(wk_key_strs[0]),
                         "%s", full_path);
                snprintf(wk_label_strs[wk_count], sizeof(wk_label_strs[0]),
                         "%s...", child->name);
                wk_is_prefix[wk_count] = true;
                wk_count++;
            } else {
                // Anonymous prefix: recurse
                collect_entries(ctx, summary_fn, child, full_path, depth + 1, seen, nseen);
            }
        }
    }

    // Traverse parent chain so inherited entries are visible
    if (km->parent)
        collect_entries(ctx, summary_fn, km->parent, prefix, depth, seen, nseen);
}

static void collect_entries_top(sexp ctx, Keymap *km, const char *prefix, int depth) {
    sexp summary_fn = sexp_env_ref(ctx, G->chibi.env,
                                   sexp_intern(ctx, "doc-summary", -1), SEXP_FALSE);
    KeyEvent seen[64];
    int nseen = 0;
    collect_entries(ctx, summary_fn, km, prefix, depth, seen, &nseen);
}

void WhichKey(AppState *state) {
    wk_count = 0;
    collect_entries_top(state->chibi.ctx, state->which_key.keymap, "", 0);
    if (wk_count == 0) return;

    // Collapse dispatch-style entries: if the same command appears >= 3 times,
    // keep only the first occurrence and rename its key to "<char>".
    {
        char seen_labels[WK_MAX][64];
        int  seen_idx[WK_MAX];
        int  seen_cnt[WK_MAX];
        int  nseen = 0;
        bool skip[WK_MAX];
        memset(skip, 0, sizeof(skip));

        for (int i = 0; i < wk_count; i++) {
            if (wk_is_prefix[i]) continue;
            bool found = false;
            for (int j = 0; j < nseen; j++) {
                if (strcmp(seen_labels[j], wk_label_strs[i]) == 0) {
                    seen_cnt[j]++;
                    skip[i] = true;
                    found = true;
                    break;
                }
            }
            if (!found && nseen < WK_MAX) {
                strncpy(seen_labels[nseen], wk_label_strs[i], 63);
                seen_labels[nseen][63] = '\0';
                seen_idx[nseen] = i;
                seen_cnt[nseen] = 1;
                nseen++;
            }
        }
        for (int j = 0; j < nseen; j++) {
            if (seen_cnt[j] >= 3)
                snprintf(wk_key_strs[seen_idx[j]], WK_KEY_SZ, "<char>");
        }
        int n = 0;
        for (int i = 0; i < wk_count; i++) {
            if (!skip[i]) {
                if (n != i) {
                    memcpy(wk_key_strs[n],   wk_key_strs[i],   WK_KEY_SZ);
                    memcpy(wk_label_strs[n], wk_label_strs[i], 64);
                    wk_is_prefix[n] = wk_is_prefix[i];
                }
                n++;
            }
        }
        wk_count = n;
    }

    float scale = state->ui.scale_factor;
    uint16_t font_size = (uint16_t)(12.0f * scale);
    float pad = 8.0f * scale;

    CachedRoles roles = state->ui.roles;
    Clay_Color bg         = ui_resolve_color(state, roles.ui_bg);
    Clay_Color fg_header  = ui_resolve_color(state, roles.text_primary);
    Clay_Color fg_label   = ui_resolve_color(state, roles.text_command);
    Clay_Color fg_prefix  = ui_resolve_color(state, roles.text_prefix);
    Clay_Color border_col = ui_resolve_color(state, roles.border_active);

    CLAY(CLAY_ID("WhichKey"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                .parent  = CLAY_ATTACH_POINT_RIGHT_BOTTOM
            },
            .offset = { -36.0f * scale, -36.0f * scale },
            .zIndex = 100
        },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .left = pad, .right = pad, .top = pad, .bottom = pad },
            .childGap = (uint16_t)(pad * 2)
        },
        .cornerRadius = CLAY_CORNER_RADIUS(6.0f * scale),
        .backgroundColor = bg,
        .border = {
            .color = border_col,
            .width = { .top = 1, .bottom = 1, .left = 1, .right = 1, .betweenChildren = 1 }
        }
    }) {
        // Header: show the accumulated prefix
        Clay_String header_cs = {
            .chars  = state->which_key.prefix_str,
            .length = (int32_t)strlen(state->which_key.prefix_str)
        };
        Keybinding(state, header_cs, font_size);

        CLAY(CLAY_ID("WK_Cols"), {
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childGap = (uint16_t)(12.0f * scale)
            }
        }) {
            CLAY(CLAY_ID("WK_Keys"), {
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap = (uint16_t)(2.0f * scale)
                }
            }) {
                for (int i = 0; i < wk_count; i++) {
                    Clay_String key_cs = {
                        .chars  = wk_key_strs[i],
                        .length = (int32_t)strlen(wk_key_strs[i])
                    };
                    Keybinding(state, key_cs, font_size);
                }
            }
            CLAY(CLAY_ID("WK_Labels"), {
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap = (uint16_t)(2.0f * scale)
                }
            }) {
                for (int i = 0; i < wk_count; i++) {
                    Clay_String label_cs = {
                        .chars  = wk_label_strs[i],
                        .length = (int32_t)strlen(wk_label_strs[i])
                    };
                    CLAY_TEXT(label_cs, CLAY_TEXT_CONFIG({
                        .fontId    = FONT_UI_NORMAL,
                        .fontSize  = font_size,
                        .textColor = wk_is_prefix[i] ? fg_prefix : fg_label,
                        .wrapMode  = CLAY_TEXT_WRAP_NONE
                    }));
                }
            }
        }
    }
}

sexp scm_which_key_toggle(sexp ctx, sexp self, sexp n) {
    G->which_key.enabled = !G->which_key.enabled;
    if (!G->which_key.enabled)
        G->which_key.active = false;
    char msg[64];
    sprintf(msg, "Key completion prompt %s", G->which_key.enabled ? "enabled" : "disabled");
    message_send(msg);
    G->needs_redraw = true;
    return SEXP_VOID;
}
