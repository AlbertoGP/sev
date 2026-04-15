#pragma once

// Initialize a bare chibi-scheme context for tests that only exercise
// the text subsystem. The scheme-layer test binary does NOT call this —
// it uses the richer scheme_test_init() instead.
void test_stubs_init_minimal_chibi(void);
