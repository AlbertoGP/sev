// Tab data structures and functions
// Tabs form a doubly-linked list, each containing a pane tree.

#pragma once

#include <stdbool.h>
#include "pane.h"

#define TAB_NAME_MAX 256

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
// Get the current tab.
Tab *tab_get_current(void);
// Sets current tab to next (circular).
void tab_next(void);
// Sets current tab to prev (circular).
void tab_prev(void);

// Get the root pane of the current tab.
Pane *tab_get_root_pane(void);
// Set the root pane of the current tab.
void tab_set_root_pane(Pane *pane);
// Close the current tab.
void tab_close_current(void);

// Clay component for rendering the tab selector bar.
void TabBar(AppState *state);
// Clay component for rendering the contents of the currently selected tab.
void TabContent(AppState *state);
