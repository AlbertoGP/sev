// Cursor Clay component.

#pragma once

#include "../state.h"

void Cursor(AppState *state, int32_t index,
            float offset, int line_height,
            float clip_x, float clip_y, float clip_w, float clip_h,
            FontID font_id, uint16_t font_size);

void cursor_cb_reset(void);
