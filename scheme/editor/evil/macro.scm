;;; macro.scm - Vim macro recording and playback

;; Recording-mode keymap: only intercepts 'q' to stop recording.
;; No parent — everything else falls through the minor-mode lookup chain.
(define evil-recording-map (make-keymap))
(set-key! evil-recording-map "q" 'evil-stop-macro)
(define-minor-mode 'evil-recording-mode evil-recording-map)

(define macro-char "")

(defcommand (evil-start-macro)
  "vim: record macro\nStart recording macro to register."
  (let ((ch (last-key-char)))
    (when ch
      (%macro-start! ch)
      (enable-minor-mode 'evil-recording-mode)
      (set! macro-char ch)
      (message (string-append "Recording macro @" (string macro-char))))))

(defcommand (evil-stop-macro)
  "vim: finish macro recording"
  (%macro-stop!)
  (disable-minor-mode 'evil-recording-mode)
  (message (string-append "Saved macro @" (string macro-char))))

(define current-evil-macro #\a)   ; last-played register for @@

(defcommand (evil-play-macro)
  "vim: replay macro\nPlay macro from register."
  (let ((ch (last-key-char))
        (count (or evil-count 1)))
    (when ch
      (set! current-evil-macro ch)
      (set! evil-count #f)
      (let loop ((n count))
        (when (> n 0)
          (%macro-play ch)
          (loop (- n 1)))))))

(defcommand (evil-play-last-macro)
  "vim: replay last macro\nReplay last macro (@@ in Vim)."
  (let ((count (or evil-count 1)))
    (set! evil-count #f)
    (let loop ((n count))
      (when (> n 0)
        (%macro-play current-evil-macro)
        (loop (- n 1))))))

;; Named prefix keymaps for macro dispatch.
(let ((q-km (make-keymap)))
  (%set-keymap-name! q-km "macro-register")
  (bind-prefix! normal-map "q" q-km))
(set-command-display-binding! 'evil-start-macro "q <reg>")

;; q + a-z → start macro recording
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "q " ch) 'evil-start-macro)))

(let ((at-km (make-keymap)))
  (%set-keymap-name! at-km "play-macro")
  (bind-prefix! normal-map "@" at-km))
(set-command-display-binding! 'evil-play-macro "@ <reg>")

;; @ + a-z → play macro; @@ → replay last
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "@ " ch) 'evil-play-macro)))
(set-key! normal-map "@ @" 'evil-play-last-macro)

;;; Activate
(evil-mode)
(evil-normal)
