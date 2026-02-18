#pragma once

#include <stddef.h>

typedef struct { char *data; size_t len; } ByteString;

typedef enum { SHAPE_CHARWISE } SelectionShape;  // stub for future shapes

typedef struct { ByteString data; SelectionShape shape; } Register;

#define REGISTER_COUNT   27   // a-z (0-25) + unnamed " (26)
#define REGISTER_UNNAMED 26

void register_write(Register regs[], char name, const char *data, size_t len);
void register_append(Register regs[], char name, const char *data, size_t len);
const ByteString *register_read(Register regs[], char name);
void register_free_all(Register regs[]);
