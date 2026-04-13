// Line-based text diff. Public API is in diff.h; everything below is
// internal to this translation unit.
//
// The divide-and-conquer LCS core (optimalLCS / longestCommonSequence /
// diff_step / diff_all / appendTriple / expandEdit) is taken from the
// fossil / FreeBSD `diff` implementation. Only the input-preparation and
// public surface have been reworked for this codebase:
//
//   - Inputs are (const char *, size_t) spans, not nul-terminated strings.
//   - DLines can be built from an existing LineTable, skipping the
//     newline-scanning pass for the buffer side.
//   - Results are returned as a DiffResult with explicit line counts, and
//     the caller can collapse them into per-line change markers via
//     diff_line_markers().

#include "diff.h"
#include "line.h"

#include "buffer.h"
#include "buffer_type.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Max line length: 2^13 = 8192 bytes. Longer lines trigger a binary-file
// reject. The lower LENGTH_MASK_SZ bits of DLine.h encode the line length.
#define LENGTH_MASK_SZ  13
#define LENGTH_MASK     ((1u<<LENGTH_MASK_SZ)-1u)

typedef struct DLine DLine;
struct DLine {
    const char  *z;      // pointer to the line's first byte (not nul-terminated)
    unsigned int h;      // hash: (raw_hash << LENGTH_MASK_SZ) | line_length
    unsigned short n;    // byte count
    unsigned int iNext;  // 1 + index of next DLine in the same hash bucket, or 0
    unsigned int iHash;  // 1 + index of first DLine in this bucket, or 0
};

typedef struct DContext DContext;
struct DContext {
    int *aEdit;
    int  nEdit;
    int  nEditAlloc;
    DLine *aFrom;
    int    nFrom;
    DLine *aTo;
    int    nTo;
    int  (*same_fn)(const DLine *, const DLine *);
};

static int same_dline(const DLine *pA, const DLine *pB) {
    return pA->h == pB->h && memcmp(pA->z, pB->z, pA->h & LENGTH_MASK) == 0;
}

static int minInt(int a, int b) { return a < b ? a : b; }

// Compute the hash/length word for a single line and insert it into the
// hash chain at index `idx`. Returns false if the line exceeds LENGTH_MASK.
static bool dline_init(DLine *a, int nLine, int idx,
                       const char *z, size_t len) {
    if (len > LENGTH_MASK) return false;

    unsigned int h = 0;
    for (size_t x = 0; x < len; x++) {
        h = h ^ (h << 2) ^ (unsigned char)z[x];
    }
    a[idx].z     = z;
    a[idx].n     = (unsigned short)len;
    a[idx].h     = (h << LENGTH_MASK_SZ) | (unsigned int)len;

    unsigned int bucket = a[idx].h % (unsigned int)nLine;
    a[idx].iNext        = a[bucket].iHash;
    a[bucket].iHash     = (unsigned int)idx + 1;
    return true;
}

// Build a DLine array by scanning `text` for newline boundaries. Matches
// the LineTable convention: a trailing '\n' creates an empty final line,
// so "foo\n" is two lines ("foo", "") just like line.c would produce.
// Returns NULL if any line is longer than LENGTH_MASK (binary reject) or
// on OOM.
static DLine *dlines_from_text(const char *text, size_t len, int *pnLine) {
    int nLine = 1;
    {
        size_t line_len = 0;
        for (size_t i = 0; i < len; i++) {
            if (text[i] == '\n') {
                if (line_len > LENGTH_MASK) return NULL;
                nLine++;
                line_len = 0;
            } else {
                line_len++;
            }
        }
        if (line_len > LENGTH_MASK) return NULL;
    }

    DLine *a = calloc((size_t)nLine, sizeof(DLine));
    if (!a) return NULL;

    int idx = 0;
    size_t line_start = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') {
            if (!dline_init(a, nLine, idx, text + line_start, i - line_start)) {
                free(a);
                return NULL;
            }
            idx++;
            line_start = i + 1;
        }
    }
    // Trailing line (content after the last '\n', or the sole line if none).
    if (idx < nLine) {
        if (!dline_init(a, nLine, idx, text + line_start, len - line_start)) {
            free(a);
            return NULL;
        }
    }

    *pnLine = nLine;
    return a;
}

