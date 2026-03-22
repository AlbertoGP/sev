// Tab data structures and functions.
// Tabs are now thin wrappers owned by PANE_DISPLAY nodes.
// Each tab holds a buffer view, vline cache, and jump list.

#pragma once

#include <stdbool.h>

#include "vline.h"
#include "../text/buffer.h"
#include "../text/jump_list.h"
#include "../state.h"

#define TAB_NAME_MAX 256

// A tab is a node in a doubly-linked list owned by a display pane.
// It holds rendering state so switching tabs preserves scroll/cache.
typedef struct Tab {
    struct Tab *next;
    struct Tab *prev;
    char name[TAB_NAME_MAX];

    Buffer *buffer;
    VLineCache vline_cache;
    JumpList jump_list;
    float width;           // last-rendered width  (vline cache invalidation)
    float height;          // last-rendered height
    uint64_t layout_version;
} Tab;

// Forward-declare Pane (defined in pane.h, which includes this header).
struct Pane;

// Initialise window pointer for title updates.
bool tab_system_init(AppState *state);

// Allocate a new Tab node (initialises vline_cache; caller links into list).
Tab *tab_alloc(const char *name, Buffer *buf);
// Free a Tab node (destroys vline_cache and jump_list).
void tab_free(Tab *tab);

// --- Per-display-pane tab operations ---

// Append a new tab for buf to dp's tab list and make it active.
Tab *display_tab_new(struct Pane *dp, Buffer *buf, const char *name);
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

// Set a tab's buffer, invalidate its vline cache, and sync its name.
void tab_set_buffer(Tab *tab, Buffer *buf);
// Sync a tab's name from its buffer's current name.
void tab_sync_name(Tab *tab);

// Update the window title to reflect the active tab.
void update_window_title(void);

// Clay component: per-pane tab bar (no global app icon).
void TabBar(AppState *state, struct Pane *dp, int32_t index);

// --- Scheme bindings ---
#include <chibi/sexp.h>
sexp scm_tab_next(sexp ctx, sexp self, sexp n);
sexp scm_tab_prev(sexp ctx, sexp self, sexp n);
sexp scm_tab_new(sexp ctx, sexp self, sexp n, sexp sbuf_name);
sexp scm_no_panes_p(sexp ctx, sexp self, sexp n);
sexp scm_buffer_close(sexp ctx, sexp self, sexp n, sexp sname);
