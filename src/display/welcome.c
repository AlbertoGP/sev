#include <string.h>

#include "icon.h"
#include "theme.h"
#include "welcome.h"
#include "../command/keymap.h"

static void XSpacer(void) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0)
            }
        }
    }){}
}

static void SuggestionRow(AppState *state, Clay_String label, Clay_String key, const char *icon_name) {
    float font_size = 12 * state->ui.scale_factor;
    TextStyle key_style  = ui_resolve_text_style(state, state->ui.roles.text_key,
                                                  FONT_BUF_NORMAL, font_size);
    int icon_size = 11.0 * state->ui.scale_factor;
    SDL_Texture *icon = icon_get(icon_name,   state, icon_size, icon_size);
    CLAY_AUTO_ID({
        .layout = { 
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .padding = { .right = 5.0 * state->ui.scale_factor, .left = 5.0 * state->ui.scale_factor },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .childGap = 7.0 * state->ui.scale_factor
        }
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width  = icon_size,
                    .height = icon_size
                }
            },
            .image = { .imageData = icon },
        }) {}
        CLAY_TEXT(label, CLAY_TEXT_CONFIG({
            .fontId = FONT_UI_NORMAL,
            .fontSize = font_size,
            .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
        }));
        XSpacer();
        CLAY_TEXT(key, CLAY_TEXT_CONFIG({
            .fontId    = key_style.font_id,
            .fontSize  = key_style.font_size,
            .textColor = key_style.color,
        }));
    }
}

void WelcomePane(AppState *state) {
    float icon_size = 64.0f * state->ui.scale_factor;
    CLAY(CLAY_ID("Welcome"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = 10.0 * state->ui.scale_factor,
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.pane_bg)
    }) {
        SDL_Texture *icon = icon_get("welcome-icon", state, (int)icon_size, (int)icon_size);
        CLAY(CLAY_ID("Banner"), {
            .layout = {
                .padding = { .right = 8.0 * state->ui.scale_factor },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childGap = 8.0 * state->ui.scale_factor,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
        }){
            CLAY(CLAY_ID("Welcome Icon"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(icon_size),
                        .height = CLAY_SIZING_FIXED(icon_size)
                    }
                },
                .image = icon
            }) {}
            CLAY(CLAY_ID("Banner Text"), {
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap = 4.0 * state->ui.scale_factor,
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                },
            }){
                CLAY_TEXT(CLAY_STRING("Welcome back to sev"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI_NORMAL,
                    .fontSize = 16.0 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
                }));
                CLAY_TEXT(CLAY_STRING("An extensible text editor."), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI_ITALIC,
                    .fontSize = 10.0 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_primary),
                }));
            }
        }
        CLAY_AUTO_ID({
            .layout = { .sizing = { .height = CLAY_SIZING_FIXED(15 * state->ui.scale_factor) } }
        }) {}
        static char kb_new_tab[64] = "SPC t n";
        static char kb_help[64]   = "SPC h";
        static char kb_command[64] = ":";
        keymap_where_is_first(state, "tab-new",                  kb_new_tab,  sizeof(kb_new_tab));
        keymap_where_is_first(state, "execute-extended-command", kb_command,  sizeof(kb_command));
        CLAY(CLAY_ID("Get Started Title"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(400 * state->ui.scale_factor)
                },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                .childGap = 5.0 * state->ui.scale_factor,
            }
        }) {
            CLAY_TEXT(CLAY_STRING("GET STARTED"), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI_NORMAL,
                .fontSize = 9.0 * state->ui.scale_factor,
                .textColor = ui_resolve_color(state, state->ui.roles.text_primary)
            }));
            CLAY(CLAY_ID_LOCAL("Get Started Divider"), {
                .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) } },
                .border = { .width = { .top = 1 }, .color = ui_resolve_color(state, state->ui.roles.border_active)   }
            }) {}
        }
        CLAY(CLAY_ID("Get Started Suggestions"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(400 * state->ui.scale_factor)
                },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 5.0 * state->ui.scale_factor,
            }
        }) {
            SuggestionRow(state, CLAY_STRING("New File"),
                                 (Clay_String){ .length = strlen(kb_new_tab), .chars = kb_new_tab }, "new-icon");
            SuggestionRow(state, CLAY_STRING("Help Commands"),
                                 (Clay_String){ .length = strlen(kb_help),    .chars = kb_help    }, "help-icon");
            SuggestionRow(state, CLAY_STRING("Open Command Palette"),
                                 (Clay_String){ .length = strlen(kb_command), .chars = kb_command }, "palette-icon");
        }
    }
}
