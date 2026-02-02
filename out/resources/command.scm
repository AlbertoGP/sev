;;; command.scm - Self-documenting command infrastructure
;;; Emacs-like docstrings, interactive commands, and introspection

(import (srfi 69))

;; Storage tables
(define *doc-table* (make-hash-table eq?))           ; sym -> docstring
(define *interactive-table* (make-hash-table eq?))   ; sym -> spec
(define *key-bindings-table* (make-hash-table eq?))  ; sym -> (list "C-x" ...)

;; Documentation API
(define (set-doc! sym doc)
  (hash-table-set! *doc-table* sym doc))

(define (get-doc sym)
  (hash-table-ref/default *doc-table* sym #f))

;; Interactive command API
(define (make-interactive! sym spec)
  (hash-table-set! *interactive-table* sym spec))

(define (interactive? sym)
  (hash-table-exists? *interactive-table* sym))

(define (interactive-spec sym)
  (hash-table-ref/default *interactive-table* sym #f))

;; Reverse keymap lookup
(define (register-key-binding! sym keystr)
  (let ((existing (hash-table-ref/default *key-bindings-table* sym '())))
    (hash-table-set! *key-bindings-table* sym (cons keystr existing))))

(define (where-is sym)
  (hash-table-ref/default *key-bindings-table* sym '()))

;; Enhanced set-key! (wraps C primitive %set-key!)
(define (set-key! keymap keystr cmd)
  (cond
    ((symbol? cmd)
     (register-key-binding! cmd keystr)
     (%set-key! keymap keystr cmd))
    (else
     (%set-key! keymap keystr cmd))))

;; call-interactively - invoke command with args based on interactive spec
(define (call-interactively sym)
  (let ((proc (eval sym))
        (spec (interactive-spec sym)))
    (cond
      ((not spec) (proc))
      ((equal? spec "") (proc))
      ((equal? spec "p") (proc (prefix-arg)))
      ((equal? spec "P") (proc (or (prefix-arg) 1)))
      (else (proc)))))

;; Introspection
(define (list-commands)
  (hash-table-keys *interactive-table*))

(define (describe-function sym)
  (let ((doc (get-doc sym))
        (keys (where-is sym))
        (int? (interactive? sym)))
    (list
      (cons 'name sym)
      (cons 'interactive? int?)
      (cons 'keys keys)
      (cons 'doc (or doc "")))))
