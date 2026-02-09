;; Icon registry wrappers and built-in icon registrations.

(define (register-icon name filename color-role)
  (%register-icon! name filename color-role))

;; Tab bar icons
(register-icon 'tab-close-active "icon-close.svg" 'text.primary)
(register-icon 'tab-close-inactive "icon-close.svg" 'text.faded)
(register-icon 'tab-icon "tab-icon.svg" 'text.primary)

(set-global! 'default-scale 2.0)
(set-global! 'default-buffer-scale 2.0)
