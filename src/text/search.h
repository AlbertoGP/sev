#pragma once

#include <stddef.h>
#include <stdbool.h>

struct Buffer;

typedef struct {
    size_t start;
    size_t end;
} SearchMatch;

typedef struct {
    struct Buffer *query_buf;  // owns the query text and cursor position
    float          text_scroll; // horizontal scroll offset (for future use)
    SearchMatch   *matches;
    size_t         match_count;
    size_t         match_cap;
    size_t         active_match_index;
    size_t         point;    // cursor position when search was opened
    bool           active;   // session exists: highlights + n/N work
    bool           bar_open; // search bar UI is visible
    char           count_str[32]; // pre-formatted "N/M" or "no matches"; stable memory for Clay
} SearchSession;

// Free dynamically allocated match storage and query buffer.
void search_session_free(SearchSession *s);

// Recompute all matches from buffer text. Updates active_match_index to
// the first match at or after s->point. Call whenever query or text changes.
void search_session_recompute(SearchSession *s, const char *text, size_t text_len,
                               const char *query, size_t query_len);

// Advance active_match_index (wraps). Returns start offset of new active match.
size_t search_session_next_match(SearchSession *s);

// Retreat active_match_index (wraps). Returns start offset of new active match.
size_t search_session_prev_match(SearchSession *s);
