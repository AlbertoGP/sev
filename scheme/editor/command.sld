(define-library (editor command)
  (import (except (scheme base) newline)
          (srfi 1) (srfi 9) (srfi 69)
          (editor primitives))
  (export
    defcommand defvar defun
    call-interactively set-key! bind-prefix! describe-function describe-key
    set-doc! get-doc get-doc-text
    make-interactive! interactive? interactive-spec
    register-spec-handler!
    list-commands list-functions list-variables where-is)
  (include "command.scm"))
