#pragma once

// Minimal chibi-scheme harness for the scheme-layer test binary.
// Boots a chibi context, registers a narrow (editor primitives) module
// whose real bindings come from src/text/*.c and whose command/display
// primitives are no-op stubs, then imports (editor vim) so vim commands
// like (vim-motion-w) are callable.
//
// scheme_test_run loads a .scm file containing a SRFI 64 suite and
// returns the runner's fail count, or -1 if the load raised an exception.

void scheme_test_init(void);
void scheme_test_quit(void);
int  scheme_test_run(const char *path);
