;;; init.scm - Editor initialization and keybindings

;; ── Global settings ───────────────────────────────────────────────────────────

(define cursor-blink #t)

(import (editor command) (editor mode) (editor icon)
        (editor built-in) (editor evil) (editor theme)
        (editor minibuffer) (editor which-key))

;; ── Global keymap (base) ─────────────────────────────────────────────────────
;; Bindings that are always available regardless of whether any pane is open.

(set-key! global-keymap "C-q" 'quit)
(set-key! global-keymap "C-e" 'eval-expression)
(set-key! global-keymap "M-:" 'eval-expression)
(set-key! global-keymap "M-x" 'execute-extended-command)
(set-key! global-keymap "M-0" 'reset-global-scale)
(set-key! global-keymap "M-=" 'increase-global-scale)
(set-key! global-keymap "M--" 'decrease-global-scale)

;; SPC prefix for global (base) — open-file, buffer-new, help
(define base-spc-map (make-keymap))
(%set-keymap-name! base-spc-map "leader")
(bind-prefix! global-keymap "SPC" base-spc-map)

;; SPC f — file operations (always available: open creates a pane if none)
(set-key! base-spc-map "f o" 'open-file)

;; SPC b — buffer operations available on welcome screen
(define base-buffer-map (make-keymap))
(%set-keymap-name! base-buffer-map "buffer")
(set-key! base-buffer-map "n" 'buffer-new)
(bind-prefix! base-spc-map "b" base-buffer-map)

;; SPC t — tab operations
(define base-tab-map (make-keymap))
(%set-keymap-name! base-tab-map "tab")
(set-key! base-tab-map "n" 'tab-new)
(bind-prefix! base-spc-map "t" base-tab-map)

;; SPC h — help (always available)
(define help-map (make-keymap))
(%set-keymap-name! help-map "help")
(set-key! help-map "f" 'describe-function)
(set-key! help-map "k" 'describe-key)
(set-key! help-map "w" 'which-key-toggle)
(bind-prefix! base-spc-map "h" help-map)

;; ── Pane keymap ───────────────────────────────────────────────────────────────
;; Bindings that only make sense when at least one pane is open.
;; The C dispatch layer inserts this into the lookup chain only when
;; pane_get_root() != NULL. Its parent is global-keymap so base bindings
;; remain reachable through the parent chain.

(%set-keymap-parent! pane-keymap global-keymap)
(%set-keymap-name!   pane-keymap "pane")

;; Mode / escape
(set-key! pane-keymap "ESC"     'evil-normal)
(set-key! pane-keymap "C-["     'evil-normal)

;; Tab navigation
(set-key! pane-keymap "C-TAB"   'tab-next)
(set-key! pane-keymap "C-S-TAB" 'tab-prev)

;; Pane management
(set-key! pane-keymap "C-w"     'tab-close)

;; File save
(set-key! pane-keymap "C-s"     'save-buffer)
(set-key! pane-keymap "C-S-s"   'save-buffer-as)

;; Buffer-local text scale
(set-key! pane-keymap "C-0"     'reset-buffer-scale)
(set-key! pane-keymap "C-="     'increase-buffer-scale)
(set-key! pane-keymap "C--"     'decrease-buffer-scale)

;; Evil motions (cursor movement)
(set-key! pane-keymap "h"       'evil-motion-h)
(set-key! pane-keymap "j"       'evil-motion-j)
(set-key! pane-keymap "k"       'evil-motion-k)
(set-key! pane-keymap "l"       'evil-motion-l)
(set-key! pane-keymap "LEFT"    'evil-motion-h)
(set-key! pane-keymap "DOWN"    'evil-motion-j)
(set-key! pane-keymap "UP"      'evil-motion-k)
(set-key! pane-keymap "RIGHT"   'evil-motion-l)

;; Pane navigation
(set-key! pane-keymap "C-h"     'pane-navigate-left)
(set-key! pane-keymap "C-j"     'pane-navigate-down)
(set-key! pane-keymap "C-k"     'pane-navigate-up)
(set-key! pane-keymap "C-l"     'pane-navigate-right)
(set-key! pane-keymap "C-LEFT"  'pane-navigate-left)
(set-key! pane-keymap "C-DOWN"  'pane-navigate-down)
(set-key! pane-keymap "C-UP"    'pane-navigate-up)
(set-key! pane-keymap "C-RIGHT" 'pane-navigate-right)

;; Pane resize
(set-key! pane-keymap "C-S-h"     'pane-v-split-decrease)
(set-key! pane-keymap "C-S-j"     'pane-h-split-increase)
(set-key! pane-keymap "C-S-k"     'pane-h-split-decrease)
(set-key! pane-keymap "C-S-l"     'pane-v-split-increase)
(set-key! pane-keymap "C-S-LEFT"  'pane-v-split-decrease)
(set-key! pane-keymap "C-S-DOWN"  'pane-h-split-increase)
(set-key! pane-keymap "C-S-UP"    'pane-h-split-decrease)
(set-key! pane-keymap "C-S-RIGHT" 'pane-v-split-increase)

;; SPC prefix for pane — extends base-spc-map via parent chain
(define pane-spc-map (make-keymap))
(%set-keymap-name!   pane-spc-map "leader")
(%set-keymap-parent! pane-spc-map base-spc-map)
(bind-prefix! pane-keymap "SPC" pane-spc-map)

;; SPC b — full buffer map (extends base-buffer-map)
(define pane-buffer-map (make-keymap))
(%set-keymap-name!   pane-buffer-map "buffer")
(%set-keymap-parent! pane-buffer-map base-buffer-map)
(set-key! pane-buffer-map "r" 'buffer-rename)
(set-key! pane-buffer-map "c" 'buffer-close)
(set-key! pane-buffer-map "s" 'switch-to-buffer)
(bind-prefix! pane-spc-map "b" pane-buffer-map)

;; SPC s — split
(define split-map (make-keymap))
(%set-keymap-name! split-map "split")
(set-key! split-map "h" 'split-horizontal)
(set-key! split-map "v" 'split-vertical)
(bind-prefix! pane-spc-map "s" split-map)

;; SPC l — line numbers
(define line-numbers-map (make-keymap))
(%set-keymap-name! line-numbers-map "line-numbers")
(set-key! line-numbers-map "n" 'toggle-line-numbers)
(set-key! line-numbers-map "r" 'toggle-relative-line-numbers)
(set-key! line-numbers-map "v" 'toggle-visual-line-numbers)
(bind-prefix! pane-spc-map "l" line-numbers-map)

;; ── Mouse handlers ────────────────────────────────────────────────────────────

;; Click: exit any visual select mode, then move point to clicked position.
(%set-mouse-click-handler!
  (lambda (button buf-pos clicks)
    (when (= button 1)
      (when (%buffer-has-minor-mode? 'evil-select-mode)
        (evil-normal))
      (point-set! buf-pos))))

;; Drag: on first motion enter visual-char mode anchored at the click position
;; (the click handler already moved point there), then track cursor.
;; In evil buffers this activates evil-select-mode; in others point just follows.
(%set-mouse-drag-handler!
  (lambda (current-pos start-pos)
    (when (and (not (%buffer-has-minor-mode? 'evil-select-mode))
               (or (%buffer-has-minor-mode? 'evil-normal-mode)
                   (%buffer-has-minor-mode? 'evil-insert-mode)
                   (%buffer-has-minor-mode? 'evil-replace-mode)))
      ;; Point is at start-pos (set by click handler); evil-select anchors
      ;; mark < there, then enables evil-select-mode with SELECT_REGULAR.
      (evil-select))
    (point-set! current-pos)))

;; ── Welcome keymap ────────────────────────────────────────────────────────────
;; Inherits global-keymap (base only). No ignore shadows needed: pane commands
;; are absent from global-keymap and unreachable here.

(define welcome-map (make-keymap))
(%set-keymap-parent! welcome-map global-keymap)
(%set-keymap-name!   welcome-map "welcome")
(%set-welcome-keymap! welcome-map)

(reset-global-scale)
(message-clear)
