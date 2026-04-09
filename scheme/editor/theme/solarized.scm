(define *solarized-canonical-map*
  '((purple      . violet)    ; violet (#6c71c4) is Solarized's purple-analog
    (pink        . magenta)   ; magenta (#d33682) is closest to canonical pink
    (fg-text     . text-0)
    (fg-text-dim . text-2)
    (fg-warm     . text-1)
    (selection-bg . sel-bg)))
; blue, cyan, green, yellow, orange, red match canonical names directly

(define-theme
  'solarized-dark
  "Solarized Dark"
  'palette
  '(;; backgrounds: base03=main, base02=panels/bar
    (bg-0      . "#073642")   ; base02 — content panels (slightly lighter)
    (bg-1      . "#002b36")   ; base03 — main window background (darkest)
    (bg-2      . "#073642")   ; base02 — status bar (same as panels)
    ;; foreground tones (dark→light for inactive chrome, faded text)
    (fg-0      . "#586e75")   ; base01 — inactive chrome, line gutter, borders
    (fg-1      . "#657b83")   ; base00 — mid tone
    (fg-2      . "#657b83")   ; base00 — faded text, secondary
    ;; accent ramp (base01 for comments/brackets in dark mode)
    (accent-0  . "#586e75")   ; base01
    (accent-1  . "#657b83")   ; base00
    (accent-2  . "#839496")   ; base0
    ;; text tones
    (text-0    . "#839496")   ; base0  — body text
    (text-1    . "#93a1a1")   ; base1  — emphasized / cursor
    (text-2    . "#586e75")   ; base01 — dim / key hints
    ;; selection
    (sel-bg    . "#07364266") ; base02 @ ~40% opacity
    ;; violet (maps to canonical 'purple via canonical-map)
    (violet    . "#6c71c4")
    ;; accent hues (names match canonical keys directly)
    (blue      . "#268bd2")
    (cyan      . "#2aa198")
    (green     . "#859900")
    (yellow    . "#b58900")
    (orange    . "#cb4b16")
    (red       . "#dc322f")
    (magenta   . "#d33682"))
  'canonical-map *solarized-canonical-map*
  'overrides
  '((label.normal     . bg-1)
    (label.insert     . bg-1)
    (label.replace    . bg-1)
    (label.select     . bg-1)
    (label.pending    . bg-1)
    (label.minibuffer . bg-1)))

(define-theme
  'solarized-light
  "Solarized Light"
  'palette
  '(;; backgrounds: base3=main, base2=panels/bar
    (bg-0      . "#eee8d5")   ; base2 — content panels (slightly off-white)
    (bg-1      . "#fdf6e3")   ; base3 — main window background (lightest)
    (bg-2      . "#eee8d5")   ; base2 — status bar (same as panels)
    ;; foreground tones (light→dark for inactive chrome)
    (fg-0      . "#93a1a1")   ; base1  — inactive chrome, line gutter, borders
    (fg-1      . "#839496")   ; base0  — mid tone
    (fg-2      . "#839496")   ; base0  — faded text, secondary
    ;; accent ramp (base1 for comments/brackets in light mode)
    (accent-0  . "#93a1a1")   ; base1
    (accent-1  . "#839496")   ; base0
    (accent-2  . "#657b83")   ; base00
    ;; text tones
    (text-0    . "#657b83")   ; base00 — body text
    (text-1    . "#586e75")   ; base01 — emphasized / cursor
    (text-2    . "#93a1a1")   ; base1  — dim / key hints
    ;; selection
    (sel-bg    . "#eee8d566") ; base2 @ ~40% opacity
    ;; violet (maps to canonical 'purple via canonical-map)
    (violet    . "#6c71c4")
    ;; accent hues (names match canonical keys directly)
    (blue      . "#268bd2")
    (cyan      . "#2aa198")
    (green     . "#859900")
    (yellow    . "#b58900")
    (orange    . "#cb4b16")
    (red       . "#dc322f")
    (magenta   . "#d33682"))
  'canonical-map *solarized-canonical-map*
  'overrides
  ; In light mode fg-0=base1 is too light for label pill backgrounds;
  ; use bg-0=base2 (cream) instead, mirroring the Catppuccin Latte fix.
  '((label.normal     . bg-0)
    (label.insert     . bg-0)
    (label.replace    . bg-0)
    (label.select     . bg-0)
    (label.pending    . bg-0)
    (label.minibuffer . bg-0)))
