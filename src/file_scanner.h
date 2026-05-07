#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <SDL3/SDL.h>

#define QUEUE_CAP       1024
#define DRAIN_PER_FRAME  256

typedef struct {
    char *path;     // full relative path; heap-allocated, owned by FileIndex after drain
    char *basename; // pointer into path at filename start
} FileEntry;

typedef struct {
    char      *items[QUEUE_CAP];
    int        head, tail;
    SDL_Mutex *mutex;
} PathQueue;

typedef struct {
    FileEntry *entries;
    size_t     count;
    size_t     capacity;
} FileIndex;

typedef struct {
    PathQueue    queue;
    FileIndex    index;
    SDL_Thread  *thread;
    SDL_AtomicInt stop;          // set to 1 by main thread to signal stop
    char         root[PATH_MAX];
} FileScanner;

bool scanner_init(FileScanner *scanner);    // one-time init: creates mutex only
void scanner_restart(FileScanner *scanner); // stop+clear+rescan from current cwd
bool scanner_drain(FileScanner *scanner);   // returns true if new entries were added
void scanner_shutdown(FileScanner *scanner);
void scanner_free(FileScanner *scanner);
