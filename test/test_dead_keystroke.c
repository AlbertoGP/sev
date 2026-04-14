// Regression test: the first `j` keypress after opening a file is a no-op.
//
// After buffer_read(), the status bar's line indicator shows a stale value
// (e.g. "178") even though the cursor is drawn at line 1. The first `j`
// doesn't visually move the cursor, but the indicator snaps to "1:1". The
// second `j` then behaves normally.
//
// Symptom trace points at point_move_by_line() in src/text/point.c:
// the clamp `count >= num_lines - cur_line` zeros the motion when cur_line
// is stale, while the trailing `cur_line = line_index + count + 1` write
// snaps cur_line to 1 as a side effect.
//
// This test reproduces it at the text-subsystem level only — no scheme,
// no SDL, no command dispatch.

#include <stdio.h>

#include "unity/unity.h"

#include "../src/text/buffer.h"
#include "../src/text/location.h"

static const char *FIXTURE_PATH = "/tmp/sev_test_dead_key.txt";

static void write_fixture(const char *path, int n_lines) {
    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    for (int i = 0; i < n_lines; i++) fprintf(f, "line %d\n", i);
    fclose(f);
}

void setUp(void) {
    buffer_list_init();
}

void tearDown(void) {
    buffer_list_quit();
}

void test_first_j_after_file_load_moves_point(void) {
    write_fixture(FIXTURE_PATH, 10);

    Buffer *buf = buffer_create(FIXTURE_PATH);
    TEST_ASSERT_NOT_NULL(buf);
    buffer_set_current(buf);
    set_file_name((char *)FIXTURE_PATH);

    TEST_ASSERT_TRUE(buffer_read());

    // Precondition: point is at the top of the file.
    TEST_ASSERT_EQUAL_UINT(0, point_get(buf).pos);

    // Act: the first `j` — this is what evil-motion-j calls.
    point_move_by_line(1);

    // SANITY CHECK: the second call should advance normally. If this fails,
    // the harness is broken rather than the bug being reproduced.
    size_t pos_after_first = point_get(buf).pos;
    point_move_by_line(1);
    TEST_ASSERT_TRUE_MESSAGE(point_get(buf).pos > pos_after_first,
        "sanity: second j did not advance — harness is broken, not the bug");

    // Assert: the first `j` should have advanced point to the start of line 2.
    TEST_ASSERT_TRUE_MESSAGE(pos_after_first > 0,
        "first j after buffer_read() did not advance point");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_first_j_after_file_load_moves_point);
    return UNITY_END();
}
