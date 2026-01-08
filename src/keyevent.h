#pragma once

#include <stdint.h>

typedef enum {
    KEYEVENT_CHAR,
    KEYEVENT_SPECIAL
} KeyEventType;

enum {
    MOD_NONE  = 0,
    MOD_CTRL  = 1 << 0,
    MOD_META  = 1 << 1,
    MOD_SHIFT = 1 << 2,
    MOD_SUPER = 1 << 3,
};

typedef enum {
    KEY_SPECIAL_ESC,
    KEY_SPECIAL_RETURN,
    KEY_SPECIAL_TAB,
    KEY_SPECIAL_BACKSPACE,
    KEY_SPECIAL_DELETE,

    KEY_SPECIAL_LEFT,
    KEY_SPECIAL_RIGHT,
    KEY_SPECIAL_UP,
    KEY_SPECIAL_DOWN,

    KEY_SPECIAL_HOME,
    KEY_SPECIAL_END,
    KEY_SPECIAL_PAGE_UP,
    KEY_SPECIAL_PAGE_DOWN,

    KEY_SPECIAL_F1,
    KEY_SPECIAL_F2,
    KEY_SPECIAL_F3,
    KEY_SPECIAL_F4,
    KEY_SPECIAL_F5,
    KEY_SPECIAL_F6,
    KEY_SPECIAL_F7,
    KEY_SPECIAL_F8,
    KEY_SPECIAL_F9,
    KEY_SPECIAL_F10,
    KEY_SPECIAL_F11,
    KEY_SPECIAL_F12,
    KEY_SPECIAL_F13,
    KEY_SPECIAL_F14,
    KEY_SPECIAL_F15,
    KEY_SPECIAL_F16,
    KEY_SPECIAL_F17,
    KEY_SPECIAL_F18,
    KEY_SPECIAL_F19,
    KEY_SPECIAL_F20,
    KEY_SPECIAL_F21,
    KEY_SPECIAL_F22,
    KEY_SPECIAL_F23,
    KEY_SPECIAL_F24,

    KEY_SPECIAL_INSERT,
    KEY_SPECIAL_MENU,     // context menu key
    KEY_SPECIAL_PRINT,    // print screen
    KEY_SPECIAL_PAUSE,
} KeySpecial;

typedef struct {
    KeyEventType type;
    uint16_t mods;       // CTRL | META | SHIFT
    union {
        uint32_t codepoint;  // Unicode
        KeySpecial keycode; // F1, ESC, arrows
    };
} KeyEvent;

static inline bool keyevent_equal(const KeyEvent *k1, const KeyEvent *k2) {
    if (k1->type == k2->type &&
        k1->mods == k2->mods &&
        k1->codepoint == k2->codepoint)
        return true;
    return false;
}

