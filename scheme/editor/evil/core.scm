;;; core.scm - Keymaps, modes, state machine, and core dispatch

;; Keymaps
(define normal-map (make-keymap))
(define insert-map (make-keymap))
(define select-map (make-keymap))
(define command-map (make-keymap))
(define pending-map (make-keymap))

;; Set parent pointers so pane commands are found via parent chain traversal.
;; pane-keymap's own parent is global-keymap, so the full chain is:
;; mode-map → pane-keymap → global-keymap
(set-keymap-parent! normal-map  pane-keymap)
(set-keymap-parent! insert-map  pane-keymap)
(set-keymap-parent! select-map  pane-keymap)
(set-keymap-parent! pending-map pane-keymap)
(set-keymap-parent! command-map pane-keymap)

;; Normal mode bindings
(set-key! normal-map "0" 'evil-zero)
(set-key! normal-map "$" 'evil-motion-$)
(set-key! normal-map "^" 'evil-motion-^)
(set-key! normal-map "w" 'evil-motion-w)
(set-key! normal-map "b" 'evil-motion-b)
(set-key! normal-map "e" 'evil-motion-e)
(set-key! normal-map "W" 'evil-motion-W)
(set-key! normal-map "B" 'evil-motion-B)
(set-key! normal-map "E" 'evil-motion-E)
(set-key! normal-map "%" 'evil-motion-%)
(set-key! normal-map "i" 'evil-insert)
(set-key! normal-map "I" 'insert-at-start)
(set-key! normal-map "R" 'evil-replace)
(set-key! normal-map "o" 'open-line-below)
(set-key! normal-map "O" 'open-line-above)
(set-key! normal-map "a" 'append-char)
(set-key! normal-map "A" 'append-line)
(set-key! normal-map "s" 'substitute-char)
(set-key! normal-map "S" 'evil-S)
(set-key! normal-map "D" 'evil-D)
(set-key! normal-map "C" 'evil-C)
(set-key! normal-map "x" 'evil-x)
(set-key! normal-map "X" 'evil-X)
(set-key! normal-map "d" 'evil-op-delete)
(set-key! normal-map "c" 'evil-op-change)
(set-key! normal-map "." 'evil-repeat)
(set-key! normal-map "u" 'evil-undo)
(set-key! normal-map "C-r" 'evil-redo)
(set-key! normal-map "U" 'evil-line-restore)
(set-key! normal-map "1" 'evil-digit-argument)
(set-key! normal-map "2" 'evil-digit-argument)
(set-key! normal-map "3" 'evil-digit-argument)
(set-key! normal-map "4" 'evil-digit-argument)
(set-key! normal-map "5" 'evil-digit-argument)
(set-key! normal-map "6" 'evil-digit-argument)
(set-key! normal-map "7" 'evil-digit-argument)
(set-key! normal-map "8" 'evil-digit-argument)
(set-key! normal-map "9" 'evil-digit-argument)
(set-key! normal-map "J" 'join-line)
(set-key! normal-map "v" 'evil-select)
(set-key! normal-map "V" 'evil-select-line)
(set-key! normal-map "C-v" 'evil-select-rectangle)
(set-key! normal-map "g g" 'evil-motion-gg)
(set-key! normal-map "G" 'evil-motion-G)
(set-key! normal-map "C-o" 'evil-jump-backward)
(set-key! normal-map "C-i" 'evil-jump-forward)

;; Insert mode bindings
(set-key! insert-map "h" 'self-insert)
(set-key! insert-map "j" 'self-insert)
(set-key! insert-map "k" 'self-insert)
(set-key! insert-map "l" 'self-insert)
(set-key! insert-map "SPC" 'self-insert)
(set-key! insert-map "M-h" 'evil-motion-h)
(set-key! insert-map "M-j" 'evil-motion-j)
(set-key! insert-map "M-k" 'evil-motion-k)
(set-key! insert-map "M-l" 'evil-motion-l)
(set-key! insert-map "RET" 'newline)
(set-key! insert-map "TAB" 'insert-tab)
(set-key! insert-map "BSP" 'delete-backward-char)
(set-key! insert-map "C-h" 'delete-backward-char)
(set-key! insert-map "DEL" 'delete-forward-char)
(set-key! insert-map "C-v" 'evil-insert-paste-clipboard)

;; Select (visual) mode bindings
(set-key! select-map "v" 'evil-select)
(set-key! select-map "V" 'evil-select-line)
(set-key! select-map "C-v" 'evil-select-rectangle)
(set-key! select-map "0" 'evil-zero)
(set-key! select-map "$" 'evil-visual-dollar)
(set-key! select-map "^" 'line-start-skip-whitespace)
(set-key! select-map "o" 'exchange-point-and-mark)
(set-key! select-map "1" 'evil-digit-argument)
(set-key! select-map "2" 'evil-digit-argument)
(set-key! select-map "3" 'evil-digit-argument)
(set-key! select-map "4" 'evil-digit-argument)
(set-key! select-map "5" 'evil-digit-argument)
(set-key! select-map "6" 'evil-digit-argument)
(set-key! select-map "7" 'evil-digit-argument)
(set-key! select-map "8" 'evil-digit-argument)
(set-key! select-map "9" 'evil-digit-argument)

;; Operator-pending mode: only motions, counts, cancel, and doubled ops
(set-key! pending-map "w" 'evil-motion-w)
(set-key! pending-map "b" 'evil-motion-b)
(set-key! pending-map "e" 'evil-motion-e)
(set-key! pending-map "W" 'evil-motion-W)
(set-key! pending-map "B" 'evil-motion-B)
(set-key! pending-map "E" 'evil-motion-E)
(set-key! pending-map "%" 'evil-motion-%)
(set-key! pending-map "$" 'evil-motion-$)
(set-key! pending-map "^" 'evil-motion-^)
(set-key! pending-map "0" 'evil-zero)
(set-key! pending-map "1" 'evil-digit-argument)
(set-key! pending-map "2" 'evil-digit-argument)
(set-key! pending-map "3" 'evil-digit-argument)
(set-key! pending-map "4" 'evil-digit-argument)
(set-key! pending-map "5" 'evil-digit-argument)
(set-key! pending-map "6" 'evil-digit-argument)
(set-key! pending-map "7" 'evil-digit-argument)
(set-key! pending-map "8" 'evil-digit-argument)
(set-key! pending-map "9" 'evil-digit-argument)
(set-key! pending-map "g g" 'evil-motion-gg)
(set-key! pending-map "G" 'evil-motion-G)
(set-key! pending-map "d" 'evil-op-delete)
(set-key! pending-map "c" 'evil-op-change)

;; Register selection state
(define current-evil-register #\")

(defcommand (evil-use-register)
  "Select register for next op/paste."
  (set! current-evil-register (last-key-char)))

;; Named prefix keymaps for dispatch commands — must come before the per-char loops.
(let ((reg-km (make-keymap)))
  (%set-keymap-name! reg-km "use-register")
  (bind-prefix! normal-map "\"" reg-km)
  (bind-prefix! select-map "\"" reg-km))
(set-command-display-binding! 'evil-use-register "\" <reg>")

;; Bind " + (a-z, A-Z, ") sequences in normal-map and select-map
(for-each
  (lambda (ch)
    (let ((seq (string-append "\"" " " (string ch))))
      (set-key! normal-map seq 'evil-use-register)
      (set-key! select-map seq 'evil-use-register)))
  (append
    (string->list "abcdefghijklmnopqrstuvwxyz")
    (string->list "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    (list #\")))

;; Special registers: "+ (clipboard) and "_ (black hole)
(set-key! normal-map "\" +" 'evil-use-register)
(set-key! select-map "\" +" 'evil-use-register)
(set-key! normal-map "\" _" 'evil-use-register)
(set-key! select-map "\" _" 'evil-use-register)

;; Named prefix keymaps for mark dispatch.
(let ((m-km (make-keymap)))
  (%set-keymap-name! m-km "set-mark")
  (bind-prefix! normal-map "m" m-km))
(let ((gl-km (make-keymap)))
  (%set-keymap-name! gl-km "goto-mark")
  (bind-prefix! normal-map "'" gl-km)
  (bind-prefix! pending-map "'" gl-km))
(let ((ge-km (make-keymap)))
  (%set-keymap-name! ge-km "goto-mark-exact")
  (bind-prefix! normal-map "`" ge-km)
  (bind-prefix! pending-map "`" ge-km))
(set-command-display-binding! 'evil-set-mark        "m <mark>")
(set-command-display-binding! 'evil-goto-mark-line  "' <mark>")
(set-command-display-binding! 'evil-goto-mark-exact "` <mark>")

;; m / ' / ` — bind all 26 letters programmatically
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map  (string-append "m " ch) 'evil-set-mark)
    (set-key! normal-map  (string-append "' " ch) 'evil-goto-mark-line)
    (set-key! normal-map  (string-append "` " ch) 'evil-goto-mark-exact)
    (set-key! pending-map (string-append "' " ch) 'evil-goto-mark-line)
    (set-key! pending-map (string-append "` " ch) 'evil-goto-mark-exact)))

;; Named prefix keymaps for character-seek dispatch.
(let ((f-km (make-keymap)) (F-km (make-keymap))
      (t-km (make-keymap)) (T-km (make-keymap)))
  (%set-keymap-name! f-km "find-char")
  (%set-keymap-name! F-km "find-char-back")
  (%set-keymap-name! t-km "to-char")
  (%set-keymap-name! T-km "to-char-back")
  (for-each (lambda (map)
    (bind-prefix! map "f" f-km)
    (bind-prefix! map "F" F-km)
    (bind-prefix! map "t" t-km)
    (bind-prefix! map "T" T-km))
    (list normal-map pending-map select-map)))
(set-command-display-binding! 'evil-motion-f "f <char>")
(set-command-display-binding! 'evil-motion-F "F <char>")
(set-command-display-binding! 'evil-motion-t "t <char>")
(set-command-display-binding! 'evil-motion-T "T <char>")

;; f/F/t/T — bind all printable non-space chars programmatically
(do ((i 33 (+ i 1)))
    ((= i 127))
  (let ((ch (string (integer->char i))))
    (for-each (lambda (map)
      (set-key! map (string-append "f " ch) 'evil-motion-f)
      (set-key! map (string-append "F " ch) 'evil-motion-F)
      (set-key! map (string-append "t " ch) 'evil-motion-t)
      (set-key! map (string-append "T " ch) 'evil-motion-T))
      (list normal-map pending-map select-map))))

;; f/F/t/T — space needs SPC notation
(for-each (lambda (map)
  (set-key! map "f SPC" 'evil-motion-f)
  (set-key! map "F SPC" 'evil-motion-F)
  (set-key! map "t SPC" 'evil-motion-t)
  (set-key! map "T SPC" 'evil-motion-T))
  (list normal-map pending-map select-map))

;; r — replace char(s) without entering insert mode
(set-key! normal-map "r" 'evil-char-replace-setup)
(set-key! select-map "r" 'evil-char-replace-setup)

;; Register modes (second arg = keymap, third arg = allows-input)
(define-minor-mode 'evil-normal-mode normal-map)
(define-minor-mode 'evil-insert-mode insert-map #t)
(define-minor-mode 'evil-replace-mode insert-map #t)
(define-minor-mode 'evil-select-mode select-map)
(define-minor-mode 'evil-command-mode command-map)
(define-minor-mode 'evil-pending-mode pending-map)

;; Register mode icons
(register-mode-icon/full 'evil-normal-mode  "icon-normal.svg"
                         'mode.normal  'label.normal  'cursor.normal  'solid)
(register-mode-icon/full 'evil-insert-mode  "icon-insert.svg"
                         'mode.insert  'label.insert  'cursor.insert  'thin)
(register-mode-icon/full 'evil-replace-mode "icon-replace.svg"
                         'mode.replace 'label.replace 'cursor.replace 'under)
(register-mode-icon/full 'evil-select-mode  "icon-select.svg"
                         'mode.select  'label.select  'cursor.select  'solid)
(register-mode-icon/full 'evil-command-mode "icon-command.svg"
                         'mode.command 'label.command 'cursor.command  'solid)
(register-mode-icon/full 'evil-pending-mode "icon-pending.svg"
                         'mode.pending 'label.pending 'cursor.pending  'under)

;; UTF-8 helpers
(define (utf8-byte-len-at pos)
  (let* ((raw (char->integer (char-at pos)))
         (b   (if (< raw 0) (+ raw 256) raw)))
    (cond ((< b 128) 1)
          ((< b 224) 2)
          ((< b 240) 3)
          (else 4))))

(define (count-chars-in-range start end)
  (let loop ((pos start) (n 0))
    (if (>= pos end)
        n
        (loop (+ pos (utf8-byte-len-at pos)) (+ n 1)))))

;; State transitions
(defcommand (evil-normal)
  "Return to normal mode."
  ;; Finalize insert/replace session change (only meaningful when panes exist)
  (unless (no-panes?)
    (when (%change-active?)
      (let ((typed (%change-current-inserts)))
        (when evil-pending-repeat-info
          (let ((ri evil-pending-repeat-info))
            (%change-set-repeat-info!
              (make-repeat-info (ri-op ri) (ri-op-count ri)
                                (ri-motion ri) (ri-motion-count ri)
                                (ri-text-object ri) (ri-text-object-kind ri)
                                (ri-setup ri) typed))))
        (set! evil-pending-repeat-info #f)
        (when evil-rect-insert-info
          (let* ((info evil-rect-insert-info)
                 (row-min (car info))
                 (row-max (cadr info))
                 (col     (car (cddr info)))
                 (ragged? (and (> (length info) 3) (list-ref info 3))))
            (set! evil-rect-insert-info #f)
            (when (> (string-length typed) 0)
              (let loop ((line row-max))
                (when (> line row-min)
                  (point-set! (if ragged?
                                  (line-content-end line)
                                  (min (+ (%line-start-position line) col)
                                       (%line-end-position line))))
                  (%insert-string typed)
                  (loop (- line 1)))))))
        (%end-change))))
  ;; Mode cleanup always runs
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (disable-minor-mode 'evil-pending-mode)
  (%set-cursor-override! #f)
  (enable-minor-mode 'evil-normal-mode)
  (%select-mode-set! 0)
  (%set-replace-mode! #f)
  (evil-reset-count)
  (set-local! 'mode-name "Normal")
  (message-clear)
  (when (%macro-recording?)
    (disable-minor-mode 'evil-recording-mode)
    (enable-minor-mode 'evil-recording-mode)))

(defcommand (evil-insert)
  "Enter insert mode."
  (unless (%change-active?)
    (%begin-change)
    (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'insert #f)))
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (disable-minor-mode 'evil-recording-mode)
  (enable-minor-mode 'evil-insert-mode)
  (%select-mode-set! 0)
  (%set-replace-mode! #f)
  (message-clear)
  (set-local! 'mode-name "Insert"))

(defcommand (evil-replace)
  "Enter replace mode."
  (unless (%change-active?)
    (%begin-change)
    (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'replace #f)))
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (disable-minor-mode 'evil-recording-mode)
  (enable-minor-mode 'evil-replace-mode)
  (%select-mode-set! 0)
  (%set-replace-mode! #t)
  (message-clear)
  (set-local! 'mode-name "Replace"))

(define (enter-visual-submode mode-int name)
  (set! evil-last-visual-text-object #f)
  (set! evil-last-visual-text-object-kind #f)
  (unless (%buffer-has-minor-mode? 'evil-select-mode)
    (%mark-set-to-point! #\<))
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-select-mode)
  (%select-mode-set! mode-int)
  (message-clear)
  (set-local! 'mode-name name))

(defcommand (evil-select)
  "Enter select mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 1))
      (evil-normal)
      (enter-visual-submode 1 "Select")))

(defcommand (evil-select-line)
  "Enter visual line mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 2))
      (evil-normal)
      (enter-visual-submode 2 "Line")))

(defcommand (evil-select-rectangle)
  "Enter visual block mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (rect-mode? (%select-mode-get)))
      (evil-normal)
      (enter-visual-submode 3 "Rectangle")))

(defcommand (evil-visual-dollar)
  "Move to end of line; in rectangle mode, enables ragged right edge."
  (when (rect-mode? (%select-mode-get))
    (%select-mode-set! 4))
  (line-end))

(defcommand (evil-command)
  "Enter command mode."
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (enable-minor-mode 'evil-command-mode)
  (set-local! 'mode-name "Command")
  (message-clear))


;; Query function for UI
(define (evil-state)
  (cond
    ((%buffer-has-minor-mode? 'evil-normal-mode) 'normal)
    ((%buffer-has-minor-mode? 'evil-insert-mode) 'insert)
    (else #f)))

;; Enable evil mode
(defcommand (evil-mode)
  "Enable vim-like modal editing."
  (enable-minor-mode 'evil-normal-mode))


;;;
;;; Evil State Machine
;;;

;; Range record
(define-record-type <range>
  (make-range start end type)
  range?
  (start range-start)
  (end range-end)
  (type range-type))  ; 'char | 'line

;; State variables
(define evil-sm-state 'normal)      ; 'normal | 'operator-pending
(define evil-count #f)              ; #f or accumulated int
(define evil-op-count #f)           ; count before operator
(define evil-pending-op #f)         ; operator symbol or #f
(define evil-op-echo #f)            ; echo string like "d" or "2d", or #f

;; Repeat-info record
(define-record-type <repeat-info>
  (make-repeat-info op op-count motion motion-count text-object text-object-kind setup insert-text)
  repeat-info?
  (op ri-op) (op-count ri-op-count)
  (motion ri-motion) (motion-count ri-motion-count)
  (text-object ri-text-object) (text-object-kind ri-text-object-kind)
  (setup ri-setup) (insert-text ri-insert-text))

;; Pending repeat-info (set during operator exec, consumed on insert exit)
(define evil-pending-repeat-info #f)

;; Visual text-object tracking (for dot repeat of visual operations)
(define evil-last-visual-text-object #f)
(define evil-last-visual-text-object-kind #f)
(define evil-rect-insert-info #f)  ; #f or (list row-min row-max col [ragged?])

(define (rect-mode? m) (or (= m 3) (= m 4)))

;; Motion registry
(define *motion-table* (make-hash-table eq?))

(define (register-motion! name proc)
  (hash-table-set! *motion-table* name proc))

(define (motion-ref name)
  (hash-table-ref/default *motion-table* name #f))

;; Operator registry
(define *operator-table* (make-hash-table eq?))

(define (register-operator! name proc)
  (hash-table-set! *operator-table* name proc))

(define (operator-ref name)
  (hash-table-ref/default *operator-table* name #f))

;; Text object registry
(define *text-object-table* (make-hash-table eq?))

(define (register-text-object! name proc)
  (hash-table-set! *text-object-table* name proc))

(define (text-object-ref name)
  (hash-table-ref/default *text-object-table* name #f))

(define (text-object-for-char ch)
  (case ch
    ((#\w) 'word)
    ((#\W) 'WORD)
    ((#\s) 'sentence)
    ((#\p) 'paragraph)
    ((#\[ #\]) 'bracket)
    ((#\( #\) #\b) 'paren)
    ((#\{ #\} #\B) 'brace)
    ((#\< #\>) 'angle)
    ((#\t) 'tag)
    ((#\") 'dquote)
    ((#\') 'squote)
    ((#\`) 'backtick)
    (else #f)))

;; Reset state machine
(define (evil-reset-count)
  (set! evil-sm-state 'normal)
  (set! evil-count #f)
  (set! evil-op-count #f)
  (set! evil-pending-op #f)
  (set! evil-op-echo #f)
  (message-clear)
  (%set-key-unbound-cb! #f)
  (disable-minor-mode 'evil-pending-mode)
  (enable-minor-mode 'evil-normal-mode)
  (set-local! 'mode-name "Normal")
  (when (%macro-recording?)
    (disable-minor-mode 'evil-recording-mode)
    (enable-minor-mode 'evil-recording-mode)))

;; Word character classification
(define (evil-word-char? c)
  (or (and (char>=? c #\a) (char<=? c #\z))
      (and (char>=? c #\A) (char<=? c #\Z))
      (and (char>=? c #\0) (char<=? c #\9))
      (char=? c #\_)))

(define (evil-whitespace? c)
  (or (char=? c #\space) (char=? c #\tab) (char=? c #\newline)
      (char=? c #\return)))

(define (evil-punctuation? c)
  (and (not (evil-word-char? c))
       (not (evil-whitespace? c))
       (not (char=? c #\null))))

;; Compute effective count: multiply op-count and motion-count
(define (evil-effective-count)
  (* (or evil-op-count 1) (or evil-count 1)))

;; Core dispatch: execute a motion
(define (evil-execute-motion motion-sym)
  (let ((proc (motion-ref motion-sym)))
    (when proc
      (if (eq? evil-sm-state 'operator-pending)
          ;; Operator-pending: save point, run motion, build range, apply op
          (let* ((the-op evil-pending-op)
                 (the-op-count evil-op-count)
                 (the-motion-count evil-count)
                 (saved-point (point-get))
                 (eff-count (evil-effective-count))
                 (_ (proc eff-count))
                 (new-point (point-get))
                 (start (min saved-point new-point))
                 (end (max saved-point new-point))
                 (range (make-range start end
                                   (if (eq? motion-sym 'current-line) 'line 'char)))
                 (op-proc (operator-ref the-op))
                 (ri (make-repeat-info the-op the-op-count
                                       motion-sym the-motion-count #f #f #f #f)))
            (point-set! saved-point)
            (evil-reset-count)
            (%begin-change)
            (when op-proc (op-proc range))
            ;; If operator entered insert mode, defer change end
            (if (%buffer-has-minor-mode? 'evil-insert-mode)
                (set! evil-pending-repeat-info ri)
                (begin
                  (%change-set-repeat-info! ri)
                  (%end-change))))
          ;; Normal state: just run motion with count
          (begin
            (proc (or evil-count 1))
            (set! evil-count #f)
            (message-clear))))))

;; Core dispatch: enter operator-pending
(define (evil-enter-operator op-sym)
  (if (and (eq? evil-sm-state 'operator-pending)
           (eq? evil-pending-op op-sym))
      ;; Doubled operator (dd, cc): move to line-start first so
      ;; saved-point in evil-execute-motion captures the beginning
      (let* ((orig-point (point-get))
             (_ (line-start))
             (line-start-pos (point-get))
             (col (+ 1 (- orig-point line-start-pos))))
        (evil-execute-motion 'current-line)
        (set-column col))
      (let* ((count-str (if evil-count (number->string evil-count) ""))
             (ch (last-key-char))
             (op-str (if ch (string ch) ""))
             (echo (string-append count-str op-str)))
        (set! evil-op-echo echo)
        (set! evil-op-count (or evil-count 1))
        (set! evil-count #f)
        (set! evil-pending-op op-sym)
        (set! evil-sm-state 'operator-pending)
        (disable-minor-mode 'evil-normal-mode)
        (enable-minor-mode 'evil-pending-mode)
        (set-local! 'mode-name "Pending")
        (%set-key-unbound-cb! 'evil-normal)
        (message-echo (string-append echo "-")))))

;; Core dispatch: execute a text object
(define (evil-execute-text-object kind)
  (let* ((ch (last-key-char))
         (sym (and ch (text-object-for-char ch)))
         (proc (and sym (text-object-ref sym))))
    (when proc
      (let ((range (proc kind (evil-effective-count))))
        (when range
          (if (eq? evil-sm-state 'operator-pending)
              ;; Operator-pending: wrap in transaction, apply operator
              (let ((op-proc (operator-ref evil-pending-op))
                    (ri (make-repeat-info evil-pending-op evil-op-count
                                          #f evil-count sym kind #f #f)))
                (evil-reset-count)
                (%begin-change)
                (when op-proc (op-proc range))
                (if (%buffer-has-minor-mode? 'evil-insert-mode)
                    (set! evil-pending-repeat-info ri)
                    (begin
                      (%change-set-repeat-info! ri)
                      (%end-change))))
              ;; Visual mode: set selection
              (when (%buffer-has-minor-mode? 'evil-select-mode)
                (set! evil-last-visual-text-object sym)
                (set! evil-last-visual-text-object-kind kind)
                (point-set! (range-start range))
                (%mark-set-to-point! #\<)
                (point-set! (- (range-end range) 1))
                (set! evil-count #f)
                (message-clear))))))))
