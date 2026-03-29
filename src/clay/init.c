#include "clay.h"
#include "../state.h"

static void HandleClayErrors(Clay_ErrorData errorData) {
    SDL_Log("%s", errorData.errorText.chars);
}

static Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    Clay_SDL3RendererData *rd = userData;
    TTF_Font *font = SDL_Clay_GetRenderFont(rd, config->fontId, (float)config->fontSize);
    int width, height;
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text = %s", SDL_GetError());
    }
    return (Clay_Dimensions) { (float) width, (float) height };
}

bool clay_init(AppState *state) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = malloc(totalMemorySize),
        .capacity = totalMemorySize
    };
    if (!clayMemory.memory)
        return false;

    int width, height;
    SDL_GetWindowSizeInPixels(state->window, &width, &height);
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) width, (float) height }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(SDL_MeasureText, &state->rendererData);

    return true;
}
