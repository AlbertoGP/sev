#include "keyboard.h"
#include "keyevent.h"
#include "keymap.h"

static uint16_t normalize_mods(SDL_Keymod m) {
    uint16_t r = 0;
    if (m & SDL_KMOD_CTRL)  r |= MOD_CTRL;
    if (m & SDL_KMOD_ALT)   r |= MOD_META;   // Emacs-style Meta
    if (m & SDL_KMOD_SHIFT) r |= MOD_SHIFT;
    if (m & SDL_KMOD_GUI)   r |= MOD_SUPER;
    return r;
}

static int utf8_decode(const char *s, uint32_t *out) {
    unsigned char c = s[0];

    if (c < 0x80) {
        *out = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        *out = ((c & 0x1F) << 6) |
               (s[1] & 0x3F);
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        *out = ((c & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6) |
               (s[2] & 0x3F);
        return 3;
    } else {
        *out = ((c & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6) |
               (s[3] & 0x3F);
        return 4;
    }
}

static bool is_non_text_key(SDL_Keycode k) {
    if (SDLK_F1 <= k && k <= SDLK_F24)
        return true;

    switch (k) {
    case SDLK_ESCAPE:
    case SDLK_RETURN:
    case SDLK_TAB:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
    case SDLK_LEFT:
    case SDLK_RIGHT:
    case SDLK_UP:
    case SDLK_DOWN:
        return true;
    default:
        return false;
    }
}

void handle_text_input(AppState *state, const SDL_TextInputEvent *text) {
    const char *utf8 = text->text;

    while (*utf8) {
        uint32_t cp;
        int len = utf8_decode(utf8, &cp);
        utf8 += len;

        KeyEvent ev = {
            .type = KEYEVENT_CHAR,
            .codepoint = cp,
            .mods = MOD_NONE
        };

        key_dispatch(state, &ev);
        state->needs_redraw = true;
    }
}

void handle_key_down(AppState *state, const SDL_KeyboardEvent *key) {
    if (key->repeat)
        return;

    uint16_t mods = normalize_mods(key->mod);

    if (mods & MOD_CTRL) {
        KeyEvent ev = {
            .type = KEYEVENT_SPECIAL,
            .keycode = key->key,
            .mods = mods
        };
        key_dispatch(state, &ev);
        return;
    }

    // Non-text, non-char keys
    if (is_non_text_key(key->key)) {
        KeyEvent ev = {
            .type = KEYEVENT_SPECIAL,
            .keycode = key->key,
            .mods = mods
        };
        key_dispatch(state, &ev);
    }
}