// Build a DLine array directly from an existing LineTable. No newline
// scanning needed: the table already knows each line's start/end offsets.
static DLine *dlines_from_line_table(const char *text, size_t len,
                                     const LineTable *lt, int *pnLine) {
    if (!lt || lt->count == 0) return NULL;

    int nLine = (int)lt->count;
    DLine *a = calloc((size_t)nLine, sizeof(DLine));
    if (!a) return NULL;

    for (int i = 0; i < nLine; i++) {
        size_t start = lt->lines[i].start;
        size_t end   = lt->lines[i].end;
        // LineTable end offsets include the trailing '\n' for all lines
        // except the final one; trim it so the hashed content matches the
        // raw-text builder.
        if (end > start && end <= len && text[end - 1] == '\n') end--;
        if (start > len || end > len || end < start) { free(a); return NULL; }
        if (!dline_init(a, nLine, i, text + start, end - start)) {
            free(a);
            return NULL;
        }
    }

    *pnLine = nLine;
    return a;
}

// ---------- core LCS algorithm (unchanged from fossil/FreeBSD) ----------

static void optimalLCS(
    DContext *p,
    int iS1, int iE1,
    int iS2, int iE2,
    int *piSX, int *piEX,
    int *piSY, int *piEY
){
    int mxLength = 0;
    int i, j, k;
    int iSXb = iS1;
    int iSYb = iS2;

    for (i = iS1; i < iE1 - mxLength; i++) {
        for (j = iS2; j < iE2 - mxLength; j++) {
            if (!p->same_fn(&p->aFrom[i], &p->aTo[j])) continue;
            if (mxLength && !p->same_fn(&p->aFrom[i + mxLength], &p->aTo[j + mxLength])) {
                continue;
            }
            k = 1;
            while (i + k < iE1 && j + k < iE2 &&
                   p->same_fn(&p->aFrom[i + k], &p->aTo[j + k])) {
                k++;
            }
            if (k > mxLength) {
                iSXb = i;
                iSYb = j;
                mxLength = k;
            }
        }
    }
    *piSX = iSXb;
    *piEX = iSXb + mxLength;
    *piSY = iSYb;
    *piEY = iSYb + mxLength;
}

static void longestCommonSequence(
    DContext *p,
    int iS1, int iE1,
    int iS2, int iE2,
    int *piSX, int *piEX,
    int *piSY, int *piEY
){
    int i, j, k, n;
    DLine *pA, *pB;
    int iSX, iSY, iEX, iEY;
    int skew = 0;
    int dist = 0;
    int mid;
    int iSXb, iSYb, iEXb, iEYb;
    int iSXp, iSYp, iEXp, iEYp;
    int64_t bestScore;
    int64_t score;
    int span;

    span = (iE1 - iS1) + (iE2 - iS2);
    bestScore = -10000;
    iSXb = iSXp = iS1;
    iEXb = iEXp = iS1;
    iSYb = iSYp = iS2;
    iEYb = iEYp = iS2;
    mid = (iE1 + iS1) / 2;
    for (i = iS1; i < iE1; i++) {
        int limit = 0;
        j = p->aTo[p->aFrom[i].h % p->nTo].iHash;
        while (j > 0
            && (j - 1 < iS2 || j >= iE2 || !p->same_fn(&p->aFrom[i], &p->aTo[j - 1]))
        ) {
            if (limit++ > 10) {
                j = 0;
                break;
            }
            j = p->aTo[j - 1].iNext;
        }
        if (j == 0) continue;
        if (i < iEXb && j >= iSYb && j < iEYb) continue;
        if (i < iEXp && j >= iSYp && j < iEYp) continue;
        iSX = i;
        iSY = j - 1;
        pA = &p->aFrom[iSX - 1];
        pB = &p->aTo[iSY - 1];
        n = minInt(iSX - iS1, iSY - iS2);
        for (k = 0; k < n && p->same_fn(pA, pB); k++, pA--, pB--) {}
        iSX -= k;
        iSY -= k;
        iEX = i + 1;
        iEY = j;
        pA = &p->aFrom[iEX];
        pB = &p->aTo[iEY];
        n = minInt(iE1 - iEX, iE2 - iEY);
        for (k = 0; k < n && p->same_fn(pA, pB); k++, pA++, pB++) {}
        iEX += k;
        iEY += k;
        skew = (iSX - iS1) - (iSY - iS2);
        if (skew < 0) skew = -skew;
        dist = (iSX + iEX) / 2 - mid;
        if (dist < 0) dist = -dist;
        score = (iEX - iSX) * (int64_t)span - (skew + dist);
        if (score > bestScore) {
            bestScore = score;
            iSXb = iSX;
            iSYb = iSY;
            iEXb = iEX;
            iEYb = iEY;
        } else if (iEX > iEXp) {
            iSXp = iSX;
            iSYp = iSY;
            iEXp = iEX;
            iEYp = iEY;
        }
    }
    if (iSXb == iEXb && (iE1 - iS1) * (iE2 - iS2) < 400) {
        optimalLCS(p, iS1, iE1, iS2, iE2, piSX, piEX, piSY, piEY);
    } else {
        *piSX = iSXb;
        *piSY = iSYb;
        *piEX = iEXb;
        *piEY = iEYb;
    }
}

