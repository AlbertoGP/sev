#include <SDL3/SDL_render.h>
#include <chibi/sexp.h>
#include <stdio.h>

#include "icon.h"
#include "mode_icon.h"
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

void Divider(AppState *state) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .height = 12.0 * state->ui.scale_factor,
                .width = CLAY_SIZING_FIXED(2),
            }
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.border_inactive)
    }) {}
}


void StatusBar(AppState *state) {
    Buffer *buf = buffer_get_current();
    if (!buf) return;

    CachedRoles roles = state->ui.roles;

    sexp mode_symbol = sexp_intern(state->chibi.ctx, "mode-name", -1);
    sexp mode_val = vartable_get(buffer_get_locals(buf), mode_symbol, SEXP_FALSE);
    const char* modeStr = sexp_to_cstring(state->chibi.ctx, mode_val, "ERROR");
    Clay_String modeName = {
        .chars = modeStr,
        .length = strlen(modeStr),
        .isStaticallyAllocated = true
    };

    char *pos = malloc(32 * sizeof(char));
    snprintf(pos, 32, "%zu:%d", buf_get_line(buf), get_column(buf));
    bar_strings_push(pos);
    Clay_String pointPos = {
         .chars = pos,
         .length = strlen(pos),
    };
    Clay_Color textColor = ui_resolve_color(state, roles.text_primary);

    CLAY(CLAY_ID("Status Bar"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .padding = {
                 .right = 10.0 * state->ui.scale_factor,
            },
            .childGap = 10.0 * state->ui.scale_factor,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .border = {
            .width = {
                .bottom = 2
            },
            .color = ui_resolve_color(state, roles.border_inactive)
        },
        .backgroundColor = ui_resolve_color(state, roles.bar_bg),
        .clip = {
            .horizontal = true
        }
    }) {
        ModeIconEntry *icon = mode_icon_for_current_buffer();
        Clay_Color mode_bg = icon
            ? ui_resolve_color(state, icon->role_mode_bg)
            : ui_resolve_color(state, roles.mode_normal);
        Clay_Color label_color = icon
            ? ui_resolve_color(state, icon->role_label)
            : (Clay_Color){255, 0, 255, 255};

        bool recording = state->macro_recording;

        CLAY(CLAY_ID("Mode Name"), {
            .layout = {
                .padding = {
                    .left = 8.0 * state->ui.scale_factor,
                    .right = 14.0 * state->ui.scale_factor
                },
                .childGap = 3.0 * state->ui.scale_factor,
                .childAlignment = {
                    .y = CLAY_ALIGN_Y_CENTER
                },
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
                .fontId = FONT_UI_BOLD,
                .fontSize = 14.0 * state->ui.scale_factor,
                .textColor = label_color,
            }));
        }
        if (recording) {
            float radius = 3.0 * state->ui.scale_factor;
            CLAY(CLAY_ID("Macro indicator"), {
                .layout = {
                    .sizing = { .height = CLAY_SIZING_GROW(0) },
                    .childAlignment = {
                        .y = CLAY_ALIGN_Y_CENTER
                    },
                    .childGap = 4.0 * state->ui.scale_factor
                },
            }){
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_GROW(0) },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                        .padding = { .top = (radius / 2) },
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {
                                .height = CLAY_SIZING_FIXED(2.0 * radius),
                                .width = CLAY_SIZING_FIXED(2.0 * radius)
                            }
                        },
                        .cornerRadius = CLAY_CORNER_RADIUS(radius),
                        .backgroundColor = ui_resolve_color(state, roles.macro_indicator)
                    }) {}
                }
                CLAY_TEXT(CLAY_STRING("REC"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI_BOLD,
                    .fontSize = 12.0 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, roles.text_primary),
                }));
            }
        }
        CLAY_AUTO_ID({
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }}
        }) {}
        Mode *major = buffer_get_major_mode(buf);
        if (major && strcmp(major->name, "scheme-mode") == 0) {
            CLAY(CLAY_ID("Scheme Mode"), {
                .layout = {
                    .childGap = 3.0 * state->ui.scale_factor,
                    .childAlignment = {
                        .y = CLAY_ALIGN_Y_CENTER
                    },
                },
            }){
                SDL_Texture *tex = icon_get("scheme-icon", state, 14, 14);
                CLAY(CLAY_ID("Scheme Mode Icon"), {
                    .layout = {
                        .sizing = {
                            .width = 14.0 * state->ui.scale_factor,
                            .height = 14.0 * state->ui.scale_factor
                        },
                    },
                    .image = tex
                }) {}
                CLAY_TEXT(CLAY_STRING("Scheme"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI_NORMAL,
                    .fontSize = 12.0 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
                }));
            }
            Divider(state);
        }
        CLAY_TEXT(pointPos, CLAY_TEXT_CONFIG({
            .fontId = FONT_UI_NORMAL,
            .fontSize = 14.0 * state->ui.scale_factor,
            .textColor = textColor,
        }));
    }
}
