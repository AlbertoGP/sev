(define minibuffer-map (make-keymap))
;; No parent: deliberately isolated from editor keymaps

(define-minor-mode 'minibuffer-mode minibuffer-map #t)

;; Dedicated cursor style: sky-coloured thin bar, distinct from evil cursors
(register-mode-icon/full 'minibuffer-mode "icon-insert.svg"
                         'mode.minibuffer 'label.minibuffer
                         'cursor.minibuffer 'thin)

;; Override global-keymap keys that would interfere with text entry
(set-key! minibuffer-map "SPC"   'self-insert)
(set-key! minibuffer-map "h"     'self-insert)
(set-key! minibuffer-map "j"     'self-insert)
(set-key! minibuffer-map "k"     'self-insert)
(set-key! minibuffer-map "l"     'self-insert)

;; Submit / cancel
(set-key! minibuffer-map "RET"   'minibuffer-submit)
(set-key! minibuffer-map "C-j"   'minibuffer-submit)
(set-key! minibuffer-map "ESC"   'minibuffer-cancel)
(set-key! minibuffer-map "C-g"   'minibuffer-cancel)

;; Editing
(set-key! minibuffer-map "BSP"   'delete-backward-char)
(set-key! minibuffer-map "DEL"   'delete-forward-char)
(set-key! minibuffer-map "LEFT"  'backward-char)
(set-key! minibuffer-map "RIGHT" 'forward-char)
(set-key! minibuffer-map "C-a"   'line-start)
(set-key! minibuffer-map "C-e"   'line-end)

;; Provider navigation
(set-key! minibuffer-map "DOWN" 'minibuffer-select-next)
(set-key! minibuffer-map "UP"   'minibuffer-select-prev)

(defcommand (minibuffer-read prompt on-submit)
  "Prompt for input in the minibuffer; call on-submit with the result."
  (%minibuffer-activate prompt on-submit #f))

(defcommand (minibuffer-submit)
  "Accept current minibuffer input."
  (%minibuffer-submit))

(defcommand (minibuffer-cancel)
  "Cancel minibuffer input."
  (%minibuffer-cancel))

(defcommand (minibuffer-select-next)
  "Select the next item in the provider list."
  (%minibuffer-select-next))

(defcommand (minibuffer-select-prev)
  "Select the previous item in the provider list."
  (%minibuffer-select-prev))

(defcommand (command-palette)
  "Open the command palette."
  (%minibuffer-activate-commands))

(defcommand (theme-picker)
  "Browse and activate available themes."
  (%minibuffer-activate-themes))

;; Register minibuffer-read as an interactive spec handler
(register-spec-handler! 'minibuffer-read
  (lambda (form-args done)
    (let ((prompt (if (pair? form-args) (car form-args) "Input...")))
      (minibuffer-read prompt done))))

(defcommand (execute-extended-command)
  "Read command name and execute it."
  (minibuffer-read "Execute a command..."
    (lambda (name)
      (let ((sym (string->symbol name)))
        (if (interactive? sym)
            (begin
              (%record-command-usage sym)
              (call-interactively sym))
            (message (string-append "Not a command: " name)))))))
