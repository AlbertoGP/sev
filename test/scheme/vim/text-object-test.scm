;;; text-object-test.scm — SRFI 64 suite for scheme/editor/vim/text-object.scm
;;;
;;; Text objects are invoked directly via (text-object-ref 'sym) with a
;;; known buffer/cursor state, bypassing vim-execute-text-object dispatch
;;; (which reads private vim-count / vim-sm-state the library does not
;;; export). The registered procs take (kind count) and return a <range>.

(define (tobj name) (text-object-ref name))

(define (run-vim-text-object-tests)

  ;; ── word ──────────────────────────────────────────────────────────────

  (test-group "word — in: cursor on word char"
    ;; "foo bar": f0 o1 o2 ' '3 b4 a5 r6
    ;; cursor=1 ('o'), word chars extend [0,3)
    (test-equal "range-start 0" 0
      (begin (seed! "foo bar" 1)
             (range-start ((tobj 'word) 'in 1))))
    (test-equal "range-end 3" 3
      (begin (seed! "foo bar" 1)
             (range-end ((tobj 'word) 'in 1)))))

  (test-group "word — around: trailing space included"
    ;; cursor=1, word [0,3), space at 3 → around [0,4)
    (test-equal "range-start 0" 0
      (begin (seed! "foo bar" 1)
             (range-start ((tobj 'word) 'around 1))))
    (test-equal "range-end 4" 4
      (begin (seed! "foo bar" 1)
             (range-end ((tobj 'word) 'around 1)))))

  (test-group "word — around: falls back to leading space"
    ;; "foo bar" cursor=5 ('a'), word 'bar' = [4,7), no trailing space →
    ;; include leading space at 3 → around [3,7)
    (test-equal "range-start 3" 3
      (begin (seed! "foo bar" 5)
             (range-start ((tobj 'word) 'around 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "foo bar" 5)
             (range-end ((tobj 'word) 'around 1)))))

  ;; ── WORD ──────────────────────────────────────────────────────────────

  (test-group "WORD — in: treats foo-bar as one WORD"
    ;; "foo-bar baz": f0 o1 o2 -3 b4 a5 r6 ' '7 b8 a9 z10
    ;; cursor=3 ('-'), WORD spans non-whitespace [0,7)
    (test-equal "range-start 0" 0
      (begin (seed! "foo-bar baz" 3)
             (range-start ((tobj 'WORD) 'in 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "foo-bar baz" 3)
             (range-end ((tobj 'WORD) 'in 1)))))

  (test-group "WORD — around: trailing space included"
    ;; cursor=3, WORD [0,7), space at 7 → around [0,8)
    (test-equal "range-start 0" 0
      (begin (seed! "foo-bar baz" 3)
             (range-start ((tobj 'WORD) 'around 1))))
    (test-equal "range-end 8" 8
      (begin (seed! "foo-bar baz" 3)
             (range-end ((tobj 'WORD) 'around 1)))))

  ;; ── paren ─────────────────────────────────────────────────────────────

  (test-group "paren — in: excludes delimiters"
    ;; "(hello)": (0 h1 e2 l3 l4 o5 )6
    ;; cursor=3, find-enclosing-pair → (0 . 6), in → [1,6)
    (test-equal "range-start 1" 1
      (begin (seed! "(hello)" 3)
             (range-start ((tobj 'paren) 'in 1))))
    (test-equal "range-end 6" 6
      (begin (seed! "(hello)" 3)
             (range-end ((tobj 'paren) 'in 1)))))

  (test-group "paren — around: includes delimiters"
    ;; cursor=3, around → [0,7)
    (test-equal "range-start 0" 0
      (begin (seed! "(hello)" 3)
             (range-start ((tobj 'paren) 'around 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "(hello)" 3)
             (range-end ((tobj 'paren) 'around 1)))))

  ;; ── bracket ───────────────────────────────────────────────────────────

  (test-group "bracket — in: excludes delimiters"
    ;; "[world]": [0 w1 o2 r3 l4 d5 ]6 → in [1,6)
    (test-equal "range-start 1" 1
      (begin (seed! "[world]" 3)
             (range-start ((tobj 'bracket) 'in 1))))
    (test-equal "range-end 6" 6
      (begin (seed! "[world]" 3)
             (range-end ((tobj 'bracket) 'in 1)))))

  (test-group "bracket — around: includes delimiters"
    (test-equal "range-start 0" 0
      (begin (seed! "[world]" 3)
             (range-start ((tobj 'bracket) 'around 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "[world]" 3)
             (range-end ((tobj 'bracket) 'around 1)))))

  ;; ── brace ─────────────────────────────────────────────────────────────

  (test-group "brace — in: excludes delimiters"
    ;; "{code}": {0 c1 o2 d3 e4 }5 → in [1,5)
    (test-equal "range-start 1" 1
      (begin (seed! "{code}" 2)
             (range-start ((tobj 'brace) 'in 1))))
    (test-equal "range-end 5" 5
      (begin (seed! "{code}" 2)
             (range-end ((tobj 'brace) 'in 1)))))

  ;; ── angle ─────────────────────────────────────────────────────────────

  (test-group "angle — in: excludes delimiters"
    ;; "<tag>": <0 t1 a2 g3 >4 → in [1,4)
    (test-equal "range-start 1" 1
      (begin (seed! "<tag>" 2)
             (range-start ((tobj 'angle) 'in 1))))
    (test-equal "range-end 4" 4
      (begin (seed! "<tag>" 2)
             (range-end ((tobj 'angle) 'in 1)))))

  ;; ── dquote ────────────────────────────────────────────────────────────

  (test-group "dquote — in: excludes quotes"
    ;; "\"hello\"": "0 h1 e2 l3 l4 o5 "6 → in [1,6)
    (test-equal "range-start 1" 1
      (begin (seed! "\"hello\"" 3)
             (range-start ((tobj 'dquote) 'in 1))))
    (test-equal "range-end 6" 6
      (begin (seed! "\"hello\"" 3)
             (range-end ((tobj 'dquote) 'in 1)))))

  (test-group "dquote — around: includes quotes"
    ;; cursor=3, around → [0,7)
    (test-equal "range-start 0" 0
      (begin (seed! "\"hello\"" 3)
             (range-start ((tobj 'dquote) 'around 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "\"hello\"" 3)
             (range-end ((tobj 'dquote) 'around 1)))))

  ;; ── squote ────────────────────────────────────────────────────────────

  (test-group "squote — in: excludes quotes"
    ;; "'test'": '0 t1 e2 s3 t4 '5 → in [1,5)
    (test-equal "range-start 1" 1
      (begin (seed! "'test'" 2)
             (range-start ((tobj 'squote) 'in 1))))
    (test-equal "range-end 5" 5
      (begin (seed! "'test'" 2)
             (range-end ((tobj 'squote) 'in 1)))))

  ;; ── backtick ──────────────────────────────────────────────────────────

  (test-group "backtick — in: excludes backticks"
    ;; "`cmd`": `0 c1 m2 d3 `4 → in [1,4)
    (test-equal "range-start 1" 1
      (begin (seed! "`cmd`" 2)
             (range-start ((tobj 'backtick) 'in 1))))
    (test-equal "range-end 4" 4
      (begin (seed! "`cmd`" 2)
             (range-end ((tobj 'backtick) 'in 1)))))

  ;; ── sentence ──────────────────────────────────────────────────────────

  (test-group "sentence — in: range of first sentence"
    ;; "Hello! World." H0 e1 l2 l3 o4 !5 ' '6 W7 o8 r9 l10 d11 .12
    ;; cursor=2, bwd hits buf start → start=0,
    ;; fwd finds sentence-end at '!'(5) + ' '(6) → end=(+5 1)=6
    (test-equal "range-start 0" 0
      (begin (seed! "Hello! World." 2)
             (range-start ((tobj 'sentence) 'in 1))))
    (test-equal "range-end 6" 6
      (begin (seed! "Hello! World." 2)
             (range-end ((tobj 'sentence) 'in 1)))))

  (test-group "sentence — around: extends over trailing space"
    ;; same position, around extends end=6 over space at 6 → ae=7
    (test-equal "range-start 0" 0
      (begin (seed! "Hello! World." 2)
             (range-start ((tobj 'sentence) 'around 1))))
    (test-equal "range-end 7" 7
      (begin (seed! "Hello! World." 2)
             (range-end ((tobj 'sentence) 'around 1)))))

  ;; ── paragraph ─────────────────────────────────────────────────────────

  (test-group "paragraph — in: range starts at block start"
    ;; "foo\nbar\n" — no blank line, both lines in one paragraph
    (test-equal "range-start 0" 0
      (begin (seed! "foo\nbar\n" 1)
             (range-start ((tobj 'paragraph) 'in 1)))))

  ;; ── tag ───────────────────────────────────────────────────────────────

  (test-group "tag — in: content between tags"
    ;; "<p>hello</p>": <0 p1 >2 h3 e4 l5 l6 o7 <8 /9 p10 >11
    ;; find-enclosing-tag → (0 3 8 12), in → [3,8)
    (test-equal "range-start 3" 3
      (begin (seed! "<p>hello</p>" 5)
             (range-start ((tobj 'tag) 'in 1))))
    (test-equal "range-end 8" 8
      (begin (seed! "<p>hello</p>" 5)
             (range-end ((tobj 'tag) 'in 1)))))

  (test-group "tag — around: entire tag pair"
    ;; around → [0,12)
    (test-equal "range-start 0" 0
      (begin (seed! "<p>hello</p>" 5)
             (range-start ((tobj 'tag) 'around 1))))
    (test-equal "range-end 12" 12
      (begin (seed! "<p>hello</p>" 5)
             (range-end ((tobj 'tag) 'around 1)))))

  )
