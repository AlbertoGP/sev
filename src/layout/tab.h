// Tab and Pane data structures and functions

#pragma once

#include <stdbool.h>
#include "../state.h"
#include "../subeditor/buffer.h"

#define TAB_NAME_MAX 256

typedef enum {
    CONTENT_TEXT,
    CONTENT_UI
} ContentType;

// A content node in a pane tree.
// Can either be associated with a buffer, or a custom UI component.
// Only one content pane can be active in a tab at any time.
typedef struct {
    bool active;
    ContentType type;
    union {
        Buffer *buffer;
        // TODO: add custom UI type.
    };
} Content;

// A vertical split node in a pane tree.
// Records the width of the left sub-pane.
typedef struct {
    float left_width;
    struct Pane *left;
    struct Pane *right;
} VSplit;

// A horizontal split node in a pane tree.
// Records the height of the upper sub-pane.
typedef struct {
    float top_height;
    struct Pane *top;
    struct Pane *bottom;
} HSplit;

typedef enum {
    PANE_CONTENT,
    PANE_V_SPLIT,
    PANE_H_SPLIT
} PaneType;

// A pane either contains content, or is split into sub-panes.
typedef struct Pane {
    PaneType type;
    union {
        Content content;
        VSplit v_split;
        HSplit h_split;
    };
    struct Pane *parent;
} Pane;

// A tab is a node in a doubly-linked list of named Panes.
typedef struct Tab {
    struct Tab *next;
    struct Tab *prev;
    char name[TAB_NAME_MAX];

    Pane *contents;
} Tab;

// List of all active tabs.
typedef struct {
    Tab *list;
    Tab *current;
} TabList;

// Initialise tab list.
bool tab_list_init(void);
// Free up resources allocated to tab list.
void tab_list_quit(void);

// Create a new, empty tab with the given name.
bool tab_create(const char *name);
// Sets current tab to next (circular).
void tab_next(void);
// Sets current tab to prev (circular).
void tab_prev(void);

// Creates and returns a pointer to a new pane.
Pane *pane_create(void);
// Closes current pane.
// If it is the root pane in a tab, the tab is closed.
void pane_close(void);
// Sets a pane to a specified buffer.
// If make_active is true, the pane becomes focused.
bool pane_set_to_buffer(Pane *pane, const char *buf, bool make_active);
// Returns the currently active pane in the currently selected tab.
Pane *pane_get_active(void);

// Splits the currently active pane horizontally.
bool pane_split_horizontal(Pane *pane);
// Splits the currently active pane vertically.
bool pane_split_vertical(Pane *pane);
bool pane_navigate_up(void);
bool pane_navigate_down(void);
bool pane_navigate_left(void);
bool pane_navigate_right(void);

// Clay component for rendering the tab selector bar.
void TabBar(AppState *state);
// Clay component for rendering the contents of the currently selected tab.
void TabContent(AppState *state);
