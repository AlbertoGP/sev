#include "../state.h"
#include "../subeditor/buffer.h"

static void Spacer(int size) {
    CLAY_AUTO_ID({
        .layout = {
        .sizing = {
        .width = CLAY_SIZING_FIXED(size)
    }}}) {}
}

void StatusBar(AppState *state) {
    Clay_String bufName = {
         .chars = buffer_get_name(),
         .length = strlen(buffer_get_name()),
    };
    static char pos[12];
    snprintf(pos, 12, "%zu", point_get().pos);
    Clay_String pointPos = {
         .chars = pos,
         .length = strlen(pos),
    };
    static char ccount[12];
    snprintf(ccount, 12, "%zu", get_char_count());
    Clay_String charCount = {
         .chars = ccount,
         .length = strlen(ccount),
    };
    static char lncount[24];
    snprintf(lncount, 24, "%zu / %zu", point_get_line(), get_line_count());
    Clay_String lineCount = {
         .chars = lncount,
         .length = strlen(lncount),
    };
    static char colcount[24];
    snprintf(colcount, 24, "%d", get_column());
    Clay_String col = {
         .chars = colcount,
         .length = strlen(colcount),
    };
    CLAY(CLAY_ID("Status Bar"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            // .padding = CLAY_PADDING_ALL(5)
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
        Spacer(25);
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
        Spacer(25);
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
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Line: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = state->colors.text,
        }));
        CLAY_TEXT(lineCount, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = state->colors.text,
        }));
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Col: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = state->colors.text,
        }));
        CLAY_TEXT(col, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = state->colors.text,
        }));
    }
}
