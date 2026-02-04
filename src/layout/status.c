#include "../state.h"
#include "../subeditor/buffer.h"
#include "pane.h"
#include "theme.h"
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

#define BAR_STRINGS_MAX 256
static char *bar_strings[BAR_STRINGS_MAX];
static int bar_strings_count = 0;

void bar_free_strings(void) {
    for (int i = 0; i < bar_strings_count; i++) {
        free(bar_strings[i]);
    }
    bar_strings_count = 0;
}

void bar_strings_push(char *p) {
    if (bar_strings_count < BAR_STRINGS_MAX) {
        bar_strings[bar_strings_count++] = p;
    }
}

static SDL_Window *window;
static SDL_Texture *icon_normal;
static SDL_Texture *icon_insert;
static SDL_Texture *icon_replace;
static SDL_Texture *icon_select;
static SDL_Texture *icon_command;

bool status_bar_icons_init(AppState *state) {
    window = state->window;

    #ifdef __EMSCRIPTEN__
    #define NORMAL_PATH "/resources/icon-normal.png"
    #define INSERT_PATH "/resources/icon-insert.png"
    #define REPLACE_PATH "/resources/icon-replace.png"
    #define SELECT_PATH "/resources/icon-select.png"
    #define COMMAND_PATH "/resources/icon-command.png"
    #else
    char* basePath = (char*)SDL_GetBasePath();
    char normal_path[1024];
    snprintf(normal_path, sizeof(normal_path), "%sresources/icon-normal.png", basePath);
    #define NORMAL_PATH normal_path
    char insert_path[1024];
    snprintf(insert_path, sizeof(insert_path), "%sresources/icon-insert.png", basePath);
    #define INSERT_PATH insert_path
    char replace_path[1024];
    snprintf(replace_path, sizeof(replace_path), "%sresources/icon-replace.png", basePath);
    #define REPLACE_PATH replace_path
    char select_path[1024];
    snprintf(select_path, sizeof(select_path), "%sresources/icon-select.png", basePath);
    #define SELECT_PATH select_path
    char command_path[1024];
    snprintf(command_path, sizeof(command_path), "%sresources/icon-command.png", basePath);
    #define COMMAND_PATH command_path
    #endif
    if (!icon_normal)
        icon_normal = IMG_LoadTexture(state->rendererData.renderer, NORMAL_PATH);
    if (!icon_normal) return false;
    if (!icon_insert)
        icon_insert = IMG_LoadTexture(state->rendererData.renderer, INSERT_PATH);
    if (!icon_insert) return false;
    if (!icon_replace)
        icon_replace = IMG_LoadTexture(state->rendererData.renderer, REPLACE_PATH);
    if (!icon_replace) return false;
    if (!icon_select)
        icon_select = IMG_LoadTexture(state->rendererData.renderer, SELECT_PATH);
    if (!icon_select) return false;
    if (!icon_command)
        icon_command = IMG_LoadTexture(state->rendererData.renderer, COMMAND_PATH);
    if (!icon_command) return false;

    return true;
}

void update_icon_colors(AppState *state) {
    Clay_Color normal = ui_resolve_color(state, state->ui.roles.label_normal);
    SDL_SetTextureColorMod(icon_normal, normal.r, normal.g, normal.b);
    Clay_Color insert = ui_resolve_color(state, state->ui.roles.label_insert);
    SDL_SetTextureColorMod(icon_insert, insert.r, insert.g, insert.b);
    Clay_Color replace = ui_resolve_color(state, state->ui.roles.label_replace);
    SDL_SetTextureColorMod(icon_replace, replace.r, replace.g, replace.b);
    Clay_Color select = ui_resolve_color(state, state->ui.roles.label_select);
    SDL_SetTextureColorMod(icon_select, select.r, select.g, select.b);
    Clay_Color command = ui_resolve_color(state, state->ui.roles.label_command);
    SDL_SetTextureColorMod(icon_command, command.r, command.g, command.b);
}

