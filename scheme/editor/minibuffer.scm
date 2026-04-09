(define minibuffer-map (make-keymap))
;; No parent: deliberately isolated from editor keymaps

(define-minor-mode 'minibuffer-mode minibuffer-map #t)

;; Dedicated cursor style: sky-coloured thin bar, distinct from evil cursors
(register-mode-icon/full 'minibuffer-mode "icon-insert.svg"
                         'mode.minibuffer 'label.minibuffer
                         'cursor.minibuffer 'thin)

;; Override global-keymap keys that would interfere with text entry
(set-key! minibuffer-map "space" 'self-insert)
(set-key! minibuffer-map "h"     'self-insert)
(set-key! minibuffer-map "j"     'self-insert)
(set-key! minibuffer-map "k"     'self-insert)
(set-key! minibuffer-map "l"     'self-insert)

;; Submit / cancel
(set-key! minibuffer-map "return"   'minibuffer-submit)
(set-key! minibuffer-map "ctrl-j"   'minibuffer-submit)
(set-key! minibuffer-map "escape"   'minibuffer-cancel)
(set-key! minibuffer-map "ctrl-g"   'minibuffer-cancel)

;; Editing
(set-key! minibuffer-map "backspace" 'delete-backward-char)
(set-key! minibuffer-map "delete"    'delete-forward-char)
(set-key! minibuffer-map "left"      'backward-char)
(set-key! minibuffer-map "right"     'forward-char)
(set-key! minibuffer-map "ctrl-a"    'line-start)
(set-key! minibuffer-map "ctrl-e"    'line-end)

;; Provider navigation
(set-key! minibuffer-map "down" 'minibuffer-select-next)
(set-key! minibuffer-map "up"   'minibuffer-select-prev)

(defun (minibuffer-read prompt on-submit)
  "Prompt for input in the minibuffer; call on-submit with the result."
  (%minibuffer-activate prompt on-submit #f))

(defcommand (minibuffer-submit)
  "minibuffer: submit\nAccept current minibuffer input."
  (%minibuffer-submit))

(defcommand (minibuffer-cancel)
  "minibuffer: escape\nCancel minibuffer input."
  (%minibuffer-cancel))

(defcommand (minibuffer-select-next)
  "minibuffer: select next\nSelect the next item in the provider list."
  (%minibuffer-select-next))

(defcommand (minibuffer-select-prev)
  "minibuffer: select previous\nSelect the previous item in the provider list."
  (%minibuffer-select-prev))

(defcommand (command-palette)
  "sev: open command palette"
  (%minibuffer-activate-commands))

(defcommand (theme-picker)
  "theme selector: toggle\nBrowse and activate available themes."
  (%minibuffer-activate-themes))

;; Register minibuffer-read as an interactive spec handler
(register-spec-handler! 'minibuffer-read
  (lambda (form-args done)
    (let ((prompt (if (pair? form-args) (car form-args) "Input...")))
      (minibuffer-read prompt done))))

