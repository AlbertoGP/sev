# src/text/ Subsystem

## Purpose

Text model layer. Owns all mutable text state: gap buffer storage, logical line tracking, cursor (point), marks/selections, registers, undo history, buffer lifecycle and buffer-local variables. Display reads this state but never mutates it.

## Files

- **gap.c / gap.h** — Gap buffer data structure. Efficient insert/delete at the cursor (gap). Grows by doubling, shrinks at <25% utilization, min 1024 bytes. No other module touches the gap buffer directly.
- **line.c / line.h** — Logical line table. Tracks byte ranges for each line; kept in sync with the gap buffer after every insert/delete. Each `Line` has a stable `line_id` (never recycled) and a `version` counter (bumped on change) — the display's vline cache uses these for incremental rebuilds. Supports binary search by byte position.
- **mark.c / mark.h** — Named marks (`'a'`–`'z'`) and selection boundaries (`'<'`/`'>'`). Marks are byte offsets and are **not auto-adjusted** after edits — callers must manage staleness. `SelectMode`: NONE, REGULAR (char), LINE, RECTANGLE.
- **location.h** — `Location` type (`{ size_t pos }`). `Mark` is a typedef alias.
- **register.c / register.h** — Named registers (`a`–`z`, `A`–`Z`, unnamed `"`). Store copied/cut text for yank/paste operations.
- **change.c / change.h** — Undo/redo history. Transactions (`Change`) record sequences of `EditOp`s (insert/delete) with point positions before/after and an optional Scheme repeat-info record. Manages undo stack linkage and redo clearing.
- **buffer.c / buffer.h / buffer_type.h** — Buffer lifecycle and `BufferList` management. `Buffer` struct aggregates all per-buffer state. `BufferList` is a global doubly-linked list with a `current` pointer. `buffer_type.h` has the struct definition; `buffer.h` has the public API.
- **buffer_edit.c** — Content mutation: insert, delete, and their Scheme bindings. Keeps gap buffer, line table, change log, and metadata in sync after every edit.
- **point.c** — Point movement and position queries: forward/backward by char/word/line, beginning/end-of-line, goto, and related Scheme bindings.
- **mode.c** — Per-buffer major/minor mode lists, local keymap, and local variable accessors.
- **search.c** — Forward/backward text search (stub, TODO).
- **var.c / var.h** — Buffer-local Scheme variables. Singly-linked list of `(sexp key, sexp value)` pairs. O(n) lookup. Used for settings like `display-line-numbers-type`.

## Key Invariants

- **Gap buffer**: `point <= gap_end <= size`; logical text = front + back segments
- **Line table**: lines are consecutive and non-overlapping; `lines[0].start == 0`; every `'\n'` creates a boundary; after any edit, all subsequent line offsets must be adjusted
- **Buffer consistency**: `num_chars == gb_used(contents)`, `num_lines == lt.count`, `point.pos` in `[0, num_chars]` — all must hold after every edit
- **Marks are not auto-adjusted**: editing text does not update mark positions; callers are responsible
- **One major mode per buffer** (or NULL); minor modes are a linked list (most-recently-enabled first)
- **`buffer_set_current()` must be called** when switching the active buffer — display reads `current`

## Boundaries

- **Display reads, never writes**: line table, text content, point, marks, select mode
- **`buffer_edit.c` is the sole entry point for text mutation**: all insert/delete goes through it
- **No direct gap buffer access** outside `buffer_edit.c` (and `buffer.c` for init/teardown)
- **Scheme bindings** (`scm_*` functions) live at the bottom of their respective `.c` files

## Common Modification Workflows

### Adding a new text operation

1. Implement in `buffer_edit.c` using `gb_*` functions on `buf->contents`
2. Call `line_insert_char()` or `line_delete_char()` to keep line table in sync
3. Call `change_record_insert/delete()` to track undo history
4. Update `buf->num_chars`, `buf->num_lines` if they change
5. Set `G->needs_redraw = true`
6. Add Scheme binding (`scm_*` function) and register in `src/command/scheme.c`

### Adding a new mark type

1. Add field to `Buffer` struct in `buffer_type.h`
2. Handle the new mark character in `mark_get()` switch in `mark.c`
3. Initialize in `buffer_create()`

### Adding a new buffer-local variable

- No C changes needed — use `(set-local! 'name value)` from Scheme
- To read from C: `vartable_get(buffer_get_locals(buf), interned_symbol, default)`

### Adding a new query exposed to Scheme

1. Write `scm_*` function in the appropriate `.c` file (edit ops → `buffer_edit.c`, movement → `point.c`, etc.)
2. Register in `src/command/scheme.c` via `SDEF()`
3. Optionally wrap in `scheme/built-in.scm` with `defcommand`
