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
                ((vim-word-char? c)
                 (let skip-word ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-word-char? (char-at pp)))
                       (move-point 1) (skip-word))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On punctuation: skip punctuation, then skip whitespace
                ((vim-punctuation? c)
                 (let skip-punct ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-punctuation? (char-at pp)))
                       (move-point 1) (skip-punct))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On whitespace: skip whitespace
                ((vim-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-whitespace? (char-at pp)))
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
                (when (and (> pp 0) (vim-whitespace? (char-at pp)))
                  (move-point -1) (skip-ws))))
            ;; Now skip same-class chars backwards
            (let ((pp (point-get)))
              (let ((c (char-at pp)))
                (cond
                  ((vim-word-char? c)
                   (let skip-word ()
                     (let ((pp2 (point-get)))
                       (when (and (> pp2 0) (vim-word-char? (char-at (- pp2 1))))
                         (move-point -1) (skip-word)))))
                  ((vim-punctuation? c)
                   (let skip-punct ()
                     (let ((pp2 (point-get)))
                       (when (and (> pp2 0) (vim-punctuation? (char-at (- pp2 1))))
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
                ((vim-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 ;; Now on a non-whitespace char, advance to end of its class
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((vim-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((vim-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ;; At end of word-class run: move forward, skip ws, find end of next run
                ((and (vim-word-char? c)
                      (or (vim-whitespace? nc) (vim-punctuation? nc)))
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((vim-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((vim-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ((and (vim-punctuation? c)
                      (or (vim-whitespace? nc) (vim-word-char? nc)))
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let ((pp (point-get)))
                   (when (< pp (- len 1))
                     (let ((cc (char-at pp)))
                       (cond
                         ((vim-word-char? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-word-char? (char-at (+ pp2 1))))
                                (move-point 1) (advance)))))
                         ((vim-punctuation? cc)
                          (let advance ()
                            (let ((pp2 (point-get)))
                              (when (and (< pp2 (- len 1)) (vim-punctuation? (char-at (+ pp2 1))))
                                (move-point 1) (advance))))))))))
                ;; In middle of word-class run: advance to end
                ((vim-word-char? c)
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-word-char? (char-at (+ pp 1))))
                       (move-point 1) (advance)))))
                ((vim-punctuation? c)
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-punctuation? (char-at (+ pp 1))))
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
                ((not (vim-whitespace? c))
                 (let skip-nonws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (not (vim-whitespace? (char-at pp))))
                       (move-point 1) (skip-nonws))))
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws)))))
                ;; On whitespace: skip ws
                ((vim-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp len) (vim-whitespace? (char-at pp)))
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
                (when (and (> pp 0) (vim-whitespace? (char-at pp)))
                  (move-point -1) (skip-ws))))
            ;; Skip non-whitespace backwards
            (let ((pp (point-get)))
              (when (and (> pp 0) (not (vim-whitespace? (char-at pp))))
                (let skip-nonws ()
                  (let ((pp2 (point-get)))
                    (when (and (> pp2 0) (not (vim-whitespace? (char-at (- pp2 1)))))
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
                ((vim-whitespace? c)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (vim-whitespace? (char-at (+ pp 1)))))
                       (move-point 1) (advance)))))
                ;; At end of non-ws run (next is whitespace): move, skip ws, find end
                ((vim-whitespace? nc)
                 (move-point 1)
                 (let skip-ws ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (vim-whitespace? (char-at pp)))
                       (move-point 1) (skip-ws))))
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (vim-whitespace? (char-at (+ pp 1)))))
                       (move-point 1) (advance)))))
                ;; In middle of non-ws run: advance to end
                (else
                 (let advance ()
                   (let ((pp (point-get)))
                     (when (and (< pp (- len 1)) (not (vim-whitespace? (char-at (+ pp 1)))))
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

(defcommand (vim-motion-h) "vim: move left"  (unless (no-panes?) (vim-execute-motion 'motion-h)))
(defcommand (vim-motion-j) "vim: move down" (unless (no-panes?) (vim-execute-motion 'motion-j)))
(defcommand (vim-motion-k) "vim: move up"   (unless (no-panes?) (vim-execute-motion 'motion-k)))
(defcommand (vim-motion-l) "vim: move right" (unless (no-panes?) (vim-execute-motion 'motion-l)))
(defcommand (vim-motion-$) "vim: move to end of line" (vim-execute-motion 'motion-$))
(defcommand (vim-motion-^) "vim: move to first non-blank" (vim-execute-motion 'motion-^))
(defcommand (vim-motion-w) "vim: move forward one word" (vim-execute-motion 'motion-w))
(defcommand (vim-motion-b) "vim: move backward one word" (vim-execute-motion 'motion-b))
(defcommand (vim-motion-e) "vim: move to end of word" (vim-execute-motion 'motion-e))
(defcommand (vim-motion-W) "vim: move forward one WORD" (vim-execute-motion 'motion-W))
(defcommand (vim-motion-B) "vim: move backward one WORD" (vim-execute-motion 'motion-B))
(defcommand (vim-motion-E) "vim: move to end of WORD" (vim-execute-motion 'motion-E))

;; Bracket matching motion
(register-motion! 'motion-%
  (lambda (count) (jump-to-matching-bracket)))

(defcommand (vim-motion-%) "vim: jump to matching bracket" (vim-execute-motion 'motion-%))

(defcommand (vim-motion-gg)
  "vim: jump to first line"
  (when (eq? vim-sm-state 'normal) (%jump-push!))
  (vim-execute-motion 'motion-goto-line))

(defcommand (vim-motion-G)
  "vim: jump to last line"
  (when (eq? vim-sm-state 'normal) (%jump-push!))
  (unless vim-count (set! vim-count (%line-count)))
  (vim-execute-motion 'motion-goto-line))

(defcommand (vim-set-mark)
  "vim: set mark"
  (let ((ch (last-key-char)))
    (when ch
      (%mark-set-to-point! ch)
      (message (string-append "Mark set: " (string ch))))))


(defcommand (vim-goto-mark-line)
  "vim: jump to mark line\nJump to line of mark."
  (let ((ch (last-key-char)))
    (when ch
      (when (eq? vim-sm-state 'normal) (%jump-push!))
      (register-motion! 'motion--mark-line-tmp
        (lambda (count) (%point-to-mark! ch) (line-start)))
      (vim-execute-motion 'motion--mark-line-tmp))))

(defcommand (vim-goto-mark-exact)
  "vim: jump to mark\nJump to exact position of mark."
  (let ((ch (last-key-char)))
    (when ch
      (when (eq? vim-sm-state 'normal) (%jump-push!))
      (register-motion! 'motion--mark-exact-tmp
        (lambda (count) (%point-to-mark! ch)))
      (vim-execute-motion 'motion--mark-exact-tmp))))

(defcommand (vim-jump-backward)
  "vim: jump backward\nJump to older entry in jump list."
  (%jump-backward!))

(defcommand (vim-jump-forward)
  "vim: jump forward\nJump to newer entry in jump list."
  (%jump-forward!))

;; f/F/t/T — character seeking (current line only)

(defcommand (vim-motion-f)
  "vim: find character forward (on)"
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
      (vim-execute-motion 'motion--seek-tmp))))

(defcommand (vim-motion-F)
  "vim: find character backward (on)"
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
      (vim-execute-motion 'motion--seek-tmp))))

(defcommand (vim-motion-t)
  "vim: find character forward (before)"
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
      (vim-execute-motion 'motion--seek-tmp))))

(defcommand (vim-motion-T)
  "vim: find character backward (after)"
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
      (vim-execute-motion 'motion--seek-tmp))))
