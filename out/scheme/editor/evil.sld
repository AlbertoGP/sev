(define-library (editor evil)
  (import (except (scheme base) newline)
          (srfi 9) (srfi 69)
          (editor primitives) (editor command) (editor mode))
  (export
    evil-normal evil-insert evil-replace
    evil-select evil-select-line evil-select-rectangle evil-command
    evil-mode evil-state
    normal-map insert-map select-map command-map pending-map
    register-motion! register-operator! motion-ref operator-ref
    register-text-object! text-object-ref
    evil-in-text-object evil-around-text-object
    make-range range-start range-end range-type
    evil-motion-h evil-motion-j evil-motion-k evil-motion-l
    evil-motion-$ evil-motion-^ evil-motion-w evil-motion-b evil-motion-e
    evil-motion-W evil-motion-B evil-motion-E evil-motion-gg evil-motion-G
    evil-op-delete evil-op-change evil-D evil-C evil-S evil-x evil-X
    evil-visual-delete evil-visual-change
    evil-digit-argument evil-zero evil-repeat
    evil-set-mark evil-goto-mark-line evil-goto-mark-exact
    evil-motion-f evil-motion-F evil-motion-t evil-motion-T
    open-line-below open-line-above insert-at-start
    append-char append-line substitute-char)
  (include "evil.scm"))
