;;; undo.scm - Undo, redo, dot repeat

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
             (cond
               ((eq? (ri-setup ri) 'char-replace)
                ;; r-replace: delete count chars and overwrite with single char
                (let ((ch (string-ref (ri-insert-text ri) 0))
                      (count (or (ri-op-count ri) 1)))
                  (let* ((p (point-get))
                         (line (%position-line p))
                         (line-end (%line-end-position line))
                         (avail (- line-end p)))
                    (when (>= avail count)
                      (let loop ((i 0))
                        (when (< i count)
                          (delete-forward-char)
                          (insert-char ch)
                          (loop (+ i 1))))
                      (point-set! (- (point-get) 1))))))
               (else
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
                    (evil-insert-text (ri-insert-text ri)))))
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
        (message-echo (string-append (or evil-op-echo "") (number->string evil-count) "-"))))))

;; 0 key: line-start or append zero to count
(defcommand (evil-zero)
  "Line start or append zero to count."
  (if evil-count
      (begin
        (set! evil-count (* evil-count 10))
        (message-echo (string-append (or evil-op-echo "") (number->string evil-count) "-")))
      (evil-execute-motion 'motion-0)))
