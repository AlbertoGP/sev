// Keypress event types and equivalence-checking function.

#pragma once

#include <stdint.h>

typedef enum {
    KEYEVENT_CHAR,
    KEYEVENT_SPECIAL
} KeyEventType;

// Internal codes for standard modifier keys.
typedef enum {
    MOD_NONE  = 0,
    MOD_CTRL  = 1 << 0,
    MOD_META  = 1 << 1,
    MOD_SHIFT = 1 << 2,
    MOD_SUPER = 1 << 3,
} KeyMod;

// Internal codes for non-character, non-modifier keys.
typedef enum {
    KEY_ESC,
    KEY_RETURN,
    KEY_TAB,
    KEY_BACKSPACE,
    KEY_DELETE,

    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,

    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    KEY_INSERT,
    KEY_MENU,     // context menu key
    KEY_PRINT,    // print screen
    KEY_PAUSE,
} KeySpecial;

typedef struct {
    KeyEventType type;
    uint16_t mods;       // CTRL | META | SHIFT | SUPER
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

