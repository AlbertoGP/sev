(define-library (editor vim)
  (import (except (scheme base) newline)
          (scheme char)
          (srfi 9) (srfi 69)
          (editor primitives) (editor command) (editor mode))
  (export
    vim-word-char? vim-whitespace? vim-punctuation?
    utf8-byte-len-at count-chars-in-range
    text-object-for-char
    vim-normal vim-insert vim-replace
    vim-select vim-select-line vim-select-rectangle vim-visual-dollar
    rect-mode?
    vim-mode vim-state
    normal-map insert-map select-map pending-map
    register-motion! register-operator! motion-ref operator-ref
    register-text-object! text-object-ref
    vim-in-text-object vim-around-text-object
    make-range range-start range-end range-type
    vim-motion-h vim-motion-j vim-motion-k vim-motion-l
    vim-motion-$ vim-motion-^ vim-motion-w vim-motion-b vim-motion-e
    vim-motion-W vim-motion-B vim-motion-E vim-motion-% vim-motion-gg vim-motion-G
    vim-D vim-C vim-S vim-X
    vim-char-replace-setup
    vim-A vim-I vim-d vim-c vim-y vim-x vim-p vim-P
    vim-insert-paste-clipboard
    vim-use-register current-vim-register
    vim-digit-argument vim-zero vim-repeat vim-undo vim-redo vim-line-restore
    vim-set-mark vim-goto-mark-line vim-goto-mark-exact
    vim-motion-f vim-motion-F vim-motion-t vim-motion-T
    open-line-below open-line-above
    append-char substitute-char
    vim-start-macro vim-stop-macro vim-play-macro vim-play-last-macro
    vim-jump-backward vim-jump-forward
    vim-search-open vim-search-next vim-search-prev)
  (include "vim/core.scm")
  (include "vim/undo.scm")
  (include "vim/motion.scm")
  (include "vim/operator.scm")
  (include "vim/text-object.scm")
  (include "vim/visual.scm")
  (include "vim/macro.scm"))
