;;; macro-test.scm — SRFI 64 suite for scheme/editor/vim/macro.scm
;;;
;;; %macro-start!, %macro-stop!, and %macro-recording? are promoted to real
;;; C bindings in scheme_test_init.c, so the recording state machine is
;;; fully testable.
;;;
;;; %macro-play is a stub: playback calls key_dispatch() which requires the
;;; full C keymap infrastructure (G->input.global_map), never initialised in
;;; the test harness. Tests that invoke vim-play-last-macro verify only that
;;; the Scheme glue doesn't error and that the buffer is left untouched.

(define (run-vim-macro-tests)

  (test-group "macro recording state"
    (test-equal "not recording before any start" #f
      (%macro-recording?))
    (test-equal "recording after %macro-start!" #t
      (begin (%macro-start! #\a) (%macro-recording?)))
    (test-equal "not recording after %macro-stop!" #f
      (begin (%macro-stop!) (%macro-recording?))))

  (test-group "vim-play-last-macro smoke"
    (test-equal "buffer unchanged after play-last-macro" "hello"
      (begin
        (seed! "hello")
        (vim-play-last-macro)
        (buffer-text)))
    (test-equal "point unchanged after play-last-macro" 0
      (at)))

  )
