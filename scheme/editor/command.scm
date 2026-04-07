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
(define *keymap-edges* (make-hash-table eq?))    ; child-keymap -> list of (parent-keymap . prefix-str)
(define *keymap-commands* (make-hash-table eq?)) ; command-sym -> list of (keymap . keystr)

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
(define (register-keymap-edge! parent prev child)
  (let ((existing (hash-table-ref/default *keymap-edges* child '())))
    (hash-table-set! *keymap-edges* child (cons (cons parent prev) existing))))

(define (register-command-binding! keymap prefix sym)
  (let ((existing (hash-table-ref/default *keymap-commands* sym '())))
    (hash-table-set! *keymap-commands* sym (cons (cons keymap prefix) existing))))

(define (resolve-paths keymap prefix)
  (let ((parents (hash-table-ref/default *keymap-edges* keymap '())))
    (if (null? parents)
        (list prefix)
        (apply append
               (map (lambda (p)
                      (let ((parent-map (car p))
                            (parent-prefix (cdr p)))
                        (resolve-paths parent-map (string-append parent-prefix " " prefix))))
                    parents)))))

(define (where-is sym)
  (let ((bindings (hash-table-ref/default *keymap-commands* sym '())))
    (apply append
           (map (lambda (b)
                  (resolve-paths (car b) (cdr b)))
                bindings))))

(define (bind-prefix! parent keystr child)
  (register-keymap-edge! parent keystr child)
  (%bind-prefix! parent keystr child))

;; Enhanced set-key! (wraps C primitive %set-key!)
(define (set-key! keymap keystr cmd)
  (cond
    ((symbol? cmd)
     (register-command-binding! keymap keystr cmd)
     (%set-key! keymap keystr cmd))
    (else
     (%set-key! keymap keystr cmd))))

;; Extensible async spec handler table
(define *spec-handlers* (make-hash-table eq?))

(define (register-spec-handler! type handler)
  ;; handler: (lambda (form-args done) ...) where done takes one value
  (hash-table-set! *spec-handlers* type handler))

;; CPS traversal: resolve each spec form left-to-right, then call done with all args
(define (resolve-spec-cps forms collected done)
  (if (null? forms)
      (done (reverse collected))
      (let* ((form    (car forms))
             (rest    (cdr forms))
             (handler (hash-table-ref/default *spec-handlers* (car form) #f)))
        (if (not handler)
            (error "Unknown interactive spec type" (car form))
            (handler (cdr form)
                     (lambda (val)
                       (resolve-spec-cps rest (cons val collected) done)))))))

;; Built-in prefix handler (synchronous: calls done immediately)
(register-spec-handler! 'prefix
  (lambda (form-args done)
    (done (if (pair? form-args)
              (or (prefix-arg) (car form-args))
              (prefix-arg)))))

;; call-interactively - invoke command with args based on interactive spec
(define (call-interactively sym)
  (let ((proc (eval sym))
        (spec (interactive-spec sym)))
    (if (or (not spec) (null? spec))
        (proc)
        (resolve-spec-cps spec '() (lambda (args) (apply proc args))))))

;; Introspection
(define (list-commands) (list-by-kind 'command))

(define (command-first-binding sym)
  (let ((ks (where-is sym)))
    (if (pair? ks) (car ks) #f)))

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

(defcommand (describe-function fname)
  "Display documentation for a named function or command."
  (interactive (minibuffer-read "Describe function: "))
  (let* ((sym  (string->symbol fname))
         (doc  (get-doc sym))
         (keys (where-is sym)))
    (%pop-to-buffer "*Help*")
    ;; Set up help-mode on first use (buffer_create auto-enables evil-normal-mode)
    (when (not (%buffer-has-minor-mode? 'help-mode))
      (%disable-minor-mode 'evil-normal-mode)
      (%enable-minor-mode 'help-mode)
      (%set-local! 'mode-name "Help"))
    ;; Header: name + kind
    (%insert-string fname)
    (%insert-string "    [")
    (%insert-string (if doc (symbol->string (doc-kind doc)) "unknown"))
    (%insert-string "]")
    (newline)
    (newline)
    ;; Key bindings
    (when (pair? keys)
      (%insert-string "Keys: ")
      (%insert-string (car keys))
      (newline)
      (newline))
    ;; Docstring
    (%insert-string (if doc (doc-text doc) "(no documentation)"))
    (newline)
    ;; Go to top
    (point-set! 0)))

(defcommand (describe-key)
  "Read a key sequence and describe its binding."
  (message "Describe key: ")
  (%read-key-binding
    (lambda (sym key-str)
      (if sym
          (describe-function (symbol->string sym))
          (message (string-append key-str " is unbound"))))))

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
