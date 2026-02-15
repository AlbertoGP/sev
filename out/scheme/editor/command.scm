;;; command.scm - Self-documenting command infrastructure
;;; Emacs-like docstrings, interactive commands, and introspection

;; Documentation record type
(define-record-type <doc>
  (make-doc kind text)
  doc?
  (kind doc-kind)
  (text doc-text))

;; Storage tables
(define *doc-table* (make-hash-table eq?))           ; sym -> <doc>
(define *interactive-table* (make-hash-table eq?))   ; sym -> spec
(define *key-bindings-table* (make-hash-table eq?))  ; sym -> (list "C-x" ...)

;; Documentation API
(define (set-doc! sym kind text)
  (hash-table-set! *doc-table* sym (make-doc kind text)))

(define (get-doc sym)
  (hash-table-ref/default *doc-table* sym #f))

(define (get-doc-text sym)
  (let ((doc (get-doc sym)))
    (and doc (doc-text doc))))

;; Query by kind
(define (list-by-kind k)
  (filter (lambda (sym)
            (let ((doc (get-doc sym)))
              (and doc (eq? k (doc-kind doc)))))
          (hash-table-keys *doc-table*)))

(define (list-functions) (list-by-kind 'function))
(define (list-variables) (list-by-kind 'variable))

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

;; S-expression interactive spec resolution
(define (resolve-arg-form form)
  (case (car form)
    ((prefix)
     (if (pair? (cdr form))
         (list (or (prefix-arg) (cadr form)))
         (list (prefix-arg))))
    (else
     (error "Unknown interactive spec" form))))

(define (resolve-interactive-spec spec)
  (apply append (map resolve-arg-form spec)))

;; call-interactively - invoke command with args based on interactive spec
(define (call-interactively sym)
  (let ((proc (eval sym))
        (spec (interactive-spec sym)))
    (if (or (not spec) (null? spec))
        (proc)
        (apply proc (resolve-interactive-spec spec)))))

;; Introspection
(define (list-commands) (list-by-kind 'command))

(define (describe-function sym)
  (let ((doc (get-doc sym))
        (keys (where-is sym))
        (int? (interactive? sym)))
    (list
      (cons 'name sym)
      (cons 'interactive? int?)
      (cons 'keys keys)
      (cons 'kind (and doc (doc-kind doc)))
      (cons 'doc (if doc (doc-text doc) "")))))

;; defcommand macro - declare commands concisely
(define-syntax defcommand
  (syntax-rules (interactive)
    ;; Scheme command with interactive spec and body
    ((defcommand (name args ...) docstring (interactive specs ...) body ...)
     (begin
       (define (name args ...) body ...)
       (set-doc! 'name 'command docstring)
       (make-interactive! 'name '(specs ...))))
    ;; Scheme command with body, no interactive spec
    ((defcommand (name args ...) docstring body ...)
     (begin
       (define (name args ...) body ...)
       (set-doc! 'name 'command docstring)
       (make-interactive! 'name '())))
    ;; C primitive with interactive spec, no body
    ((defcommand name docstring (interactive specs ...))
     (begin
       (set-doc! 'name 'command docstring)
       (make-interactive! 'name '(specs ...))))
    ;; C primitive, no interactive spec, no body
    ((defcommand name docstring)
     (begin
       (set-doc! 'name 'command docstring)
       (make-interactive! 'name '())))))

;; defvar macro - declare documented variables concisely
(define-syntax defvar
  (syntax-rules ()
    ((defvar name docstring value)
     (begin
       (define name value)
       (set-doc! 'name 'variable docstring)))))

;; defun macro - declare documented functions concisely
(define-syntax defun
  (syntax-rules ()
    ((defun (name args ...) docstring body ...)
     (begin
       (define (name args ...) body ...)
       (set-doc! 'name 'function docstring)))))
