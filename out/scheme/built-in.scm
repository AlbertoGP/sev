;;; build-in.scm - built-in editor command definitions.

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

;; compound Scheme commands
(defcommand (line-start-skip-whitespace)
  "Set cursor to first non-blank character on current line."
  (line-start) (skip-whitespace))
(defcommand (open-line-below)
  "Create a new empty line below current line, move to it and enter insert mode."
  (line-end) (newline) (evil-insert))
(defcommand (open-line-above)
  "Create a new empty line above current line, move to it and enter insert mode."
  (line-start) (newline) (prev-line) (evil-insert))
(defcommand (insert-at-start)
  "Set cursor to first non-blank character on current line and enter insert mode."
  (line-start-skip-whitespace) (evil-insert))
(defcommand (append-char)
  "Enter insert mode after the character currently under the cursor."
  (forward-char) (evil-insert))
(defcommand (append-line)
  "Set the cursor to the final column on the current line and enter insert mode."
  (line-end) (evil-insert))
(defcommand (join-line)
  "Delete newline character between the current and subsequent line."
  (line-end) (delete-forward-char))