static SDL_Texture *get_mode_icon(AppState* state) {
    if (buffer_has_minor_mode(buffer_get_current(), "evil-normal-mode"))
        return icon_normal;
    if (buffer_has_minor_mode(buffer_get_current(), "evil-insert-mode"))
        return icon_insert;
    if (buffer_has_minor_mode(buffer_get_current(), "evil-replace-mode"))
        return icon_replace;
    if (buffer_has_minor_mode(buffer_get_current(), "evil-select-mode"))
        return icon_select;
    if (buffer_has_minor_mode(buffer_get_current(), "evil-command-mode"))
        return icon_command;
    return NULL;
}

static Clay_Color get_label_color(AppState* state) {
    if (buffer_has_minor_mode(buffer_get_current(), "evil-normal-mode"))
        return ui_resolve_color(state, state->ui.roles.label_normal);
    if (buffer_has_minor_mode(buffer_get_current(), "evil-insert-mode"))
        return ui_resolve_color(state, state->ui.roles.label_insert);
    if (buffer_has_minor_mode(buffer_get_current(), "evil-replace-mode"))
        return ui_resolve_color(state, state->ui.roles.label_replace);
    if (buffer_has_minor_mode(buffer_get_current(), "evil-select-mode"))
        return ui_resolve_color(state, state->ui.roles.label_select);
    if (buffer_has_minor_mode(buffer_get_current(), "evil-command-mode"))
        return ui_resolve_color(state, state->ui.roles.label_command);
    return (Clay_Color){255, 0, 255, 255};
}

void StatusBar(AppState *state, Pane *pane) {
    bool active = pane->content.active;
    Buffer *buf = pane->content.buffer;
    sexp mode_symbol = sexp_intern(state->chibi.ctx, "mode-name", -1);
    
    const char* modeStr = sexp_to_cstring(
        state->chibi.ctx,
        vartable_get(buffer_get_locals(buf),
                     mode_symbol,
                     sexp_c_string(state->chibi.ctx, "ERROR", -1)),
        "ERROR");
    Clay_String modeName = {
        .chars = modeStr,
        .length = strlen(modeStr),
        .isStaticallyAllocated = true
    };

    Clay_String bufName = {
         .chars = buffer_get_name(buf),
         .length = strlen(buffer_get_name(buf)),
    };
    char *pos = malloc(32 * sizeof(char));
    snprintf(pos, 32, "%zu:%d", buf_get_line(buf), get_column(buf));
    bar_strings_push(pos);
    Clay_String pointPos = {
         .chars = pos,
         .length = strlen(pos),
    };
    Clay_Color textColor = active
        ? ui_resolve_color(state, state->ui.roles.bar_text_active)
        : ui_resolve_color(state, state->ui.roles.text_faded);
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .padding = {
                 .right = 10.0 * state->ui.scale_factor,
                 .left = pane->content.active ? 0 : 10.0 * state->ui.scale_factor
            },
            .childGap = 10.0 * state->ui.scale_factor,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
        .clip = {
            .horizontal = true
        }
    }) {
        if (pane->content.active) {
            CLAY(CLAY_ID("Mode Name"), {
                .layout = {
                    .padding = {
                        .left = 8.0 * state->ui.scale_factor,
                        .right = 14.0 * state->ui.scale_factor
                    },
                    .childAlignment = {
                        .y = CLAY_ALIGN_Y_CENTER
                    },
                    .childGap = 4.0 * state->ui.scale_factor
                },
                .cornerRadius = {
                    .topRight = 8.0 * state->ui.scale_factor,
                    .bottomRight = 8.0 * state->ui.scale_factor
                 },
                .backgroundColor = ui_get_mode_color(state)
            }){
                CLAY(CLAY_ID("Mode Icon"), {
                    .layout = {
                        .sizing = {
                            .width = 16.0 * state->ui.scale_factor,
                            .height = 16.0 * state->ui.scale_factor
                        },
                    },
                    .image = get_mode_icon(state),
                }) {}
                CLAY_TEXT(modeName, CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14.0 * state->ui.scale_factor,
                    .textColor = get_label_color(state),
                }));
            }
        }
        CLAY_TEXT(bufName, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = textColor,
        }));
        CLAY_AUTO_ID({
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }}
        }) {}
        CLAY_TEXT(pointPos, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = textColor,
        }));
    }
}
