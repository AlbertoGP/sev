;; Mode system wrapper functions

;; Register a major mode with an optional keymap
;; Returns the mode object or #f on failure
(define (define-major-mode name . args)
  (let ((keymap (if (pair? args) (car args) #f)))
    (%register-mode name 'major keymap)))

;; Register a minor mode with an optional keymap
;; Returns the mode object or #f on failure
(define (define-minor-mode name . args)
  (let ((keymap (if (pair? args) (car args) #f)))
    (%register-mode name 'minor keymap)))

;; Set the major mode of the current buffer
(define (set-major-mode! name)
  (%set-major-mode name))

;; Enable a minor mode in the current buffer
(define (enable-minor-mode name)
  (%enable-minor-mode name))

;; Disable a minor mode in the current buffer
(define (disable-minor-mode name)
  (%disable-minor-mode name))

;; Get the major mode of the current buffer
(define (buffer-major-mode)
  (%buffer-major-mode))

;; Get the list of minor modes enabled in the current buffer
(define (buffer-minor-modes)
  (%buffer-minor-modes))

;; Set a buffer-local variable
(define (set-local! name val)
  (%set-local! name val))

;; Get a buffer-local variable with a default
(define (get-local name . args)
  (let ((default (if (pair? args) (car args) #f)))
    (%get-local name default)))

;; Define fundamental-mode as the default major mode
(define-major-mode 'fundamental-mode)
