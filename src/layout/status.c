#include "../state.h"
#include "../subeditor/buffer.h"
#include "pane.h"

static void Spacer(int size) {
    CLAY_AUTO_ID({
        .layout = {
        .sizing = {
        .width = CLAY_SIZING_FIXED(size)
    }}}) {}
}

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
    Clay_String bufName = {
         .chars = buffer_get_name(buf),
         .length = strlen(buffer_get_name(buf)),
    };
    char *pos = malloc(12 * sizeof(char));
    snprintf(pos, 12, "%zu", point_get(buf).pos);
    bar_strings_push(pos);
    Clay_String pointPos = {
         .chars = pos,
         .length = strlen(pos),
    };
    char *ccount = malloc(12 * sizeof(char));
    snprintf(ccount, 12, "%zu", get_char_count(buf));
    bar_strings_push(ccount);
    Clay_String charCount = {
         .chars = ccount,
         .length = strlen(ccount),
    };
    char *lncount = malloc(24 * sizeof(char));
    snprintf(lncount, 24, "%zu / %zu", point_get_line(buf), get_line_count(buf));
    bar_strings_push(lncount);
    Clay_String lineCount = {
         .chars = lncount,
         .length = strlen(lncount),
    };
    char *colcount = malloc(24 * sizeof(char));
    snprintf(colcount, 24, "%d", get_column(buf));
    bar_strings_push(colcount);
    Clay_String col = {
         .chars = colcount,
         .length = strlen(colcount),
    };
    Clay_Color textColor = active ? state->colors.text : state->colors.textFaded;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .padding = { .left = 10 }
        },
        .backgroundColor = state->colors.bar,
        .clip = {
            .horizontal = true
        }
    }) {
        CLAY_TEXT(CLAY_STRING("Buffer: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_TEXT(bufName, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Point: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_TEXT(pointPos, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Chars: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_TEXT(charCount, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Line: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_TEXT(lineCount, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
        Spacer(25);
        CLAY_TEXT(CLAY_STRING("Col: "), CLAY_TEXT_CONFIG({
            .fontId = FONT_BOLD,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_TEXT(col, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
    }
}
