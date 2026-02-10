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

;; Register a mode icon with full control over theme roles and cursor type.
;; Used by evil.scm for per-mode colors.
(define (register-mode-icon/full mode-name filename
                                 role-bg role-label role-cursor cursor-type)
  (%register-mode-icon! mode-name filename role-bg role-label role-cursor cursor-type))

;; Simple API for custom modes — uses mode.normal/label.normal/cursor.normal defaults.
(define (register-mode-icon mode-name filename cursor-type)
  (%register-mode-icon! mode-name filename
                        'mode.normal 'label.normal 'cursor.normal cursor-type))

;; Define fundamental-mode as the default major mode
(define-major-mode 'fundamental-mode)

;; Line number display
(define-minor-mode 'display-line-numbers-mode)

(defun (display-line-numbers-mode enable)
  "Toggle display of line numbers in the current buffer."
  (if enable
    (begin
      (enable-minor-mode 'display-line-numbers-mode)
      (when (not (get-local 'display-line-numbers-type))
        (set-local! 'display-line-numbers-type #t)))
    (begin
      (disable-minor-mode 'display-line-numbers-mode)
      (set-local! 'display-line-numbers-type #f))))
