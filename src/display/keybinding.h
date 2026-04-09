#pragma once

#include "../state.h"

// Renders a keybinding str (e.g. "ctrl-k right"), splitting by space 
// and using a small layout gap between the segments.
void Keybinding(AppState *state, Clay_String keybinding_str, uint16_t font_size);
