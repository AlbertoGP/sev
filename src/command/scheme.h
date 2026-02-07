#pragma once

#include "../state.h"

void scheme_init(AppState *state);
void scheme_eval_file(AppState *state, const char *path);
