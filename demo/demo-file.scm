;;; demo-file.scm — sev editor demo
;;;
;;; This file is pre-loaded into the WASM virtual filesystem and opened
;;; automatically on first launch to demonstrate syntax highlighting.

(import (scheme base)
        (scheme write)
        (scheme case-lambda))

;; ---------------------------------------------------------------------------
;; Fibonacci — naive recursive and tail-recursive variants
;; ---------------------------------------------------------------------------

(define (fib n)
  (cond ((= n 0) 0)
        ((= n 1) 1)
        (else (+ (fib (- n 1))
                 (fib (- n 2))))))

(define (fib/tail n)
  (let loop ((i n) (a 0) (b 1))
    (if (= i 0)
        a
        (loop (- i 1) b (+ a b)))))

;; ---------------------------------------------------------------------------
;; Higher-order utilities
;; ---------------------------------------------------------------------------

(define (compose . procs)
  (if (null? procs)
      (lambda (x) x)
      (let ((f (car procs))
            (rest (apply compose (cdr procs))))
        (lambda (x) (f (rest x))))))

(define (curry f . args)
  (lambda rest
    (apply f (append args rest))))

(define double  (curry * 2))
(define inc     (curry + 1))
(define double-then-inc (compose inc double))

;; ---------------------------------------------------------------------------
;; Association list utilities
;; ---------------------------------------------------------------------------

(define (alist-get key alist default)
  (let ((pair (assoc key alist)))
    (if pair (cdr pair) default)))

(define colors
  '((red   . #xff0000)
    (green . #x00ff00)
    (blue  . #x0000ff)
    (alpha . #xff)))

;; ---------------------------------------------------------------------------
;; String helpers
;; ---------------------------------------------------------------------------

(define (string-repeat s n)
  (let loop ((i n) (acc ""))
    (if (= i 0)
        acc
        (loop (- i 1) (string-append acc s)))))

(define (words str)
  (let loop ((chars (string->list str)) (word '()) (result '()))
    (cond
      ((null? chars)
       (if (null? word)
           (reverse result)
           (reverse (cons (list->string (reverse word)) result))))
      ((char=? (car chars) #\space)
       (if (null? word)
           (loop (cdr chars) '() result)
           (loop (cdr chars) '() (cons (list->string (reverse word)) result))))
      (else
       (loop (cdr chars) (cons (car chars) word) result)))))

;; ---------------------------------------------------------------------------
;; Simple record — a 2-D point
;; ---------------------------------------------------------------------------

(define-record-type <point>
  (make-point x y)
  point?
  (x point-x)
  (y point-y))

(define (point-distance p1 p2)
  (let ((dx (- (point-x p2) (point-x p1)))
        (dy (- (point-y p2) (point-y p1))))
    (sqrt (+ (* dx dx) (* dy dy)))))

(define origin (make-point 0 0))
(define unit   (make-point 1 1))

;; ---------------------------------------------------------------------------
;; Entry point
;; ---------------------------------------------------------------------------

(display (map fib '(0 1 2 3 4 5 6 7 8 9 10)))
(newline)
(display (point-distance origin unit))
(newline)
