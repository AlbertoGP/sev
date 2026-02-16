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
  (message-clear))

(defcommand (evil-insert)
  "Enter insert mode."
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-insert-mode)
  (%set-replace-mode! #f)
  (set-local! 'mode-name "Insert")
  (message "-- INSERT --"))

(defcommand (evil-replace)
  "Enter replace mode."
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-replace-mode)
  (%set-replace-mode! #t)
  (set-local! 'mode-name "Replace")
  (message "-- REPLACE --"))

(define (enter-visual-submode mode-int name msg)
  (unless (%buffer-has-minor-mode? 'evil-select-mode)
    (%mark-set-to-point! #\<))
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-select-mode)
  (%select-mode-set! mode-int)
  (set-local! 'mode-name name)
  (message msg))

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

;; Repeat state
(define evil-last-op #f)
(define evil-last-motion #f)
(define evil-last-op-count #f)
(define evil-last-motion-count #f)
(define evil-last-text-object #f)
(define evil-last-text-object-kind #f)

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
  (set-local! 'mode-name "Normal"))

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
          (let* ((saved-point (point-get))
                 (eff-count (evil-effective-count))
                 (_ (proc eff-count))
                 (new-point (point-get))
                 (start (min saved-point new-point))
                 (end (max saved-point new-point))
                 (range (make-range start end 'char))
                 (op-proc (operator-ref evil-pending-op)))
            ;; Record for repeat
            (set! evil-last-op evil-pending-op)
            (set! evil-last-motion motion-sym)
            (set! evil-last-text-object #f)
            (set! evil-last-text-object-kind #f)
            (set! evil-last-op-count evil-op-count)
            (set! evil-last-motion-count evil-count)
            ;; Reset state first, then apply operator
            ;; (operator may change mode, e.g. op-change enters insert)
            (point-set! saved-point)
            (evil-reset-count)
            (when op-proc (op-proc range)))
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
              ;; Operator-pending: record for repeat, apply operator
              (let ((op-proc (operator-ref evil-pending-op)))
                (set! evil-last-op evil-pending-op)
                (set! evil-last-text-object sym)
                (set! evil-last-text-object-kind kind)
                (set! evil-last-motion #f)
                (set! evil-last-op-count evil-op-count)
                (set! evil-last-motion-count evil-count)
                (evil-reset-count)
                (when op-proc (op-proc range)))
              ;; Visual mode: set selection
              (when (%buffer-has-minor-mode? 'evil-select-mode)
                (point-set! (range-start range))
                (%mark-set-to-point! #\<)
                (point-set! (- (range-end range) 1))
                (set! evil-count #f)
                (message-clear))))))))

;; Dot repeat
(defcommand (evil-repeat)
  "Repeat last operator+motion."
  (when evil-last-op
    (let ((op-proc (operator-ref evil-last-op)))
      (when op-proc
        (let ((eff-count (* (or evil-last-op-count 1)
                            (or evil-count evil-last-motion-count 1))))
          (if evil-last-text-object
              ;; Text object repeat
              (let* ((to-proc (text-object-ref evil-last-text-object))
                     (range (and to-proc (to-proc evil-last-text-object-kind eff-count))))
                (when range (op-proc range)))
              ;; Motion repeat
              (let ((motion-proc (motion-ref evil-last-motion)))
                (when motion-proc
                  (when (eq? evil-last-motion 'current-line) (line-start))
                  (let* ((saved-point (point-get))
                         (_ (motion-proc eff-count))
                         (new-point (point-get))
                         (start (min saved-point new-point))
                         (end (max saved-point new-point))
                         (range (make-range start end 'char)))
                    (point-set! saved-point)
                    (op-proc range)))))))))
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
;;; Operator definitions
;;;

(register-operator! 'op-delete
  (lambda (range)
    (delete-range (range-start range) (range-end range))))

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
         (let ((c (char-at pos)))
           (cond
             ((char=? c open) (find-close-from pos))
             ((char=? c close) (find-open-from (- pos 1)))
             (else (find-open-from pos)))))))

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
                                        (find-close (+ q 1) d))))))
                       (find-open (- p 1) (- depth 1))))))
              (find-open (- p 1) depth))))))

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
                    (max-line (- (%line-count) 1)))
               ;; Find start: search backward for blank line boundary
               (let ((start-line (let bwd ((l cur-line))
                                   (if (or (<= l 0) (blank-line? l))
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
    (let ((len (buffer-length))
          (p (point-get)))
      (let ((actual (min count (- len p))))
        (when (> actual 0)
          (delete-range p (+ p actual)))))
    (set! evil-count #f)
    (message-clear)))

(defcommand (evil-X)
  "Delete character(s) backward."
  (let ((count (or evil-count 1)))
    (let ((p (point-get)))
      (let ((actual (min count p)))
        (when (> actual 0)
          (delete-range (- p actual) p))))
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
  (let ((mode (%select-mode-get)))
    (if (= mode 3)
        ;; rectangle
        (begin (evil-rect-apply delete-range) (evil-normal))
        ;; char or line
        (let ((range (evil-visual-range)))
          (point-set! (range-start range))
          (delete-range (range-start range) (range-end range))
          (evil-normal)))))

(defcommand (evil-visual-change)
  "Change the visual selection."
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

;; Visual mode operator bindings
(set-key! select-map "d" 'evil-visual-delete)
(set-key! select-map "x" 'evil-visual-delete)
(set-key! select-map "c" 'evil-visual-change)
(set-key! select-map "s" 'evil-visual-change)
(set-key! select-map "D" 'evil-visual-delete)
(set-key! select-map "C" 'evil-visual-change)
(set-key! select-map "S" 'evil-visual-change)

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
  (line-end) (newline) (evil-insert))
(defcommand (open-line-above)
  "Create a new empty line above current line, move to it and enter insert mode."
  (line-start) (newline) (prev-line) (evil-insert))
(defcommand (insert-at-start)
  "Set cursor to first non-blank character on current line and enter insert mode."
  (line-start) (skip-whitespace) (evil-insert))
(defcommand (append-char)
  "Enter insert mode after the character currently under the cursor."
  (forward-char) (evil-insert))
(defcommand (append-line)
  "Set the cursor to the final column on the current line and enter insert mode."
  (line-end) (evil-insert))
(defcommand (substitute-char)
  "Delete the character under cursor and enter insert mode."
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

;;; Activate
(evil-mode)
(evil-normal)
