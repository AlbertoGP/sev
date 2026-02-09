;;; evil.scm - Vim-like modal editing

(defcommand ignore "Do nothing.")

;; Keymaps
(define normal-map (make-keymap))
(define insert-map (make-keymap))
(define replace-map (make-keymap))
(define select-map (make-keymap))
(define command-map (make-keymap))

;; Normal mode: ignore unbound keys (no self-insert)
(%set-keymap-default! normal-map 'ignore)

(defcommand (line-start-skip-whitespace)
  "Set cursor to first non-blank character on current line."
  (line-start) (skip-whitespace))
(defcommand (open-line-below)
  "Create a new empty line below current line, move to it and enter insert mode."
  (line-end) (newline) (evil-insert))
(defcommand (open-line-above)
  "Create a new empty line above current line, move to it and enter insert mode."
  (line-start) (newline) (prev-line) (evil-insert))
(defcommand (insert-at-start)
  "Set cursor to first non-blank character on current line and enter insert mode."
  (line-start-skip-whitespace) (evil-insert))
(defcommand (append-char)
  "Enter insert mode after the character currently under the cursor."
  (forward-char) (evil-insert))
(defcommand (append-line)
  "Set the cursor to the final column on the current line and enter insert mode."
  (line-end) (evil-insert))
(defcommand (join-line)
  "Delete newline character between the current and subsequent line."
  (line-end) (delete-forward-char))

;; Normal mode bindings
(set-key! normal-map "C-q" 'quit)
(set-key! normal-map "C-e" 'eval-buffer)
(set-key! normal-map "h" 'backward-char)
(set-key! normal-map "j" 'next-line)
(set-key! normal-map "k" 'prev-line)
(set-key! normal-map "l" 'forward-char)
(set-key! normal-map "LEFT" 'backward-char)
(set-key! normal-map "RIGHT" 'forward-char)
(set-key! normal-map "UP" 'prev-line)
(set-key! normal-map "DOWN" 'next-line)
(set-key! normal-map "0" 'line-start)
(set-key! normal-map "$" 'line-end)
(set-key! normal-map "^" 'line-start-skip-whitespace)
(set-key! normal-map "i" 'evil-insert)
(set-key! normal-map "I" 'insert-at-start)
(set-key! normal-map "a" 'append-char)
(set-key! normal-map "A" 'append-line)
(set-key! normal-map "o" 'open-line-below)
(set-key! normal-map "O" 'open-line-above)
(set-key! normal-map "J" 'join-line)
(set-key! normal-map "R" 'evil-replace)
(set-key! normal-map "v" 'evil-select)
(set-key! normal-map ":" 'evil-command)
(set-key! normal-map "C-TAB" 'tab-next)
(set-key! normal-map "C-S-TAB" 'tab-prev)
(set-key! normal-map "C-w" 'pane-close)
(set-key! normal-map "M-0" 'reset-global-scale)
(set-key! normal-map "M-=" 'increase-global-scale)
(set-key! normal-map "M--" 'decrease-global-scale)
(set-key! normal-map "C-0" 'reset-buffer-scale)
(set-key! normal-map "C-=" 'increase-buffer-scale)
(set-key! normal-map "C--" 'decrease-buffer-scale)
(set-key! normal-map "C-s h" 'split-horizontal)
(set-key! normal-map "C-s v" 'split-vertical)
(set-key! normal-map "C-UP" 'pane-navigate-up)
(set-key! normal-map "C-DOWN" 'pane-navigate-down)
(set-key! normal-map "C-LEFT" 'pane-navigate-left)
(set-key! normal-map "C-RIGHT" 'pane-navigate-right)
(set-key! normal-map "C-S-UP" 'pane-h-split-decrease)
(set-key! normal-map "C-S-DOWN" 'pane-h-split-increase)
(set-key! normal-map "C-S-LEFT" 'pane-v-split-decrease)
(set-key! normal-map "C-S-RIGHT" 'pane-v-split-increase)
(set-key! normal-map "`" 'clay-debug)

;; Insert mode bindings (no default - falls through for self-insert)
(set-key! insert-map "ESC" 'evil-normal)
(set-key! insert-map "C-[" 'evil-normal)
(set-key! insert-map "LEFT" 'backward-char)
(set-key! insert-map "M-h" 'backward-char)
(set-key! insert-map "DOWN" 'next-line)
(set-key! insert-map "M-j" 'next-line)
(set-key! insert-map "UP" 'prev-line)
(set-key! insert-map "M-k" 'prev-line)
(set-key! insert-map "RIGHT" 'forward-char)
(set-key! insert-map "M-l" 'forward-char)

(set-key! replace-map "ESC" 'evil-normal)
(set-key! replace-map "C-[" 'evil-normal)
(set-key! select-map "ESC" 'evil-normal)
(set-key! select-map "C-[" 'evil-normal)
(set-key! command-map "ESC" 'evil-normal)
(set-key! command-map "C-[" 'evil-normal)

;; Register modes
(define-minor-mode 'evil-normal-mode normal-map)
(define-minor-mode 'evil-insert-mode insert-map)
(define-minor-mode 'evil-replace-mode replace-map)
(define-minor-mode 'evil-select-mode select-map)
(define-minor-mode 'evil-command-mode command-map)

;; Register mode icons
(register-mode-icon/full 'evil-normal-mode  "icon-normal.svg"
                         'mode.normal  'label.normal  'cursor.normal  'solid)
(register-mode-icon/full 'evil-insert-mode  "icon-insert.svg"
                         'mode.insert  'label.insert  'cursor.insert  'thin)
(register-mode-icon/full 'evil-replace-mode "icon-replace.svg"
                         'mode.replace 'label.replace 'cursor.replace 'under)
(register-mode-icon/full 'evil-select-mode  "icon-select.svg"
                         'mode.select  'label.select  'cursor.select  'hollow)
(register-mode-icon/full 'evil-command-mode "icon-command.svg"
                         'mode.command 'label.command 'cursor.command  'solid)

;; State transitions
(define (evil-normal)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-normal-mode)
  (set-local! 'mode-name "NORMAL")
  (message-clear))

(define (evil-insert)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-insert-mode)
  (set-local! 'mode-name "INSERT")
  (message "-- INSERT --"))

(define (evil-replace)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-replace-mode)
  (set-local! 'mode-name "REPLACE")
  (message "-- REPLACE --"))

(define (evil-select)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-select-mode)
  (set-local! 'mode-name "SELECT")
  (message "-- SELECT --"))

(define (evil-command)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (enable-minor-mode 'evil-command-mode)
  (set-local! 'mode-name "COMMAND")
  (message-clear))

(defcommand evil-normal "Return to normal mode.")
(defcommand evil-insert "Enter insert mode.")
(defcommand evil-replace "Enter replace mode.")
(defcommand evil-select "Enter select mode.")
(defcommand evil-command "Enter command mode.")

;; Query function for UI
(define (evil-state)
  (cond
    ((%buffer-has-minor-mode? 'evil-normal-mode) 'normal)
    ((%buffer-has-minor-mode? 'evil-insert-mode) 'insert)
    (else #f)))

;; Enable evil mode
(define (evil-mode)
  (enable-minor-mode 'evil-normal-mode))

(defcommand evil-mode "Enable vim-like modal editing.")

(evil-mode)
(evil-normal)
