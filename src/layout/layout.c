#include "../state.h"

#include "current_mode.h"
#include "toggle_button.h"

#include "../subeditor/buffer.h"
#include <stdio.h>

Clay_RenderCommandArray create_app_layout(AppState *state) {
    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    Clay_BeginLayout();

    CLAY(CLAY_ID("Layout Background"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = layoutExpand,
            .padding = CLAY_PADDING_ALL(32),
            .childGap = 8,
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER
            }
        },
        .backgroundColor = state->colors.background
    }) {
        CLAY_TEXT(CLAY_STRING("Going Interactive"), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 32,
            .textColor = state->colors.text,
        }));
        CLAY(CLAY_ID("Main Container"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = layoutExpand,
                .padding = {
                    .top = 24
                },
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER
                },
                .childGap = 24
            },
            .backgroundColor = state->colors.foreground,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            current_mode(state);
            toggle_button(state);

            int length = get_char_count();
            char *chars = buffer_text();
            int point = point_get().pos;
            Clay_String head = {
                .chars = chars,
                .length = point,
                .isStaticallyAllocated = true
            };
            Clay_String tail = {
                .chars = chars + point,
                .length = length - point,
                .isStaticallyAllocated = true
            };

            CLAY(CLAY_ID("buffer"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0)
                    },
                    .padding = CLAY_PADDING_ALL(24)
                }
            }) {
                CLAY_TEXT(head, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 18,
                    .textColor = state->colors.text,
                }));
                CLAY_TEXT(tail, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 18,
                    .textColor = state->colors.textFaded,
                }));
            }
        }
        Clay_String bufName = {
             .chars = buffer_get_name(),
             .length = strlen(buffer_get_name()),
        };
        static char pos[12];
        snprintf(pos, 12, "%d", point_get().pos);
        Clay_String pointPos = {
             .chars = pos,
             .length = strlen(pos),
        };
        static char ccount[12];
        snprintf(ccount, 12, "%d", get_char_count());
        Clay_String charCount = {
             .chars = ccount,
             .length = strlen(ccount),
        };
        CLAY(CLAY_ID("Status Bar"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(5)
            },
            .backgroundColor = state->colors.bar,
        }) {
             CLAY_TEXT(CLAY_STRING("Buffer: "), CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
             CLAY_TEXT(bufName, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
             CLAY(CLAY_ID("spacer"), {
             .layout = {
             .sizing = {
             .width = CLAY_SIZING_FIXED(25)
             }
             }
             }) {}
             CLAY_TEXT(CLAY_STRING("Point: "), CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
             CLAY_TEXT(pointPos, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
             CLAY(CLAY_ID("spacer 2"), {
             .layout = {
             .sizing = {
             .width = CLAY_SIZING_FIXED(25)
             }
             }
             }) {}
             CLAY_TEXT(CLAY_STRING("Chars: "), CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
             CLAY_TEXT(charCount, CLAY_TEXT_CONFIG({
                    .fontId = FONT_NORMAL,
                    .fontSize = 14,
                    .textColor = state->colors.text,
                }));
        }
    }

    return Clay_EndLayout();
}
