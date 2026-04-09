#include "keybinding.h"
#include "theme.h"
#include "../clay/renderer.h"

void Keybinding(AppState *state, Clay_String keybinding_str, uint16_t font_size) {
    if (keybinding_str.length == 0) return;

    TextStyle key_style = ui_resolve_text_style(state, state->ui.roles.text_key,
                                                 FONT_UI_NORMAL, font_size);

    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 6.0f * state->ui.scale_factor,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        }
    }) {
        int start = 0;
        int len = keybinding_str.length;
        while (start < len) {
            // Find next space
            int end = start;
            while (end < len && keybinding_str.chars[end] != ' ') end++;
            
            if (end > start) {
                Clay_String part = {
                    .chars = keybinding_str.chars + start,
                    .length = end - start
                };
                CLAY_TEXT(part, CLAY_TEXT_CONFIG({
                    .fontId = key_style.font_id,
                    .fontSize = key_style.font_size,
                    .textColor = key_style.color,
                    .wrapMode = CLAY_TEXT_WRAP_NONE
                }));
            }
            
            start = end;
            while (start < len && keybinding_str.chars[start] == ' ') start++;
        }
    }
}
