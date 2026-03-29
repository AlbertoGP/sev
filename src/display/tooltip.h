#pragma once

#include "../state.h"

#define TOOLTIP_SHOW_EVENT 2

// Render callback for tooltip content. Caller makes Clay calls inside.
// user_data is whatever was passed to Tooltip().
typedef void (*TooltipRenderFn)(void *user_data);

// Call inside a CLAY() block to attach a tooltip to the current element.
// unique_id: stable int distinguishing this call site (e.g. pane_index*256 + tab_index)
// render_fn: called synchronously to declare the tooltip's inner Clay layout
// user_data: passed through to render_fn; caller owns any pointed-to memory
void Tooltip(AppState *state, bool is_hovered, int unique_id,
             TooltipRenderFn render_fn, void *user_data);

// Convenience wrapper: tooltip containing a single line of primary-coloured text.
// text: caller owns the string
void TextTooltip(AppState *state, bool is_hovered, int unique_id, const char *text);

// Call from event.c when SDL_EVENT_USER code == TOOLTIP_SHOW_EVENT
void tooltip_handle_show(AppState *state);
