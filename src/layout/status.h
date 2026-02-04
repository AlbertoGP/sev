// Each visual pane has an associated status bar.

#pragma once

#include "../state.h"
#include "pane.h"

// Load icons for status bar modes.
bool status_bar_icons_init(AppState *state);

// Update icon colors (must be called on theme change)
void update_icon_colors(AppState *state);

// Clay component for pane status bar
void StatusBar(AppState *state, Pane *pane);
//
// Free all strings allocated during layout. Call after rendering.
void bar_free_strings(void);
