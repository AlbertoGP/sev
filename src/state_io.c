#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <chibi/eval.h>
#include "../vendored/jsmn.h"

#include "state_io.h"
#include "command/message.h"
#include "command/scheme_internal.h"

#define STATE_IO_MAX_TOKENS 2048

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Build the state.json path, creating the directory tree if needed.
// Returns true on success, false on mkdir failure.
static bool state_io_path(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if (!home || !home[0]) return false;

    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/.local", home);
    if (mkdir(dir, 0700) < 0 && errno != EEXIST) return false;
    snprintf(dir, sizeof(dir), "%s/.local/state", home);
    if (mkdir(dir, 0700) < 0 && errno != EEXIST) return false;
    snprintf(dir, sizeof(dir), "%s/.local/state/sev", home);
    if (mkdir(dir, 0700) < 0 && errno != EEXIST) return false;

    snprintf(buf, len, "%s/.local/state/sev/state.json", home);
    return true;
}

static int tok_len(const jsmntok_t *tok) {
    return tok->end - tok->start;
}

static bool tok_eq(const char *js, const jsmntok_t *tok, const char *s) {
    return (int)strlen(s) == tok_len(tok) &&
           strncmp(js + tok->start, s, tok_len(tok)) == 0;
}

// Returns the number of tokens in the subtree rooted at tokens[i].
static int subtree_size(const jsmntok_t *tokens, int i, int total) {
    if (i >= total) return 0;
    const jsmntok_t *t = &tokens[i];
    int count = 1;
    if (t->type == JSMN_OBJECT) {
        int pairs = t->size;
        int j = i + 1;
        for (int p = 0; p < pairs; p++) {
            j += subtree_size(tokens, j, total);   // key
            j += subtree_size(tokens, j, total);   // value
        }
        return j - i;
    } else if (t->type == JSMN_ARRAY) {
        int elems = t->size;
        int j = i + 1;
        for (int e = 0; e < elems; e++)
            j += subtree_size(tokens, j, total);
        return j - i;
    }
    return count;
}

// Parse a decimal integer from js[tok->start..tok->end].
static int64_t tok_int(const char *js, const jsmntok_t *tok) {
    char tmp[32];
    int n = tok_len(tok);
    if (n <= 0 || n >= (int)sizeof(tmp)) return 0;
    memcpy(tmp, js + tok->start, n);
    tmp[n] = '\0';
    return (int64_t)atoll(tmp);
}

// Write a JSON-escaped string to f.
static void write_json_string(FILE *f, const char *s) {
    fputc('"', f);
    for (const char *p = s; *p; p++) {
        if (*p == '"')       fputs("\\\"", f);
        else if (*p == '\\') fputs("\\\\", f);
        else if (*p == '\n') fputs("\\n",  f);
        else if (*p == '\r') fputs("\\r",  f);
        else if (*p == '\t') fputs("\\t",  f);
        else                 fputc(*p, f);
    }
    fputc('"', f);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void state_io_record_command(AppState *state, const char *name) {
    for (int i = 0; i < state->command_stats_count; i++) {
        if (strcmp(state->command_stats[i].name, name) == 0) {
            state->command_stats[i].freq++;
            state->command_stats[i].last_used = (int64_t)time(NULL);
            return;
        }
    }
    if (state->command_stats_count >= COMMAND_STATS_MAX) return;
    CommandStat *s = &state->command_stats[state->command_stats_count++];
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
    s->freq = 1;
    s->last_used = (int64_t)time(NULL);
}

bool state_io_save(const AppState *state) {
    char path[PATH_MAX];
    if (!state_io_path(path, sizeof(path))) return false;

    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": 1,\n");
    fprintf(f, "  \"first_launch\": %s,\n", state->first_launch ? "true" : "false");

    fprintf(f, "  \"recent_projects\": [");
    for (int i = 0; i < state->recent_projects_count; i++) {
        if (i > 0) fprintf(f, ",");
        fprintf(f, "\n    {\"path\": ");
        write_json_string(f, state->recent_projects[i].path);
        fprintf(f, ", \"last_opened\": %lld}",
                (long long)state->recent_projects[i].last_opened);
    }
    if (state->recent_projects_count > 0) fprintf(f, "\n  ");
    fprintf(f, "],\n");

    fprintf(f, "  \"commands\": {");
    for (int i = 0; i < state->command_stats_count; i++) {
        if (i > 0) fprintf(f, ",");
        fprintf(f, "\n    ");
        write_json_string(f, state->command_stats[i].name);
        fprintf(f, ": {\"freq\": %d, \"last_used\": %lld}",
                state->command_stats[i].freq,
                (long long)state->command_stats[i].last_used);
    }
    if (state->command_stats_count > 0) fprintf(f, "\n  ");
    fprintf(f, "}\n");

    fprintf(f, "}\n");
    fclose(f);

    if (rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }
    return true;
}

