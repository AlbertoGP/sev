// Pane data structures and functions.
// Panes form a binary tree of splits and content leaves.
// PANE_CONTENT leaves each own a tab list.

#pragma once

#include <stdbool.h>

#include "tab.h"
#include "../state.h"

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// A content pane: owns a doubly-linked tab list and tracks which tab is active.
// Also stores per-pane geometry for mouse hit-testing (updated each frame).
typedef struct ContentPane {
    Tab  *list;          // head of tab list (NULL only while being constructed)
    Tab  *active_tab;    // currently displayed tab
    bool  active;        // true if this is the focused content pane

    // Geometry updated each frame during rendering; used for mouse hit-testing.
    float    text_origin_x;
    float    text_origin_y;
    float    text_origin_w;
    float    text_origin_h;
    float    gutter_width_px;
    int      line_height_px;
    uint16_t render_font_id;
    uint16_t render_font_size;
    float    pane_right;      // box.x + box.width of Buffer Text element (includes padding)

    // Scrollbar hit-test region (updated each frame).
    float scrollbar_x;        // left edge of hit-test strip
    float scrollbar_y;        // top of track
    float scrollbar_track_h;  // height of track (= text_origin_h)
    bool  has_scrollbar;
    float scrollbar_thumb_y;  // screen y of thumb top (absolute, updated each frame)
    float scrollbar_thumb_h;  // thumb height in pixels
} ContentPane;

// A vertical split node in a pane tree.
typedef struct {
    float left_width;
    struct Pane *left;
    struct Pane *right;
} VSplit;

// A horizontal split node in a pane tree.
typedef struct {
    float top_height;
    struct Pane *top;
    struct Pane *bottom;
} HSplit;

typedef enum {
    PANE_CONTENT,   // leaf: owns a tab list
    PANE_V_SPLIT,
    PANE_H_SPLIT
} PaneType;

// A pane is either a content leaf (owns tabs) or a split node.
typedef struct Pane {
    PaneType type;
    union {
        ContentPane content;
        VSplit v_split;
        HSplit h_split;
    };
    struct Pane *parent;
} Pane;

// Initialise pane system (store window pointer; root starts NULL → welcome).
bool pane_init(AppState *state);
// Shutdown: destroy root pane tree.
void pane_quit(void);
// Get the global root pane (NULL if no panes open).
Pane *pane_get_root(void);

// Create a new PANE_CONTENT with a single tab showing buf.
// Does NOT link it into the tree or set root_pane; caller does that.
Pane *pane_content_create(Buffer *buf);
// Recursively free resources allocated for a pane sub-tree.
void pane_destroy(Pane *pane);
// Close the active display pane (or just its active tab if multiple tabs).
void pane_close(void);

// Returns the currently active PANE_DISPLAY in the pane tree.
Pane *pane_get_active(void);
// Makes the specified display pane the active one.
bool pane_set_active(Pane *pane);

// Splits a pane. Use PANE_H_SPLIT or PANE_V_SPLIT.
// The new sibling pane gets one tab showing the same buffer as pane's active tab.
Pane *pane_split(Pane *pane, PaneType split_type);
// Navigate to adjacent pane in the given direction.
bool pane_navigate(Direction dir);
// Check if pane has a neighbour in the given direction.
bool pane_has_neighbour(Pane *pane, Direction dir);
// Get the sibling of a pane (NULL if root).
Pane *pane_get_sibling(Pane *pane);
// Replace a child in a parent split node (or update root if parent is NULL).
void pane_replace_child(Pane *parent, Pane *old_child, Pane *new_child);

// Convenience wrappers.
static inline Pane *pane_split_horizontal(Pane *pane) { return pane_split(pane, PANE_H_SPLIT); }
static inline Pane *pane_split_vertical(Pane *pane)   { return pane_split(pane, PANE_V_SPLIT); }
static inline bool pane_navigate_up(void)    { return pane_navigate(DIR_UP); }
static inline bool pane_navigate_down(void)  { return pane_navigate(DIR_DOWN); }
static inline bool pane_navigate_left(void)  { return pane_navigate(DIR_LEFT); }
static inline bool pane_navigate_right(void) { return pane_navigate(DIR_RIGHT); }

bool pane_v_split_increase(void);
bool pane_v_split_decrease(void);
bool pane_h_split_increase(void);
bool pane_h_split_decrease(void);

// Sync the current buffer to match the active pane's active tab.
// Call after changing active pane or switching tabs.
void sync_active_buffer(void);

// Walk the pane tree and return the PANE_DISPLAY leaf whose screen area
// contains (x, y), or NULL if none match.
Pane *pane_at_coords(Pane *root, float x, float y);

// Clay component for rendering pane contents.
void PaneContent(AppState *state, Pane *pane, int32_t index, float width, float height);

// No-op kept for call-site compatibility; string cleanup now in tab_free_strings().
void pane_free_strings(void);

// Push current buffer position onto the active pane's active tab's jump list.
void pane_push_jump(void);

// Scheme bindings.
#include <chibi/sexp.h>
sexp scm_pane_navigate_up(sexp ctx, sexp self, sexp n);
sexp scm_pane_navigate_down(sexp ctx, sexp self, sexp n);
sexp scm_pane_navigate_left(sexp ctx, sexp self, sexp n);
sexp scm_pane_navigate_right(sexp ctx, sexp self, sexp n);
sexp scm_pane_v_split_increase(sexp ctx, sexp self, sexp n);
sexp scm_pane_v_split_decrease(sexp ctx, sexp self, sexp n);
sexp scm_pane_h_split_increase(sexp ctx, sexp self, sexp n);
sexp scm_pane_h_split_decrease(sexp ctx, sexp self, sexp n);
sexp scm_split_vertical(sexp ctx, sexp self, sexp n);
sexp scm_split_horizontal(sexp ctx, sexp self, sexp n);
sexp scm_pane_close(sexp ctx, sexp self, sexp n);
sexp scm_jump_push(sexp ctx, sexp self, sexp n);
sexp scm_jump_backward(sexp ctx, sexp self, sexp n);
sexp scm_jump_forward(sexp ctx, sexp self, sexp n);
sexp scm_set_mouse_click_handler(sexp ctx, sexp self, sexp n, sexp cb);
sexp scm_set_mouse_drag_handler(sexp ctx, sexp self, sexp n, sexp cb);
