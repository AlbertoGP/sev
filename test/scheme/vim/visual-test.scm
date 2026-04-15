;;; visual-test.scm — SRFI 64 suite for scheme/editor/vim/visual.scm
;;;
;;; visual-mode internals (vim-visual-delete, vim-visual-yank, ...) are
;;; not exported. We exercise them via the defcommand wrappers (vim-d,
;;; vim-y, vim-c, vim-x) which dispatch to the visual path when
;;; (%select-mode-get) > 0. Selection is set up by `select!` in helpers.
;;;
;;; Select-mode codes: 1 = char (inclusive), 2 = line, 3/4 = rect
;;; (rect is deferred to a later phase).

(define (run-vim-visual-tests)

  (test-group "vim-y — charwise selection"
    ;; "hello world" — anchor 0, point 4, char mode is inclusive so the
    ;; yank range covers [0, 5) → "hello". visual-yank sets point to
    ;; sel-min (0) and returns to normal.
    (test-equal "register holds selection" "hello"
      (begin (clear-reg!) (seed! "hello world") (select! 0 4 1)
             (vim-y)
             (%register-get #\")))
    (test-equal "register shape is charwise" 'charwise
      (%register-shape #\"))
    (test-equal "buffer unchanged" "hello world"
      (buffer-text))
    (test-equal "point at sel-min" 0
      (point-get)))

  (test-group "vim-y — linewise selection"
    ;; "a\nb\nc\n" — select lines 0..1 in line mode: full lines "a" and "b"
    ;; expand to the range covering both whole lines, yielding "a\nb\n".
    (test-equal "register holds two full lines" "a\nb\n"
      (begin (clear-reg!) (seed! "a\nb\nc\n") (select! 0 2 2)
             (vim-y)
             (%register-get #\")))
    (test-equal "register shape is linewise" 'linewise
      (%register-shape #\"))
    (test-equal "buffer unchanged" "a\nb\nc\n"
      (buffer-text)))

  (test-group "vim-d — charwise selection"
    ;; "hello world" — delete [6, 11) → "hello "; point clamps to sel-min
    (test-equal "buffer after delete" "hello "
      (begin (clear-reg!) (seed! "hello world") (select! 6 10 1)
             (vim-d)
             (buffer-text)))
    (test-equal "register holds deleted text" "world"
      (%register-get #\"))
    (test-equal "point at sel-min" 6
      (point-get)))

  (test-group "vim-d — linewise selection"
    (test-equal "buffer after line delete" "c\n"
      (begin (clear-reg!) (seed! "a\nb\nc\n") (select! 0 2 2)
             (vim-d)
             (buffer-text)))
    (test-equal "register holds deleted lines" "a\nb\n"
      (%register-get #\"))
    (test-equal "register shape is linewise" 'linewise
      (%register-shape #\")))

  (test-group "vim-d — linewise last-line pulls preceding newline"
    ;; "a\nb" — select the 'b' line, delete should leave "a" not "a\n".
    (test-equal "buffer collapses empty trailing line" "a"
      (begin (clear-reg!) (seed! "a\nb") (select! 2 2 2)
             (vim-d)
             (buffer-text))))

  (test-group "vim-c — charwise selection"
    (test-equal "buffer after change" "hello "
      (begin (clear-reg!) (seed! "hello world") (select! 6 10 1)
             (vim-c)
             (buffer-text)))
    (test-equal "point at sel-min" 6
      (point-get)))

  (test-group "vim-x == vim-d in visual mode"
    ;; One spot-check: vim-x dispatches to vim-visual-delete when
    ;; select-mode > 0, same as vim-d.
    (test-equal "vim-x produces same buffer as vim-d" "hello "
      (begin (clear-reg!) (seed! "hello world") (select! 6 10 1)
             (vim-x)
             (buffer-text))))

  )
