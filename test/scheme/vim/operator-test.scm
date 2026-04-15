;;; operator-test.scm — SRFI 64 suite for scheme/editor/vim/operator.scm
;;;
;;; Operators are invoked directly via (operator-ref 'op-X) with a
;;; hand-built <range>, bypassing vim-enter-operator / vim-execute-motion
;;; dispatch (which reads private vim-pending-op / vim-count state the
;;; vim library does not export). Registered operators and make-range /
;;; range-* are the unit of behaviour we want to pin.

(define (op name)
  (operator-ref name))

(define (run-vim-operator-tests)

  (test-group "op-delete — charwise"
    (test-equal "buffer after delete [0,5)" " world"
      (begin (clear-reg!) (seed! "hello world")
             ((op 'op-delete) (make-range 0 5 'char))
             (buffer-text)))
    (test-equal "register holds deleted text" "hello"
      (%register-get #\"))
    (test-equal "register shape is charwise" 'charwise
      (%register-shape #\")))

  (test-group "op-delete — linewise"
    (test-equal "buffer after delete first line" "b\nc\n"
      (begin (clear-reg!) (seed! "a\nb\nc\n")
             ((op 'op-delete) (make-range 0 2 'line))
             (buffer-text)))
    (test-equal "register holds line text" "a\n"
      (%register-get #\"))
    (test-equal "register shape is linewise" 'linewise
      (%register-shape #\")))

  (test-group "op-delete — linewise last-line pulls preceding newline"
    ;; "a\nb" — deleting the 'b' line (range [2,3) 'line) should leave "a",
    ;; not "a\n". operator.scm:110-115 expands start back past the \n.
    (test-equal "buffer collapses trailing empty line" "a"
      (begin (clear-reg!) (seed! "a\nb")
             ((op 'op-delete) (make-range 2 3 'line))
             (buffer-text))))

  (test-group "op-yank"
    ;; "foo bar" — yank [0,3), point was at 5, op-yank resets to start.
    (test-equal "buffer unchanged after yank" "foo bar"
      (begin (clear-reg!) (seed! "foo bar" 5)
             ((op 'op-yank) (make-range 0 3 'char))
             (buffer-text)))
    (test-equal "register holds yanked text" "foo"
      (%register-get #\"))
    (test-equal "point moved to range start" 0
      (point-get)))

  (test-group "op-change"
    ;; "foo\nbar\n" — change [0,4) 'line. operator.scm:123-126 trims end
    ;; back past the newline so we stay on the same line. Expected buffer
    ;; "\nbar\n" (the 'foo' is gone but the \n between 'foo' and 'bar'
    ;; remains), point at 0.
    (test-equal "buffer after change" "\nbar\n"
      (begin (clear-reg!) (seed! "foo\nbar\n")
             ((op 'op-change) (make-range 0 4 'line))
             (buffer-text)))
    (test-equal "point at range start" 0
      (point-get)))

  (test-group "vim-x — default count in normal mode"
    (test-equal "delete single char at point 0" "bc"
      (begin (clear-reg!) (seed! "abc") (%select-mode-set! 0)
             (vim-x)
             (buffer-text))))

  )
