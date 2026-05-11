#pragma once
#ifdef __APPLE__
#include <SDL3/SDL.h>

typedef struct {
    float titlebar_height;
    float traffic_light_width;
} MacOSTitlebarInfo;

MacOSTitlebarInfo macos_setup_titlebar(SDL_Window *sdl_window);
#endif
