/**
 * @file    test_smrt_core_auto.cpp
 * @brief   Unit tests for automation rules engine
 * @project HOMENODE
 * @version 1.4.0
 */

#include <unity.h>
#include "smrt_core_auto.h"
#include "../../src/core/smrt_core_auto.cpp"

void setUp(void) {}
void tearDown(void) {}

/* ─── Evaluate tests ─── */

void test_eval_gt_true(void) {
    TEST_ASSERT_EQUAL(1, smrt_auto_evaluate(25.0f, 20.0f, SMRT_AUTO_CMP_GT, 0));
}
void test_eval_gt_false(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(15.0f, 20.0f, SMRT_AUTO_CMP_GT, 0));
}
void test_eval_gt_equal(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(20.0f, 20.0f, SMRT_AUTO_CMP_GT, 0));
}
void test_eval_lt_true(void) {
    TEST_ASSERT_EQUAL(1, smrt_auto_evaluate(15.0f, 20.0f, SMRT_AUTO_CMP_LT, 0));
}
void test_eval_lt_false(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(25.0f, 20.0f, SMRT_AUTO_CMP_LT, 0));
}
void test_eval_eq_true(void) {
    TEST_ASSERT_EQUAL(1, smrt_auto_evaluate(20.05f, 20.0f, SMRT_AUTO_CMP_EQ, 0));
}
void test_eval_eq_false(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(20.5f, 20.0f, SMRT_AUTO_CMP_EQ, 0));
}
void test_eval_change_true(void) {
    TEST_ASSERT_EQUAL(1, smrt_auto_evaluate(22.0f, 0, SMRT_AUTO_CMP_CHANGE, 20.0f));
}
void test_eval_change_false(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(20.005f, 0, SMRT_AUTO_CMP_CHANGE, 20.0f));
}
void test_eval_invalid_comparator(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_evaluate(10, 5, 99, 0));
}

/* ─── Validate comparator ─── */

void test_validate_cmp_gt(void)  { TEST_ASSERT_EQUAL(1, smrt_auto_validate_comparator(SMRT_AUTO_CMP_GT)); }
void test_validate_cmp_lt(void)  { TEST_ASSERT_EQUAL(1, smrt_auto_validate_comparator(SMRT_AUTO_CMP_LT)); }
void test_validate_cmp_eq(void)  { TEST_ASSERT_EQUAL(1, smrt_auto_validate_comparator(SMRT_AUTO_CMP_EQ)); }
void test_validate_cmp_chg(void) { TEST_ASSERT_EQUAL(1, smrt_auto_validate_comparator(SMRT_AUTO_CMP_CHANGE)); }
void test_validate_cmp_bad(void) { TEST_ASSERT_EQUAL(0, smrt_auto_validate_comparator(5)); }

/* ─── Validate rule ─── */

void test_validate_rule_valid(void) {
    smrt_auto_rule_t r = {};
    r.enabled = 1;
    strcpy(r.source_channel, "env.temperature");
    strcpy(r.action, "rly_set:0:1");
    r.comparator = SMRT_AUTO_CMP_GT;
    r.cooldown_s = 60;
    TEST_ASSERT_EQUAL(1, smrt_auto_validate_rule(&r));
}

void test_validate_rule_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_auto_validate_rule(NULL));
}

void test_validate_rule_no_channel(void) {
    smrt_auto_rule_t r = {};
    strcpy(r.action, "rly_set:0:1");
    TEST_ASSERT_EQUAL(0, smrt_auto_validate_rule(&r));
}

void test_validate_rule_no_action(void) {
    smrt_auto_rule_t r = {};
    strcpy(r.source_channel, "env.temperature");
    TEST_ASSERT_EQUAL(0, smrt_auto_validate_rule(&r));
}

void test_validate_rule_bad_cmp(void) {
    smrt_auto_rule_t r = {};
    strcpy(r.source_channel, "env.temperature");
    strcpy(r.action, "rly_set:0:1");
    r.comparator = 99;
    TEST_ASSERT_EQUAL(0, smrt_auto_validate_rule(&r));
}

void test_validate_rule_bad_cooldown(void) {
    smrt_auto_rule_t r = {};
    strcpy(r.source_channel, "env.temperature");
    strcpy(r.action, "rly_set:0:1");
    r.cooldown_s = 100000;
    TEST_ASSERT_EQUAL(0, smrt_auto_validate_rule(&r));
}

/* ─── Runner ─── */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_eval_gt_true);
    RUN_TEST(test_eval_gt_false);
    RUN_TEST(test_eval_gt_equal);
    RUN_TEST(test_eval_lt_true);
    RUN_TEST(test_eval_lt_false);
    RUN_TEST(test_eval_eq_true);
    RUN_TEST(test_eval_eq_false);
    RUN_TEST(test_eval_change_true);
    RUN_TEST(test_eval_change_false);
    RUN_TEST(test_eval_invalid_comparator);

    RUN_TEST(test_validate_cmp_gt);
    RUN_TEST(test_validate_cmp_lt);
    RUN_TEST(test_validate_cmp_eq);
    RUN_TEST(test_validate_cmp_chg);
    RUN_TEST(test_validate_cmp_bad);

    RUN_TEST(test_validate_rule_valid);
    RUN_TEST(test_validate_rule_null);
    RUN_TEST(test_validate_rule_no_channel);
    RUN_TEST(test_validate_rule_no_action);
    RUN_TEST(test_validate_rule_bad_cmp);
    RUN_TEST(test_validate_rule_bad_cooldown);

    return UNITY_END();
}
