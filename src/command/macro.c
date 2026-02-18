#include <SDL3/SDL.h>
#include <chibi/eval.h>

#include "scheme_internal.h"   // G, AppState
#include "keymap.h"            // key_dispatch()
#include "../text/register.h"

// Serialize one KeyEvent to at most 4 bytes. Returns byte count (0 = skip).
static int keyevent_to_bytes(const KeyEvent *ev, uint8_t out[4]) {
    if (ev->type == KEYEVENT_CHAR) {
        if (ev->mods == MOD_NONE) {
            uint32_t cp = ev->codepoint;
            if (cp < 0x80) {
                out[0] = (uint8_t)cp; return 1;
            } else if (cp < 0x800) {
                out[0] = 0xc0 | (cp >> 6);
                out[1] = 0x80 | (cp & 0x3f);
                return 2;
            } else if (cp < 0x10000) {
                out[0] = 0xe0 | (cp >> 12);
                out[1] = 0x80 | ((cp >> 6) & 0x3f);
                out[2] = 0x80 | (cp & 0x3f);
                return 3;
            } else {
                out[0] = 0xf0 | (cp >> 18);
                out[1] = 0x80 | ((cp >> 12) & 0x3f);
                out[2] = 0x80 | ((cp >> 6) & 0x3f);
                out[3] = 0x80 | (cp & 0x3f);
                return 4;
            }
        }
        if (ev->mods == MOD_CTRL &&
            ev->codepoint >= 'a' && ev->codepoint <= 'z') {
            out[0] = (uint8_t)(ev->codepoint - 'a' + 1);
            return 1;
        }
        return 0; // skip other modifier combos
    }

    // KEYEVENT_SPECIAL
    switch (ev->keycode) {
        case KEY_ESC:       out[0] = 0x1b; return 1;
        case KEY_RETURN:    out[0] = 0x0a; return 1;
        case KEY_TAB:       out[0] = 0x09; return 1;
        case KEY_BACKSPACE: out[0] = 0x08; return 1;
        case KEY_DELETE:    out[0] = 0x7f; return 1;
        default:
            out[0] = 0xff;
            out[1] = (uint8_t)ev->keycode;
            return 2;
    }
}

// Deserialize one KeyEvent from bytes. Returns bytes consumed (always > 0 if len > 0).
static size_t bytes_to_keyevent(const uint8_t *bytes, size_t len, KeyEvent *out) {
    if (len == 0) return 0;
    uint8_t b = bytes[0];

    if (b == 0xff) {
        if (len >= 2) {
            *out = (KeyEvent){ .type = KEYEVENT_SPECIAL, .mods = MOD_NONE,
                               .keycode = (KeySpecial)bytes[1] };
            return 2;
        }
        // Orphan escape byte: skip as literal
        *out = (KeyEvent){ .type = KEYEVENT_CHAR, .mods = MOD_NONE, .codepoint = 0xff };
        return 1;
    }

    switch (b) {
        case 0x08: *out = (KeyEvent){ .type=KEYEVENT_SPECIAL, .mods=MOD_NONE, .keycode=KEY_BACKSPACE }; return 1;
        case 0x09: *out = (KeyEvent){ .type=KEYEVENT_SPECIAL, .mods=MOD_NONE, .keycode=KEY_TAB };       return 1;
        case 0x0a:
        case 0x0d: *out = (KeyEvent){ .type=KEYEVENT_SPECIAL, .mods=MOD_NONE, .keycode=KEY_RETURN };    return 1;
        case 0x1b: *out = (KeyEvent){ .type=KEYEVENT_SPECIAL, .mods=MOD_NONE, .keycode=KEY_ESC };       return 1;
        case 0x7f: *out = (KeyEvent){ .type=KEYEVENT_SPECIAL, .mods=MOD_NONE, .keycode=KEY_DELETE };    return 1;
    }

    // C-a through C-z (0x01–0x1a, excluding those mapped above)
    if (b >= 0x01 && b <= 0x1a) {
        *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_CTRL, .codepoint='a'+(b-1) };
        return 1;
    }

    // Printable ASCII
    if (b >= 0x20 && b <= 0x7e) {
        *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_NONE, .codepoint=b };
        return 1;
    }

    // UTF-8 multi-byte sequences
    if ((b & 0xe0) == 0xc0 && len >= 2) {
        *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_NONE,
                           .codepoint=((uint32_t)(b&0x1f)<<6)|(bytes[1]&0x3f) };
        return 2;
    }
    if ((b & 0xf0) == 0xe0 && len >= 3) {
        *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_NONE,
                           .codepoint=((uint32_t)(b&0x0f)<<12)|((uint32_t)(bytes[1]&0x3f)<<6)|(bytes[2]&0x3f) };
        return 3;
    }
    if ((b & 0xf8) == 0xf0 && len >= 4) {
        *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_NONE,
                           .codepoint=((uint32_t)(b&0x07)<<18)|((uint32_t)(bytes[1]&0x3f)<<12)|((uint32_t)(bytes[2]&0x3f)<<6)|(bytes[3]&0x3f) };
        return 4;
    }

    // Unknown: treat as literal codepoint
    *out = (KeyEvent){ .type=KEYEVENT_CHAR, .mods=MOD_NONE, .codepoint=b };
    return 1;
}

