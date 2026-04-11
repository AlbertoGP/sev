// pre.js — injected at the start of app.js by the emscripten linker (--pre-js).
// Sets up persistent storage via localStorage and clipboard helpers.

// ---------------------------------------------------------------------------
// Persistent state via localStorage
// ---------------------------------------------------------------------------
// Before main() runs, populate /tmp/sev-state.json from localStorage so that
// state_io_load() finds the previously-saved state. The preRun callback runs
// synchronously — no async coordination required.

Module.preRun = Module.preRun || [];
Module.preRun.push(function() {
    var saved = localStorage.getItem('sev-state');
    if (saved) {
        FS.writeFile('/tmp/sev-state.json', saved);
    }
});

// After state_io_save() writes /tmp/sev-state.json, read it back and push to
// localStorage so the data survives page reloads. This is also called from
// a visibilitychange listener below as a belt-and-suspenders save.
function sevFlushStateToLocalStorage() {
    try {
        var content = FS.readFile('/tmp/sev-state.json', { encoding: 'utf8' });
        localStorage.setItem('sev-state', content);
    } catch(e) {
        // File may not exist yet on first launch — ignore.
    }
}

// Belt-and-suspenders: flush when the tab is hidden (covers tab-close / nav-away).
document.addEventListener('visibilitychange', function() {
    if (document.visibilityState === 'hidden') {
        sevFlushStateToLocalStorage();
    }
});

