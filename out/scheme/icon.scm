;; Icon registry wrappers and built-in icon registrations.

(define (register-icon name filename color-role)
  (%register-icon! name filename color-role))

;; Tab bar icons
(register-icon 'tab-close-active "icon-close.svg" 'text.primary)
(register-icon 'tab-close-inactive "icon-close.svg" 'text.faded)
(register-icon 'tab-icon "tab-icon.svg" 'text.primary)

(define default-scale 1.0)
(set-doc! 'default-scale 'variable "Default scaling factor for app UI.")
(define default-buffer-scale 1.0)
(set-doc! 'default-buffer-scale 'variable "Default scaling factor for buffer text.")
