#pragma once
#include "state.h"

// Load ~/.local/state/sev/state.json into AppState.
// Sets state->first_launch = true if the file doesn't exist.
// Returns false only on a hard I/O error (missing file is not an error).
bool state_io_load(AppState *state);

// Atomically write AppState to ~/.local/state/sev/state.json.
// Writes to a .tmp file then renames it.
// Returns false on failure.
bool state_io_save(const AppState *state);

// Record that a command was invoked from the minibuffer.
// Increments freq and updates last_used for the named command.
void state_io_record_command(AppState *state, const char *name);

// Upsert path into recent_projects: update last_opened if found,
// otherwise append (or evict the least recently accessed entry).
void state_io_update_recent_project(AppState *state, const char *path);