static void expandEdit(DContext *p, int nEdit) {
    p->aEdit = realloc(p->aEdit, nEdit * sizeof(int));
    p->nEditAlloc = nEdit;
}

static void appendTriple(DContext *p, int nCopy, int nDel, int nIns) {
    if (p->nEdit >= 3) {
        if (p->aEdit[p->nEdit - 1] == 0) {
            if (p->aEdit[p->nEdit - 2] == 0) {
                p->aEdit[p->nEdit - 3] += nCopy;
                p->aEdit[p->nEdit - 2] += nDel;
                p->aEdit[p->nEdit - 1] += nIns;
                return;
            }
            if (nCopy == 0) {
                p->aEdit[p->nEdit - 2] += nDel;
                p->aEdit[p->nEdit - 1] += nIns;
                return;
            }
        }
        if (nCopy == 0 && nDel == 0) {
            p->aEdit[p->nEdit - 1] += nIns;
            return;
        }
    }
    if (p->nEdit + 3 > p->nEditAlloc) {
        expandEdit(p, p->nEdit * 2 + 15);
        if (p->aEdit == 0) return;
    }
    p->aEdit[p->nEdit++] = nCopy;
    p->aEdit[p->nEdit++] = nDel;
    p->aEdit[p->nEdit++] = nIns;
}

static void diff_step(DContext *p, int iS1, int iE1, int iS2, int iE2) {
    int iSX, iEX, iSY, iEY;

    if (iE1 <= iS1) {
        if (iE2 > iS2) appendTriple(p, 0, 0, iE2 - iS2);
        return;
    }
    if (iE2 <= iS2) {
        appendTriple(p, 0, iE1 - iS1, 0);
        return;
    }

    longestCommonSequence(p, iS1, iE1, iS2, iE2, &iSX, &iEX, &iSY, &iEY);

    if (iEX > iSX) {
        diff_step(p, iS1, iSX, iS2, iSY);
        if (iEX > iSX) appendTriple(p, iEX - iSX, 0, 0);
        diff_step(p, iEX, iE1, iEY, iE2);
    } else {
        appendTriple(p, 0, iE1 - iS1, iE2 - iS2);
    }
}

static void diff_all(DContext *p) {
    int mnE, iS, iE1, iE2;

    iE1 = p->nFrom;
    iE2 = p->nTo;
    while (iE1 > 0 && iE2 > 0 && p->same_fn(&p->aFrom[iE1 - 1], &p->aTo[iE2 - 1])) {
        iE1--;
        iE2--;
    }
    mnE = iE1 < iE2 ? iE1 : iE2;
    for (iS = 0; iS < mnE && p->same_fn(&p->aFrom[iS], &p->aTo[iS]); iS++) {}

    if (iS > 0) appendTriple(p, iS, 0, 0);
    diff_step(p, iS, iE1, iS, iE2);
    if (iE1 < p->nFrom) appendTriple(p, p->nFrom - iE1, 0, 0);

    expandEdit(p, p->nEdit + 3);
    if (p->aEdit) {
        p->aEdit[p->nEdit++] = 0;
        p->aEdit[p->nEdit++] = 0;
        p->aEdit[p->nEdit++] = 0;
    }
}

