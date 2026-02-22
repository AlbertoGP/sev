#include <SDL3/SDL_render.h>
#include <chibi/sexp.h>

#include "icon.h"
#include "mode_icon.h"
#include "pane.h"
#include "theme.h"
#include "../state.h"
#include "../text/buffer.h"

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


void StatusBar(AppState *state, Pane *pane) {
    bool active = pane->content.active;
    Buffer *buf = pane->content.buffer;
    sexp mode_symbol = sexp_intern(state->chibi.ctx, "mode-name", -1);
    
    sexp mode_val = vartable_get(buffer_get_locals(buf), mode_symbol, SEXP_FALSE);
    const char* modeStr = sexp_to_cstring(state->chibi.ctx, mode_val, "ERROR");
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
            ModeIconEntry *icon = mode_icon_for_current_buffer();
            Clay_Color mode_bg = icon
                ? ui_resolve_color(state, icon->role_mode_bg)
                : ui_resolve_color(state,
                      sexp_intern(state->chibi.ctx, "mode.normal", -1));
            Clay_Color label_color = icon
                ? ui_resolve_color(state, icon->role_label)
                : (Clay_Color){255, 0, 255, 255};
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
                .backgroundColor = mode_bg
            }){
                if (icon) {
                    SDL_Texture *tex = icon_get(icon->icon_name, state, 16, 16);
                    CLAY(CLAY_ID("Mode Icon"), {
                        .layout = {
                            .sizing = {
                                .width = 16.0 * state->ui.scale_factor,
                                .height = 16.0 * state->ui.scale_factor
                            },
                        },
                        .image = tex
                    }) {}
                }
                CLAY_TEXT(modeName, CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14.0 * state->ui.scale_factor,
                    .textColor = label_color,
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
