#include <string.h>
#include <unistd.h>

#include "decoration.h"
#include "icon.h"
#include "keybinding.h"
#include "theme.h"
#include "welcome.h"
#include "../command/keymap.h"
#include "../command/message.h"
#include "../command/scheme_internal.h"
#include "../state_io.h"

static const char *pending_welcome_cmd = NULL;
static const char *pending_project_path = NULL;

static void HandleWelcomeRowClick(Clay_ElementId id, Clay_PointerData p, void *userData) {
    (void)id;
    if (p.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    pending_welcome_cmd = (const char *)userData;
}

static void HandleProjectRowClick(Clay_ElementId id, Clay_PointerData p, void *userData) {
    (void)id;
    if (p.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;
    pending_project_path = (const char *)userData;
}

void welcome_flush_pending(void) {
    if (pending_project_path) {
        const char *path = pending_project_path;
        pending_project_path = NULL;
        if (chdir(path) == 0) {
            state_io_update_recent_project(G, path);
            static char msg[PATH_MAX + 32];
            snprintf(msg, sizeof(msg), "Opened project %s", path);
            message_echo(msg);
        } else {
            static char msg[PATH_MAX + 32];
            snprintf(msg, sizeof(msg), "Cannot open project: %s", path);
            message_echo(msg);
        }
        G->needs_extra_frame = true;
        return;
    }
    if (!pending_welcome_cmd) return;
    sexp ctx = G->chibi.ctx;
    sexp sym = sexp_intern(ctx, pending_welcome_cmd, -1);
    sexp result = sexp_apply(ctx, G->chibi.call_interactively, sexp_list1(ctx, sym));
    if (sexp_exceptionp(result))
        sexp_print_exception(ctx, result, sexp_current_error_port(ctx));
    pending_welcome_cmd = NULL;
    G->needs_extra_frame = true;
}

static void SuggestionRow(AppState *state, Clay_String label, Clay_String key, const char *icon_name, const char *cmd) {
    float font_size = 12 * state->ui.scale_factor;
    float key_font_size = 11 * state->ui.scale_factor;
    int icon_size = 11.0 * state->ui.scale_factor;
    SDL_Texture *icon = icon_get(icon_name,   state, icon_size, icon_size);
    Clay_Color bg = ui_resolve_color(state, state->ui.roles.line_bg);
    Clay_Color hover_bg = { bg.r, bg.g, bg.b, 128 };
    CLAY_AUTO_ID({
        .layout = { 
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .padding = {
                .left = 5 * state->ui.scale_factor,
                .right = 5 * state->ui.scale_factor,
                .top = 3 * state->ui.scale_factor,
                .bottom = 3 * state->ui.scale_factor,
            },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .childGap = 7.0 * state->ui.scale_factor
        },
        .backgroundColor = Clay_Hovered() && cmd ? hover_bg : (Clay_Color){0},
        .cornerRadius = CLAY_CORNER_RADIUS(3 * state->ui.scale_factor)
    }) {
        if (cmd) {
            if (Clay_Hovered()) state->input.desired_cursor = SDL_SYSTEM_CURSOR_POINTER;
            Clay_OnHover(HandleWelcomeRowClick, (void *)cmd);
        }
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
        Keybinding(state, key, key_font_size);
    }
}

static void ProjectRow(AppState *state, Clay_String label, Clay_String key, const char *path) {
    float font_size = 12 * state->ui.scale_factor;
    float key_font_size = 11 * state->ui.scale_factor;
    int icon_size = 11.0 * state->ui.scale_factor;
    SDL_Texture *icon = icon_get("project-icon", state, icon_size, icon_size);
    Clay_Color bg = ui_resolve_color(state, state->ui.roles.line_bg);
    Clay_Color hover_bg = { bg.r, bg.g, bg.b, 128 };
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .padding = {
                .left = 5 * state->ui.scale_factor,
                .right = 5 * state->ui.scale_factor,
                .top = 3 * state->ui.scale_factor,
                .bottom = 3 * state->ui.scale_factor,
            },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .childGap = 7.0 * state->ui.scale_factor
        },
        .backgroundColor = Clay_Hovered() ? hover_bg : (Clay_Color){0},
        .cornerRadius = CLAY_CORNER_RADIUS(3 * state->ui.scale_factor)
    }) {
        if (Clay_Hovered()) state->input.desired_cursor = SDL_SYSTEM_CURSOR_POINTER;
        Clay_OnHover(HandleProjectRowClick, (void *)path);
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
        Keybinding(state, key, key_font_size);
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
                .padding = { .right = 8.0 * state->ui.scale_factor, .bottom = 15.0 * state->ui.scale_factor },
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
        static char kb_new_tab[64] = "";
        static char kb_open_project[64] = "";
        static char kb_help[64]   = "";
        static char kb_command[64] = "";
        keymap_where_is_first(state, "tab-new",                  kb_new_tab,       sizeof(kb_new_tab));
        keymap_where_is_first(state, "open-project",             kb_open_project,  sizeof(kb_open_project));
        keymap_where_is_first(state, "command-palette",          kb_command,       sizeof(kb_command));
        keymap_where_is_first(state, "describe-command",         kb_help,          sizeof(kb_help));
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
                .padding = { .bottom = 15.0 * state->ui.scale_factor },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }) {
            SuggestionRow(state, CLAY_STRING("New File"),
                                 (Clay_String){ .length = strlen(kb_new_tab), .chars = kb_new_tab }, "new-icon",
                                 "tab-new");
            SuggestionRow(state, CLAY_STRING("Open Project"),
                                 (Clay_String){ .length = strlen(kb_open_project), .chars = kb_open_project }, "open-icon",
                                 "open-project");
            SuggestionRow(state, CLAY_STRING("Describe Command"),
                                 (Clay_String){ .length = strlen(kb_help),    .chars = kb_help    }, "help-icon",
                                 "describe-command");
            SuggestionRow(state, CLAY_STRING("Open Command Palette"),
                                 (Clay_String){ .length = strlen(kb_command), .chars = kb_command }, "palette-icon",
                                 "command-palette");
        }
        if (state->recent_projects_count > 0) {
            // Build a display order sorted by last_opened descending
            int order[RECENT_PROJECTS_MAX];
            int n = state->recent_projects_count;
            for (int i = 0; i < n; i++) order[i] = i;
            // Simple insertion sort (n <= 5)
            for (int i = 1; i < n; i++) {
                int tmp = order[i];
                int j = i - 1;
                while (j >= 0 && state->recent_projects[order[j]].last_opened <
                                  state->recent_projects[tmp].last_opened) {
                    order[j + 1] = order[j];
                    j--;
                }
                order[j + 1] = tmp;
            }

            CLAY(CLAY_ID("Recent Projects Title"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(400 * state->ui.scale_factor)
                    },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                    .childGap = 5.0 * state->ui.scale_factor,
                }
            }) {
                CLAY_TEXT(CLAY_STRING("RECENT PROJECTS"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI_NORMAL,
                    .fontSize = 9.0 * state->ui.scale_factor,
                    .textColor = ui_resolve_color(state, state->ui.roles.text_primary)
                }));
                CLAY(CLAY_ID_LOCAL("Recent Projects Divider"), {
                    .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) } },
                    .border = { .width = { .top = 1 }, .color = ui_resolve_color(state, state->ui.roles.border_active) }
                }) {}
            }
            CLAY(CLAY_ID("Recent Projects"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(400 * state->ui.scale_factor)
                    },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                }
            }) {
                static const char *key_labels[] = { "1", "2", "3", "4", "5" };
                for (int i = 0; i < n; i++) {
                    const RecentProject *rp = &state->recent_projects[order[i]];
                    // Last path segment for display
                    const char *slash = strrchr(rp->path, '/');
                    const char *name  = (slash && slash[1]) ? slash + 1 : rp->path;
                    Clay_String label = { .length = (int)strlen(name), .chars = name };
                    Clay_String key   = { .length = 1, .chars = key_labels[i] };
                    ProjectRow(state, label, key, rp->path);
                }
            }
        }
    }
}
