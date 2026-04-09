(define *themes* (make-hash-table))
(define *current-theme* #f)
(define (current-theme) *current-theme*)

;;; --- Helpers ---

(define (parse-theme-args args)
  (let loop ((a args) (pal '()) (cmap '()) (ovr '()))
    (cond
      ((null? a) (values pal cmap ovr))
      ((eq? (car a) 'palette)       (loop (cddr a) (cadr a) cmap ovr))
      ((eq? (car a) 'canonical-map) (loop (cddr a) pal (cadr a) ovr))
      ((eq? (car a) 'overrides)     (loop (cddr a) pal cmap (cadr a)))
      (else (error "define-theme: unknown key" (car a))))))

(define (define-theme name display-name . kwargs)
  (call-with-values
    (lambda () (parse-theme-args kwargs))
    (lambda (pal cmap ovr)
      (hash-table-set! *themes* name (list display-name pal cmap ovr)))))

(define (theme-display-name sym)
  (let ((t (hash-table-ref/default *themes* sym #f)))
    (and t (car t))))

(define (list-themes)
  (map (lambda (s) (cons s (theme-display-name s)))
       (hash-table-keys *themes*)))

;;; --- Canonical palette schema ---

(define *canonical-keys*
  '(;; neutrals
    bg-0 bg-1 bg-2 bg-3 fg-0 fg-1 fg-2
    ;; core hues
    red orange yellow green cyan blue purple
    ;; optional hues
    magenta pink teal lime gold
    ;; accents
    accent-0 accent-1 accent-2
    ;; extended virtual keys (supplied via canonical-map)
    fg-text fg-text-dim fg-warm selection-bg))

(define (build-canonical palette cmap)
  (map (lambda (ckey)
         (let ((mapped (assq ckey cmap)))
           (cons ckey
                 (if mapped
                     (let ((p (assq (cdr mapped) palette)))
                       (and p (cdr p)))
                     (let ((p (assq ckey palette)))
                       (and p (cdr p)))))))
       *canonical-keys*))

(define (merge-roles base overrides)
  (let loop ((base base) (result overrides))
    (if (null? base) result
        (loop (cdr base)
              (if (assq (caar base) overrides)
                  result
                  (cons (car base) result))))))

;;; --- Default role mapping ---

(define *default-roles*
  '(;; structural
    (ui.bg             . bg-1)
    (pane.bg           . bg-0)
    (bar.bg            . bg-2)
    (line.bg           . fg-0)
    ;; borders
    (border.active     . purple)
    (border.inactive   . fg-0)
    (border.bell       . yellow)
    ;; chrome
    (scrollbar         . fg-0)
    (scrollbar.hover   . accent-0)
    (tab.active        . bg-0)
    (tab.hover         . bg-0)
    (tab.inactive      . bg-2)
    (tab.close         . fg-0)
    ;; text
    (text.primary      . fg-text)
    (text.faded        . fg-2)
    (text.key          . fg-text-dim)
    (text.command      . fg-text)
    (text.prefix       . fg-text)
    (text.linenum      . purple)
    (selection         . selection-bg)
    ;; mode indicators
    (mode.normal       . blue)
    (mode.insert       . green)
    (mode.replace      . red)
    (mode.select       . yellow)
    (mode.pending      . pink)
    (mode.minibuffer   . cyan)
    ;; mode labels
    (label.normal      . fg-0)
    (label.insert      . fg-0)
    (label.replace     . fg-0)
    (label.select      . fg-0)
    (label.pending     . fg-0)
    (label.minibuffer  . fg-0)
    ;; cursors
    (cursor.normal     . fg-warm)
    (cursor.insert     . fg-warm)
    (cursor.replace    . fg-warm)
    (cursor.select     . fg-warm)
    (cursor.pending    . fg-warm)
    (cursor.minibuffer . fg-warm)
    ;; macro
    (macro.indicator   . red)
    (macro.bg          . fg-2)
    ;; syntax
    (hl.keyword        . (:color purple))
    (hl.string         . (:color green))
    (hl.comment        . (:color accent-0 :font buf-italic))
    (hl.number         . (:color orange))
    (hl.constant       . (:color orange))
    (hl.function       . (:color blue))
    (hl.builtin        . (:color cyan))
    (hl.operator       . (:color fg-text))
    (hl.bracket        . (:color accent-0))
    (hl.bracket.match  . (:color fg-warm :bg selection-bg))))

;;; --- Activation ---

(defun (activate-theme input)
  "Switch to the named theme."
  (let* ((sym           (if (string? input) (string->symbol input) input))
         (entry         (hash-table-ref *themes* sym))
         (raw-palette   (list-ref entry 1))
         (canonical-map (list-ref entry 2))
         (overrides     (list-ref entry 3)))

    (%clear-palette!)
    (%clear-roles!)

    ;; 1. load raw palette
    (for-each (lambda (p) (%set-palette! (car p) (cdr p))) raw-palette)

    ;; 2. push canonical entries into palette_table under canonical names
    ;;    so default role symbols like 'purple, 'fg-text resolve via C unchanged
    (for-each
     (lambda (e) (when (cdr e) (%set-palette! (car e) (cdr e))))
     (build-canonical raw-palette canonical-map))

    ;; 3. apply merged roles (defaults first; theme overrides win)
    (for-each (lambda (r) (%set-role! (car r) (cdr r)))
              (merge-roles *default-roles* overrides))

    (set! *current-theme* sym)
    (%update-icon-colors!)))

;;; --- Bundled themes (one file per family in theme/) ---
