#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"

void search_session_free(SearchSession *s) {
    free(s->matches);
    s->matches   = NULL;
    s->match_cap = 0;
}

void search_session_recompute(SearchSession *s, const char *text, size_t text_len) {
    s->match_count = 0;
    if (s->query_len == 0 || text_len == 0) return;

    const char *p   = text;
    const char *end = text + text_len;
    while (p < end) {
        const char *hit = memmem(p, (size_t)(end - p), s->query, s->query_len);
        if (!hit) break;

        if (s->match_count >= s->match_cap) {
            size_t new_cap = s->match_cap ? s->match_cap * 2 : 64;
            SearchMatch *nm = realloc(s->matches, new_cap * sizeof(SearchMatch));
            if (!nm) break;
            s->matches   = nm;
            s->match_cap = new_cap;
        }
        s->matches[s->match_count++] = (SearchMatch){
            .start = (size_t)(hit - text),
            .end   = (size_t)(hit - text) + s->query_len
        };
        p = hit + 1;
    }

    // Set active_match_index to first match at or after the search origin.
    s->active_match_index = 0;
    for (size_t i = 0; i < s->match_count; i++) {
        if (s->matches[i].start >= s->point) {
            s->active_match_index = i;
            break;
        }
    }
    // All matches are before the origin; wraps to 0 (first match).

    if (s->match_count == 0)
        snprintf(s->count_str, sizeof(s->count_str), "0/0");
    else
        snprintf(s->count_str, sizeof(s->count_str), "%zu/%zu",
                 s->active_match_index + 1, s->match_count);
}

size_t search_session_next_match(SearchSession *s) {
    if (s->match_count == 0) return 0;
    s->active_match_index = (s->active_match_index + 1) % s->match_count;
    snprintf(s->count_str, sizeof(s->count_str), "%zu/%zu",
             s->active_match_index + 1, s->match_count);
    return s->matches[s->active_match_index].start;
}

size_t search_session_prev_match(SearchSession *s) {
    if (s->match_count == 0) return 0;
    s->active_match_index =
        (s->active_match_index + s->match_count - 1) % s->match_count;
    snprintf(s->count_str, sizeof(s->count_str), "%zu/%zu",
             s->active_match_index + 1, s->match_count);
    return s->matches[s->active_match_index].start;
}
