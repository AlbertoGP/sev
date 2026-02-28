;;; build-in.scm - built-in editor command definitions.

;; Counter for auto-naming new buffers (untitled-1, untitled-2, ...)
(define *new-buffer-counter* 0)

;; C primitive commands
(defcommand quit "Exit the editor.")
(defcommand self-insert "Insert the character that invoked this command.")
(defcommand eval-buffer "Evaluate the current buffer as Scheme code.")
(defcommand next-line "Move cursor to the next line.")
(defcommand prev-line "Move cursor to the previous line.")
(defcommand forward-char "Move cursor forward one character.")
(defcommand backward-char "Move cursor backward one character.")
(defcommand line-start "Move cursor to start of current line.")
(defcommand line-end "Move cursor to end of current line.")
(defcommand skip-whitespace "Move cursor to next non-space, non-tab character.")
(defcommand newline "Insert a newline at cursor.")
(defcommand insert-tab "Insert a tab character at cursor.")
(defcommand delete-backward-char "Delete the character before cursor.")
(defcommand delete-forward-char "Delete the character after cursor.")
(defcommand move-cursor "Move cursor by COUNT characters.")
(defcommand delete-char "Delete COUNT characters before cursor.")
(defcommand insert-char "Insert character CH at cursor.")
(defcommand tab-next "Switch to the next tab.")
(defcommand tab-prev "Switch to the previous tab.")
(defcommand reset-global-scale "Reset global UI scaling factor.")
(defcommand increase-global-scale "Increase global UI scaling factor.")
(defcommand decrease-global-scale "Decrease global UI scaling factor.")
(defcommand reset-buffer-scale "Reset buffer text scaling factor.")
(defcommand increase-buffer-scale "Increase buffer text scaling factor.")
(defcommand decrease-buffer-scale "Decrease buffer text scaling factor.")
(defcommand split-horizontal "Split the current pane horizontally.")
(defcommand split-vertical "Split the current pane vertically.")
(defcommand pane-close "Close the current pane.")
(defcommand pane-navigate-up "Navigate to the pane above.")
(defcommand pane-navigate-down "Navigate to the pane below.")
(defcommand pane-navigate-left "Navigate to the pane on the left.")
(defcommand pane-navigate-right "Navigate to the pane on the right.")
(defcommand pane-h-split-increase "Increase horizontal split size.")
(defcommand pane-h-split-decrease "Decrease horizontal split size.")
(defcommand pane-v-split-increase "Increase vertical split size.")
(defcommand pane-v-split-decrease "Decrease vertical split size.")
(defcommand clay-debug "Toggle Clay debug mode.")
(defcommand exchange-point-and-mark "Swap point and selection anchor.")

;; compound Scheme commands
(defcommand (line-start-skip-whitespace)
  "Set cursor to first non-blank character on current line."
  (line-start) (skip-whitespace))
(defcommand (join-line)
  "Delete newline character between the current and subsequent line."
  (line-end) (delete-forward-char))

(defcommand (save-buffer-as)
  "Save the current buffer under a new file name."
  (interactive)
  (minibuffer-read "Save as: "
    (lambda (filename)
      (if (string=? filename "")
          (message "Filename cannot be empty")
          (begin
            (%set-buffer-file-name! filename)
            (%buffer-set-name! filename)
            (if (%buffer-write)
                (message (string-append "Saved " filename))
                (message (string-append "Failed to save " filename))))))))

