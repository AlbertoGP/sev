// The buffer / buffer list data structure, and associated functions
// that manipulate text buffers.

#pragma once

#include "location.h"
#include "mark.h"
#include "mode.h"
#include <stddef.h>

typedef struct Buffer Buffer;

// Initialises the global buffer list.
bool buffer_list_init(void);
// Frees the global buffer list.
void buffer_list_quit(void);

// Takes a name and creates an empty buffer with that name.
// Note: no two buffers may share the same name.
Buffer *buffer_create(const char *name);
// Removes all characters and marks from the specified buffer.
bool buffer_clear(Buffer *buf);
// Deletes the specified buffer. If the specified buffer is the current one,
// the next buffer in the chain becomes the current one. If no buffers are
// left, the initial "scratch" buffer is automatically recreated.
bool buffer_delete(Buffer *buf);
// Returns a pointer to the currently selected buffer.
Buffer *buffer_get_current(void);
// Sets the current buffer to the one specified.
bool buffer_set_current(Buffer *buf);
// Sets the current buffer to the next one in the buffer list.
// Returns the name of the newly selected buffer.
char *buffer_set_next(void);
// Sets the current buffer to the previous one in the buffer list.
// Returns the name of the newly selected buffer.
char *buffer_set_prev(void);
// Changes the name of the current buffer to the specified string.
bool buffer_set_name(const char *name);
// Returns the name of the current buffer.
char *buffer_get_name(Buffer *buf);
// Returns a pointer to buffer matching name.
Buffer *buffer_get_by_name(const char *name);

// Sets the point to the specified location in the current buffer.
bool point_set(Location loc);
// Moves the point forward (if count is positive) or backward (if negative)
// by abs(count) characters.
bool point_move(int count);
// Moves count lines up or down (down if count is positive, up if negative).
// Sets the column to as close to col_saved as possible.
// Does not change col_saved.
bool point_move_by_line(int count);
// Returns the current location in the specified buf.
Location point_get(Buffer *buf);
// Returns the number of the line that the the point is on in the specified buffer.
size_t point_get_line(Buffer *buf);
// Returns the location of the start of the buffer.
Location buffer_start(void);
// Returns the location of the end of the buffer.
Location buffer_end(void);

// Returns:
//  1 if location loc1 is after loc2,
//  0 if they are the same,
//  -1 if loc2 is after loc1.
int compare_locations(Location loc1, Location loc2);
// Accepts a location and returns the number of characters between the location
// and the beginning of the buffer.
size_t location_to_count(Location loc);
// Accepts a nonnegative count and converts it to the corresponding location.
Location count_to_location(size_t count);

// Returns the character after the point.
// Results are undefined if the point is at the end of the buffer.
char get_char(void);
// Returns up to count characters from the current buffer,
// starting from the point.
// It will return fewer than count characters if the end of the buffer is encountered.
// Requires a pointer to char to store the result.
void get_string(char *string, size_t count);
// Returns the number of characters in the specified buffer.
size_t get_char_count(Buffer *buf);
// Returns the number of lines in the specified buffer.
// It is undefined whether or not one counts an incomplete last line.
size_t get_line_count(Buffer *buf);

// Returns the file name that is currently associated with the current buffer.
// The size of the buffer allocated for the returned filename is passed in.
void get_file_name(char *file_name, int size);
// Sets the file name for the current buffer.
bool set_file_name(char *file_name);
// Writes the current buffer to its currently associated file name, making any
// required conversions between the internal and external representations.
// The is_modified flag is cleared and file_time is updated to current time.
bool buffer_write(void);
// Clears the buffer and reads the currently associated file name into it,
// making any required conversions between the external and internal representations.
// The is_modified flag is cleared and file_time is updated to current time.
bool buffer_read(void);
// Inserts the contents of the specified file into the buffer at the point,
// making any required conversions between the external and internal representations.
// The modified flag is set if the file was not empty.
bool buffer_insert(char *file_name);
// Returns true if the file has been changed since it was last read or written.
bool is_file_changed(void);
// Sets the state of the is_modified flag to supplied value. Most often used
// manually to clear the modification flag in the case where the user is sure
// that any changes should be discarded. This flag is set by any
// insertion, deletion or other change to the buffer.
void set_modified(bool is_modified);
// Returns the modification flag for the current buffer.
bool get_modified(void);

// Inserts a single character at the point.
// The point is placed after the inserted character.
void insert_char(Buffer *buf, char c);
// Inserts a string of characters at the point.
// The point is placed after the last character in the string.
void insert_string(Buffer *buf, const char *string);
// Replaces the character directly after the point with another.
void replace_char(char c);
// Replaces a string as if replace_char had been called on each of its characters.
void replace_string(char *string);
// Deletes the specified number of characters from the buffer.
// Characters are removed after the point if count is positive,
// or before the point if count is negative.
// If the specified count extends beyond the start or end of the buffer,
// the excess is ignored.
bool delete_chars(Buffer *buf, int count);
// Removes all characters between the point and the mark.
bool delete_region(Mark *mark);
// Copies the characters between the point and the mark to the specified buffer,
// inserting them at the point.
bool copy_region(char *buffer_name, Mark *mark);
// Searches forward for the first occurence of string after the point.
// If found, leave the point at the end of found string.
// If not found, the point is not moved.
bool search_forward(char *string);
// Searches backward for the first occurence of string before the point.
// If found, leave the point at the start of found string.
// If not found, the point is not moved.
bool search_backward(char *string);
// Returns true if the string matches the contents of the buffer from the point.
// Essentially, is the point at the start of a matching string.
bool is_a_match(char *string);
// Searches the buffer forwards starting from the point for the first occurence of
// any character in the supplied string, leaving the point before first match.
// Unlike search_* routines, the point is left at the end of the buffer
// if no match is found.
bool find_first_in_forward(char *string);
// Searches the buffer backwards starting from the point for the first occurence of
// any character in the supplied string, leaving the point before first match.
// Unlike search_* routines, the point is left at the start of the buffer
// if no match is found.
bool find_first_in_backward(char *string);
// Searches the buffer forwards starting from the point for the first occurence of
// any character not in the supplied string, leaving the point before first match.
// Unlike search_* routines, the point is left at the end of the buffer
// if no match is found.
bool find_first_not_in_forward(char *string);
// Searches the buffer backwards starting from the point for the first occurence of
// any character not in the supplied string, leaving the point before first match.
// Unlike search_* routines, the point is left at the start of the buffer
// if no match is found.
bool find_first_not_in_backward(char *string);

// Returns the zero-origin column that the point is in, after taking into account
// tab stops, variable-width characters, and other special cases, but *not*
// taking into account the screen width.
int get_column(Buffer *buf);
// Moves the point to the desired column, stopping at the end of a line if
// the line is not long enough.
// If the specified column cannot be reached exactly (due to tab stops,
// or other special cases), it uses the round flag.
// If round is true, the point is "rounded up" to the next available column.
// If round is false, the point is "rounded down".
void set_column(int column, bool round);

char *buffer_text(Buffer *buf);
char char_at_point(void);
char char_from_point(int n);
int buf_char_at(Buffer *buf, size_t index);
int buf_size(Buffer *buf);
void line_table_print(void);

void print_buffer_list(void);
