;;; macro.scm - Vim macro recording and playback

;; Recording-mode keymap: only intercepts 'q' to stop recording.
;; No parent — everything else falls through the minor-mode lookup chain.
(define vim-recording-map (make-keymap))
(set-key! vim-recording-map "q" 'vim-stop-macro)
(define-minor-mode 'vim-recording-mode vim-recording-map)

(define macro-char "")

(defcommand (vim-start-macro)
  "vim: record macro\nStart recording macro to register."
  (let ((ch (last-key-char)))
    (when ch
      (%macro-start! ch)
      (enable-minor-mode 'vim-recording-mode)
      (set! macro-char ch)
      (message (string-append "Recording macro @" (string macro-char))))))

(defcommand (vim-stop-macro)
  "vim: finish macro recording"
  (%macro-stop!)
  (disable-minor-mode 'vim-recording-mode)
  (message (string-append "Saved macro @" (string macro-char))))

(define current-vim-macro #\a)   ; last-played register for @@

(defcommand (vim-play-macro)
  "vim: replay macro\nPlay macro from register."
  (let ((ch (last-key-char))
        (count (or vim-count 1)))
    (when ch
      (set! current-vim-macro ch)
      (set! vim-count #f)
      (let loop ((n count))
        (when (> n 0)
          (%macro-play ch)
          (loop (- n 1)))))))

(defcommand (vim-play-last-macro)
  "vim: replay last macro\nReplay last macro (@@ in Vim)."
  (let ((count (or vim-count 1)))
    (set! vim-count #f)
    (let loop ((n count))
      (when (> n 0)
        (%macro-play current-vim-macro)
        (loop (- n 1))))))

;; Named prefix keymaps for macro dispatch.
(let ((q-km (make-keymap)))
  (%set-keymap-name! q-km "macro-register")
  (bind-prefix! normal-map "q" q-km))
(set-command-display-binding! 'vim-start-macro "q <reg>")

;; q + a-z → start macro recording
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "q " ch) 'vim-start-macro)))

(let ((at-km (make-keymap)))
  (%set-keymap-name! at-km "play-macro")
  (bind-prefix! normal-map "@" at-km))
(set-command-display-binding! 'vim-play-macro "@ <reg>")

;; @ + a-z → play macro; @@ → replay last
(do ((i 0 (+ i 1)))
    ((= i 26))
  (let ((ch (string (integer->char (+ (char->integer #\a) i)))))
    (set-key! normal-map (string-append "@ " ch) 'vim-play-macro)))
(set-key! normal-map "@ @" 'vim-play-last-macro)
