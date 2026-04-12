// Major mode info registry.
// Maps major mode names to display names and optional icon names.
// Textures are managed by the general icon registry (icon.h).

#pragma once

#include <stdbool.h>

#define MAJOR_MODE_INFO_MAX 64

typedef struct MajorModeInfoEntry {
    const char *mode_name;    // e.g. "scheme-mode"
    const char *display_name; // e.g. "Scheme"
    char icon_name[64];       // e.g. "scheme-icon", or "" for no icon
} MajorModeInfoEntry;

// Register a major mode's display name and optional icon.
// mode_name and display_name are copied internally.
// icon_name may be NULL or "" to indicate no icon. Returns true on success.
bool major_mode_info_register(const char *mode_name,
                              const char *display_name,
                              const char *icon_name);

// Look up a registered entry by major mode name.
// Returns NULL if not found.
MajorModeInfoEntry *major_mode_info_get(const char *mode_name);
