// Global status bar reflecting the active pane/tab/buffer.

#pragma once

#include "../state.h"

// Clay component for the global status bar.
void StatusBar(AppState *state);

// Free all strings allocated during layout. Call after rendering.
void bar_free_strings(void);
