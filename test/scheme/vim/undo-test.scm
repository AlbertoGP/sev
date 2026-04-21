;;; undo-test.scm — SRFI 64 suite for scheme/editor/vim/undo.scm
;;;
;;; %undo, %redo, %line-restore, %begin-change, and %end-change are all real
;;; C bindings (scheme_test_init.c). vim-undo, vim-redo, and vim-line-restore
;;; are thin wrappers that delegate to them; the tests confirm the delegation
;;; actually fires and produces correct buffer/point state.
;;;
;;; vim-repeat is not tested here: it requires a repeat-info record with a
;;; known motion or setup, which can only be constructed via the private
;;; vim module internals. The operator and visual suites exercise the paths
;;; that write repeat-info as a side effect.

(define (run-vim-undo-tests)

  (test-group "vim-undo — reverts insertion"
    (test-equal "buffer restored after undo" "hello"
      (begin
        (seed! "hello")
        (%begin-change)
        (insert-char #\X)
        (%end-change)
        (vim-undo)
        (buffer-text))))

  (test-group "vim-redo — re-applies after undo"
    (test-equal "buffer re-modified after redo" "Xhello"
      (begin
        (seed! "hello")
        (%begin-change)
        (insert-char #\X)
        (%end-change)
        (vim-undo)
        (vim-redo)
        (buffer-text))))

  (test-group "vim-undo — no-op when history is empty"
    (test-equal "buffer unchanged after undo on fresh seed" "abc"
      (begin
        (seed! "abc")
        (vim-undo)
        (buffer-text))))

  (test-group "vim-line-restore — reverts current-line edits"
    (test-equal "buffer restored to pre-edit state" "foo\nbar\n"
      (begin
        (seed! "foo\nbar\n")
        (%begin-change)
        (insert-char #\Z)
        (insert-char #\Z)
        (%end-change)
        (vim-line-restore)
        (buffer-text)))
    (test-equal "point at line start after restore" 0
      (at)))

  (test-group "vim-zero — moves to line start when no count pending"
    ;; vim-count is #f on import → vim-zero dispatches motion-0 → line start.
    ;; "abc\ndef": a0 b1 c2 \n3 d4 e5 f6
    ;; Starting at pos 5 (col 1 of line 2), vim-zero should land at pos 4.
    (test-equal "point at start of current line" 4
      (begin
        (seed! "abc\ndef" 5)
        (vim-zero)
        (at))))

  )
