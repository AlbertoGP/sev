// Buffer content rendering component.
// Renders the text area, gutter, cursor, selection, and scrollbar for a tab.

#pragma once

#include <stdint.h>
#include "../state.h"

struct ContentPane;
struct Tab;

// Render the buffer content area for the given tab.
// Called from pane.c inside a parent Clay layout container.
void BufferContentRender(AppState *state, struct ContentPane *cp,
                         struct Tab *tab, int32_t index);

// Reset per-frame state (selection pool). Call once before create_app_layout().
void buf_render_reset(void);