bool state_io_load(AppState *state) {
    char path[PATH_MAX];
    if (!state_io_path(path, sizeof(path))) return false;

    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) {
            state->first_launch = true;
            return true;
        }
        return false;
    }

    // Read entire file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    if (fsize <= 0) { fclose(f); return true; }

    char *js = malloc((size_t)fsize + 1);
    if (!js) { fclose(f); return false; }
    size_t nread = fread(js, 1, (size_t)fsize, f);
    fclose(f);
    js[nread] = '\0';

    jsmn_parser parser;
    jsmntok_t tokens[STATE_IO_MAX_TOKENS];
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, js, nread, tokens, STATE_IO_MAX_TOKENS);
    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
        fprintf(stderr, "state_io_load: failed to parse %s (r=%d)\n", path, r);
        free(js);
        return false;
    }

    // Walk top-level object key/value pairs
    int i = 1;
    while (i + 1 < r) {
        const jsmntok_t *key = &tokens[i];
        const jsmntok_t *val = &tokens[i + 1];

        if (tok_eq(js, key, "recent_projects") && val->type == JSMN_ARRAY) {
            int count = val->size;
            if (count > RECENT_PROJECTS_MAX) count = RECENT_PROJECTS_MAX;
            int j = i + 2;  // first element
            for (int p = 0; p < val->size && j + 1 < r; p++) {
                if (tokens[j].type != JSMN_OBJECT) {
                    j += subtree_size(tokens, j, r);
                    continue;
                }
                RecentProject *rp = (p < RECENT_PROJECTS_MAX)
                    ? &state->recent_projects[state->recent_projects_count]
                    : NULL;
                int obj_pairs = tokens[j].size;
                int k = j + 1;
                for (int q = 0; q < obj_pairs && k + 1 < r; q++) {
                    if (rp && tok_eq(js, &tokens[k], "path") &&
                            tokens[k+1].type == JSMN_STRING) {
                        int n = tok_len(&tokens[k+1]);
                        if (n >= PATH_MAX) n = PATH_MAX - 1;
                        memcpy(rp->path, js + tokens[k+1].start, n);
                        rp->path[n] = '\0';
                    } else if (rp && tok_eq(js, &tokens[k], "last_opened")) {
                        rp->last_opened = tok_int(js, &tokens[k+1]);
                    }
                    k += subtree_size(tokens, k, r);   // key
                    k += subtree_size(tokens, k, r);   // value
                }
                if (rp) state->recent_projects_count++;
                j += subtree_size(tokens, j, r);
            }
            i += 1 + subtree_size(tokens, i + 1, r);

        } else if (tok_eq(js, key, "commands") && val->type == JSMN_OBJECT) {
            int cmd_pairs = val->size;
            int j = i + 2;  // first key in commands object
            for (int p = 0; p < cmd_pairs && j + 1 < r; p++) {
                const jsmntok_t *cmd_key = &tokens[j];
                const jsmntok_t *cmd_val = &tokens[j + 1];

                if (cmd_val->type == JSMN_OBJECT &&
                        state->command_stats_count < COMMAND_STATS_MAX) {
                    CommandStat *cs =
                        &state->command_stats[state->command_stats_count];
                    int n = tok_len(cmd_key);
                    if (n >= (int)sizeof(cs->name)) n = (int)sizeof(cs->name) - 1;
                    memcpy(cs->name, js + cmd_key->start, n);
                    cs->name[n] = '\0';

                    int obj_pairs = cmd_val->size;
                    int k = j + 2;
                    for (int q = 0; q < obj_pairs && k + 1 < r; q++) {
                        if (tok_eq(js, &tokens[k], "freq"))
                            cs->freq = (int)tok_int(js, &tokens[k+1]);
                        else if (tok_eq(js, &tokens[k], "last_used"))
                            cs->last_used = tok_int(js, &tokens[k+1]);
                        k += subtree_size(tokens, k, r);
                        k += subtree_size(tokens, k, r);
                    }
                    state->command_stats_count++;
                }

                j += subtree_size(tokens, j, r);   // cmd_key
                j += subtree_size(tokens, j, r);   // cmd_val
            }
            i += 1 + subtree_size(tokens, i + 1, r);

        } else {
            // Unknown key: skip key + value
            i += 1 + subtree_size(tokens, i + 1, r);
        }
    }

    free(js);
    return true;
}

