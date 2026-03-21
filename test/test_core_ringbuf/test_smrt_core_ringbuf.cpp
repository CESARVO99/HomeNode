/**
 * @file    test_smrt_core_ringbuf.cpp
 * @brief   Unit tests for circular buffer
 * @project HOMENODE
 * @version 1.2.0
 */

#include <unity.h>
#include "smrt_core_ringbuf.h"
#include "../../src/core/smrt_core_ringbuf.cpp"

#define TEST_CAP 8

static smrt_ringbuf_t rb;
static smrt_ringbuf_sample_t samples[TEST_CAP];

void setUp(void) {
    smrt_ringbuf_init(&rb, samples, TEST_CAP, "test.channel");
}
void tearDown(void) {}

//=============================================================================
// Init tests
//=============================================================================
void test_init_success(void) {
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_init(&rb, samples, TEST_CAP, "ch1"));
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_count(&rb));
    TEST_ASSERT_EQUAL_STRING("ch1", rb.name);
}

void test_init_null_rb(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_init(NULL, samples, TEST_CAP, "x"));
}

void test_init_null_samples(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_init(&rb, NULL, TEST_CAP, "x"));
}

void test_init_zero_capacity(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_init(&rb, samples, 0, "x"));
}

void test_init_over_max_capacity(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_init(&rb, samples, SMRT_RINGBUF_MAX_CAPACITY + 1, "x"));
}

void test_init_null_name(void) {
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_init(&rb, samples, TEST_CAP, NULL));
    TEST_ASSERT_EQUAL_STRING("", rb.name);
}

//=============================================================================
// Push + count tests
//=============================================================================
void test_push_single(void) {
    smrt_ringbuf_push(&rb, 22.5f, 1000);
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_count(&rb));
}

void test_push_fill(void) {
    for (int i = 0; i < TEST_CAP; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i);
    }
    TEST_ASSERT_EQUAL(TEST_CAP, smrt_ringbuf_count(&rb));
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_is_full(&rb));
}

void test_push_overflow_wraps(void) {
    for (int i = 0; i < TEST_CAP + 3; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i);
    }
    /* Count capped at capacity */
    TEST_ASSERT_EQUAL(TEST_CAP, smrt_ringbuf_count(&rb));
}

void test_push_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_push(NULL, 1.0f, 100));
}

//=============================================================================
// Get tests
//=============================================================================
void test_get_oldest_newest(void) {
    for (int i = 0; i < 5; i++) {
        smrt_ringbuf_push(&rb, (float)(i * 10), 1000 + i);
    }
    smrt_ringbuf_sample_t s;
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_get(&rb, 0, &s));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.value); /* oldest */
    TEST_ASSERT_EQUAL(1000, s.timestamp);

    TEST_ASSERT_EQUAL(1, smrt_ringbuf_get(&rb, 4, &s));
    TEST_ASSERT_EQUAL_FLOAT(40.0f, s.value); /* newest */
    TEST_ASSERT_EQUAL(1004, s.timestamp);
}

void test_get_after_wrap(void) {
    /* Push TEST_CAP + 2 samples, oldest should be index 2 */
    for (int i = 0; i < TEST_CAP + 2; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i);
    }
    smrt_ringbuf_sample_t s;
    smrt_ringbuf_get(&rb, 0, &s);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, s.value); /* First two overwritten */
    TEST_ASSERT_EQUAL(1002, s.timestamp);

    smrt_ringbuf_get(&rb, TEST_CAP - 1, &s);
    TEST_ASSERT_EQUAL_FLOAT((float)(TEST_CAP + 1), s.value); /* newest */
}

void test_get_out_of_range(void) {
    smrt_ringbuf_push(&rb, 1.0f, 100);
    smrt_ringbuf_sample_t s;
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_get(&rb, 1, &s));
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_get(&rb, 100, &s));
}

void test_get_empty(void) {
    smrt_ringbuf_sample_t s;
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_get(&rb, 0, &s));
}

//=============================================================================
// Latest tests
//=============================================================================
void test_latest_single(void) {
    smrt_ringbuf_push(&rb, 42.0f, 9999);
    smrt_ringbuf_sample_t s;
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_latest(&rb, &s));
    TEST_ASSERT_EQUAL_FLOAT(42.0f, s.value);
    TEST_ASSERT_EQUAL(9999, s.timestamp);
}

void test_latest_after_multiple(void) {
    smrt_ringbuf_push(&rb, 1.0f, 100);
    smrt_ringbuf_push(&rb, 2.0f, 200);
    smrt_ringbuf_push(&rb, 3.0f, 300);
    smrt_ringbuf_sample_t s;
    smrt_ringbuf_latest(&rb, &s);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, s.value);
}

