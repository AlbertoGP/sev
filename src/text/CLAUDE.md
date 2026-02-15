# src/text/ Subsystem

## Purpose

Text model layer: gap buffer storage, logical line tracking, marks/selections, buffer lifecycle, buffer-local variables, and the echo area. All text mutation flows through this layer. Display reads it but never mutates it.

## Files

### gap.c / gap.h — Gap Buffer
- `GapBuf` struct: `buffer[0..point)` = front text, `buffer[gap_end..size)` = back text
- `gb_insert(buf, c)` — insert at point; grows by doubling if full
- `gb_delete(buf)` / `gb_backspace(buf)` — remove forward/backward; shrinks to half if <25% used
- `gb_text(buf)` — allocates contiguous copy (caller must free)
- `gb_char_at(buf, idx)` — read char at logical index, skipping gap
- `gb_point_get/set()` — query/move gap position
- `gb_front(buf)`, `gb_back(buf)`, `gb_used(buf)` — size helpers
- Min size: 1024 bytes

### line.c / line.h — Logical Line Table
- `Line` struct: `line_id` (stable, never reused), `start`/`end` byte offsets, `version` (bumped on change)
- `LineTable`: growable array of `Line` + `next_line_id` counter
- `line_insert_char(lt, pos, ch)` — newline splits line and inserts new entry; non-newline extends current line's end; updates all subsequent line offsets
- `line_backspace_char(lt, pos, ch)` / `line_delete_char(lt, pos, ch)` — merge lines on newline removal; adjust offsets
- `line_index_at(lt, pos)` — binary search for line containing byte position
- Grows by doubling, shrinks at <25% utilization; initial capacity 8

### mark.c / mark.h — Marks and Selection
- `Mark` = `Location` = `{ size_t pos }`
- 26 named marks (`'a'`–`'z'`), plus `'<'` / `'>'` for selection boundaries
- `mark_set_to_point(c)` / `point_to_mark(mark)` / `swap_point_and_mark(mark)`
- `SelectMode`: `SELECT_NONE`, `SELECT_REGULAR` (char), `SELECT_LINE`, `SELECT_RECTANGLE` (column block)
- Scheme bindings: `scm_mark_set_to_point`, `scm_mark_position`, `scm_point_to_named_mark`, `scm_select_mode_get/set`, `scm_swap_point_and_mark`

### location.h — Location Type
- `typedef struct { size_t pos; } Location` — byte offset into buffer
- `Mark` is a typedef alias for `Location`

### buffer.c / buffer.h / buffer_type.h — Buffer Management
- `Buffer` struct: gap buffer (`contents`), `LineTable`, point, marks, selection state, modes, local keymap, `VarTable`, scale factor, name, filename, modified flag
- `BufferList`: doubly-linked list of buffers + `current` pointer
- **Lifecycle**: `buffer_list_init()` creates `*scratch*`; `buffer_create(name)` appends; `buffer_delete(buf)` removes (recreates `*scratch*` if list would be empty)
- **Text ops**: `insert_char()`, `insert_string()`, `delete_chars(count)` — each updates gap buffer, line table, num_chars/num_lines, and sets `needs_redraw`
- **Point movement**: `point_move(count)`, `point_move_by_line(count)`, `point_to_line_start/end()`
- **Column tracking**: `col_saved` preserved across vertical moves; `set_column(col, round)` for horizontal placement
- **Queries**: `buffer_text()` (allocated copy), `buf_char_at()`, `get_line_count()`, `get_char_count()`, `buffer_get_line_table()`
- **Modes**: `buffer_set_major_mode()`, `buffer_enable/disable_minor_mode()`, `buffer_has_minor_mode()`
- `buffer_type.h` has the full struct definition; `buffer.h` has the public API

### var.c / var.h — Buffer-Local Variables
- `VarTable`: singly-linked list of `VarEntry { sexp key, sexp value }`
- `vartable_get(vt, key, default)` / `vartable_set(vt, key, value)` — O(n) lookup, no duplicates
- Scheme bindings: `(%setq-local name val)`, `(%getq-local name default)`
- Keys are Scheme symbols (interned, not copied); values are arbitrary sexp

### message.c / message.h — Echo Area
- Static 2048-byte buffer + `Clay_String` shared with UI renderer
- `message_send(msg)` — set message; `message_clear()` — blank it
- Scheme bindings: `(message msg)`, `(message-clear)`

## Key Invariants

### Gap Buffer
- `point <= gap_end <= size` always
- Logical text = `buffer[0..point) ++ buffer[gap_end..size)`
- After any gap move, `buffer->point.pos` must be synced via `update_point()`

### Line Table
- Lines are consecutive, non-overlapping: `lines[i].end == lines[i+1].start`
- `lines[0].start == 0` always
- Every `'\n'` creates a line boundary
- After insert/delete of any char, all subsequent lines' start/end offsets must be adjusted
- `line_id` is stable across edits (never recycled); `version` increments on any change to that line

### Buffer
- `num_chars == gb_used(contents)` after every edit
- `num_lines == lt.count` after every edit
- `point.pos` in range `[0, num_chars]`
- `cur_line` in range `[1, num_lines]`
- Exactly one major mode per buffer (or NULL); minor modes form a linked list
- `buffer_set_current()` must be called when switching active buffer (display reads `current`)

### Marks
- Mark positions can become stale after edits — marks are not auto-adjusted
- Named marks `'a'`–`'z'` are per-buffer; `'<'`/`'>'` are selection bounds

## Boundaries

- **Display reads, never writes**: `buffer_get_line_table()`, `buf_char_at()`, `buffer_text()`, point/mark positions, select mode
- **Commands mutate via public API**: `insert_char()`, `delete_chars()`, `point_move()`, mark operations
- **Scheme accesses through C bindings** defined at the bottom of `buffer.c`, `mark.c`, `var.c`, `message.c`
- **No direct gap buffer access** outside `buffer.c` — all text ops go through buffer API

## Common Modification Workflows

### Adding a new text operation
1. Implement in `buffer.c` using `gb_*` functions on `buf->contents`
2. Call `line_insert_char()` or `line_delete_char()` to keep line table in sync
3. Update `buf->num_chars`, `buf->num_lines` if they change
4. Set `G->needs_redraw = true`
5. Add Scheme binding (`scm_*` function) and register in `src/command/scheme.c`

### Adding a new mark type
1. Add field to `Buffer` struct in `buffer_type.h`
2. Handle the new mark character in `mark_get()` switch in `mark.c`
3. Initialize in `buffer_create()`

### Adding a new buffer-local variable
- No C changes needed — use `(set-local! 'name value)` from Scheme
- To read from C: `vartable_get(buffer_get_locals(buf), interned_symbol, default)`

### Adding a new query exposed to Scheme
1. Write a `scm_*` function in `buffer.c` that reads buffer state
2. Register it as a foreign function in `src/command/scheme.c` `scheme_init()`
3. Optionally wrap in Scheme (e.g., `built-in.scm`) with `defcommand`
