;;; text-object.scm - Text object helpers, implementations, and bindings

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
