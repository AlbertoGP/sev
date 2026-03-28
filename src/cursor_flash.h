#pragma once

#include "state.h"

// Reset the cursor blink cycle: make cursor visible and restart the 500ms timer.
// Safe to call from any user-input handler.
void cursor_flash_reset(AppState *state);
