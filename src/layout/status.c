#include "../state.h"
#include "../subeditor/buffer.h"
#include "pane.h"
#include "theme.h"

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
        ? ui_resolve_color(state, state->ui.roles.text_primary)
        : ui_resolve_color(state, state->ui.roles.text_faded);
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
            },
            .padding = { .right = 10, .left = pane->content.active ? 0 : 10 },
            .childGap = 10
        },
        .backgroundColor = ui_resolve_color(state, state->ui.roles.bar_bg),
        .clip = {
            .horizontal = true
        }
    }) {
        if (pane->content.active) {
            CLAY(CLAY_ID("Mode Name"), {
                .layout = {
                    .padding = { .left = 10, .right = 10 }
                },
                .backgroundColor = ui_get_mode_color(state)
            }){
                CLAY_TEXT(modeName, CLAY_TEXT_CONFIG({
                    .fontId = FONT_BOLD,
                    .fontSize = 14,
                    .textColor = ui_resolve_color(state, state->ui.roles.ui_bg),
                }));
            }
        }
        CLAY_TEXT(bufName, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
        CLAY_AUTO_ID({
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }}
        }) {}
        CLAY_TEXT(pointPos, CLAY_TEXT_CONFIG({
            .fontId = FONT_NORMAL,
            .fontSize = 14,
            .textColor = textColor,
        }));
    }
}
