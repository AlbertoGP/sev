;;; evil.scm - Vim-like modal editing

(set-doc! 'ignore 'command "Do nothing.")
(make-interactive! 'ignore "")

;; Keymaps
(define evil-normal-map (make-keymap))
(define evil-insert-map (make-keymap))

;; Normal mode: ignore unbound keys (no self-insert)
(%set-keymap-default! evil-normal-map 'ignore)

;; Normal mode bindings
(set-key! evil-normal-map "h" 'backward-char)
(set-key! evil-normal-map "j" 'next-line)
(set-key! evil-normal-map "k" 'prev-line)
(set-key! evil-normal-map "l" 'forward-char)
(set-key! evil-normal-map "LEFT" 'backward-char)
(set-key! evil-normal-map "RIGHT" 'forward-char)
(set-key! evil-normal-map "UP" 'prev-line)
(set-key! evil-normal-map "DOWN" 'next-line)
(set-key! evil-normal-map "i" 'evil-insert)

;; Insert mode bindings (no default - falls through for self-insert)
(set-key! evil-insert-map "ESC" 'evil-normal)
(set-key! evil-insert-map "LEFT" 'backward-char)
(set-key! evil-insert-map "RIGHT" 'forward-char)
(set-key! evil-insert-map "UP" 'prev-line)
(set-key! evil-insert-map "DOWN" 'next-line)

;; Register modes
(define-minor-mode 'evil-normal-mode evil-normal-map)
(define-minor-mode 'evil-insert-mode evil-insert-map)

(define (set-cursor! type)
  (cond ((eq? type 'solid)  (%set-cursor! 0))
        ((eq? type 'hollow) (%set-cursor! 1))
        ((eq? type 'thin)   (%set-cursor! 2))
        ((eq? type 'under)  (%set-cursor! 3))
        (else (error "unknown cursor type" type))))

;; State transitions
(define (evil-insert)
  (disable-minor-mode 'evil-normal-mode)
  (enable-minor-mode 'evil-insert-mode)
  (set-cursor! 'thin))

(define (evil-normal)
  (disable-minor-mode 'evil-insert-mode)
  (enable-minor-mode 'evil-normal-mode)
  (set-cursor! 'solid))

(set-doc! 'evil-insert 'command "Enter insert mode.")
(set-doc! 'evil-normal 'command "Return to normal mode.")
(make-interactive! 'evil-insert "")
(make-interactive! 'evil-normal "")

;; Query function for UI
(define (evil-state)
  (cond
    ((%buffer-has-minor-mode? 'evil-normal-mode) 'normal)
    ((%buffer-has-minor-mode? 'evil-insert-mode) 'insert)
    (else #f)))

;; Enable evil mode
(define (evil-mode)
  (enable-minor-mode 'evil-normal-mode))

(set-doc! 'evil-mode 'command "Enable vim-like modal editing.")
(make-interactive! 'evil-mode "")
