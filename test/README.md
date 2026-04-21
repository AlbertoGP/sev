# Tests

Tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored in `test/unity/`) as the C test runner, and [SRFI 64](https://srfi.schemers.org/srfi-64/) for the Scheme layer.

```
make test          # run all suites
make test-text     # C text-subsystem tests only
make test-scheme   # Scheme-layer tests only
```

## C tests (text subsystem)

Each C test file has its own `main()` with `UNITY_BEGIN()` / `UNITY_END()` and calls `RUN_TEST()` for each case. `setUp` and `tearDown` bracket every test.

`test_stubs.c` provides `test_stubs_init_minimal_chibi()` — a bare chibi context sufficient for tests that only touch `src/text/`. Use it when the bug doesn't cross into Scheme.

To add a new C test:

1. Create `test/test_<name>.c`, include `unity/unity.h` and `test_stubs.h`.
2. Implement `setUp` / `tearDown`, your `test_*` functions, and `main`.
3. Add a `test-<name>` target in `Makefile` following the pattern of `test-text`, then add it to the `test` phony dependency.

## Scheme tests

The Scheme suite runs through a thin C gate (`test_scheme_vim_motion.c`) that boots chibi, imports `(editor vim)` alongside the real `src/text/` bindings, then calls `scheme_test_run("./test/scheme/main.scm")` and fails the Unity test if SRFI 64 reports any failures.

`test/scheme/main.scm` is the entry point — it includes each suite file and calls its top-level `run-*-tests` procedure between `(test-begin …)` and `(test-end …)`.

`test/scheme/helpers.scm` provides a small DSL:

- `(seed! text [pos])` — fill the current buffer and place point
- `(motion name [count])` — invoke a motion proc from `*motion-table*` directly
- `(at)` — return the current point position
- `(buffer-text)` — return full buffer contents as a string
- `(select! anchor cur mode)` — set up a visual-mode selection
- `(clear-reg!)` — reset the unnamed register between tests

To add a new Scheme test suite:

1. Create `test/scheme/<area>/<suite>-test.scm` with a `(define (run-<suite>-tests) …)` procedure containing `(test-group …)` blocks.
2. `(include …)` the new file in `test/scheme/main.scm` and call `(run-<suite>-tests)` between the existing `test-begin` / `test-end` forms.
