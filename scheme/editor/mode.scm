;; Mode system wrapper functions

;; Register a major mode with an optional keymap
;; Returns the mode object or #f on failure
(define (define-major-mode name . args)
  (let ((keymap (if (pair? args) (car args) #f)))
    (%register-mode name 'major keymap)))

;; Register a minor mode with an optional keymap and allows-input flag
;; Returns the mode object or #f on failure
(define (define-minor-mode name . args)
  (let ((keymap (if (pair? args) (car args) #f))
        (allows-input (and (pair? args) (pair? (cdr args)) (cadr args))))
    (%register-mode name 'minor keymap)
    (when allows-input
      (%set-mode-allows-input! name #t))))

;; Set a keymap's parent for inheritance
(define (set-keymap-parent! keymap parent)
  (%set-keymap-parent! keymap parent))

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

;; Scheme major mode — activates tree-sitter highlighting
(define-major-mode 'scheme-mode)
(define (scheme-mode)
  (%ts-enable!)
  (%set-major-mode 'scheme-mode))

;; Line number display
(define-minor-mode 'display-line-numbers-mode)

(defun (display-line-numbers-mode enable)
  "Set line number display mode in the current buffer."
  (if enable
    (begin
      (enable-minor-mode 'display-line-numbers-mode)
      (when (not (get-local 'display-line-numbers-type))
        (set-local! 'display-line-numbers-type #t)))
    (begin
      (disable-minor-mode 'display-line-numbers-mode)
      (set-local! 'display-line-numbers-type #f))))

(defcommand (toggle-line-numbers)
  "Toggle display of line numbers in the current buffer."
  (display-line-numbers-mode
    (not (get-local 'display-line-numbers-type))))

(defcommand (toggle-relative-line-numbers)
  "Toggle relative line numbering in the current buffer."
  (set-local! 'display-line-numbers-type
    (if (eq? (get-local 'display-line-numbers-type)
             'relative)
        #t 'relative)))

(defcommand (toggle-visual-line-numbers)
  "Toggle visual line numbering in the current buffer."
  (set-local! 'display-line-numbers-type
    (if (eq? (get-local 'display-line-numbers-type)
             'visual)
        #t 'visual)))

;; Help buffer mode
(define help-map (make-keymap))
(set-key! help-map "ESC"   'pane-close)
(set-key! help-map "q"     'pane-close)
(set-key! help-map "j"     'next-line)
(set-key! help-map "k"     'prev-line)
(set-key! help-map "h"     'backward-char)
(set-key! help-map "l"     'forward-char)
(set-key! help-map "UP"    'prev-line)
(set-key! help-map "DOWN"  'next-line)
(set-key! help-map "LEFT"  'backward-char)
(set-key! help-map "RIGHT" 'forward-char)
(define-minor-mode 'help-mode help-map)
(register-mode-icon/full 'help-mode "icon-help.svg"
                         'mode.help 'label.help 'cursor.help 'solid)