// ---------- public API ----------

static DiffResult run_diff(DLine *aFrom, int nFrom, DLine *aTo, int nTo) {
    DContext c;
    memset(&c, 0, sizeof(c));
    c.same_fn = same_dline;
    c.aFrom = aFrom; c.nFrom = nFrom;
    c.aTo   = aTo;   c.nTo   = nTo;

    diff_all(&c);

    DiffResult r = { .edits = c.aEdit, .n_from = nFrom, .n_to = nTo };
    return r;
}

DiffResult diff_text(const char *from, size_t from_len,
                     const char *to,   size_t to_len) {
    DiffResult empty = { 0 };
    int nFrom = 0, nTo = 0;
    DLine *aFrom = dlines_from_text(from, from_len, &nFrom);
    DLine *aTo   = dlines_from_text(to,   to_len,   &nTo);
    if (!aFrom || !aTo) {
        free(aFrom);
        free(aTo);
        return empty;
    }
    DiffResult r = run_diff(aFrom, nFrom, aTo, nTo);
    free(aFrom);
    free(aTo);
    return r;
}

DiffResult diff_text_vs_line_table(const char *from,    size_t from_len,
                                   const char *to_text, size_t to_len,
                                   const LineTable *to_lt) {
    DiffResult empty = { 0 };
    int nFrom = 0, nTo = 0;
    DLine *aFrom = dlines_from_text(from, from_len, &nFrom);
    DLine *aTo   = dlines_from_line_table(to_text, to_len, to_lt, &nTo);
    if (!aFrom || !aTo) {
        free(aFrom);
        free(aTo);
        return empty;
    }
    DiffResult r = run_diff(aFrom, nFrom, aTo, nTo);
    free(aFrom);
    free(aTo);
    return r;
}

void diff_result_free(DiffResult *r) {
    if (!r) return;
    free(r->edits);
    r->edits = NULL;
    r->n_from = r->n_to = 0;
}

uint8_t *diff_line_markers(const DiffResult *result) {
    if (!result || !result->edits || result->n_to <= 0) return NULL;

    uint8_t *out = calloc((size_t)result->n_to, sizeof(uint8_t));
    if (!out) return NULL;

    int cursor = 0;
    for (int *e = result->edits; ; e += 3) {
        int nCopy = e[0], nDel = e[1], nIns = e[2];
        if (nCopy == 0 && nDel == 0 && nIns == 0) break;

        cursor += nCopy;

        int n_mod = (nDel < nIns) ? nDel : nIns;
        for (int k = 0; k < n_mod && cursor + k < result->n_to; k++) {
            out[cursor + k] |= LINE_CHANGE_MODIFIED;
        }
        int n_add = nIns - n_mod;
        for (int k = 0; k < n_add && cursor + n_mod + k < result->n_to; k++) {
            out[cursor + n_mod + k] |= LINE_CHANGE_ADDED;
        }
        cursor += nIns;

        // Deletion squares attach to surviving line boundaries for pure
        // deletions only. Mixed triples (d > i > 0) rely on the modified
        // stripes to convey the change.
        if (nDel > 0 && nIns == 0) {
            if (cursor > 0) out[cursor - 1]   |= LINE_CHANGE_DEL_BELOW;
            if (cursor < result->n_to) out[cursor] |= LINE_CHANGE_DEL_ABOVE;
        }
    }
    return out;
}

uint8_t *buffer_get_line_markers(Buffer *buf, int *out_n_lines) {
    if (out_n_lines) *out_n_lines = 0;
    if (!buf || !buf->saved_text || !buf->is_modified) return NULL;

    char *current = buffer_text(buf);
    if (!current) return NULL;
    size_t current_len = strlen(current);

    DiffResult r = diff_text_vs_line_table(
        buf->saved_text, buf->saved_len,
        current, current_len,
        &buf->lt);

    free(current);

    if (!r.edits) return NULL;

    uint8_t *markers = diff_line_markers(&r);
    if (out_n_lines) *out_n_lines = r.n_to;
    diff_result_free(&r);
    return markers;
}
