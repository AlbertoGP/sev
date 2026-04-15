// Stubs for symbols that text/*.c references but that live in the command
// or display subsystems. The test only exercises text-subsystem code paths,
// so these can be no-ops / static storage.
//
// `reset_buffer_scale` (called from `buffer_create` via a static-inline
// display header) reads `default-buffer-scale` from the scheme env, so
// tests must call `test_stubs_init_minimal_chibi()` before creating any
// buffer — except the scheme-layer test binary, which calls its own
// fuller `scheme_test_init()` and should not call this one.

#include <chibi/eval.h>

#include "../src/state.h"
#include "../src/command/keyevent.h"
#include "../src/command/mode.h"
#include "test_stubs.h"

static AppState g_fake_state;
AppState *G = &g_fake_state;

KeyEvent last_event;

void message_send(const char *message) { (void)message; }
void message_clear(void)                { }

void modelist_init(ModeList *ml)    { if (ml) { ml->head = NULL; ml->count = 0; } }
void modelist_destroy(ModeList *ml) { (void)ml; }
Mode *mode_get_default(void)        { return NULL; }
Mode *mode_lookup(const char *name, ModeType type) { (void)name; (void)type; return NULL; }

void test_stubs_init_minimal_chibi(void) {
    sexp_scheme_init();
    sexp ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 0);
    sexp env = sexp_load_standard_env(ctx, NULL, SEXP_SEVEN);
    G->chibi.ctx = ctx;
    G->chibi.env = env;
}
