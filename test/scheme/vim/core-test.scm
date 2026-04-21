;;; core-test.scm — SRFI 64 suite for scheme/editor/vim/core.scm
;;;
;;; Tests cover the pure utility functions and registries that are stable
;;; and do not require reading unexported state-machine variables.
;;; Character classification, UTF-8 helpers, text-object-for-char, and
;;; vim-state are the units under test.

(define (run-vim-core-tests)

  ;; ── vim-word-char? ────────────────────────────────────────────────────

  (test-group "vim-word-char? — word characters"
    (test-equal "lowercase letter" #t (vim-word-char? #\a))
    (test-equal "uppercase letter" #t (vim-word-char? #\Z))
    (test-equal "digit"            #t (vim-word-char? #\5))
    (test-equal "underscore"       #t (vim-word-char? #\_)))

  (test-group "vim-word-char? — non-word characters"
    (test-equal "space"     #f (vim-word-char? #\space))
    (test-equal "period"    #f (vim-word-char? #\.))
    (test-equal "hyphen"    #f (vim-word-char? #\-)))

  ;; ── vim-whitespace? ───────────────────────────────────────────────────

  (test-group "vim-whitespace? — whitespace characters"
    (test-equal "space"   #t (vim-whitespace? #\space))
    (test-equal "tab"     #t (vim-whitespace? #\tab))
    (test-equal "newline" #t (vim-whitespace? #\newline))
    (test-equal "return"  #t (vim-whitespace? #\return)))

  (test-group "vim-whitespace? — non-whitespace"
    (test-equal "letter" #f (vim-whitespace? #\a))
    (test-equal "hyphen" #f (vim-whitespace? #\-)))

  ;; ── vim-punctuation? ──────────────────────────────────────────────────

  (test-group "vim-punctuation? — punctuation characters"
    (test-equal "hyphen"     #t (vim-punctuation? #\-))
    (test-equal "period"     #t (vim-punctuation? #\.))
    (test-equal "exclamation" #t (vim-punctuation? #\!)))

  (test-group "vim-punctuation? — non-punctuation"
    (test-equal "letter" #f (vim-punctuation? #\a))
    (test-equal "space"  #f (vim-punctuation? #\space))
    (test-equal "null"   #f (vim-punctuation? #\null)))

  ;; ── utf8-byte-len-at ──────────────────────────────────────────────────

  (test-group "utf8-byte-len-at — ASCII is 1 byte"
    (test-equal "ASCII 'h'" 1
      (begin (seed! "hello") (utf8-byte-len-at 0))))

  (test-group "utf8-byte-len-at — 2-byte sequence"
    ;; U+00E9 é = 0xC3 0xA9; first byte (0xC3=195) → len 2
    (test-equal "2-byte lead byte" 2
      (begin (seed! "é") (utf8-byte-len-at 0))))

  (test-group "utf8-byte-len-at — 3-byte sequence"
    ;; U+4E2D 中 = 0xE4 0xB8 0xAD; first byte (0xE4=228) → len 3
    (test-equal "3-byte lead byte" 3
      (begin (seed! "中") (utf8-byte-len-at 0))))

  ;; ── count-chars-in-range ──────────────────────────────────────────────

  (test-group "count-chars-in-range — ASCII"
    (test-equal "5 ASCII chars = 5 bytes" 5
      (begin (seed! "hello") (count-chars-in-range 0 5))))

  (test-group "count-chars-in-range — multi-byte"
    ;; "éa": é=2 bytes (pos 0-1) + a=1 byte (pos 2) → 3 bytes, 2 chars
    (test-equal "2-byte char + ASCII = 2 chars" 2
      (begin (seed! "éa") (count-chars-in-range 0 3))))

  ;; ── text-object-for-char ──────────────────────────────────────────────

  (test-group "text-object-for-char — word/WORD/sentence/paragraph"
    (test-equal "w → word"      'word      (text-object-for-char #\w))
    (test-equal "W → WORD"      'WORD      (text-object-for-char #\W))
    (test-equal "s → sentence"  'sentence  (text-object-for-char #\s))
    (test-equal "p → paragraph" 'paragraph (text-object-for-char #\p)))

  (test-group "text-object-for-char — bracket/paren/brace/angle"
    (test-equal "[ → bracket"   'bracket (text-object-for-char #\[))
    (test-equal "] → bracket"   'bracket (text-object-for-char #\]))
    (test-equal "( → paren"     'paren   (text-object-for-char #\())
    (test-equal ") → paren"     'paren   (text-object-for-char #\)))
    (test-equal "b → paren"     'paren   (text-object-for-char #\b))
    (test-equal "{ → brace"     'brace   (text-object-for-char #\{))
    (test-equal "} → brace"     'brace   (text-object-for-char #\}))
    (test-equal "B → brace"     'brace   (text-object-for-char #\B))
    (test-equal "< → angle"     'angle   (text-object-for-char #\<))
    (test-equal "> → angle"     'angle   (text-object-for-char #\>)))

  (test-group "text-object-for-char — tag and quotes"
    (test-equal "t → tag"       'tag      (text-object-for-char #\t))
    (test-equal "\" → dquote"   'dquote   (text-object-for-char #\"))
    (test-equal "' → squote"    'squote   (text-object-for-char #\'))
    (test-equal "` → backtick"  'backtick (text-object-for-char #\`)))

  (test-group "text-object-for-char — unknown char"
    (test-equal "x → #f" #f (text-object-for-char #\x))
    (test-equal "z → #f" #f (text-object-for-char #\z)))

  ;; ── vim-state ─────────────────────────────────────────────────────────

  (test-group "vim-state — returns #f when mode stubs always return false"
    ;; The test harness stubs %buffer-has-minor-mode? to always return #f,
    ;; so vim-state falls through to the else branch and returns #f.
    (test-equal "vim-state is #f in stub environment" #f (vim-state)))

  )
