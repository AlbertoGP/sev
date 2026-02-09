#pragma once

#include <chibi/sexp.h>

#include "../state.h"

extern AppState *G;

#define SCM_CMD(cname, action) \
    sexp cname(sexp ctx, sexp self, sexp n) { \
        G->needs_redraw = true; \
        action; \
        return SEXP_VOID; \
    }
