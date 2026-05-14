// Search bar UI layer — rendering, input handling, and Scheme bindings.
// Core search algorithm lives in src/text/search.c.

#pragma once

#include <stdint.h>

#include "../state.h"
#include "../command/keyevent.h"

typedef struct Pane Pane;

void SearchBar(AppState *state, Pane *pane, int32_t index);
void search_handle_key(AppState *state, const KeyEvent *ev);
