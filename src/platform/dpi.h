#pragma once
#include <SDL3/SDL.h>

// Returns the physical PPI of the display containing the window, or 0.0f if
// unavailable. Callers should fall back to SDL_GetWindowDisplayScale when 0.
float get_display_ppi(SDL_Window *window);
