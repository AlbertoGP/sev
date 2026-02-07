// Each visual pane has an associated status bar.

#pragma once

#include "../state.h"
#include "pane.h"

// Clay component for pane status bar
void StatusBar(AppState *state, Pane *pane);
//
// Free all strings allocated during layout. Call after rendering.
void bar_free_strings(void);
