// The message area at the bottom of the screen's buffer + functions.
// Note that this message is always completely overwritten, hence the use
// of a regular, fixed-length array instead of a Buffer.

#include "../clay/clay.h"
#include <string.h>

#define MAX_MESSAGE_LENGTH 2048
static char message_buf[MAX_MESSAGE_LENGTH];

Clay_String message_string = {
    .chars = message_buf,
    .length = 0,
    .isStaticallyAllocated = true
};

void message_clear(void) {
    for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
        message_buf[i] = 0;
}

void message_send(const char* message) {
    message_clear();
    strncat(message_buf, message, MAX_MESSAGE_LENGTH - 1);
    message_string.length = strlen(message_buf);
}
