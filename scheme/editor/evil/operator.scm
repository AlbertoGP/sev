;;; operator.scm - Register helpers, operators, editing commands, and paste

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
           (shape (if (eq? (range-type range) 'line) 'linewise 'charwise))
           ;; Linewise delete on the last line (no trailing newline): include
           ;; the preceding newline so the empty line doesn't remain.
           (start (if (and (eq? (range-type range) 'line)
                           (= end (buffer-length))
                           (> start 0)
                           (char=? (char-at (- start 1)) #\newline))
                      (- start 1)
                      start)))
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
;;; Operator command wrappers (bound to keys)
;;;

(define (evil-op-delete)
  (evil-enter-operator 'op-delete))

(define (evil-op-yank)
  (evil-enter-operator 'op-yank))

(define (evil-op-change)
  (evil-enter-operator 'op-change))

(defcommand (evil-D)
  "vim: delete to end of line or delete lines\nIn normal mode, delete from cursor to end of line.\nIn visual mode, delete all lines spanned by the selection."
  (unless (buffer-read-only?)
    (if (> (%select-mode-get) 0)
        (evil-visual-force-linewise-delete)
        (begin
          (evil-enter-operator 'op-delete)
          (evil-execute-motion 'motion-$)))))

(defcommand (evil-C)
  "vim: change to end of line"
  (unless (buffer-read-only?)
    (evil-enter-operator 'op-change)
    (evil-execute-motion 'motion-$)))

(defcommand (evil-S)
  "vim: substitute entire line"
  (unless (buffer-read-only?)
    (evil-enter-operator 'op-change)
    (evil-enter-operator 'op-change)))

;;;
;;; Count-aware x/X
;;;

(define (evil-x-impl)
  (unless (buffer-read-only?)
  (let ((count (or evil-count 1)))
    (%begin-change)
    (let ((len (buffer-length))
          (p (point-get)))
      (let ((actual (min count (- len p))))
        (when (> actual 0)
          (let loop ((i 0))
            (when (and (< i actual) (< (point-get) (buffer-length)))
              (let ((p (point-get)))
                (delete-range p (+ p (utf8-byte-len-at p))))
              (loop (+ i 1)))))))
    (%change-set-repeat-info!
      (make-repeat-info 'op-delete count 'motion-l 1 #f #f #f #f))
    (%end-change)
    (set! evil-count #f)
    (message-clear))))

(defcommand (evil-X)
  "vim: backspace character or delete lines\nIn normal mode, delete character(s) backward.\nIn visual mode, delete all lines spanned by the selection."
  (unless (buffer-read-only?)
    (if (> (%select-mode-get) 0)
        (evil-visual-force-linewise-delete)
        (let ((count (or evil-count 1)))
          (%begin-change)
          (let ((p (point-get)))
            (let ((actual (min count p)))
              (when (> actual 0)
                (let loop ((i 0))
                  (when (< i actual)
                    (delete-backward-char)
                    (loop (+ i 1)))))))
          (%change-set-repeat-info!
            (make-repeat-info 'op-delete count 'motion-h 1 #f #f #f #f))
          (%end-change)
          (set! evil-count #f)
          (message-clear)))))

;;;
;;; r — character replace
;;;

(defcommand (evil-char-replace-setup)
  "vim: replace character or selection\nEnter char-replace pending state (show UNDER cursor until char typed)."
  (unless (buffer-read-only?)
    (%set-cursor-override! 'under)
    (message-echo (if evil-count (string-append (number->string evil-count) "r-") "r-"))
    (%read-key-binding
      (lambda (sym key-str)
        (%set-cursor-override! #f)
        (when (last-key-char)
          (if (> (%select-mode-get) 0)
              (evil-visual-char-replace)
              (evil-char-replace)))))))

(define (evil-char-replace)
  (let ((ch (last-key-char))
        (count (or evil-count 1)))
    (when ch
      (let* ((p (point-get))
             (line (%position-line p))
             (line-end (%line-end-position line))
             (avail (- line-end p)))
        (when (>= avail count)
          (%begin-change)
          (let loop ((i 0))
            (when (< i count)
              (let ((p (point-get)))
                (delete-range p (+ p (utf8-byte-len-at p))))
              (insert-char ch)
              (loop (+ i 1))))
          (point-set! (- (point-get) 1))
          (%change-set-repeat-info!
            (make-repeat-info #f count #f #f #f #f 'char-replace (string ch)))
          (%end-change))))
    (set! evil-count #f)
    (message-clear)))

(define (evil-visual-char-replace)
  (let ((ch (last-key-char))
        (mode (%select-mode-get)))
    (when ch
      (%begin-change)
      (if (= mode 3)
          ;; Rectangle: replace each cell in the block
          (let* ((anchor (%mark-position #\<))
                 (line-a (%position-line anchor))
                 (line-b (%position-line (point-get)))
                 (row-min (min line-a line-b))
                 (row-max (max line-a line-b))
                 (col-a (- anchor (%line-start-position line-a)))
                 (col-b (- (point-get) (%line-start-position line-b)))
                 (col-min (min col-a col-b))
                 (col-max (+ (max col-a col-b) 1))
                 (width (- col-max col-min)))
            (let row-loop ((line row-min))
              (when (<= line row-max)
                (let* ((ls (%line-start-position line))
                       (le (%line-end-position line))
                       (start (+ ls col-min))
                       (avail (- (min (+ ls col-max) le) start)))
                  (when (> avail 0)
                    (point-set! start)
                    (let col-loop ((n avail))
                      (when (> n 0)
                        (delete-forward-char)
                        (insert-char ch)
                        (col-loop (- n 1))))))
                (row-loop (+ line 1))))
            ;; cursor to upper-left of rectangle
            (point-set! (min (+ (%line-start-position row-min) col-min)
                             (%line-end-position row-min))))
          ;; Char / line: walk the flat range, skip newlines
          (begin
            (let* ((range (evil-visual-range))
                   (start (range-start range))
                   (end (range-end range))
                   (n (count-chars-in-range start end)))
              (point-set! start)
              (let loop ((i 0))
                (when (< i n)
                  (let ((c (char-at-point)))
                    (if (char=? c #\newline)
                        (begin (point-set! (+ (point-get) 1)) (loop (+ i 1)))
                        (let ((p (point-get)))
                          (delete-range p (+ p (utf8-byte-len-at p)))
                          (insert-char ch)
                          (loop (+ i 1))))))))
            (when (> (point-get) 0)
              (point-set! (- (point-get) 1)))))
      (%end-change)
      (evil-normal))))

;;;
;;; Compound commands (evil-dependent, moved from built-in.scm)
;;;

(defcommand (open-line-below)
  "vim: new line below\nCreate a new empty line below current line, move to it and enter insert mode."
  (unless (buffer-read-only?)
    (%begin-change)
    (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'open-below #f))
    (line-end) (newline) (evil-insert)))
(defcommand (open-line-above)
  "vim: new line above\nCreate a new empty line above current line, move to it and enter insert mode."
  (unless (buffer-read-only?)
    (%begin-change)
    (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'open-above #f))
    (line-start) (newline) (prev-line) (evil-insert)))
(define (insert-at-start)
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'insert-at-start #f))
  (line-start) (skip-whitespace) (evil-insert))
(defcommand (append-char)
  "vim: append character\nEnter insert mode after the character currently under the cursor."
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'append-char #f))
  (forward-char) (evil-insert))
(define (append-line)
  (%begin-change)
  (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'append-line #f))
  (line-end) (evil-insert))
(defcommand (substitute-char)
  "vim: change character\nDelete the character under cursor and enter insert mode."
  (unless (buffer-read-only?)
    (%begin-change)
    (set! evil-pending-repeat-info (make-repeat-info #f 1 #f #f #f #f 'substitute #f))
    (delete-forward-char) (evil-insert)))

;;;
;;; Yank and paste commands
;;;

(set-key! normal-map "y"  'evil-y)
(set-key! pending-map "y" 'evil-op-yank)   ; yy = yank line

(define (evil-paste-after)
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

(define (evil-paste-before)
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

(set-key! normal-map "p" 'evil-p)
(set-key! normal-map "P" 'evil-P)

(defcommand (evil-insert-paste-clipboard)
  "vim: paste from system clipboard"
  (let ((text (%clipboard-get)))
    (when (not (string=? text ""))
      (%insert-string text))))
