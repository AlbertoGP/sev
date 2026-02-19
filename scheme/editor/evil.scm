;;; evil.scm - Vim-like modal editing

;; Keymaps
(define normal-map (make-keymap))
(define insert-map (make-keymap))
(define select-map (make-keymap))
(define command-map (make-keymap))
(define pending-map (make-keymap))

;; Set parent pointers so system bindings inherit from global-keymap
(set-keymap-parent! normal-map  global-keymap)
(set-keymap-parent! insert-map  global-keymap)
(set-keymap-parent! select-map  global-keymap)
(set-keymap-parent! pending-map global-keymap)
(set-keymap-parent! command-map global-keymap)

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
(set-key! normal-map ":" 'evil-command)
(set-key! normal-map "g g" 'evil-motion-gg)
(set-key! normal-map "G" 'evil-motion-G)

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
(set-key! select-map "$" 'line-end)
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

;; m / ' / ` — bind all 26 letters programmatically
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map  (string-append "m " ch) 'evil-set-mark)
    (set-key! normal-map  (string-append "' " ch) 'evil-goto-mark-line)
    (set-key! normal-map  (string-append "` " ch) 'evil-goto-mark-exact)
    (set-key! pending-map (string-append "' " ch) 'evil-goto-mark-line)
    (set-key! pending-map (string-append "` " ch) 'evil-goto-mark-exact)))

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
                         'mode.select  'label.select  'cursor.select  'hollow)
(register-mode-icon/full 'evil-command-mode "icon-command.svg"
                         'mode.command 'label.command 'cursor.command  'solid)
(register-mode-icon/full 'evil-pending-mode "icon-pending.svg"
                         'mode.pending 'label.pending 'cursor.pending  'under)

;; State transitions
(defcommand (evil-normal)
  "Return to normal mode."
  ;; Finalize insert/replace session change
  (when (%change-active?)
    (let ((typed (%change-current-inserts)))
      (when evil-pending-repeat-info
        (let ((ri evil-pending-repeat-info))
          (%change-set-repeat-info!
            (make-repeat-info (ri-op ri) (ri-op-count ri)
                              (ri-motion ri) (ri-motion-count ri)
                              (ri-text-object ri) (ri-text-object-kind ri)
                              (ri-setup ri) typed)))))
    (set! evil-pending-repeat-info #f)
    (%end-change))
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (disable-minor-mode 'evil-pending-mode)
  (enable-minor-mode 'evil-normal-mode)
  (%select-mode-set! 0)
  (%set-replace-mode! #f)
  (evil-reset-count)
  (set-local! 'mode-name "Normal") 
  (unless (%macro-recording?)
    (message-unlock)
    (message-clear))
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
  (%set-replace-mode! #f)
  (set-local! 'mode-name "Insert")
  (message "-- INSERT --")
  (message-lock))

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
  (%set-replace-mode! #t)
  (set-local! 'mode-name "Replace")
  (message "-- REPLACE --")
  (message-lock))

(define (enter-visual-submode mode-int name msg)
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
  (set-local! 'mode-name name)
  (message msg)
  (message-lock))

(defcommand (evil-select)
  "Enter select mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 1))
      (evil-normal)
      (enter-visual-submode 1 "Select" "-- SELECT --")))

(defcommand (evil-select-line)
  "Enter visual line mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 2))
      (evil-normal)
      (enter-visual-submode 2 "Line" "-- SELECT LINE --")))

(defcommand (evil-select-rectangle)
  "Enter visual block mode."
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 3))
      (evil-normal)
      (enter-visual-submode 3 "Rectangle" "-- SELECT RECTANGLE --")))

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
      (begin
        (set! evil-op-count (or evil-count 1))
        (set! evil-count #f)
        (set! evil-pending-op op-sym)
        (set! evil-sm-state 'operator-pending)
        (disable-minor-mode 'evil-normal-mode)
        (enable-minor-mode 'evil-pending-mode)
        (set-local! 'mode-name "Pending")
        (message-clear))))

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

;; Undo
(defcommand (evil-undo)
  "Undo last change."
  (%undo))

;; Redo
(defcommand (evil-redo)
  "Redo last undone change."
  (%redo))

;; Line restore
(defcommand (evil-line-restore)
  "Restore line to its state before edits began on it."
  (%begin-change)
  (%line-restore)
  (%end-change))

;; Replay helpers for dottable operations
(define (evil-replay-setup setup)
  (case setup
    ((insert) #t)
    ((replace) #t)
    ((append-char) (forward-char))
    ((append-line) (line-end))
    ((insert-at-start) (line-start) (skip-whitespace))
    ((open-below) (line-end) (newline))
    ((open-above) (line-start) (newline) (prev-line))
    ((substitute) (delete-forward-char))
    ((paste-after) (forward-char))
    ((paste-before) #t)
    ((paste-after-linewise) (line-end) (%insert-string "\n"))
    ((paste-before-linewise) (line-start))
    (else #t)))

(define (evil-insert-text text)
  (let ((len (string-length text)))
    (let loop ((i 0))
      (when (< i len)
        (insert-char (string-ref text i))
        (loop (+ i 1))))))

;; Dot repeat
(defcommand (evil-repeat)
  "Repeat last change."
  (let ((ri (%change-last-repeat-info))
        (user-count evil-count))
    (when (repeat-info? ri)
      (let ((count (or user-count (ri-op-count ri))))
        (cond
          ;; Operator + motion
          ((ri-motion ri)
           (let ((mproc (motion-ref (ri-motion ri)))
                 (op-proc (operator-ref (ri-op ri))))
             (when (and mproc op-proc)
               (when (eq? (ri-motion ri) 'current-line) (line-start))
               (%begin-change)
               (let* ((eff (* (or count 1) (or (ri-motion-count ri) 1)))
                      (saved (point-get))
                      (_ (mproc eff))
                      (end (point-get))
                      (start (min saved end))
                      (end2 (max saved end)))
                 (point-set! saved)
                 (op-proc (make-range start end2 'char)))
               ;; If operator entered insert mode and we have typed text, replay it
               (if (and (%buffer-has-minor-mode? 'evil-insert-mode)
                        (ri-insert-text ri)
                        (> (string-length (ri-insert-text ri)) 0))
                   (begin
                     (evil-insert-text (ri-insert-text ri))
                     (%change-set-repeat-info! ri)
                     (%end-change)
                     (evil-normal))
                   (if (%buffer-has-minor-mode? 'evil-insert-mode)
                       (set! evil-pending-repeat-info ri)
                       (begin
                         (%change-set-repeat-info! ri)
                         (%end-change)))))))
          ;; Operator + text object
          ((ri-text-object ri)
           (let ((tproc (text-object-ref (ri-text-object ri)))
                 (op-proc (operator-ref (ri-op ri))))
             (when (and tproc op-proc)
               (let* ((eff (* (or count 1) (or (ri-motion-count ri) 1)))
                      (r (tproc (ri-text-object-kind ri) eff)))
                 (when r
                   (%begin-change)
                   (op-proc r)
                   ;; If operator entered insert mode and we have typed text, replay it
                   (if (and (%buffer-has-minor-mode? 'evil-insert-mode)
                            (ri-insert-text ri)
                            (> (string-length (ri-insert-text ri)) 0))
                       (begin
                         (evil-insert-text (ri-insert-text ri))
                         (%change-set-repeat-info! ri)
                         (%end-change)
                         (evil-normal))
                       (if (%buffer-has-minor-mode? 'evil-insert-mode)
                           (set! evil-pending-repeat-info ri)
                           (begin
                             (%change-set-repeat-info! ri)
                             (%end-change)))))))))
          ;; Setup-based session (insert, replace, append, open, substitute)
          ((ri-setup ri)
           (when (and (ri-insert-text ri)
                      (> (string-length (ri-insert-text ri)) 0))
             (%begin-change)
             (evil-replay-setup (ri-setup ri))
             (if (eq? (ri-setup ri) 'replace)
                 ;; Replace mode: delete then insert per character
                 (let* ((text (ri-insert-text ri))
                        (len (string-length text)))
                   (let loop ((i 0))
                     (when (< i len)
                       (let ((c (char-at-point)))
                         (when (and (not (char=? c #\null))
                                    (not (char=? c #\newline)))
                           (delete-forward-char)))
                       (insert-char (string-ref text i))
                       (loop (+ i 1)))))
                 ;; Normal insert
                 (evil-insert-text (ri-insert-text ri)))
             (%change-set-repeat-info! ri)
             (%end-change)))
          (else #f)))))
  (set! evil-count #f)
  (message-clear))

;; Count accumulation
(defcommand (evil-digit-argument)
  "Accumulate count prefix."
  (let ((ch (last-key-char)))
    (when (and ch (not (eq? ch #f)))
      (let ((digit (- (char->integer ch) (char->integer #\0))))
        (set! evil-count (+ (* (or evil-count 0) 10) digit))
        (message (string-append (number->string evil-count) "-"))))))

;; 0 key: line-start or append zero to count
(defcommand (evil-zero)
  "Line start or append zero to count."
  (if evil-count
      (begin
        (set! evil-count (* evil-count 10))
        (message (string-append (number->string evil-count) "-")))
      (evil-execute-motion 'motion-0)))

;;;
;;; Motion definitions
;;;

(register-motion! 'motion-h
  (lambda (count) (move-point (- count))))

(register-motion! 'motion-l
  (lambda (count) (move-point count)))

(register-motion! 'motion-j
  (lambda (count) (move-point-by-line count)))

(register-motion! 'motion-k
  (lambda (count) (move-point-by-line (- count))))

(register-motion! 'motion-0
  (lambda (count) (line-start)))

(register-motion! 'motion-$
  (lambda (count)
    (when (> count 1)
      (move-point-by-line (- count 1)))
    (line-end)))

(register-motion! 'motion-^
  (lambda (count) (line-start) (skip-whitespace)))

;; Word forward motion
(register-motion! 'motion-w
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((len (buffer-length))
              (p (point-get)))
          (when (< p len)
            (let ((c (char-at p)))
              (cond
                ;; On a word char: skip word chars, then skip whitespace
                ((evil-word-char? c)
                 (let skip-word ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-word-char? (char-at pp)))
                       (move-point 1) (skip-word))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On punctuation: skip punctuation, then skip whitespace
                ((evil-punctuation? c)
                 (let skip-punct ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-punctuation? (char-at pp)))
                       (move-point 1) (skip-punct))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On whitespace: skip whitespace
                ((evil-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))))))
        (loop (+ i 1))))))

;; Word backward motion
(register-motion! 'motion-b
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((p (point-get)))
          (when (> p 0)
            ;; Move back one char first
            (move-point -1)
            ;; Skip whitespace backwards
            (let skip-ws ()
              (let ((pp (point-get)))
                (when (and (> pp 0) (evil-whitespace? (char-at pp)))
                  (move-point -1) (skip-ws))))
            ;; Now skip same-class chars backwards
            (let ((pp (point-get)))
              (let ((c (char-at pp)))
                (cond
                  ((evil-word-char? c)
                   (let skip-word ()
                     (let ((pp2 (point-get)))
                       (when (and (> pp2 0) (evil-word-char? (char-at (- pp2 1))))
                         (move-point -1) (skip-word)))))
                  ((evil-punctuation? c)
                   (let skip-punct ()
                     (let ((pp2 (point-get)))
                       (when (and (> pp2 0) (evil-punctuation? (char-at (- pp2 1))))
                         (move-point -1) (skip-punct))))))))))
        (loop (+ i 1))))))

;; End-of-word forward motion
(register-motion! 'motion-e
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((len (buffer-length))
              (p (point-get)))
          (when (< p (- len 1))
            ;; Check if we're at the last char of current word-class run
            (let ((c (char-at p))
                  (nc (char-at (+ p 1))))
              (cond
                ;; On whitespace: skip whitespace, then find end of next run
                ((evil-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 ;; Now on a non-whitespace char, advance to end of its class
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((evil-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((evil-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ;; At end of word-class run: move forward, skip ws, find end of next run
                ((and (evil-word-char? c)
                      (or (evil-whitespace? nc) (evil-punctuation? nc)))
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((evil-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((evil-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ((and (evil-punctuation? c)
                      (or (evil-whitespace? nc) (evil-word-char? nc)))
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((evil-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((evil-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (evil-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ;; In middle of word-class run: advance to end
                ((evil-word-char? c)
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-word-char? (char-at (+ pp 1))))
                       (move-point 1) (advance)))))
                ((evil-punctuation? c)
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-punctuation? (char-at (+ pp 1))))
                       (move-point 1) (advance)))))))))
        (loop (+ i 1))))))

;; WORD forward motion (whitespace-delimited)
(register-motion! 'motion-W
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((len (buffer-length))
              (p (point-get)))
          (when (< p len)
            (let ((c (char-at p)))
              (cond
                ;; On non-whitespace: skip non-ws, then skip ws
                ((not (evil-whitespace? c))
                 (let skip-nonws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (not (evil-whitespace? (char-at pp))))
                       (move-point 1) (skip-nonws))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On whitespace: skip ws
                ((evil-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))))))
        (loop (+ i 1))))))

;; WORD backward motion (whitespace-delimited)
(register-motion! 'motion-B
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((p (point-get)))
          (when (> p 0)
            ;; Move back one char first
            (move-point -1)
            ;; Skip whitespace backwards
            (let skip-ws ()
              (let ((pp (point-get)))
                (when (and (> pp 0) (evil-whitespace? (char-at pp)))
                  (move-point -1) (skip-ws))))
            ;; Skip non-whitespace backwards
            (let ((pp (point-get)))
              (when (and (> pp 0) (not (evil-whitespace? (char-at pp))))
                (let skip-nonws ()
                  (let ((pp2 (point-get)))
                    (when (and (> pp2 0) (not (evil-whitespace? (char-at (- pp2 1)))))
                      (move-point -1) (skip-nonws))))))))
        (loop (+ i 1))))))

;; End-of-WORD forward motion (whitespace-delimited)
(register-motion! 'motion-E
  (lambda (count)
    (let loop ((i 0))
      (when (< i count)
        (let ((len (buffer-length))
              (p (point-get)))
          (when (< p (- len 1))
            (let ((c (char-at p))
                  (nc (char-at (+ p 1))))
              (cond
                ;; On whitespace: skip ws, then advance to end of non-ws run
                ((evil-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (evil-whitespace? (char-at (+ pp 1)))))
                       (move-point 1) (advance)))))
                ;; At end of non-ws run (next is whitespace): move, skip ws, find end
                ((evil-whitespace? nc)
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (evil-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (evil-whitespace? (char-at (+ pp 1)))))
                       (move-point 1) (advance)))))
                ;; In middle of non-ws run: advance to end
                (else
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (evil-whitespace? (char-at (+ pp 1)))))
                       (move-point 1) (advance)))))))))
        (loop (+ i 1))))))

;; Goto-line motion
(register-motion! 'motion-goto-line
  (lambda (count) (%goto-line count)))

;; Current line motion (for dd, cc)
(register-motion! 'current-line
  (lambda (count)
    (line-start)
    (let ((start (point-get)))
      (move-point-by-line (- count 1))
      (line-end)
      ;; Include trailing newline if present
      (let ((p (point-get))
            (len (buffer-length)))
        (when (and (< p len) (char=? (char-at p) #\newline))
          (move-point 1))))))

;;;
;;; Register helpers
;;;

(define (strip-trailing-newline text)
  (let ((len (string-length text)))
    (if (and (> len 0) (char=? (string-ref text (- len 1)) #\newline))
        (substring text 0 (- len 1))
        text)))

(define (string-split-newlines s)
  (let ((len (string-length s)))
    (let loop ((i 0) (start 0) (acc '()))
      (cond
        ((= i len) (reverse (cons (substring s start i) acc)))
        ((char=? (string-ref s i) #\newline)
         (loop (+ i 1) (+ i 1) (cons (substring s start i) acc)))
        (else (loop (+ i 1) start acc))))))

(define (string-join-newlines parts)
  (if (null? parts)
      ""
      (let loop ((rest (cdr parts)) (acc (car parts)))
        (if (null? rest)
            acc
            (loop (cdr rest) (string-append acc "\n" (car rest)))))))

;; Insert block `lines` (list of strings) at column `col` on consecutive lines
;; starting from the cursor's current line. Processes bottom-to-top.
(define (evil-paste-block lines col)
  (let* ((start-line (%position-line (point-get)))
         (n (length lines))
         (needed-last (+ start-line (- n 1)))
         (cur-last (%line-count)))
    ;; Append empty lines at end of buffer if needed
    (when (> needed-last cur-last)
      (point-set! (buffer-length))
      (let add ((k cur-last))
        (when (< k needed-last)
          (insert-char #\newline)
          (add (+ k 1)))))
    ;; Insert each block line bottom-to-top
    (let loop ((i (- n 1)))
      (when (>= i 0)
        (let* ((target-line (+ start-line i))
               (chunk (list-ref lines i))
               (ls (%line-start-position target-line))
               (le (%line-end-position target-line))
               (line-len (- le ls)))
          (if (< line-len col)
              (begin
                ;; Pad line to reach col, then insert chunk
                (point-set! le)
                (let pad ((k line-len))
                  (when (< k col)
                    (insert-char #\space)
                    (pad (+ k 1))))
                (%insert-string chunk))
              (begin
                (point-set! (+ ls col))
                (%insert-string chunk))))
        (loop (- i 1))))))

(define (evil-register-write! text . rest)
  (let* ((shape (if (pair? rest) (car rest) 'charwise))
         ;; Linewise registers always end with \n for consistent paste behaviour
         (text (if (and (eq? shape 'linewise)
                        (or (= (string-length text) 0)
                            (not (char=? (string-ref text (- (string-length text) 1)) #\newline))))
                   (string-append text "\n")
                   text)))
    (let ((reg current-evil-register))
      (cond
        ((char=? reg #\_)
         ;; Black hole: discard silently
         #f)
        ((char=? reg #\+)
         ;; Clipboard: write to OS clipboard and update unnamed
         (%clipboard-set! text)
         (%register-set! #\" text)
         (%register-set-shape! #\" shape))
        ((char-upper-case? reg)
         (let ((lc (char-downcase reg)))
           (%register-append! lc text)
           (%register-set-shape! lc shape)
           (%register-set! #\" text)
           (%register-set-shape! #\" shape)))
        (else
         (%register-set! reg text)
         (%register-set-shape! reg shape)
         (when (not (char=? reg #\"))
           (%register-set! #\" text)
           (%register-set-shape! #\" shape))))
      (set! current-evil-register #\"))))

;;;
;;; Operator definitions
;;;

(register-operator! 'op-delete
  (lambda (range)
    (let* ((start (range-start range))
           (end   (range-end range))
           (text  (%buffer-substring start end))
           (shape (if (eq? (range-type range) 'line) 'linewise 'charwise)))
      (evil-register-write! text shape)
      (delete-range start end))))

(register-operator! 'op-change
  (lambda (range)
    (let* ((start (range-start range))
           (end (range-end range))
           (end (if (and (> end start)
                         (char=? (char-at (- end 1)) #\newline))
                    (- end 1)
                    end)))
      (delete-range start end)
      (evil-insert))))

(register-operator! 'op-yank
  (lambda (range)
    (let* ((start (range-start range))
           (end   (range-end range))
           (text  (%buffer-substring start end))
           (shape (if (eq? (range-type range) 'line) 'linewise 'charwise)))
      (evil-register-write! text shape)
      (point-set! start))))

;;;
;;; Text object helpers
;;;

(define (find-enclosing-pair open close)
  (let ((len (buffer-length))
        (pos (point-get)))
    (define (find-close-from start)
      (let loop ((p (+ start 1)) (depth 0))
        (if (>= p len) #f
            (let ((c (char-at p)))
              (cond
                ((char=? c open) (loop (+ p 1) (+ depth 1)))
                ((char=? c close)
                 (if (= depth 0) (cons start p)
                     (loop (+ p 1) (- depth 1))))
                (else (loop (+ p 1) depth)))))))
    (define (find-open-from p)
      (let loop ((q p) (depth 0))
        (if (< q 0) #f
            (let ((c (char-at q)))
              (cond
                ((char=? c close) (loop (- q 1) (+ depth 1)))
                ((char=? c open)
                 (if (= depth 0) (find-close-from q)
                     (loop (- q 1) (- depth 1))))
                (else (loop (- q 1) depth)))))))
    (and (> len 0) (< pos len)
         (or (let ((c (char-at pos)))
               (cond
                 ((char=? c open) (find-close-from pos))
                 ((char=? c close) (find-open-from (- pos 1)))
                 (else (find-open-from pos))))
             ;; Forward seek: find next open delimiter in buffer
             (let seek ((p (+ pos 1)))
               (if (>= p len) #f
                   (if (char=? (char-at p) open)
                       (find-close-from p)
                       (seek (+ p 1)))))))))


(define (find-quote-pair qchar)
  (let* ((pos (point-get))
         (len (buffer-length))
         (line (%position-line pos))
         (ls (%line-start-position line))
         (le (%line-end-position line)))
    ;; Collect all positions of qchar on current line
    (let collect ((p ls) (acc '()))
      (if (>= p le)
          (let ((positions (reverse acc)))
            ;; Pair sequentially, find pair containing cursor
            (let pair-up ((ps positions))
              (if (or (null? ps) (null? (cdr ps)))
                  ;; No containing pair; find next pair ahead of cursor
                  (let ahead ((ps2 positions))
                    (if (or (null? ps2) (null? (cdr ps2))) #f
                        (if (> (car ps2) pos)
                            (cons (car ps2) (cadr ps2))
                            (ahead (cddr ps2)))))
                  (let ((a (car ps)) (b (cadr ps)))
                    (if (and (>= pos a) (<= pos b))
                        (cons a b)
                        (pair-up (cddr ps)))))))
          (if (and (< p len) (char=? (char-at p) qchar))
              (collect (+ p 1) (cons p acc))
              (collect (+ p 1) acc))))))

(define (find-enclosing-tag)
  (let ((len (buffer-length))
        (pos (point-get)))
    (define (extract-tag-name start)
      (let loop ((p start) (acc '()))
        (if (or (>= p len)
                (let ((c (char-at p)))
                  (or (char=? c #\space) (char=? c #\tab)
                      (char=? c #\>) (char=? c #\/) (char=? c #\newline))))
            (list->string (reverse acc))
            (loop (+ p 1) (cons (char-at p) acc)))))
    (define (find-gt p)
      (let loop ((q p))
        (if (>= q len) #f
            (if (char=? (char-at q) #\>) (+ q 1)
                (loop (+ q 1))))))
    (define (self-closing? p)
      (let loop ((q p))
        (if (>= q len) #f
            (let ((c (char-at q)))
              (cond
                ((char=? c #\>) #f)
                ((and (char=? c #\/)
                      (< (+ q 1) len)
                      (char=? (char-at (+ q 1)) #\>)) #t)
                (else (loop (+ q 1))))))))
    (define (opening-tag? p)
      (and (< (+ p 1) len)
           (char=? (char-at p) #\<)
           (let ((next (char-at (+ p 1))))
             (and (not (char=? next #\/))
                  (not (char=? next #\!))
                  (not (char=? next #\?))
                  (not (self-closing? p))))))
    ;; Given position of an opening tag <, find its matching close tag
    (define (match-open-tag p)
      (let* ((name (extract-tag-name (+ p 1)))
             (open-end (find-gt p)))
        (and open-end
             (let find-close ((q open-end) (d 0))
               (if (>= q len) #f
                   (if (and (char=? (char-at q) #\<)
                            (< (+ q 1) len))
                       (let ((qn (char-at (+ q 1))))
                         (cond
                           ((char=? qn #\/)
                            (let ((cn (extract-tag-name (+ q 2)))
                                  (ce (find-gt q)))
                              (if (string=? cn name)
                                  (if (= d 0)
                                      (and ce (list p open-end q ce))
                                      (find-close (or ce len) (- d 1)))
                                  (find-close (or ce (+ q 1)) d))))
                           ((and (not (char=? qn #\!))
                                 (not (char=? qn #\?))
                                 (not (self-closing? q)))
                            (let ((in (extract-tag-name (+ q 1)))
                                  (ie (find-gt q)))
                              (if (string=? in name)
                                  (find-close (or ie (+ q 1)) (+ d 1))
                                  (find-close (or ie (+ q 1)) d))))
                           (else
                            (find-close (or (find-gt q) (+ q 1)) d))))
                       (find-close (+ q 1) d)))))))
    (or
     ;; Search backward for unmatched opening tag
     (let find-open ((p pos) (depth 0))
       (if (< p 0) #f
           (if (and (char=? (char-at p) #\<) (< (+ p 1) len))
               (let ((next (char-at (+ p 1))))
                 (cond
                   ((or (char=? next #\!) (char=? next #\?))
                    (find-open (- p 1) depth))
                   ((char=? next #\/)
                    (find-open (- p 1) (+ depth 1)))
                   ((self-closing? p)
                    (find-open (- p 1) depth))
                   (else
                    (if (= depth 0)
                        (or (match-open-tag p)
                            (find-open (- p 1) depth))
                        (find-open (- p 1) (- depth 1))))))
               (find-open (- p 1) depth))))
     ;; Forward seek: find next opening tag
     (let seek ((p (+ pos 1)))
       (if (>= p len) #f
           (if (opening-tag? p)
               (match-open-tag p)
               (seek (+ p 1))))))))

;;;
;;; Text object implementations
;;;

;; word text object
(register-text-object! 'word
  (lambda (kind count)
    (let ((len (buffer-length))
          (pos (point-get)))
      (and (> len 0) (< pos len)
           (let* ((c (char-at pos))
                  (pred (cond ((evil-word-char? c) evil-word-char?)
                              ((evil-punctuation? c) evil-punctuation?)
                              (else evil-whitespace?))))
             (let ((start (let bwd ((s pos))
                            (if (and (> s 0) (pred (char-at (- s 1))))
                                (bwd (- s 1)) s)))
                   (end (let fwd ((e pos))
                          (if (and (< e len) (pred (char-at e)))
                              (fwd (+ e 1)) e))))
               ;; Extend for count
               (let ext ((e end) (n (- count 1)))
                 (if (<= n 0)
                     (if (eq? kind 'around)
                         (let ((ae (let tr ((p e))
                                     (if (and (< p len) (evil-whitespace? (char-at p)))
                                         (tr (+ p 1)) p))))
                           (if (> ae e)
                               (make-range start ae 'char)
                               (let ((as (let ld ((p start))
                                           (if (and (> p 0) (evil-whitespace? (char-at (- p 1))))
                                               (ld (- p 1)) p))))
                                 (make-range as end 'char))))
                         (make-range start end 'char))
                     (let ((ws (let sk ((p e))
                                 (if (and (< p len) (evil-whitespace? (char-at p)))
                                     (sk (+ p 1)) p))))
                       (if (>= ws len) (ext ws 0)
                           (let ((cp (if (evil-word-char? (char-at ws))
                                         evil-word-char? evil-punctuation?)))
                             (ext (let sk ((p ws))
                                    (if (and (< p len) (cp (char-at p)))
                                        (sk (+ p 1)) p))
                                  (- n 1)))))))))))))

;; WORD text object
(register-text-object! 'WORD
  (lambda (kind count)
    (let ((len (buffer-length))
          (pos (point-get)))
      (and (> len 0) (< pos len)
           (let ((ws? (evil-whitespace? (char-at pos))))
             (let ((start (let bwd ((s pos))
                            (if (and (> s 0)
                                     (if ws? (evil-whitespace? (char-at (- s 1)))
                                         (not (evil-whitespace? (char-at (- s 1))))))
                                (bwd (- s 1)) s)))
                   (end (let fwd ((e pos))
                          (if (and (< e len)
                                   (if ws? (evil-whitespace? (char-at e))
                                       (not (evil-whitespace? (char-at e)))))
                              (fwd (+ e 1)) e))))
               ;; Extend for count
               (let ext ((e end) (n (- count 1)))
                 (if (<= n 0)
                     (if (eq? kind 'around)
                         (let ((ae (let tr ((p e))
                                     (if (and (< p len) (evil-whitespace? (char-at p)))
                                         (tr (+ p 1)) p))))
                           (if (> ae e)
                               (make-range start ae 'char)
                               (let ((as (let ld ((p start))
                                           (if (and (> p 0) (evil-whitespace? (char-at (- p 1))))
                                               (ld (- p 1)) p))))
                                 (make-range as end 'char))))
                         (make-range start end 'char))
                     (let ((ws (let sk ((p e))
                                 (if (and (< p len) (evil-whitespace? (char-at p)))
                                     (sk (+ p 1)) p))))
                       (if (>= ws len) (ext ws 0)
                           (ext (let sk ((p ws))
                                  (if (and (< p len) (not (evil-whitespace? (char-at p))))
                                      (sk (+ p 1)) p))
                                (- n 1))))))))))))

;; sentence text object
(register-text-object! 'sentence
  (lambda (kind count)
    (let ((len (buffer-length))
          (pos (point-get)))
      (and (> len 0) (< pos len)
           (let ()
             (define (sentence-end? p)
               (and (< p len)
                    (let ((c (char-at p)))
                      (and (or (char=? c #\.) (char=? c #\!) (char=? c #\?))
                           (or (>= (+ p 1) len)
                               (evil-whitespace? (char-at (+ p 1))))))))
             ;; Find sentence start: search backward for sentence-end + ws
             (let ((start (let bwd ((p (- pos 1)))
                            (if (< p 0) 0
                                (if (sentence-end? p)
                                    (let skip-ws ((q (+ p 1)))
                                      (if (and (< q len) (evil-whitespace? (char-at q)))
                                          (skip-ws (+ q 1)) q))
                                    (bwd (- p 1)))))))
               ;; Find sentence end (repeat for count)
               (let ext ((e pos) (n count))
                 (if (<= n 0)
                     (let ((end (+ e 1)))
                       (if (eq? kind 'around)
                           (let ((ae (let tr ((p end))
                                       (if (and (< p len) (evil-whitespace? (char-at p)))
                                           (tr (+ p 1)) p))))
                             (make-range start ae 'char))
                           (make-range start end 'char)))
                     (let fwd ((p (if (= n count) pos (+ e 1))))
                       (if (>= p len) (ext (- len 1) 0)
                           (if (sentence-end? p)
                               (ext p (- n 1))
                               (fwd (+ p 1)))))))))))))

;; paragraph text object
(register-text-object! 'paragraph
  (lambda (kind count)
    (let ((len (buffer-length))
          (pos (point-get)))
      (and (> len 0) (< pos len)
           (let ()
             (define (blank-line? line)
               (= (%line-start-position line) (%line-end-position line)))
             (let* ((cur-line (%position-line pos))
                    (max-line (%line-count)))
               ;; Find start: search backward for blank line boundary
               (let ((start-line (let bwd ((l cur-line))
                                   (if (or (<= l 1) (blank-line? l))
                                       (if (blank-line? l) (+ l 1) l)
                                       (bwd (- l 1))))))
                 ;; Find end: search forward for blank line (repeat for count)
                 (let ext ((end-line cur-line) (n count))
                   (if (<= n 0)
                       (let ((start (%line-start-position start-line))
                             (end (let ((le (%line-end-position end-line)))
                                    (if (and (< le len) (char=? (char-at le) #\newline))
                                        (+ le 1) le))))
                         (if (eq? kind 'around)
                             ;; Include trailing blank lines
                             (let tr ((l (+ end-line 1)))
                               (if (and (<= l max-line) (blank-line? l))
                                   (tr (+ l 1))
                                   (let ((ae (if (> l (+ end-line 1))
                                                 (let ((le (%line-end-position (- l 1))))
                                                   (if (and (< le len) (char=? (char-at le) #\newline))
                                                       (+ le 1) le))
                                                 end)))
                                     (make-range start ae 'char))))
                             (make-range start end 'char)))
                       (let fwd ((l (+ end-line 1)))
                         (if (> l max-line) (ext max-line 0)
                             (if (blank-line? l)
                                 (ext (- l 1) (- n 1))
                                 (fwd (+ l 1))))))))))))))

;; Paired delimiter text objects
(define (make-pair-text-object open close)
  (lambda (kind count)
    (let ((pair (find-enclosing-pair open close)))
      (and pair
           (let ((o (car pair)) (c (cdr pair)))
             (if (eq? kind 'in)
                 (make-range (+ o 1) c 'char)
                 (make-range o (+ c 1) 'char)))))))

(register-text-object! 'bracket (make-pair-text-object #\[ #\]))
(register-text-object! 'paren   (make-pair-text-object #\( #\)))
(register-text-object! 'brace   (make-pair-text-object #\{ #\}))
(register-text-object! 'angle   (make-pair-text-object #\< #\>))

;; Quote text objects
(define (make-quote-text-object qchar)
  (lambda (kind count)
    (let ((pair (find-quote-pair qchar)))
      (and pair
           (let ((o (car pair)) (c (cdr pair)))
             (if (eq? kind 'in)
                 (make-range (+ o 1) c 'char)
                 (make-range o (+ c 1) 'char)))))))

(register-text-object! 'dquote   (make-quote-text-object #\"))
(register-text-object! 'squote   (make-quote-text-object #\'))
(register-text-object! 'backtick (make-quote-text-object #\`))

;; Tag text object
(register-text-object! 'tag
  (lambda (kind count)
    (let ((result (find-enclosing-tag)))
      (and result
           (let ((open-start (list-ref result 0))
                 (open-end (list-ref result 1))
                 (close-start (list-ref result 2))
                 (close-end (list-ref result 3)))
             (if (eq? kind 'in)
                 (make-range open-end close-start 'char)
                 (make-range open-start close-end 'char)))))))

;;;
;;; Motion command wrappers (bound to keys)
;;;

(defcommand (evil-motion-h) "Move left." (evil-execute-motion 'motion-h))
(defcommand (evil-motion-j) "Move down." (evil-execute-motion 'motion-j))
(defcommand (evil-motion-k) "Move up." (evil-execute-motion 'motion-k))
(defcommand (evil-motion-l) "Move right." (evil-execute-motion 'motion-l))
(defcommand (evil-motion-$) "Move to end of line." (evil-execute-motion 'motion-$))
(defcommand (evil-motion-^) "Move to first non-blank." (evil-execute-motion 'motion-^))
(defcommand (evil-motion-w) "Move forward one word." (evil-execute-motion 'motion-w))
(defcommand (evil-motion-b) "Move backward one word." (evil-execute-motion 'motion-b))
(defcommand (evil-motion-e) "Move to end of word." (evil-execute-motion 'motion-e))
(defcommand (evil-motion-W) "Move forward one WORD." (evil-execute-motion 'motion-W))
(defcommand (evil-motion-B) "Move backward one WORD." (evil-execute-motion 'motion-B))
(defcommand (evil-motion-E) "Move to end of WORD." (evil-execute-motion 'motion-E))

(defcommand (evil-motion-gg)
  "Go to first line."
  (evil-execute-motion 'motion-goto-line))

(defcommand (evil-motion-G)
  "Go to last line."
  (unless evil-count (set! evil-count (%line-count)))
  (evil-execute-motion 'motion-goto-line))

(defcommand (evil-set-mark)
  "Set named mark."
  (let ((ch (last-key-char)))
    (when ch
      (%mark-set-to-point! ch)
      (message (string-append "Mark set: " (string ch))))))


(defcommand (evil-goto-mark-line)
  "Jump to line of mark."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--mark-line-tmp
        (lambda (count) (%point-to-mark! ch) (line-start)))
      (evil-execute-motion 'motion--mark-line-tmp))))

(defcommand (evil-goto-mark-exact)
  "Jump to exact position of mark."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--mark-exact-tmp
        (lambda (count) (%point-to-mark! ch)))
      (evil-execute-motion 'motion--mark-exact-tmp))))

;; f/F/t/T — character seeking (current line only)

(defcommand (evil-motion-f)
  "Find character forward (on)."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--seek-tmp
        (lambda (count)
          (let ((len (buffer-length))
                (le  (%line-end-position (%position-line (point-get)))))
            (let loop ((n count) (p (+ (point-get) 1)))
              (when (<= p le)
                (if (and (< p len) (char=? (char-at p) ch))
                    (if (<= n 1) (point-set! p) (loop (- n 1) (+ p 1)))
                    (loop n (+ p 1))))))))
      (evil-execute-motion 'motion--seek-tmp))))

(defcommand (evil-motion-F)
  "Find character backward (on)."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--seek-tmp
        (lambda (count)
          (let ((ls (%line-start-position (%position-line (point-get)))))
            (let loop ((n count) (p (- (point-get) 1)))
              (when (>= p ls)
                (if (char=? (char-at p) ch)
                    (if (<= n 1) (point-set! p) (loop (- n 1) (- p 1)))
                    (loop n (- p 1))))))))
      (evil-execute-motion 'motion--seek-tmp))))

(defcommand (evil-motion-t)
  "Find character forward (before)."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--seek-tmp
        (lambda (count)
          (let ((len (buffer-length))
                (le  (%line-end-position (%position-line (point-get)))))
            (let loop ((n count) (p (+ (point-get) 1)))
              (when (<= p le)
                (if (and (< p len) (char=? (char-at p) ch))
                    (if (<= n 1) (point-set! (- p 1)) (loop (- n 1) (+ p 1)))
                    (loop n (+ p 1))))))))
      (evil-execute-motion 'motion--seek-tmp))))

(defcommand (evil-motion-T)
  "Find character backward (after)."
  (let ((ch (last-key-char)))
    (when ch
      (register-motion! 'motion--seek-tmp
        (lambda (count)
          (let ((ls (%line-start-position (%position-line (point-get)))))
            (let loop ((n count) (p (- (point-get) 1)))
              (when (>= p ls)
                (if (char=? (char-at p) ch)
                    (if (<= n 1) (point-set! (+ p 1)) (loop (- n 1) (- p 1)))
                    (loop n (- p 1))))))))
      (evil-execute-motion 'motion--seek-tmp))))

;;;
;;; Operator command wrappers (bound to keys)
;;;

(defcommand (evil-op-delete)
  "Delete operator."
  (evil-enter-operator 'op-delete))

(defcommand (evil-op-yank)
  "Yank operator."
  (evil-enter-operator 'op-yank))

(defcommand (evil-op-change)
  "Change operator."
  (evil-enter-operator 'op-change))

(defcommand (evil-D)
  "Delete to end of line."
  (evil-enter-operator 'op-delete)
  (evil-execute-motion 'motion-$))

(defcommand (evil-C)
  "Change to end of line."
  (evil-enter-operator 'op-change)
  (evil-execute-motion 'motion-$))

(defcommand (evil-S)
  "Substitute entire line."
  (evil-enter-operator 'op-change)
  (evil-enter-operator 'op-change))

;;;
;;; Count-aware x/X
;;;

(defcommand (evil-x)
  "Delete character(s) forward."
  (let ((count (or evil-count 1)))
    (%begin-change)
    (let ((len (buffer-length))
          (p (point-get)))
      (let ((actual (min count (- len p))))
        (when (> actual 0)
          (delete-range p (+ p actual)))))
    (%change-set-repeat-info!
      (make-repeat-info 'op-delete count 'motion-l 1 #f #f #f #f))
    (%end-change)
    (set! evil-count #f)
    (message-clear)))

(defcommand (evil-X)
  "Delete character(s) backward."
  (let ((count (or evil-count 1)))
    (%begin-change)
    (let ((p (point-get)))
      (let ((actual (min count p)))
        (when (> actual 0)
          (delete-range (- p actual) p))))
    (%change-set-repeat-info!
      (make-repeat-info 'op-delete count 'motion-h 1 #f #f #f #f))
    (%end-change)
    (set! evil-count #f)
    (message-clear)))

;;;
;;; Visual mode operators
;;;

(define (evil-visual-range)
  (let* ((anchor (%mark-position #\<))
         (point (point-get))
         (sel-min (min anchor point))
         (sel-max (max anchor point))
         (mode (%select-mode-get)))
    (case mode
      ((1) ;; char: inclusive of both ends
       (make-range sel-min (+ sel-max 1) 'char))
      ((2) ;; line: expand to full line boundaries
       (let* ((line-min (%position-line sel-min))
              (line-max (%position-line sel-max))
              (start (%line-start-position line-min))
              (end (%line-end-position line-max))
              (len (buffer-length))
              (end (if (and (< end len) (char=? (char-at end) #\newline))
                       (+ end 1) end)))
         (make-range start end 'line))))))

(define (evil-rect-apply op)
  (let* ((anchor (%mark-position #\<))
         (point (point-get))
         (line-a (%position-line anchor))
         (line-b (%position-line point))
         (row-min (min line-a line-b))
         (row-max (max line-a line-b))
         (col-a (- anchor (%line-start-position line-a)))
         (col-b (- point (%line-start-position line-b)))
         (col-min (min col-a col-b))
         (col-max (+ (max col-a col-b) 1)))
    ;; iterate bottom to top so deletions don't shift lines above
    (let loop ((line row-max))
      (when (>= line row-min)
        (let* ((ls (%line-start-position line))
               (le (%line-end-position line))
               (content-end le)
               (start (+ ls col-min))
               (end (min (+ ls col-max) content-end)))
          (when (and (<= start content-end) (> end start))
            (op start end)))
        (loop (- line 1))))
    ;; cursor to upper-left corner
    (let ((ls (%line-start-position row-min)))
      (point-set! (min (+ ls col-min)
                       (%line-end-position row-min))))))

(defcommand (evil-visual-delete)
  "Delete the visual selection."
  (%begin-change)
  (let ((mode (%select-mode-get)))
    (if (= mode 3)
        (evil-rect-apply delete-range)
        (let* ((shape (if (= mode 2) 'linewise 'charwise))
               (range (evil-visual-range)))
          (let* ((start (range-start range))
                 (end   (range-end range))
                 (text  (%buffer-substring start end)))
            (evil-register-write! text shape)
            (point-set! start)
            (delete-range start end)))))
  (%change-set-repeat-info!
    (make-repeat-info 'op-delete 1 #f #f
                      evil-last-visual-text-object
                      evil-last-visual-text-object-kind #f #f))
  (%end-change)
  (evil-normal))

(defcommand (evil-visual-change)
  "Change the visual selection."
  (%begin-change)
  (set! evil-pending-repeat-info
    (make-repeat-info 'op-change 1 #f #f
                      evil-last-visual-text-object
                      evil-last-visual-text-object-kind #f #f))
  (let ((mode (%select-mode-get)))
    (if (= mode 3)
        ;; rectangle: delete rect, enter insert at upper-left
        (begin (evil-rect-apply delete-range) (evil-insert))
        ;; char or line
        (let ((range (evil-visual-range)))
          (let* ((start (range-start range))
                 (end (range-end range))
                 ;; for line mode: strip trailing newline so we stay on same line
                 (end (if (and (eq? (range-type range) 'line)
                               (> end start)
                               (char=? (char-at (- end 1)) #\newline))
                          (- end 1) end)))
            (point-set! start)
            (delete-range start end)
            (evil-insert))))))

(defcommand (evil-visual-yank)
  "Yank visual selection."
  (let* ((mode (%select-mode-get))
         (anchor (%mark-position #\<))
         (cur-point (point-get))
         (sel-min (min anchor cur-point)))
    (if (= mode 3)
        ;; block mode: collect per-line content within the rectangle
        (let* ((line-a (%position-line anchor))
               (line-b (%position-line cur-point))
               (row-min (min line-a line-b))
               (row-max (max line-a line-b))
               (col-a (- anchor (%line-start-position line-a)))
               (col-b (- cur-point (%line-start-position line-b)))
               (col-min (min col-a col-b))
               (col-max (+ (max col-a col-b) 1))
               (width (- col-max col-min)))
          (let collect ((line row-min) (parts '()))
            (if (> line row-max)
                (let* ((text (string-join-newlines (reverse parts)))
                       (saved-reg current-evil-register))
                  (evil-register-write! text 'blockwise)
                  (let ((actual (if (char-upper-case? saved-reg)
                                    (char-downcase saved-reg)
                                    saved-reg)))
                    (%register-set-block-width! actual width)
                    (when (not (char=? actual #\"))
                      (%register-set-block-width! #\" width))))
                (let* ((ls (%line-start-position line))
                       (le (%line-end-position line))
                       (start (+ ls col-min))
                       (end   (min (+ ls col-max) le))
                       (chunk (if (<= start le)
                                  (%buffer-substring start end)
                                  "")))
                  (collect (+ line 1) (cons chunk parts))))))
        (let ((range (evil-visual-range)))
          (evil-register-write!
            (%buffer-substring (range-start range) (range-end range))
            (if (= mode 2) 'linewise 'charwise))))
    (point-set! sel-min)
    (evil-normal)))

;; Visual mode operator bindings
(set-key! select-map "d" 'evil-visual-delete)
(set-key! select-map "x" 'evil-visual-delete)
(set-key! select-map "c" 'evil-visual-change)
(set-key! select-map "s" 'evil-visual-change)
(set-key! select-map "D" 'evil-visual-delete)
(set-key! select-map "C" 'evil-visual-change)
(set-key! select-map "S" 'evil-visual-change)
(set-key! select-map "y" 'evil-visual-yank)

;; Visual mode motion bindings
(set-key! select-map "w" 'evil-motion-w)
(set-key! select-map "b" 'evil-motion-b)
(set-key! select-map "e" 'evil-motion-e)
(set-key! select-map "W" 'evil-motion-W)
(set-key! select-map "B" 'evil-motion-B)
(set-key! select-map "E" 'evil-motion-E)
(set-key! select-map "g g" 'evil-motion-gg)
(set-key! select-map "G" 'evil-motion-G)

;;;
;;; Compound commands (evil-dependent, moved from built-in.scm)
;;;

(defcommand (open-line-below)
  "Create a new empty line below current line, move to it and enter insert mode."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'open-below #f))
  (line-end) (newline) (evil-insert))
(defcommand (open-line-above)
  "Create a new empty line above current line, move to it and enter insert mode."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'open-above #f))
  (line-start) (newline) (prev-line) (evil-insert))
(defcommand (insert-at-start)
  "Set cursor to first non-blank character on current line and enter insert mode."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'insert-at-start #f))
  (line-start) (skip-whitespace) (evil-insert))
(defcommand (append-char)
  "Enter insert mode after the character currently under the cursor."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'append-char #f))
  (forward-char) (evil-insert))
(defcommand (append-line)
  "Set the cursor to the final column on the current line and enter insert mode."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'append-line #f))
  (line-end) (evil-insert))
(defcommand (substitute-char)
  "Delete the character under cursor and enter insert mode."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'substitute #f))
  (delete-forward-char) (evil-insert))

;;;
;;; Text object commands and bindings
;;;

(defcommand (evil-in-text-object)
  "Inner text object."
  (evil-execute-text-object 'in))

(defcommand (evil-around-text-object)
  "Around text object."
  (evil-execute-text-object 'around))

;; Bind i/a + char in pending and select maps
(for-each (lambda (ch)
  (let ((key (string ch)))
    (set-key! pending-map (string-append "i " key) 'evil-in-text-object)
    (set-key! pending-map (string-append "a " key) 'evil-around-text-object)
    (set-key! select-map (string-append "i " key) 'evil-in-text-object)
    (set-key! select-map (string-append "a " key) 'evil-around-text-object)))
  '(#\w #\W #\s #\p #\[ #\] #\( #\) #\b #\{ #\} #\B #\< #\> #\t #\" #\' #\`))

;;;
;;; Yank and paste commands
;;;

(set-key! normal-map "y"  'evil-op-yank)
(set-key! pending-map "y" 'evil-op-yank)   ; yy = yank line

(defcommand (evil-paste-after)
  "Paste register contents after cursor."
  (let* ((reg current-evil-register)
         (text (if (char=? reg #\+)
                   (let ((s (%clipboard-get)))
                     (if (string=? s "") #f s))
                   (%register-get reg)))
         (shape (if (char=? reg #\+) 'charwise (%register-shape reg))))
    (when text
      (let ((shape shape))
        (cond
          ((eq? shape 'blockwise)
           (let* ((cur-line (%position-line (point-get)))
                  (cur-ls   (%line-start-position cur-line))
                  (col      (+ (- (point-get) cur-ls) 1))
                  (lines    (string-split-newlines text)))
             (%begin-change)
             (evil-paste-block lines col)
             (point-set! (+ (%line-start-position cur-line) col))
             (%end-change)))
          ((eq? shape 'linewise)
           (let ((stripped (strip-trailing-newline text)))
             (%begin-change)
             (line-end)
             (%insert-string "\n")
             (let ((paste-start (point-get)))
               (%insert-string stripped)
               (point-set! paste-start))
             (%change-set-repeat-info!
               (make-repeat-info #f 1 #f #f #f #f 'paste-after-linewise stripped))
             (%end-change)))
          (else
           (%begin-change)
           (forward-char)
           (%insert-string text)
           (%change-set-repeat-info!
             (make-repeat-info #f 1 #f #f #f #f 'paste-after text))
           (%end-change))))))
  (set! current-evil-register #\"))

(defcommand (evil-paste-before)
  "Paste register contents before cursor."
  (let* ((reg current-evil-register)
         (text (if (char=? reg #\+)
                   (let ((s (%clipboard-get)))
                     (if (string=? s "") #f s))
                   (%register-get reg)))
         (shape (if (char=? reg #\+) 'charwise (%register-shape reg))))
    (when text
      (let ((shape shape))
        (cond
          ((eq? shape 'blockwise)
           (let* ((cur-line (%position-line (point-get)))
                  (cur-ls   (%line-start-position cur-line))
                  (col      (- (point-get) cur-ls))
                  (lines    (string-split-newlines text)))
             (%begin-change)
             (evil-paste-block lines col)
             (point-set! (+ (%line-start-position cur-line) col))
             (%end-change)))
          ((eq? shape 'linewise)
           (let ((stripped (strip-trailing-newline text)))
             (%begin-change)
             (line-start)
             (let ((paste-start (point-get)))
               (%insert-string stripped)
               (%insert-string "\n")
               (point-set! paste-start))
             (%change-set-repeat-info!
               (make-repeat-info #f 1 #f #f #f #f 'paste-before-linewise text))
             (%end-change)))
          (else
           (%begin-change)
           (%insert-string text)
           (%change-set-repeat-info!
             (make-repeat-info #f 1 #f #f #f #f 'paste-before text))
           (%end-change))))))
  (set! current-evil-register #\"))

(set-key! normal-map "p" 'evil-paste-after)
(set-key! normal-map "P" 'evil-paste-before)

(defcommand (evil-insert-paste-clipboard)
  "Paste from system clipboard."
  (let ((text (%clipboard-get)))
    (when (not (string=? text ""))
      (%insert-string text))))

;;;
;;; Vim Macros
;;;

;; Recording-mode keymap: only intercepts 'q' to stop recording.
;; No parent — everything else falls through the minor-mode lookup chain.
(define evil-recording-map (make-keymap))
(set-key! evil-recording-map "q" 'evil-stop-macro)
(define-minor-mode 'evil-recording-mode evil-recording-map)

(defcommand (evil-start-macro)
  "Start recording macro to register."
  (let ((ch (last-key-char)))
    (when ch
      (%macro-start! ch)
      (enable-minor-mode 'evil-recording-mode)
      (message (string-append "recording @" (string ch)))
      (message-lock))))

(defcommand (evil-stop-macro)
  "Stop macro recording."
  (%macro-stop!)
  (disable-minor-mode 'evil-recording-mode)
  (message-unlock)
  (message-clear))

(define current-evil-macro #\a)   ; last-played register for @@

(defcommand (evil-play-macro)
  "Play macro from register."
  (let ((ch (last-key-char))
        (count (or evil-count 1)))
    (when ch
      (set! current-evil-macro ch)
      (set! evil-count #f)
      (let loop ((n count))
        (when (> n 0)
          (%macro-play ch)
          (loop (- n 1)))))))

(defcommand (evil-play-last-macro)
  "Replay last macro (@@ in Vim)."
  (let ((count (or evil-count 1)))
    (set! evil-count #f)
    (let loop ((n count))
      (when (> n 0)
        (%macro-play current-evil-macro)
        (loop (- n 1))))))

;; q + a-z → start macro recording
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "q " ch) 'evil-start-macro)))

;; @ + a-z → play macro; @@ → replay last
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "@ " ch) 'evil-play-macro)))
(set-key! normal-map "@ @" 'evil-play-last-macro)

;;; Activate
(evil-mode)
(evil-normal)
