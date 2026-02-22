// Vim-style jump list: ring buffer of cursor positions for C-o/C-i navigation.

#pragma once

#include <stdbool.h>

#include "location.h"

#define JUMP_LIST_MAX 100

typedef struct {
    char    *buf_name;   // strdup'd buffer name
    Location point;
    char    *filename;   // strdup'd, may be NULL
} Jump;

typedef struct {
    Jump list[JUMP_LIST_MAX];
    int  length;         // raw counter, starts 0, always increments
    int  current_index;  // index into list[] of "current" position
} JumpList;

// Returns the index of the oldest valid entry in the list.
static inline int jump_list_start(const JumpList *jl) {
    return jl->length <= JUMP_LIST_MAX ? 0 : jl->length % JUMP_LIST_MAX;
}

void jump_list_push(JumpList *jl, Jump j);
// Returns true if navigation occurred.
bool jump_list_backward(JumpList *jl);
bool jump_list_forward(JumpList *jl);
void jump_list_free(JumpList *jl);
