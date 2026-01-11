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
            .type = KEYEVENT_CHAR,
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
            .keycode = sdl_to_keyspecial(key->key),
            .mods = mods
        };
        key_dispatch(state, &ev);
    }
}

KeySpecial sdl_to_keyspecial(SDL_Keycode keycode) {
    switch (keycode) {
    case SDLK_ESCAPE:
        return KEY_SPECIAL_ESC;
    case SDLK_RETURN:
        return KEY_SPECIAL_RETURN;
    case SDLK_TAB:
        return KEY_SPECIAL_TAB;
    case SDLK_BACKSPACE:
        return KEY_SPECIAL_BACKSPACE;
    case SDLK_DELETE:
        return KEY_SPECIAL_DELETE;

    case SDLK_LEFT:
        return KEY_SPECIAL_LEFT;
    case SDLK_RIGHT:
        return KEY_SPECIAL_RIGHT;
    case SDLK_UP:
        return KEY_SPECIAL_UP;
    case SDLK_DOWN:
        return KEY_SPECIAL_DOWN;

    case SDLK_HOME:
        return KEY_SPECIAL_HOME;
    case SDLK_END:
        return KEY_SPECIAL_END;
    case SDLK_PAGEUP:
        return KEY_SPECIAL_PAGE_UP;
    case SDLK_PAGEDOWN:
        return KEY_SPECIAL_PAGE_DOWN;
        
    case SDLK_F1:
        return KEY_SPECIAL_F1;
    case SDLK_F2:
        return KEY_SPECIAL_F2;
    case SDLK_F3:
        return KEY_SPECIAL_F3;
    case SDLK_F4:
        return KEY_SPECIAL_F4;
    case SDLK_F5:
        return KEY_SPECIAL_F5;
    case SDLK_F6:
        return KEY_SPECIAL_F6;
    case SDLK_F7:
        return KEY_SPECIAL_F7;
    case SDLK_F8:
        return KEY_SPECIAL_F8;
    case SDLK_F9:
        return KEY_SPECIAL_F9;
    case SDLK_F10:
        return KEY_SPECIAL_F10;
    case SDLK_F11:
        return KEY_SPECIAL_F11;
    case SDLK_F12:
        return KEY_SPECIAL_F12;
    case SDLK_F13:
        return KEY_SPECIAL_F13;
    case SDLK_F14:
        return KEY_SPECIAL_F14;
    case SDLK_F15:
        return KEY_SPECIAL_F15;
    case SDLK_F16:
        return KEY_SPECIAL_F16;
    case SDLK_F17:
        return KEY_SPECIAL_F17;
    case SDLK_F18:
        return KEY_SPECIAL_F18;
    case SDLK_F19:
        return KEY_SPECIAL_F19;
    case SDLK_F20:
        return KEY_SPECIAL_F20;
    case SDLK_F21:
        return KEY_SPECIAL_F21;
    case SDLK_F22:
        return KEY_SPECIAL_F22;
    case SDLK_F23:
        return KEY_SPECIAL_F23;
    case SDLK_F24:
        return KEY_SPECIAL_F24;

    case SDLK_INSERT:
        return KEY_SPECIAL_INSERT;
    case SDLK_MENU:
        return KEY_SPECIAL_MENU;     // context menu key
    case SDLK_PRINTSCREEN:
        return KEY_SPECIAL_PRINT;    // print screen
    case SDLK_PAUSE:
        return KEY_SPECIAL_PAUSE;
    default:
        return 0;
    }
}