// ---------------------------------------------------------------------------
// Scheme bindings
// ---------------------------------------------------------------------------

sexp scm_record_command_usage(sexp ctx, sexp self, sexp n, sexp sym) {
    if (!sexp_symbolp(sym)) return SEXP_VOID;
    const char *name = sexp_string_data(sexp_symbol_to_string(ctx, sym));
    state_io_record_command(G, name);
    return SEXP_VOID;
}

void state_io_update_recent_project(AppState *state, const char *path) {
    int64_t now = (int64_t)time(NULL);

    for (int i = 0; i < state->recent_projects_count; i++) {
        if (strcmp(state->recent_projects[i].path, path) == 0) {
            state->recent_projects[i].last_opened = now;
            return;
        }
    }

    if (state->recent_projects_count < RECENT_PROJECTS_MAX) {
        RecentProject *rp = &state->recent_projects[state->recent_projects_count++];
        strncpy(rp->path, path, sizeof(rp->path) - 1);
        rp->path[sizeof(rp->path) - 1] = '\0';
        rp->last_opened = now;
    } else {
        int oldest = 0;
        for (int i = 1; i < state->recent_projects_count; i++) {
            if (state->recent_projects[i].last_opened <
                    state->recent_projects[oldest].last_opened)
                oldest = i;
        }
        strncpy(state->recent_projects[oldest].path, path,
                sizeof(state->recent_projects[oldest].path) - 1);
        state->recent_projects[oldest].path[sizeof(state->recent_projects[oldest].path) - 1] = '\0';
        state->recent_projects[oldest].last_opened = now;
    }
}

// (%update-recent-project! path) — Scheme binding
sexp scm_update_recent_project(sexp ctx, sexp self, sexp n, sexp spath) {
    (void)ctx; (void)self; (void)n;
    if (!sexp_stringp(spath)) return SEXP_VOID;
    state_io_update_recent_project(G, sexp_string_data(spath));
    return SEXP_VOID;
}

// (%chdir path) — change the process working directory.
// Returns #t on success, #f on failure.
sexp scm_chdir(sexp ctx, sexp self, sexp n, sexp spath) {
    (void)ctx; (void)self; (void)n;
    if (!sexp_stringp(spath)) return SEXP_FALSE;
    if (chdir(sexp_string_data(spath)) == 0)
        return SEXP_TRUE;
    return SEXP_FALSE;
}

// (%open-recent-project! n) — open the nth (1-based) most recently accessed
// project in display order (sorted by last_opened desc). Returns #f silently
// if n is out of range or chdir fails.
sexp scm_open_recent_project(sexp ctx, sexp self, sexp n, sexp sidx) {
    (void)ctx; (void)self; (void)n;
    if (!sexp_fixnump(sidx)) return SEXP_FALSE;
    int idx = (int)sexp_unbox_fixnum(sidx);
    AppState *state = G;
    int count = state->recent_projects_count;
    if (idx < 1 || idx > count) return SEXP_FALSE;

    int order[RECENT_PROJECTS_MAX];
    for (int i = 0; i < count; i++) order[i] = i;
    for (int i = 1; i < count; i++) {
        int tmp = order[i], j = i - 1;
        while (j >= 0 && state->recent_projects[order[j]].last_opened <
                         state->recent_projects[tmp].last_opened) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = tmp;
    }

    const char *path = state->recent_projects[order[idx - 1]].path;
    if (chdir(path) != 0) return SEXP_FALSE;
    state_io_update_recent_project(state, path);
    static char msg[PATH_MAX + 32];
    snprintf(msg, sizeof(msg), "Opened project %s", path);
    message_echo(msg);
    return SEXP_TRUE;
}
