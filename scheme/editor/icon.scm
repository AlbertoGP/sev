;; Icon registry wrappers and built-in icon registrations.

(define (register-icon name filename color-role)
  (%register-icon! name filename color-role))

;; Tab bar icons
(register-icon 'tab-close "icon-close.svg" 'text.primary)
(register-icon 'tab-icon "tab-icon.svg" 'text.primary)
(register-icon 'welcome-icon "tab-icon.svg" 'text.primary)
(register-icon 'scheme-icon "icon-scheme.svg" 'text.primary)

(register-icon 'new-icon "icon-plus.svg" 'text.primary)
(register-icon 'open-icon "icon-open.svg" 'text.primary)
(register-icon 'help-icon "icon-help.svg" 'text.primary)
(register-icon 'palette-icon "icon-palette.svg" 'text.primary)
(register-icon 'project-icon "icon-project.svg" 'text.primary)

(defvar default-scale "Default scaling factor for app UI." 1.0)
(defvar default-buffer-scale "Default scaling factor for buffer text." 1.0)
