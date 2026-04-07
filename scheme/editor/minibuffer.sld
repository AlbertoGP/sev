(define-library (editor minibuffer)
  (import (except (scheme base) newline)
          (editor primitives) (editor command) (editor mode))
  (export minibuffer-read minibuffer-submit minibuffer-cancel
          minibuffer-select-next minibuffer-select-prev
          command-palette theme-picker)
  (include "minibuffer.scm"))
