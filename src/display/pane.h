// Pane data structures and functions
// Panes form a binary tree of splits/content within a tab.

#pragma once

#include <stdbool.h>

#include "vline.h"
#include "../state.h"
#include "../text/buffer.h"
#include "../text/jump_list.h"

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    CONTENT_TEXT,
    CONTENT_UI
} ContentType;

// A content node in a pane tree.
// Can either be associated with a buffer, or a custom UI component.
// Only one content pane can be active in a tab at any time.
typedef struct {
    ContentType type;
    union {
        Buffer *buffer;
        // TODO: add custom UI type.
    };
    uint64_t layout_version;
    float width;
    float height;
    bool active;
    VLineCache vline_cache;  // Cache for visual (wrapped) lines
    JumpList jump_list;
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

// Creates and returns a pointer to a new pane.
Pane *pane_create(void);
// Recursively free resources allocated for a pane sub-tree.
void pane_destroy(Pane *pane);
// Closes current pane.
// If it is the root pane in a tab, the tab is closed.
void pane_close(void);
// Sets a pane's content to a specified buffer.
bool pane_set_buffer(Pane *pane, Buffer *buf);
// Returns the currently active pane in the currently selected tab.
Pane *pane_get_active(void);
// Makes the specified pane the active pane.
bool pane_set_active(Pane *pane);

// Splits a pane. Use PANE_H_SPLIT or PANE_V_SPLIT.
Pane *pane_split(Pane *pane, PaneType split_type);
// Navigate to adjacent pane in the given direction.
bool pane_navigate(Direction dir);
// Check if pane has a neighbour in the given direction.
bool pane_has_neighbour(Pane *pane, Direction dir);
// Get the sibling of a pane (NULL if root).
Pane *pane_get_sibling(Pane *pane);
// Replace a child in a parent split node.
void pane_replace_child(Pane *parent, Pane *old_child, Pane *new_child);

// Convenience wrappers (call pane_split/pane_navigate internally).
static inline Pane *pane_split_horizontal(Pane *pane) { return pane_split(pane, PANE_H_SPLIT); }
static inline Pane *pane_split_vertical(Pane *pane) { return pane_split(pane, PANE_V_SPLIT); }
static inline bool pane_navigate_up(void) { return pane_navigate(DIR_UP); }
static inline bool pane_navigate_down(void) { return pane_navigate(DIR_DOWN); }
static inline bool pane_navigate_left(void) { return pane_navigate(DIR_LEFT); }
static inline bool pane_navigate_right(void) { return pane_navigate(DIR_RIGHT); }

bool pane_v_split_increase(void);
bool pane_v_split_decrease(void);
bool pane_h_split_increase(void);
bool pane_h_split_decrease(void);

// Sync the current buffer to match the active pane.
// Call after changing active pane or switching tabs.
void sync_active_buffer(void);

// Clay component for rendering pane contents.
void PaneContent(AppState *state, Pane *pane, int32_t index, float width, float height);

// Free all strings allocated during layout. Call after rendering.
void pane_free_strings(void);

// Push current buffer position onto the active pane's jump list.
void pane_push_jump(void);
