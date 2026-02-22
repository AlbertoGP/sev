#include <stdlib.h>
#include <string.h>

#include "jump_list.h"

void jump_list_push(JumpList *jl, Jump j) {
    int slot = jl->length % JUMP_LIST_MAX;
    // Free old strings if we are overwriting a ring slot.
    if (jl->length >= JUMP_LIST_MAX) {
        free(jl->list[slot].buf_name);
        free(jl->list[slot].filename);
    }
    jl->list[slot].buf_name = j.buf_name;
    jl->list[slot].point    = j.point;
    jl->list[slot].filename = j.filename;
    jl->length++;
    if (jl->length < JUMP_LIST_MAX)
        jl->current_index = jl->length;
    else
        jl->current_index = (jl->length - 1) % JUMP_LIST_MAX;
}

bool jump_list_backward(JumpList *jl) {
    if (jl->length == 0) return false;
    int start = jump_list_start(jl);
    if (jl->current_index == start) return false;

    if (jl->length < JUMP_LIST_MAX)
        jl->current_index--;
    else
        jl->current_index = (jl->current_index - 1 + JUMP_LIST_MAX) % JUMP_LIST_MAX;
    return true;
}

bool jump_list_forward(JumpList *jl) {
    if (jl->length == 0) return false;

    if (jl->length < JUMP_LIST_MAX) {
        if (jl->current_index == jl->length) return false;
        jl->current_index++;
        if (jl->current_index == jl->length) return false;
    } else {
        int next = (jl->current_index + 1) % JUMP_LIST_MAX;
        int start = jump_list_start(jl);
        if (next == start) return false;
        jl->current_index = next;
    }
    return true;
}

void jump_list_free(JumpList *jl) {
    int count = jl->length < JUMP_LIST_MAX ? jl->length : JUMP_LIST_MAX;
    for (int i = 0; i < count; i++) {
        free(jl->list[i].buf_name);
        free(jl->list[i].filename);
        jl->list[i].buf_name = NULL;
        jl->list[i].filename = NULL;
    }
    jl->length = 0;
    jl->current_index = 0;
}
