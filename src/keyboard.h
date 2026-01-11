// Keyboard event handler functions.

#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include "state.h"

// Dispatch on text character (utf-8) input.
void handle_text_input(AppState *app, const SDL_TextInputEvent *text);

// Dispatch on non-text character input, and text + modifiers such as CTRL.
void handle_key_down(AppState *app, const SDL_KeyboardEvent *key);
