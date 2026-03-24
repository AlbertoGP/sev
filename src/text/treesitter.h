#pragma once

#include <tree_sitter/api.h>
#include "gap.h"

typedef struct {
    TSParser         *parser;
    TSTree           *tree;
    const TSLanguage *language;
} TSState;

typedef struct Buffer Buffer;

void ts_buffer_init(Buffer *buf);
void ts_buffer_parse(Buffer *buf);
void ts_buffer_free(Buffer *buf);
