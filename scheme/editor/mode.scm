;; Mode system wrapper functions

;; ---------------------------------------------------------------------------
;; Mode parent registry (for derived-mode? checks)
;; ---------------------------------------------------------------------------

(define %mode-parents '())  ; alist: child-symbol → parent-symbol

(define (set-mode-parent! child parent)
  (set! %mode-parents (cons (cons child parent) %mode-parents)))

(define (mode-parent mode)
  (let ((entry (assq mode %mode-parents)))
    (and entry (cdr entry))))

;; Returns #t if the current buffer's major mode is BASE or derives from it.
(define (derived-mode? base)
  (let loop ((m (%buffer-major-mode)))
    (cond ((not m) #f)
          ((eq? m base) #t)
          (else (loop (mode-parent m))))))

;; ---------------------------------------------------------------------------
;; Settings resolution system
;; ---------------------------------------------------------------------------

;; Registered settings: alist of (key . global-fallback)
(define %resolvable-settings '())

(define (register-setting-default! key fallback)
  (unless (assq key %resolvable-settings)
    (set! %resolvable-settings
          (cons (cons key fallback) %resolvable-settings))))

;; App-supplied defaults (lower priority than user rules)
(define %app-settings-rules '())  ; list of (predicate . settings-alist)

(define (%app-settings-rules-add! pred settings)
  (set! %app-settings-rules
        (append %app-settings-rules (list (cons pred settings)))))

;; User overrides — set this in your config file
(define user-settings-rules '())  ; list of (predicate . settings-alist)

;; Find the first matching value for KEY in a rule list.
;; Returns (list value) when found (so #f values are distinguishable), #f when not found.
(define (find-in-rules key rules)
  (let loop ((rs rules))
    (cond ((null? rs) #f)
          (((caar rs))  ; call predicate thunk
           (let ((entry (assq key (cdar rs))))
             (if entry (list (cdr entry)) (loop (cdr rs)))))
          (else (loop (cdr rs))))))

;; Append two symbols together.
(define (%sym-append a b)
  (string->symbol (string-append (symbol->string a) (symbol->string b))))

;; Resolve a single setting for the current buffer.
;; If the buffer has an explicit interactive override, returns that; otherwise
;; walks user-rules → app-rules → fallback.
(define (resolve-buffer-setting key fallback)
  (if (get-local (%sym-append key '/explicit?) #f)
      (get-local key fallback)
      (let ((r (or (find-in-rules key user-settings-rules)
                   (find-in-rules key %app-settings-rules))))
        (if r (car r) fallback))))

;; Apply all registered settings to the current buffer (skips explicit ones).
(define (apply-buffer-settings)
  (for-each
    (lambda (key/default)
      (unless (get-local (%sym-append (car key/default) '/explicit?) #f)
        (set-local! (car key/default)
                    (resolve-buffer-setting (car key/default) (cdr key/default)))))
    %resolvable-settings))

;; ---------------------------------------------------------------------------
;; Register a major mode with a display name, optional icon, and optional keymap
;; Returns the mode object or #f on failure
;; ---------------------------------------------------------------------------
(define (define-major-mode name display-name . rest)
  ;; rest: ([icon-or-false [keymap]])
  (let* ((icon   (if (pair? rest) (car rest) #f))
         (keymap (if (and (pair? rest) (pair? (cdr rest))) (cadr rest) #f)))
    (%register-mode name 'major keymap)
    (%register-major-mode-info! name display-name icon)
    (set! %all-major-modes (cons (cons name display-name) %all-major-modes))))

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

;; Set the major mode of the current buffer and re-apply settings defaults.
(define (set-major-mode! name)
  (%set-major-mode name)
  (apply-buffer-settings))

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

;; Alist of (mode-sym . display-name) for all registered major modes, built up
;; by define-major-mode. Used by the mode picker to enumerate available modes.
(define %all-major-modes '())

;; Check if MODE (a symbol) derives from BASE (a symbol) via the parent chain.
(define (mode-derives-from? mode base)
  (let loop ((m mode))
    (cond ((not m) #f)
          ((eq? m base) #t)
          (else (loop (mode-parent m))))))

;; Return an alist of (sym . "Display Name") for all major modes that do not
;; derive from read-only-mode. Used by the major-mode picker.
(define (list-writable-major-modes)
  (filter (lambda (pair)
            (let ((mode (car pair)))
              (and (not (mode-derives-from? mode 'read-only-mode))
                   (not (eq? mode 'prog-mode)))))
          %all-major-modes))

;; Base categories for settings inheritance
(define-major-mode 'prog-mode "Prog")
(define-major-mode 'text-mode "Plain Text" 'plaintext-icon)

;; App-supplied line-number defaults per category
(%app-settings-rules-add!
  (lambda () (derived-mode? 'prog-mode))
  '((display-line-numbers-type . #t)))
(%app-settings-rules-add!
  (lambda () (derived-mode? 'text-mode))
  '((display-line-numbers-type . #f)))

;; Tab width setting: buffer-local, resolved per mode.
(register-setting-default! 'tab-width 4)
(%app-settings-rules-add!
  (lambda () (derived-mode? 'scheme-mode))
  '((tab-width . 2)))
(%app-settings-rules-add!
  (lambda () (derived-mode? 'prog-mode))
  '((tab-width . 4)))

;; indent-tabs-mode: #f = expand tabs to spaces on insert, #t = insert literal tab.
;; Global default is #t (preserve); prog-mode enables expansion.
(register-setting-default! 'indent-tabs-mode #t)
(%app-settings-rules-add!
  (lambda () (derived-mode? 'prog-mode))
  '((indent-tabs-mode . #f)))

;; wrap-lines: #t = word-wrap (default), #f = no-wrap with horizontal scroll.
(register-setting-default! 'wrap-lines #t)
(%app-settings-rules-add!
  (lambda () (derived-mode? 'prog-mode))
  '((wrap-lines . #f)))

(defcommand (toggle-wrap-lines)
  "editor: toggle line wrap in buffer\nToggle line wrapping in the current buffer."
  (unless (no-panes?)
    (set-local! 'wrap-lines (not (get-local 'wrap-lines #t)))
    (set-local! 'wrap-lines/explicit? #t)))

;; Plain text major mode — the default/fallback mode
(define (text-mode)
  (%ts-disable!)
  (set-major-mode! 'text-mode))

;; Scheme major mode — activates tree-sitter highlighting
(define-major-mode 'scheme-mode "Scheme" 'scheme-icon)
(set-mode-parent! 'scheme-mode 'prog-mode)
(define (scheme-mode)
  (%ts-enable!)
  (set-major-mode! 'scheme-mode))

;; Line number display
(define-minor-mode 'display-line-numbers-mode)

;; Register display-line-numbers-type as a resolvable setting (default: off).
(register-setting-default! 'display-line-numbers-type #f)

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
  "editor: toggle line numbers\nToggle display of line numbers in the current buffer."
  (unless (no-panes?)
    (let ((next (not (get-local 'display-line-numbers-type))))
      (set-local! 'display-line-numbers-type next)
      (set-local! 'display-line-numbers-type/explicit? #t))))

(defcommand (toggle-relative-line-numbers)
  "editor: toggle relative line numbers\nToggle relative line numbering in the current buffer."
  (unless (no-panes?)
    (let ((next (if (eq? (get-local 'display-line-numbers-type) 'relative)
                    #t 'relative)))
      (set-local! 'display-line-numbers-type next)
      (set-local! 'display-line-numbers-type/explicit? #t))))

(defcommand (toggle-visual-line-numbers)
  "editor: toggle visual line numbers\nToggle visual line numbering in the current buffer."
  (unless (no-panes?)
    (let ((next (if (eq? (get-local 'display-line-numbers-type) 'visual)
                    #t 'visual)))
      (set-local! 'display-line-numbers-type next)
      (set-local! 'display-line-numbers-type/explicit? #t))))

;; Read-only base major mode — derive from this to make a buffer read-only.
(define-major-mode 'read-only-mode "Read-Only")

;; Help buffer major mode
(define help-map (make-keymap))
(set-key! help-map "j"     'next-line)
(set-key! help-map "k"     'prev-line)
(set-key! help-map "h"     'backward-char)
(set-key! help-map "l"     'forward-char)
(set-key! help-map "up"    'prev-line)
(set-key! help-map "down"  'next-line)
(set-key! help-map "left"  'backward-char)
(set-key! help-map "right" 'forward-char)
(define-major-mode 'help-mode "Help" 'help-icon help-map)
(set-mode-parent! 'help-mode 'read-only-mode)

;; Predicate — true for any buffer whose major mode derives from read-only-mode.
(define (buffer-read-only?) (derived-mode? 'read-only-mode))

;; Open the major-mode picker. Does nothing in read-only buffers.
(defcommand (set-buffer-mode)
  "editor: set major mode\nSelect a major mode for the current buffer from a completion list."
  (interactive)
  (unless (or (no-panes?) (buffer-read-only?))
    (%minibuffer-activate-major-modes!)))
