#pragma once

#include <stdint.h>

// Byte count of the UTF-8 sequence starting at s[0].
static inline int utf8_seq_len_fwd(const char *s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80)           return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    return 4;
}

// Byte count of the UTF-8 sequence whose last byte is at s[-1].
// Walks back over continuation bytes (0x80–0xBF).
static inline int utf8_seq_len_back(const char *s) {
    int n = 0;
    do { n++; s--; } while ((*s & 0xC0) == 0x80);
    return n;
}

// Encodes codepoint cp into out[]; returns byte count.
static inline int utf8_encode(uint32_t cp, char out[4]) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}
