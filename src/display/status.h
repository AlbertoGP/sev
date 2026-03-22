// Each visual pane has an associated status bar.

#pragma once

#include "pane.h"
#include "../state.h"

// Clay component for pane status bar
void StatusBar(AppState *state, Pane *pane, int32_t index);
//
// Free all strings allocated during layout. Call after rendering.
void bar_free_strings(void);
