;;; helpers.scm — small DSL used by the vim-motion suite.
;;;
;;; seed! replaces the current buffer's contents with a known string and
;;; places the point at the given offset (default 0). motion invokes a
;;; motion proc by its registered name, passing a count — this bypasses
;;; the full vim-execute-motion dispatch (which reads the private
;;; vim-count state variable) and exercises the motion logic directly.

(define (seed! text . rest)
  (test-reset-buffer! text)
  (let ((pos (if (pair? rest) (car rest) 0)))
    (point-set! pos)))

(define (at) (point-get))

(define (motion name . rest)
  (let ((count (if (pair? rest) (car rest) 1))
        (proc (motion-ref name)))
    (when proc (proc count))))

;;; Full buffer contents as a string — used by operator/visual tests to
;;; assert buffer state after a destructive command.
(define (buffer-text)
  (%buffer-substring 0 (buffer-length)))

;;; Set up a visual-mode selection: mark #\< at anchor, point at cur, and
;;; the given select mode (1=char, 2=line). Assumes caller already seeded
;;; the buffer.
(define (select! anchor cur mode)
  (point-set! anchor)
  (%mark-set-to-point! #\<)
  (point-set! cur)
  (%select-mode-set! mode))

;;; Reset the unnamed register so one test's residue can't mask the next
;;; test's assertion.
(define (clear-reg!)
  (%register-set! #\" "")
  (%register-set-shape! #\" 'charwise))
