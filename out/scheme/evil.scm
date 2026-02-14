;;; evil.scm - Vim-like modal editing

(defcommand ignore "Do nothing.")

;; Keymaps
(define normal-map (make-keymap))
(define insert-map (make-keymap))
(define replace-map (make-keymap))
(define select-map (make-keymap))
(define command-map (make-keymap))
(define pending-map (make-keymap))

;; Normal mode: ignore unbound keys (no self-insert)
(%set-keymap-default! normal-map 'ignore)

;; Normal mode bindings
(set-key! normal-map "C-q" 'quit)
(set-key! normal-map "C-e" 'eval-buffer)
(set-key! normal-map "h" 'evil-motion-h)
(set-key! normal-map "j" 'evil-motion-j)
(set-key! normal-map "k" 'evil-motion-k)
(set-key! normal-map "l" 'evil-motion-l)
(set-key! normal-map "LEFT" 'backward-char)
(set-key! normal-map "DOWN" 'next-line)
(set-key! normal-map "UP" 'prev-line)
(set-key! normal-map "RIGHT" 'forward-char)
(set-key! normal-map "0" 'evil-zero)
(set-key! normal-map "$" 'evil-motion-$)
(set-key! normal-map "^" 'evil-motion-^)
(set-key! normal-map "w" 'evil-motion-w)
(set-key! normal-map "b" 'evil-motion-b)
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
(set-key! normal-map "C-TAB" 'tab-next)
(set-key! normal-map "C-S-TAB" 'tab-prev)
(set-key! normal-map "C-w" 'pane-close)
(set-key! normal-map "M-0" 'reset-global-scale)
(set-key! normal-map "M-=" 'increase-global-scale)
(set-key! normal-map "M--" 'decrease-global-scale)
(set-key! normal-map "C-0" 'reset-buffer-scale)
(set-key! normal-map "C-=" 'increase-buffer-scale)
(set-key! normal-map "C--" 'decrease-buffer-scale)
(set-key! normal-map "C-s h" 'split-horizontal)
(set-key! normal-map "C-s v" 'split-vertical)
(set-key! normal-map "C-UP" 'pane-navigate-up)
(set-key! normal-map "C-DOWN" 'pane-navigate-down)
(set-key! normal-map "C-LEFT" 'pane-navigate-left)
(set-key! normal-map "C-RIGHT" 'pane-navigate-right)
(set-key! normal-map "C-S-UP" 'pane-h-split-decrease)
(set-key! normal-map "C-S-DOWN" 'pane-h-split-increase)
(set-key! normal-map "C-S-LEFT" 'pane-v-split-decrease)
(set-key! normal-map "C-S-RIGHT" 'pane-v-split-increase)
(set-key! normal-map "`" 'clay-debug)

(%set-keymap-default! insert-map 'self-insert)
;; Insert mode bindings (no default - falls through for self-insert)
(set-key! insert-map "ESC" 'evil-normal)
(set-key! insert-map "C-[" 'evil-normal)
(set-key! insert-map "C-q" 'quit)
(set-key! insert-map "LEFT" 'backward-char)
(set-key! insert-map "M-h" 'backward-char)
(set-key! insert-map "DOWN" 'next-line)
(set-key! insert-map "M-j" 'next-line)
(set-key! insert-map "UP" 'prev-line)
(set-key! insert-map "M-k" 'prev-line)
(set-key! insert-map "RIGHT" 'forward-char)
(set-key! insert-map "M-l" 'forward-char)
(set-key! insert-map "C-e" 'eval-buffer)
(set-key! insert-map "RET" 'newline)
(set-key! insert-map "TAB" 'insert-tab)
(set-key! insert-map "BSP" 'delete-backward-char)
(set-key! insert-map "C-h" 'delete-backward-char)
(set-key! insert-map "DEL" 'delete-forward-char)
(set-key! insert-map "C-TAB" 'tab-next)
(set-key! insert-map "C-S-TAB" 'tab-prev)
(set-key! insert-map "C-w" 'pane-close)
(set-key! insert-map "M-0" 'reset-global-scale)
(set-key! insert-map "M-=" 'increase-global-scale)
(set-key! insert-map "M--" 'decrease-global-scale)
(set-key! insert-map "C-0" 'reset-buffer-scale)
(set-key! insert-map "C-=" 'increase-buffer-scale)
(set-key! insert-map "C--" 'decrease-buffer-scale)
(set-key! insert-map "C-s h" 'split-horizontal)
(set-key! insert-map "C-s v" 'split-vertical)
(set-key! insert-map "C-UP" 'pane-navigate-up)
(set-key! insert-map "C-DOWN" 'pane-navigate-down)
(set-key! insert-map "C-LEFT" 'pane-navigate-left)
(set-key! insert-map "C-RIGHT" 'pane-navigate-right)
(set-key! insert-map "C-S-UP" 'pane-h-split-decrease)
(set-key! insert-map "C-S-DOWN" 'pane-h-split-increase)
(set-key! insert-map "C-S-LEFT" 'pane-v-split-decrease)
(set-key! insert-map "C-S-RIGHT" 'pane-v-split-increase)

(set-key! replace-map "ESC" 'evil-normal)
(set-key! replace-map "C-[" 'evil-normal)

;; Select (visual) mode bindings
(%set-keymap-default! select-map 'ignore)
(set-key! select-map "ESC" 'evil-normal)
(set-key! select-map "C-[" 'evil-normal)
(set-key! select-map "v"   'evil-select)
(set-key! select-map "V"   'evil-select-line)
(set-key! select-map "C-v" 'evil-select-rectangle)
(set-key! select-map "h"   'backward-char)
(set-key! select-map "j"   'next-line)
(set-key! select-map "k"   'prev-line)
(set-key! select-map "l"   'forward-char)
(set-key! select-map "LEFT"  'backward-char)
(set-key! select-map "DOWN"  'next-line)
(set-key! select-map "UP"    'prev-line)
(set-key! select-map "RIGHT" 'forward-char)
(set-key! select-map "0"   'line-start)
(set-key! select-map "$"   'line-end)
(set-key! select-map "^"   'line-start-skip-whitespace)
(set-key! select-map "o"   'exchange-point-and-mark)

(set-key! command-map "ESC" 'evil-normal)
(set-key! command-map "C-[" 'evil-normal)

;; Operator-pending mode: only motions, counts, cancel, and doubled ops
(%set-keymap-default! pending-map 'ignore)
(set-key! pending-map "h" 'evil-motion-h)
(set-key! pending-map "j" 'evil-motion-j)
(set-key! pending-map "k" 'evil-motion-k)
(set-key! pending-map "l" 'evil-motion-l)
(set-key! pending-map "w" 'evil-motion-w)
(set-key! pending-map "b" 'evil-motion-b)
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
(set-key! pending-map "d" 'evil-op-delete)
(set-key! pending-map "c" 'evil-op-change)
(set-key! pending-map "ESC" 'evil-normal)
(set-key! pending-map "C-[" 'evil-normal)

;; Register modes
(define-minor-mode 'evil-normal-mode normal-map)
(define-minor-mode 'evil-insert-mode insert-map)
(define-minor-mode 'evil-replace-mode insert-map)
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
(define (evil-normal)
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

(define (evil-insert)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (disable-minor-mode 'evil-command-mode)
  (enable-minor-mode 'evil-insert-mode)
  (%set-replace-mode! #f)
  (set-local! 'mode-name "Insert")
  (message "-- INSERT --"))

(define (evil-replace)
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

(define (evil-select)
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 1))
      (evil-normal)
      (enter-visual-submode 1 "Select" "-- SELECT --")))

(define (evil-select-line)
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 2))
      (evil-normal)
      (enter-visual-submode 2 "Line" "-- SELECT LINE --")))

(define (evil-select-rectangle)
  (if (and (%buffer-has-minor-mode? 'evil-select-mode) (= (%select-mode-get) 3))
      (evil-normal)
      (enter-visual-submode 3 "Rectangle" "-- SELECT RECTANGLE --")))

(define (evil-command)
  (disable-minor-mode 'evil-normal-mode)
  (disable-minor-mode 'evil-insert-mode)
  (disable-minor-mode 'evil-replace-mode)
  (disable-minor-mode 'evil-select-mode)
  (enable-minor-mode 'evil-command-mode)
  (set-local! 'mode-name "Command")
  (message-clear))

(defcommand evil-normal "Return to normal mode.")
(defcommand evil-insert "Enter insert mode.")
(defcommand evil-replace "Enter replace mode.")
(defcommand evil-select "Enter select mode.")
(defcommand evil-select-line "Enter visual line mode.")
(defcommand evil-select-rectangle "Enter visual block mode.")
(defcommand evil-command "Enter command mode.")

;; Query function for UI
(define (evil-state)
  (cond
    ((%buffer-has-minor-mode? 'evil-normal-mode) 'normal)
    ((%buffer-has-minor-mode? 'evil-insert-mode) 'insert)
    (else #f)))

;; Enable evil mode
(define (evil-mode)
  (enable-minor-mode 'evil-normal-mode))

(defcommand evil-mode "Enable vim-like modal editing.")

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

;; Dot repeat
(define (evil-repeat)
  (when evil-last-op
    (let ((motion-proc (motion-ref evil-last-motion))
          (op-proc (operator-ref evil-last-op)))
      (when (and motion-proc op-proc)
        ;; Allow current count to override motion count
        (let ((eff-count (* (or evil-last-op-count 1)
                            (or evil-count evil-last-motion-count 1))))
          (when (eq? evil-last-motion 'current-line) (line-start))
          (let* ((saved-point (point-get))
                 (_ (motion-proc eff-count))
                 (new-point (point-get))
                 (start (min saved-point new-point))
                 (end (max saved-point new-point))
                 (range (make-range start end 'char)))
            (point-set! saved-point)
            (op-proc range))))))
  (set! evil-count #f)
  (message-clear))

(defcommand evil-repeat "Repeat last operator+motion.")

;; Count accumulation
(define (evil-digit-argument)
  (let ((ch (last-key-char)))
    (when (and ch (not (eq? ch #f)))
      (let ((digit (- (char->integer ch) (char->integer #\0))))
        (set! evil-count (+ (* (or evil-count 0) 10) digit))
        (message (string-append (number->string evil-count) "-"))))))

(defcommand evil-digit-argument "Accumulate count prefix.")

;; 0 key: line-start or append zero to count
(define (evil-zero)
  (if evil-count
      (begin
        (set! evil-count (* evil-count 10))
        (message (string-append (number->string evil-count) "-")))
      (evil-execute-motion 'motion-0)))

(defcommand evil-zero "Line start or append zero to count.")

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
    (delete-range (range-start range) (range-end range))
    (evil-insert)))

;;;
;;; Motion command wrappers (bound to keys)
;;;

(define (evil-motion-h) (evil-execute-motion 'motion-h))
(define (evil-motion-j) (evil-execute-motion 'motion-j))
(define (evil-motion-k) (evil-execute-motion 'motion-k))
(define (evil-motion-l) (evil-execute-motion 'motion-l))
(define (evil-motion-$) (evil-execute-motion 'motion-$))
(define (evil-motion-^) (evil-execute-motion 'motion-^))
(define (evil-motion-w) (evil-execute-motion 'motion-w))
(define (evil-motion-b) (evil-execute-motion 'motion-b))

(defcommand evil-motion-h "Move left.")
(defcommand evil-motion-j "Move down.")
(defcommand evil-motion-k "Move up.")
(defcommand evil-motion-l "Move right.")
(defcommand evil-motion-$ "Move to end of line.")
(defcommand evil-motion-^ "Move to first non-blank.")
(defcommand evil-motion-w "Move forward one word.")
(defcommand evil-motion-b "Move backward one word.")

;;;
;;; Operator command wrappers (bound to keys)
;;;

(define (evil-op-delete) (evil-enter-operator 'op-delete))
(define (evil-op-change) (evil-enter-operator 'op-change))

(defcommand evil-op-delete "Delete operator.")
(defcommand evil-op-change "Change operator.")

(define (evil-D)
  (evil-enter-operator 'op-delete)
  (evil-execute-motion 'motion-$))
(defcommand evil-D "Delete to end of line.")

(define (evil-S)
  (evil-enter-operator 'op-change)
  (evil-enter-operator 'op-change)
  (open-line-above))
(defcommand evil-S "Substitute entire line.")

;;;
;;; Count-aware x/X
;;;

(define (evil-x)
  (let ((count (or evil-count 1)))
    (let ((len (buffer-length))
          (p (point-get)))
      (let ((actual (min count (- len p))))
        (when (> actual 0)
          (delete-range p (+ p actual)))))
    (set! evil-count #f)
    (message-clear)))

(define (evil-X)
  (let ((count (or evil-count 1)))
    (let ((p (point-get)))
      (let ((actual (min count p)))
        (when (> actual 0)
          (delete-range (- p actual) p))))
    (set! evil-count #f)
    (message-clear)))

(defcommand evil-x "Delete character(s) forward.")
(defcommand evil-X "Delete character(s) backward.")

;;; Activate
(evil-mode)
(evil-normal)