sexp scm_macro_start(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char ch = (char)sexp_unbox_character(sname);
    int idx = (ch >= 'a' && ch <= 'z') ? ch - 'a' : -1;
    if (idx < 0 || G->macro_recording) return SEXP_VOID;
    G->macro_target_reg = idx;
    G->macro_buf_len = 0;
    G->macro_recording = true;
    G->macro_skip_next = true;  // don't record the key that triggered us
    return SEXP_VOID;
}

sexp scm_macro_stop(sexp ctx, sexp self, sexp n) {
    if (!G->macro_recording) return SEXP_VOID;
    G->macro_recording = false;

    char reg_name = 'a' + G->macro_target_reg;

    if (G->macro_buf_len == 0) {
        register_write(G->registers, reg_name, "", 0);
        return SEXP_VOID;
    }

    // Serialize KeyEvent[] → byte string → text register
    size_t max_bytes = G->macro_buf_len * 4;
    uint8_t *bytes = SDL_malloc(max_bytes);
    if (!bytes) return SEXP_VOID;

    size_t total = 0;
    for (size_t i = 0; i < G->macro_buf_len; i++) {
        uint8_t tmp[4];
        int nb = keyevent_to_bytes(&G->macro_buf[i], tmp);
        for (int j = 0; j < nb; j++)
            bytes[total++] = tmp[j];
    }

    register_write(G->registers, reg_name, (const char *)bytes, total);
    SDL_free(bytes);
    return SEXP_VOID;
}

sexp scm_macro_play(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char ch = (char)sexp_unbox_character(sname);
    if (G->macro_recording) return SEXP_VOID;

    const ByteString *bs = register_read(G->registers, ch);
    if (!bs || !bs->data || bs->len == 0) return SEXP_VOID;

    // Block-yanked registers are not playable as macros
    if (register_get_shape(G->registers, ch) == SHAPE_BLOCKWISE) return SEXP_VOID;

    // Deserialize byte string → KeyEvent[]
    const uint8_t *bytes = (const uint8_t *)bs->data;
    size_t len = bs->len;

    // Worst case: one event per byte
    KeyEvent *evs = SDL_malloc(len * sizeof(KeyEvent));
    if (!evs) return SEXP_VOID;

    size_t pos = 0, count = 0;
    while (pos < len) {
        size_t consumed = bytes_to_keyevent(bytes + pos, len - pos, &evs[count]);
        if (consumed == 0) break;
        pos += consumed;
        count++;
    }

    // Reset key state so replayed events dispatch from global map
    G->input.current_map = G->input.global_map;
    for (size_t i = 0; i < count; i++)
        key_dispatch(G, &evs[i]);

    SDL_free(evs);
    return SEXP_VOID;
}

sexp scm_macro_is_recording(sexp ctx, sexp self, sexp n) {
    return G->macro_recording ? SEXP_TRUE : SEXP_FALSE;
}
