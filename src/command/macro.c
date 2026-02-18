#include <SDL3/SDL.h>
#include <chibi/eval.h>

#include "scheme_internal.h"   // G, AppState
#include "keymap.h"            // key_dispatch()

sexp scm_macro_start(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char ch = (char)sexp_unbox_character(sname);
    int idx = (ch >= 'a' && ch <= 'z') ? ch - 'a' : -1;
    if (idx < 0 || G->macro_recording) return SEXP_VOID;
    G->macro_target_reg = idx;
    G->macro_buf_len = 0;   // clear recording buffer (reuse allocation)
    G->macro_recording = true;
    G->macro_skip_next = true;  // don't record the key that triggered us
    return SEXP_VOID;
}

sexp scm_macro_stop(sexp ctx, sexp self, sexp n) {
    if (!G->macro_recording) return SEXP_VOID;
    G->macro_recording = false;
    int idx = G->macro_target_reg;
    // Copy buf → macro_store[idx]
    SDL_free(G->macro_store[idx]);
    G->macro_store[idx] = NULL;
    G->macro_store_len[idx] = 0;
    if (G->macro_buf_len > 0) {
        size_t bytes = G->macro_buf_len * sizeof(KeyEvent);
        G->macro_store[idx] = SDL_malloc(bytes);
        if (G->macro_store[idx]) {
            SDL_memcpy(G->macro_store[idx], G->macro_buf, bytes);
            G->macro_store_len[idx] = G->macro_buf_len;
        }
    }
    return SEXP_VOID;
}

sexp scm_macro_play(sexp ctx, sexp self, sexp n, sexp sname) {
    sexp_assert_type(ctx, sexp_charp, SEXP_CHAR, sname);
    char ch = (char)sexp_unbox_character(sname);
    int idx = (ch >= 'a' && ch <= 'z') ? ch - 'a' : -1;
    if (idx < 0 || G->macro_recording) return SEXP_VOID;
    size_t len = G->macro_store_len[idx];
    KeyEvent *evs = G->macro_store[idx];
    if (!evs || len == 0) return SEXP_VOID;
    // Make a local copy in case playback modifies store
    KeyEvent *copy = SDL_malloc(len * sizeof(KeyEvent));
    if (!copy) return SEXP_VOID;
    SDL_memcpy(copy, evs, len * sizeof(KeyEvent));
    // Reset key state so events replay from global map, not any active prefix sub-map
    G->input.current_map = G->input.global_map;
    for (size_t i = 0; i < len; i++)
        key_dispatch(G, &copy[i]);
    SDL_free(copy);
    return SEXP_VOID;
}

sexp scm_macro_is_recording(sexp ctx, sexp self, sexp n) {
    return G->macro_recording ? SEXP_TRUE : SEXP_FALSE;
}
