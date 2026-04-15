;;; main.scm — entry point loaded by the C test harness.
;;;
;;; The harness has already imported (scheme base) and (editor vim) into
;;; the test env, so this file only needs the SRFI 64 runner. After
;;; (test-end) the harness reads the top-level binding *fail-count* to
;;; gate the Unity test.

(import (srfi 64))

(include "test/scheme/helpers.scm")
(include "test/scheme/vim/motion-test.scm")
(include "test/scheme/vim/operator-test.scm")
(include "test/scheme/vim/visual-test.scm")

(test-begin "sev-scheme")

(run-vim-motion-tests)
(run-vim-operator-tests)
(run-vim-visual-tests)

(define runner (test-runner-current))
(define *fail-count* (test-runner-fail-count runner))

(test-end "sev-scheme")
