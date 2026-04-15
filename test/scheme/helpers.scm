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
