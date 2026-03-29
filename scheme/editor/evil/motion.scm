;;; motion.scm - Motion registrations and command wrappers

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
;;; Motion command wrappers (bound to keys)
;;;

(defcommand (evil-motion-h) "Move left."  (unless (no-panes?) (evil-execute-motion 'motion-h)))
(defcommand (evil-motion-j) "Move down." (unless (no-panes?) (evil-execute-motion 'motion-j)))
(defcommand (evil-motion-k) "Move up."   (unless (no-panes?) (evil-execute-motion 'motion-k)))
(defcommand (evil-motion-l) "Move right." (unless (no-panes?) (evil-execute-motion 'motion-l)))
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
  (when (eq? evil-sm-state 'normal) (%jump-push!))
  (evil-execute-motion 'motion-goto-line))

(defcommand (evil-motion-G)
  "Go to last line."
  (when (eq? evil-sm-state 'normal) (%jump-push!))
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
      (when (eq? evil-sm-state 'normal) (%jump-push!))
      (register-motion! 'motion--mark-line-tmp
        (lambda (count) (%point-to-mark! ch) (line-start)))
      (evil-execute-motion 'motion--mark-line-tmp))))

(defcommand (evil-goto-mark-exact)
  "Jump to exact position of mark."
  (let ((ch (last-key-char)))
    (when ch
      (when (eq? evil-sm-state 'normal) (%jump-push!))
      (register-motion! 'motion--mark-exact-tmp
        (lambda (count) (%point-to-mark! ch)))
      (evil-execute-motion 'motion--mark-exact-tmp))))

(defcommand (evil-jump-backward)
  "Jump to older entry in jump list."
  (%jump-backward!))

(defcommand (evil-jump-forward)
  "Jump to newer entry in jump list."
  (%jump-forward!))

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
