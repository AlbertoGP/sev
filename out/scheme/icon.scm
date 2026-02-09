;; Icon registry wrappers and built-in icon registrations.

(define (register-icon name filename color-role)
  (%register-icon! name filename color-role))

;; Tab bar icons
(register-icon 'tab-close-active "icon-close.png" 'text.primary)
(register-icon 'tab-close-inactive "icon-close.png" 'text.faded)
(register-icon 'tab-icon "tab-icon.png" 'text.primary)
