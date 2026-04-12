(define-library (editor mode)
  (import (except (scheme base) newline)
          (editor primitives) (editor command))
  (export
    define-major-mode define-minor-mode
    set-keymap-parent! set-major-mode!
    enable-minor-mode disable-minor-mode
    buffer-major-mode buffer-minor-modes
    set-local! get-local
    register-mode-icon register-mode-icon/full
    toggle-line-numbers toggle-relative-line-numbers
    toggle-visual-line-numbers toggle-wrap-lines
    scheme-mode
    set-mode-parent! derived-mode?
    user-settings-rules apply-buffer-settings
    register-setting-default!)
  (include "mode.scm"))
