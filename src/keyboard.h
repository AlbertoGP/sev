#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include "state.h"

void handle_text_input(AppState *app, const SDL_TextInputEvent *text);

void handle_key_down(AppState *app, const SDL_KeyboardEvent *key);

KeySpecial sdl_to_keyspecial(SDL_Keycode keycode);
