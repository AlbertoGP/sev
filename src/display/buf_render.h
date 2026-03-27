// Buffer content rendering component.
// Renders the text area, gutter, cursor, selection, and scrollbar for a tab.

#pragma once

#include <stdint.h>
#include "../state.h"

struct ContentPane;
struct Tab;

// Render the buffer content area for the given tab.
// Called from pane.c inside a parent Clay layout container.
// container_id is the Clay element ID of the enclosing pane (ContentPane),
// used to query its full bounding box for split divider hit-testing.
void BufferContentRender(AppState *state, struct ContentPane *cp,
                         struct Tab *tab, int32_t index,
                         Clay_ElementId container_id);

// Reset per-frame state (selection pool). Call once before create_app_layout().
void buf_render_reset(void);
