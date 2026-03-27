// Tab data structures and functions.
// Tabs are linked-list nodes owned by ContentPane (PANE_CONTENT leaves).
// Each tab wraps a TabContent, which holds the actual view state.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vline.h"
#include "../text/buffer.h"
#include "../text/jump_list.h"
#include "../state.h"

// Buffer-specific tab content: rendering state for a text buffer view.
typedef struct {
    Buffer    *buffer;
    VLineCache vline_cache;
    JumpList   jump_list;
    float      width;           // last-rendered width  (vline cache invalidation)
    float      height;          // last-rendered height
    uint64_t   layout_version;
} BufferContent;

typedef enum {
    TAB_BUFFER,
} TabContentType;

// Union wrapper making tabs extensible to non-buffer content in future.
typedef struct {
    TabContentType type;
    union {
        BufferContent buffer;
    };
} TabContent;

// A tab is a node in a doubly-linked list owned by a content pane.
typedef struct Tab {
    struct Tab *next;
    struct Tab *prev;
    TabContent content;
} Tab;

// Forward-declare Pane and ContentPane (defined in pane.h, which includes this header).
struct Pane;
struct ContentPane;

// Initialise window pointer for title updates.
bool tab_system_init(AppState *state);

// Allocate a new Tab node (initialises vline_cache; caller links into list).
Tab *tab_alloc(Buffer *buf);
// Free a Tab node (destroys vline_cache and jump_list).
void tab_free(Tab *tab);

// --- Per-display-pane tab operations ---

// Append a new tab for buf to dp's tab list and make it active.
Tab *display_tab_new(struct Pane *dp, Buffer *buf);
// Close dp's active tab. Returns true if the tab list is now empty
// (caller should then close the pane).
bool display_tab_close(struct Pane *dp);
// Cycle to the next tab in dp's list (circular).
bool display_tab_next(struct Pane *dp);
// Cycle to the previous tab in dp's list (circular).
bool display_tab_prev(struct Pane *dp);

// Create a new tab backed by the named buffer (creating buffer if needed)
// and open it in the active display pane (creating a root pane if none exists).
// Returns true on success.
bool tab_new_with_buffer(const char *buf_name);

// Set a tab's buffer and invalidate its vline cache.
void tab_set_buffer(Tab *tab, Buffer *buf);

// Update the window title to reflect the active tab.
void update_window_title(void);

// Reset the tab callback slot pool. Call once per frame before layout.
void tab_cb_reset(void);
// Execute any pending middle-click tab close and clear the middle-click flag.
// Call once per frame after layout (before rendering).
void tab_flush_pending_close(void);
// Free strings allocated during tab rendering. Call after SDL_Clay_RenderClayCommands().
void tab_free_strings(void);
// Register a heap-allocated string for deferred free via tab_free_strings().
// Used by buf_render.c to register buffer text and line-number string slabs.
void tab_register_string(char *s);
// Clay component: per-pane tab bar (no global app icon).
void TabBar(AppState *state, struct Pane *dp, int32_t index);

// Close the active tab in the active pane (closes pane if last tab).
void tab_close(void);

// --- Scheme bindings ---
#include <chibi/sexp.h>
sexp scm_tab_close(sexp ctx, sexp self, sexp n);
sexp scm_tab_next(sexp ctx, sexp self, sexp n);
sexp scm_tab_prev(sexp ctx, sexp self, sexp n);
sexp scm_tab_new(sexp ctx, sexp self, sexp n, sexp sbuf_name);
sexp scm_no_panes_p(sexp ctx, sexp self, sexp n);
sexp scm_buffer_close(sexp ctx, sexp self, sexp n, sexp sname);
