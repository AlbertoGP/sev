#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_scanner.h"

// ---- Queue ------------------------------------------------------------------

static void queue_push(PathQueue *q, char *path) {
    SDL_LockMutex(q->mutex);
    int next_tail = (q->tail + 1) % QUEUE_CAP;
    if (next_tail == q->head) {
        // Queue full — drop the path
        SDL_UnlockMutex(q->mutex);
        free(path);
        return;
    }
    q->items[q->tail] = path;
    q->tail = next_tail;
    SDL_UnlockMutex(q->mutex);
}

static char *queue_pop(PathQueue *q) {
    SDL_LockMutex(q->mutex);
    if (q->head == q->tail) {
        SDL_UnlockMutex(q->mutex);
        return NULL;
    }
    char *path = q->items[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    SDL_UnlockMutex(q->mutex);
    return path;
}

// ---- Directory scanner ------------------------------------------------------

static bool should_skip(const char *name) {
    if (name[0] == '.') return true;                      // hidden files/dirs and "." ".."
    if (strcmp(name, "node_modules") == 0) return true;
    return false;
}

static void scan_dir(FileScanner *scanner, const char *dir_path, int depth) {
    if (depth > 32) return;
    if (SDL_GetAtomicInt(&scanner->stop)) return;

    DIR *d = opendir(dir_path);
    if (!d) return;

    size_t root_len = strlen(scanner->root);
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (SDL_GetAtomicInt(&scanner->stop)) break;
        if (should_skip(entry->d_name)) continue;

        // Build absolute path for this entry
        size_t dir_len  = strlen(dir_path);
        size_t name_len = strlen(entry->d_name);
        char *abs = malloc(dir_len + 1 + name_len + 1);
        if (!abs) continue;
        memcpy(abs, dir_path, dir_len);
        abs[dir_len] = '/';
        memcpy(abs + dir_len + 1, entry->d_name, name_len + 1);

        // Determine type (handle DT_UNKNOWN via stat)
        int is_dir = 0, is_reg = 0;
        if (entry->d_type == DT_DIR) {
            is_dir = 1;
        } else if (entry->d_type == DT_REG) {
            is_reg = 1;
        } else if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK) {
            struct stat st;
            if (stat(abs, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
                is_reg = S_ISREG(st.st_mode);
            }
        }

        if (is_dir) {
            scan_dir(scanner, abs, depth + 1);
            free(abs);
        } else if (is_reg) {
            // Build relative path by stripping the root prefix (and trailing slash)
            const char *rel_start = abs + root_len;
            if (*rel_start == '/') rel_start++;

            char *rel = strdup(rel_start);
            free(abs);
            if (!rel) continue;

            // basename is the last '/' component
            char *basename = strrchr(rel, '/');
            if (basename) basename++;
            else basename = rel;

            // Allocate a FileEntry-ready path; store basename offset inline
            // queue_push takes ownership of rel
            // We temporarily pack basename as a sentinel: we'll recompute it on drain
            queue_push(&scanner->queue, rel);
        } else {
            free(abs);
        }
    }

    closedir(d);
}

static int scanner_thread_fn(void *data) {
    FileScanner *scanner = data;
    scan_dir(scanner, scanner->root, 0);
    return 0;
}

// ---- Public API -------------------------------------------------------------

bool scanner_init(FileScanner *scanner) {
    memset(scanner, 0, sizeof(*scanner));
    SDL_SetAtomicInt(&scanner->stop, 0);
    scanner->queue.mutex = SDL_CreateMutex();
    return scanner->queue.mutex != NULL;
}

void scanner_restart(FileScanner *scanner) {
    // Stop any running thread
    SDL_SetAtomicInt(&scanner->stop, 1);
    if (scanner->thread) {
        SDL_WaitThread(scanner->thread, NULL);
        scanner->thread = NULL;
    }

    // Free existing index
    for (size_t i = 0; i < scanner->index.count; i++)
        free(scanner->index.entries[i].path);
    free(scanner->index.entries);
    scanner->index.entries  = NULL;
    scanner->index.count    = 0;
    scanner->index.capacity = 0;

    // Drain any leftover queue items
    char *path;
    while ((path = queue_pop(&scanner->queue)) != NULL)
        free(path);

    // Reset stop flag and capture new root from cwd
    SDL_SetAtomicInt(&scanner->stop, 0);
    getcwd(scanner->root, sizeof(scanner->root));

    scanner->thread = SDL_CreateThread(scanner_thread_fn, "file-scanner", scanner);
}

bool scanner_drain(FileScanner *scanner) {
    bool added = false;
    for (int i = 0; i < DRAIN_PER_FRAME; i++) {
        char *path = queue_pop(&scanner->queue);
        if (!path) break;

        // Grow the index if needed
        if (scanner->index.count >= scanner->index.capacity) {
            size_t new_cap = scanner->index.capacity == 0 ? 256 : scanner->index.capacity * 2;
            FileEntry *new_entries = realloc(scanner->index.entries,
                                             new_cap * sizeof(FileEntry));
            if (!new_entries) { free(path); continue; }
            scanner->index.entries  = new_entries;
            scanner->index.capacity = new_cap;
        }

        FileEntry *e = &scanner->index.entries[scanner->index.count++];
        e->path     = path;
        char *bn    = strrchr(path, '/');
        e->basename = bn ? bn + 1 : path;
        added = true;
    }
    return added;
}

void scanner_shutdown(FileScanner *scanner) {
    SDL_SetAtomicInt(&scanner->stop, 1);
    if (scanner->thread) {
        SDL_WaitThread(scanner->thread, NULL);
        scanner->thread = NULL;
    }
}

void scanner_free(FileScanner *scanner) {
    // Free index entries
    for (size_t i = 0; i < scanner->index.count; i++)
        free(scanner->index.entries[i].path);
    free(scanner->index.entries);
    scanner->index.entries  = NULL;
    scanner->index.count    = 0;
    scanner->index.capacity = 0;

    // Drain and free any remaining queue items
    char *path;
    while ((path = queue_pop(&scanner->queue)) != NULL)
        free(path);

    if (scanner->queue.mutex) {
        SDL_DestroyMutex(scanner->queue.mutex);
        scanner->queue.mutex = NULL;
    }
}
