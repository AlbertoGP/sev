#pragma once

#include <stddef.h>

typedef struct { char *data; size_t len; } ByteString;

typedef enum { SHAPE_CHARWISE, SHAPE_LINEWISE, SHAPE_BLOCKWISE } SelectionShape;

typedef struct { ByteString data; SelectionShape shape; size_t block_width; } Register;

#define REGISTER_COUNT   27   // a-z (0-25) + unnamed " (26)
#define REGISTER_UNNAMED 26

void register_write(Register regs[], char name, const char *data, size_t len);
void register_append(Register regs[], char name, const char *data, size_t len);
const ByteString *register_read(Register regs[], char name);
void register_free_all(Register regs[]);
void register_set_shape(Register regs[], char name, SelectionShape shape);
SelectionShape register_get_shape(Register regs[], char name);
void register_set_block_width(Register regs[], char name, size_t width);
size_t register_get_block_width(Register regs[], char name);
