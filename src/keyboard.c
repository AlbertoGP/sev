// Keyboard event handler functions' implementation.

#include "keyboard.h"
#include "keyevent.h"
#include "keymap.h"

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

// Convert from SDL3's keymod types to internal KeyMod enum so that
// keymap logic does not need to be aware of SDL3.
static KeyMod normalize_mods(SDL_Keymod m) {
    uint16_t r = 0;
    if (m & SDL_KMOD_CTRL)  r |= MOD_CTRL;
    if (m & SDL_KMOD_ALT)   r |= MOD_META;   // Emacs-style Meta
    if (m & SDL_KMOD_SHIFT) r |= MOD_SHIFT;
    if (m & SDL_KMOD_GUI)   r |= MOD_SUPER;
    return r;
}

// Returns true if an SDL_Keycode is a non-text key (e.g. Esc, Tab, Backspace)
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

// Convert from SDL3's keycode types to internal KeySpecial enum so that
// keymap logic does not need to be aware of SDL3.
static KeySpecial sdl_to_keyspecial(SDL_Keycode keycode) {
    switch (keycode) {
    case SDLK_ESCAPE:
        return KEY_ESC;
    case SDLK_RETURN:
        return KEY_RETURN;
    case SDLK_TAB:
        return KEY_TAB;
    case SDLK_BACKSPACE:
        return KEY_BACKSPACE;
    case SDLK_DELETE:
        return KEY_DELETE;

    case SDLK_LEFT:
        return KEY_LEFT;
    case SDLK_RIGHT:
        return KEY_RIGHT;
    case SDLK_UP:
        return KEY_UP;
    case SDLK_DOWN:
        return KEY_DOWN;

    case SDLK_HOME:
        return KEY_HOME;
    case SDLK_END:
        return KEY_END;
    case SDLK_PAGEUP:
        return KEY_PAGE_UP;
    case SDLK_PAGEDOWN:
        return KEY_PAGE_DOWN;
        
    case SDLK_F1:
        return KEY_F1;
    case SDLK_F2:
        return KEY_F2;
    case SDLK_F3:
        return KEY_F3;
    case SDLK_F4:
        return KEY_F4;
    case SDLK_F5:
        return KEY_F5;
    case SDLK_F6:
        return KEY_F6;
    case SDLK_F7:
        return KEY_F7;
    case SDLK_F8:
        return KEY_F8;
    case SDLK_F9:
        return KEY_F9;
    case SDLK_F10:
        return KEY_F10;
    case SDLK_F11:
        return KEY_F11;
    case SDLK_F12:
        return KEY_F12;
    case SDLK_F13:
        return KEY_F13;
    case SDLK_F14:
        return KEY_F14;
    case SDLK_F15:
        return KEY_F15;
    case SDLK_F16:
        return KEY_F16;
    case SDLK_F17:
        return KEY_F17;
    case SDLK_F18:
        return KEY_F18;
    case SDLK_F19:
        return KEY_F19;
    case SDLK_F20:
        return KEY_F20;
    case SDLK_F21:
        return KEY_F21;
    case SDLK_F22:
        return KEY_F22;
    case SDLK_F23:
        return KEY_F23;
    case SDLK_F24:
        return KEY_F24;

    case SDLK_INSERT:
        return KEY_INSERT;
    case SDLK_MENU:
        return KEY_MENU;     // context menu key
    case SDLK_PRINTSCREEN:
        return KEY_PRINT;    // print screen
    case SDLK_PAUSE:
        return KEY_PAUSE;
    default:
        return 0;
    }
}

void handle_key_down(AppState *state, const SDL_KeyboardEvent *key) {
    if (key->repeat)
        return;

    uint16_t mods = normalize_mods(key->mod);

    if (mods & MOD_CTRL) {
        KeyEvent ev = {
            .type = KEYEVENT_CHAR,
            .codepoint = key->key,
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