void test_latest_after_wrap(void) {
    for (int i = 0; i < TEST_CAP + 5; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i);
    }
    smrt_ringbuf_sample_t s;
    smrt_ringbuf_latest(&rb, &s);
    TEST_ASSERT_EQUAL_FLOAT((float)(TEST_CAP + 4), s.value);
}

void test_latest_empty(void) {
    smrt_ringbuf_sample_t s;
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_latest(&rb, &s));
}

//=============================================================================
// Clear tests
//=============================================================================
void test_clear(void) {
    smrt_ringbuf_push(&rb, 1.0f, 100);
    smrt_ringbuf_push(&rb, 2.0f, 200);
    smrt_ringbuf_clear(&rb);
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_count(&rb));
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_is_full(&rb));
}

void test_clear_null(void) {
    smrt_ringbuf_clear(NULL); /* Should not crash */
}

//=============================================================================
// Query tests
//=============================================================================
void test_query_all(void) {
    for (int i = 0; i < 5; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i * 10);
    }
    smrt_ringbuf_sample_t out[10];
    uint16_t count;
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_query(&rb, 0, 0, out, 10, &count));
    TEST_ASSERT_EQUAL(5, count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[0].value);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, out[4].value);
}

void test_query_time_range(void) {
    for (int i = 0; i < 5; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i * 10);
    }
    smrt_ringbuf_sample_t out[10];
    uint16_t count;
    /* Query from timestamp 1010 to 1030 (indices 1,2,3) */
    smrt_ringbuf_query(&rb, 1010, 1030, out, 10, &count);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, out[0].value);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, out[2].value);
}

void test_query_downsample(void) {
    for (int i = 0; i < TEST_CAP; i++) {
        smrt_ringbuf_push(&rb, (float)i, 1000 + i);
    }
    smrt_ringbuf_sample_t out[3];
    uint16_t count;
    /* Request max 3 points from 8 samples → stride of 3 */
    smrt_ringbuf_query(&rb, 0, 0, out, 3, &count);
    TEST_ASSERT_EQUAL(3, count);
    /* Should get evenly spaced: indices 0, 3, 6 */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[0].value);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, out[1].value);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, out[2].value);
}

void test_query_empty(void) {
    smrt_ringbuf_sample_t out[5];
    uint16_t count;
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_query(&rb, 0, 0, out, 5, &count));
    TEST_ASSERT_EQUAL(0, count);
}

void test_query_no_match(void) {
    smrt_ringbuf_push(&rb, 1.0f, 100);
    smrt_ringbuf_sample_t out[5];
    uint16_t count;
    smrt_ringbuf_query(&rb, 200, 300, out, 5, &count);
    TEST_ASSERT_EQUAL(0, count);
}

void test_query_null_params(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_query(NULL, 0, 0, NULL, 0, NULL));
}

//=============================================================================
// is_full tests
//=============================================================================
void test_is_full_false(void) {
    smrt_ringbuf_push(&rb, 1.0f, 100);
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_is_full(&rb));
}

void test_is_full_true(void) {
    for (int i = 0; i < TEST_CAP; i++) {
        smrt_ringbuf_push(&rb, (float)i, 100);
    }
    TEST_ASSERT_EQUAL(1, smrt_ringbuf_is_full(&rb));
}

void test_is_full_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_ringbuf_is_full(NULL));
}

//=============================================================================
// Test runner
//=============================================================================
int main(void) {
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_init_success);
    RUN_TEST(test_init_null_rb);
    RUN_TEST(test_init_null_samples);
    RUN_TEST(test_init_zero_capacity);
    RUN_TEST(test_init_over_max_capacity);
    RUN_TEST(test_init_null_name);

    /* Push + count */
    RUN_TEST(test_push_single);
    RUN_TEST(test_push_fill);
    RUN_TEST(test_push_overflow_wraps);
    RUN_TEST(test_push_null);

    /* Get */
    RUN_TEST(test_get_oldest_newest);
    RUN_TEST(test_get_after_wrap);
    RUN_TEST(test_get_out_of_range);
    RUN_TEST(test_get_empty);

    /* Latest */
    RUN_TEST(test_latest_single);
    RUN_TEST(test_latest_after_multiple);
    RUN_TEST(test_latest_after_wrap);
    RUN_TEST(test_latest_empty);

    /* Clear */
    RUN_TEST(test_clear);
    RUN_TEST(test_clear_null);

    /* Query */
    RUN_TEST(test_query_all);
    RUN_TEST(test_query_time_range);
    RUN_TEST(test_query_downsample);
    RUN_TEST(test_query_empty);
    RUN_TEST(test_query_no_match);
    RUN_TEST(test_query_null_params);

    /* is_full */
    RUN_TEST(test_is_full_false);
    RUN_TEST(test_is_full_true);
    RUN_TEST(test_is_full_null);

    return UNITY_END();
}