(defcommand (save-buffer)
  "Save the current buffer. If no file name is set, prompt for one."
  (interactive)
  (let ((filename (%buffer-file-name)))
    (if (not (eq? filename #f))
        (if (%buffer-write)
            (message (string-append "Saved " filename))
            (message (string-append "Failed to save " filename)))
        (call-interactively 'save-buffer-as))))

(defcommand (open-file)
  "Open an existing file into a new buffer, or switch to it if already open."
  (interactive)
  (minibuffer-read "Find file: "
    (lambda (filename)
      (if (string=? filename "")
          (message "Filename cannot be empty")
          (if (not (file-exists? filename))
              (message (string-append "File not found: " filename))
              (begin
                (%jump-push!)
                (if (%buffer-create filename)
                    (begin
                      (%pane-set-buffer! filename)
                      (%set-buffer-file-name! filename)
                      (if (%buffer-read)
                          (message (string-append "Opened " filename))
                          (message (string-append "Failed to read " filename))))
                    (begin
                      (%pane-set-buffer! filename)
                      (message (string-append "Switched to buffer " filename))))))))))

(defcommand (read-file)
  "Insert the contents of a file at the point in the current buffer without moving point or other text."
  (interactive)
  (minibuffer-read "Insert file: "
    (lambda (filename)
      (if (string=? filename "")
          (message "Filename cannot be empty")
          (begin
            (if (%buffer-insert filename)
                (message (string-append "Inserted " filename))
                (message (string-append "Could not read " filename))))))))

(defcommand (buffer-new)
  "Create a new empty buffer named untitled-n and display it in the current pane."
  (interactive)
  (set! *new-buffer-counter* (+ *new-buffer-counter* 1))
  (let ((name (string-append "untitled-" (number->string *new-buffer-counter*))))
    (if (no-tabs?)
        (%tab-new! name)
        (begin
          (%buffer-create name)
          (%pane-set-buffer! name)))
    (message (string-append "Created buffer " name))))

(defcommand (buffer-rename)
  "Rename the current buffer."
  (interactive)
  (minibuffer-read "Rename buffer to: "
    (lambda (new-name)
      (if (string=? new-name "")
          (message "Buffer name cannot be empty")
          (if (%buffer-set-name! new-name)
              (message (string-append "Buffer renamed to " new-name))
              (message (string-append "Name already in use: " new-name)))))))

(defcommand (scratch-buffer)
  "Switch to the *scratch* buffer, creating it if it does not exist."
  (interactive)
  (if (no-tabs?)
      (%tab-new! "*scratch*")
      (unless (%pane-set-buffer! "*scratch*")
        (%buffer-create "*scratch*")
        (%pane-set-buffer! "*scratch*")))
  (message "Switched to *scratch*"))

(defcommand (switch-to-buffer)
  "Switch the current pane to a named buffer."
  (interactive)
  (minibuffer-read "Switch to buffer: "
    (lambda (name)
      (if (string=? name "")
          (message "Buffer name cannot be empty")
          (if (%pane-set-buffer! name)
              (message (string-append "Switched to " name))
              (message (string-append "No buffer named \"" name "\"")))))))

(defcommand (buffer-close)
  "Close a named buffer, removing all panes and tabs that display it."
  (interactive)
  (minibuffer-read "Close buffer: "
    (lambda (name)
      (if (string=? name "")
          (message "Buffer name cannot be empty")
          (if (%buffer-close! name)
              (message (string-append "Closed buffer " name))
              (message (string-append "No buffer named \"" name "\"")))))))

(defcommand (tab-new)
  "Create a new tab. Prompts for a buffer name; auto-generates untitled-n if left empty."
  (interactive)
  (minibuffer-read "Buffer name for new tab (empty for auto): "
    (lambda (name)
      (let ((buf-name
             (if (string=? name "")
                 (begin
                   (set! *new-buffer-counter* (+ *new-buffer-counter* 1))
                   (string-append "untitled-" (number->string *new-buffer-counter*)))
                 name)))
        (if (%tab-new! buf-name)
            (message (string-append "Opened tab " buf-name))
            (message (string-append "Failed to create tab " buf-name)))))))

;; Default mouse click handler: left-click moves point.
(define (default-mouse-click button buf-pos clicks)
  (when (= button 1)
    (point-set! buf-pos)))

;; Default mouse drag handler: drag moves point (selection can be layered on top).
(define (default-mouse-drag current-pos start-pos)
  (point-set! current-pos))
