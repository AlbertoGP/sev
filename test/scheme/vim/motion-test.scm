;;; motion-test.scm — SRFI 64 suite for scheme/editor/vim/motion.scm
;;;
;;; Each test seeds the buffer with a known string, invokes a motion
;;; proc from *motion-table* directly (via the `motion` helper), and
;;; asserts the resulting point position.
;;;
;;; Going through the motion table rather than the `vim-motion-X`
;;; command wrappers avoids the full vim-execute-motion dispatch —
;;; that path reads private state (vim-count, vim-sm-state) that the
;;; vim library does not export. The motion procs themselves take
;;; count as an argument and are the unit of behaviour we want to pin.

(define (run-vim-motion-tests)

  (test-group "motion h/l — horizontal"
    (test-equal "l count 1 from 0" 1
      (begin (seed! "hello") (motion 'motion-l 1) (at)))
    (test-equal "l count 3 from 0" 3
      (begin (seed! "hello") (motion 'motion-l 3) (at)))
    (test-equal "h count 2 from 4" 2
      (begin (seed! "hello" 4) (motion 'motion-h 2) (at))))

  (test-group "motion j/k — vertical"
    ;; "abc\ndef\nghi" — positions: a0 b1 c2 \n3 d4 e5 f6 \n7 g8 h9 i10
    (test-equal "j from col 0 line 0 → line 1 col 0" 4
      (begin (seed! "abc\ndef\nghi") (motion 'motion-j 1) (at)))
    (test-equal "j count 2 → line 2 col 0" 8
      (begin (seed! "abc\ndef\nghi") (motion 'motion-j 2) (at)))
    (test-equal "k from line 2 → line 1 col 0" 4
      (begin (seed! "abc\ndef\nghi" 8) (motion 'motion-k 1) (at))))

  (test-group "motion 0/$/^"
    ;; "  hi\n"
    (test-equal "0 from middle of indented line" 0
      (begin (seed! "  hi\n" 3) (motion 'motion-0 1) (at)))
    (test-equal "^ from 0 on indented line → first non-blank" 2
      (begin (seed! "  hi\n") (motion 'motion-^ 1) (at)))
    ;; "hello\n" — $ goes to last char before newline (pos 4, the 'o')
    (test-equal "$ from 0 → end of line before newline" 5
      (begin (seed! "hello\nworld\n") (motion 'motion-$ 1) (at))))

  (test-group "motion w — word forward"
    ;; "foo bar baz" — f0 o1 o2 _3 b4 a5 r6 _7 b8 a9 z10
    (test-equal "w from 0 → start of 'bar'" 4
      (begin (seed! "foo bar baz") (motion 'motion-w 1) (at)))
    (test-equal "w from 4 → start of 'baz'" 8
      (begin (seed! "foo bar baz" 4) (motion 'motion-w 1) (at)))
    ;; punctuation boundary: "foo-bar baz" — word 'foo', punct '-', word 'bar'
    ;; w from 0 skips word "foo" → at '-' (pos 3), not ws so stops there.
    (test-equal "w from 0 on 'foo-bar' → '-'" 3
      (begin (seed! "foo-bar baz") (motion 'motion-w 1) (at))))

  (test-group "motion b — word backward"
    ;; "foo bar baz" — from pos 10 ('z'), b → start of 'baz' (8)
    (test-equal "b from 'z' → start of 'baz'" 8
      (begin (seed! "foo bar baz" 10) (motion 'motion-b 1) (at)))
    (test-equal "b from start of 'baz' → start of 'bar'" 4
      (begin (seed! "foo bar baz" 8) (motion 'motion-b 1) (at))))

  (test-group "motion e — end of word"
    ;; "foo bar" — e from 0 → 'o' at end of "foo" (pos 2)
    (test-equal "e from 0 → end of 'foo'" 2
      (begin (seed! "foo bar") (motion 'motion-e 1) (at)))
    (test-equal "e from 2 → end of 'bar'" 6
      (begin (seed! "foo bar" 2) (motion 'motion-e 1) (at))))

  (test-group "motion W/B/E — WORD variants"
    ;; "foo-bar baz" — W treats 'foo-bar' as one WORD
    (test-equal "W from 0 → start of 'baz'" 8
      (begin (seed! "foo-bar baz") (motion 'motion-W 1) (at)))
    ;; B moves back one WORD; from pos 10 ('z') that's start of 'baz'.
    (test-equal "B from 'z' in 'baz' → start of 'baz'" 8
      (begin (seed! "foo-bar baz" 10) (motion 'motion-B 1) (at)))
    (test-equal "B count 2 from 'z' → start of 'foo-bar'" 0
      (begin (seed! "foo-bar baz" 10) (motion 'motion-B 2) (at)))
    (test-equal "E from 0 → end of 'foo-bar'" 6
      (begin (seed! "foo-bar baz") (motion 'motion-E 1) (at))))

  (test-group "motion gg/G — line jumps"
    ;; "line1\nline2\nline3" (no trailing newline) — line starts 0, 6, 12
    (test-equal "goto-line 1 from end → pos 0" 0
      (begin (seed! "line1\nline2\nline3" 15)
             (motion 'motion-goto-line 1) (at)))
    (test-equal "goto-line 3 → start of third line" 12
      (begin (seed! "line1\nline2\nline3")
             (motion 'motion-goto-line 3) (at)))
    (test-equal "goto-line beyond total clamps to last line" 12
      (begin (seed! "line1\nline2\nline3")
             (motion 'motion-goto-line 99) (at))))

  )
