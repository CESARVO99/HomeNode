/**
 * @file    test_smrt_core_node.cpp
 * @brief   Unit tests for node identity and validation
 * @project HOMENODE
 * @version 1.0.0
 */

#include <unity.h>
#include "smrt_core_node.h"
#include "../../src/core/smrt_core_node.cpp"

//=============================================================================
// smrt_node_validate_modules tests
//=============================================================================

void test_validate_empty_mask(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(0x00));
}

void test_validate_single_module(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_ENV));
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_RLY));
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_SEC));
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_PLG));
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_NRG));
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_ACC));
}

void test_validate_env_rly_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_RLY));
}

void test_validate_env_sec_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_SEC));
}

void test_validate_env_acc_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_ACC));
}

void test_validate_rly_sec_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_RLY | SMRT_NODE_MOD_SEC));
}

void test_validate_sec_acc_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_SEC | SMRT_NODE_MOD_ACC));
}

void test_validate_nrg_acc_ok(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(SMRT_NODE_MOD_NRG | SMRT_NODE_MOD_ACC));
}

void test_validate_plg_nrg_conflict(void) {
    uint8_t result = smrt_node_validate_modules(SMRT_NODE_MOD_PLG | SMRT_NODE_MOD_NRG);
    TEST_ASSERT_NOT_EQUAL(0, result);
    TEST_ASSERT_BITS(SMRT_NODE_CONFLICT_PLG_NRG, SMRT_NODE_CONFLICT_PLG_NRG, result);
}

void test_validate_plg_acc_conflict(void) {
    uint8_t result = smrt_node_validate_modules(SMRT_NODE_MOD_PLG | SMRT_NODE_MOD_ACC);
    TEST_ASSERT_NOT_EQUAL(0, result);
    TEST_ASSERT_BITS(SMRT_NODE_CONFLICT_PLG_ACC, SMRT_NODE_CONFLICT_PLG_ACC, result);
}

void test_validate_plg_nrg_acc_double_conflict(void) {
    uint8_t result = smrt_node_validate_modules(SMRT_NODE_MOD_PLG | SMRT_NODE_MOD_NRG | SMRT_NODE_MOD_ACC);
    TEST_ASSERT_NOT_EQUAL(0, result);
    /* Both conflicts present */
    TEST_ASSERT_BITS(SMRT_NODE_CONFLICT_PLG_NRG, SMRT_NODE_CONFLICT_PLG_NRG, result);
    TEST_ASSERT_BITS(SMRT_NODE_CONFLICT_PLG_ACC, SMRT_NODE_CONFLICT_PLG_ACC, result);
}

void test_validate_env_rly_sec_acc_ok(void) {
    uint8_t mask = SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_RLY | SMRT_NODE_MOD_SEC | SMRT_NODE_MOD_ACC;
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(mask));
}

void test_validate_all_except_plg_ok(void) {
    uint8_t mask = SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_RLY | SMRT_NODE_MOD_SEC | SMRT_NODE_MOD_NRG | SMRT_NODE_MOD_ACC;
    TEST_ASSERT_EQUAL(0, smrt_node_validate_modules(mask));
}

//=============================================================================
// smrt_node_validate_name tests
//=============================================================================

void test_name_valid(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_validate_name("Living Room"));
}

void test_name_single_char(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_validate_name("A"));
}

void test_name_max_length(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_validate_name("12345678901234567890123456789012")); /* 32 chars */
}

void test_name_too_long(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_name("123456789012345678901234567890123")); /* 33 chars */
}

void test_name_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_name(NULL));
}

void test_name_empty(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_name(""));
}

void test_name_control_char(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_name("Hello\nWorld"));
}

void test_name_tab_rejected(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_validate_name("Hello\tWorld"));
}

void test_name_with_numbers(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_validate_name("Room 101"));
}

void test_name_with_special_chars(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_validate_name("Habitacion-Principal_2"));
}

//=============================================================================
// smrt_node_modules_to_string tests
//=============================================================================

void test_modules_str_empty(void) {
    char buf[32];
    smrt_node_modules_to_string(0x00, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_modules_str_env_only(void) {
    char buf[32];
    smrt_node_modules_to_string(SMRT_NODE_MOD_ENV, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("env", buf);
}

void test_modules_str_env_rly(void) {
    char buf[32];
    smrt_node_modules_to_string(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_RLY, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("env,rly", buf);
}

void test_modules_str_all(void) {
    char buf[32];
    smrt_node_modules_to_string(SMRT_NODE_MOD_ALL, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("env,rly,sec,plg,nrg,acc", buf);
}

void test_modules_str_sec_acc(void) {
    char buf[32];
    smrt_node_modules_to_string(SMRT_NODE_MOD_SEC | SMRT_NODE_MOD_ACC, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("sec,acc", buf);
}

void test_modules_str_small_buffer(void) {
    char buf[8];
    smrt_node_modules_to_string(SMRT_NODE_MOD_ALL, buf, sizeof(buf));
    /* Should not overflow, string truncated */
    TEST_ASSERT_TRUE(strlen(buf) < 8);
}

void test_modules_str_null_buffer(void) {
    smrt_node_modules_to_string(SMRT_NODE_MOD_ENV, NULL, 0);
    /* Should not crash */
}

//=============================================================================
// smrt_node_module_count tests
//=============================================================================

void test_count_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_node_module_count(0x00));
}

void test_count_one(void) {
    TEST_ASSERT_EQUAL(1, smrt_node_module_count(SMRT_NODE_MOD_ENV));
}

void test_count_two(void) {
    TEST_ASSERT_EQUAL(2, smrt_node_module_count(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_RLY));
}

void test_count_all(void) {
    TEST_ASSERT_EQUAL(6, smrt_node_module_count(SMRT_NODE_MOD_ALL));
}

void test_count_three(void) {
    TEST_ASSERT_EQUAL(3, smrt_node_module_count(SMRT_NODE_MOD_ENV | SMRT_NODE_MOD_SEC | SMRT_NODE_MOD_ACC));
}

void test_count_ignores_high_bits(void) {
    TEST_ASSERT_EQUAL(6, smrt_node_module_count(0xFF)); /* Only lower 6 bits counted */
}

//=============================================================================
// Test runner
//=============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    /* Validation: module conflicts */
    RUN_TEST(test_validate_empty_mask);
    RUN_TEST(test_validate_single_module);
    RUN_TEST(test_validate_env_rly_ok);
    RUN_TEST(test_validate_env_sec_ok);
    RUN_TEST(test_validate_env_acc_ok);
    RUN_TEST(test_validate_rly_sec_ok);
    RUN_TEST(test_validate_sec_acc_ok);
    RUN_TEST(test_validate_nrg_acc_ok);
    RUN_TEST(test_validate_plg_nrg_conflict);
    RUN_TEST(test_validate_plg_acc_conflict);
    RUN_TEST(test_validate_plg_nrg_acc_double_conflict);
    RUN_TEST(test_validate_env_rly_sec_acc_ok);
    RUN_TEST(test_validate_all_except_plg_ok);

    /* Validation: name */
    RUN_TEST(test_name_valid);
    RUN_TEST(test_name_single_char);
    RUN_TEST(test_name_max_length);
    RUN_TEST(test_name_too_long);
    RUN_TEST(test_name_null);
    RUN_TEST(test_name_empty);
    RUN_TEST(test_name_control_char);
    RUN_TEST(test_name_tab_rejected);
    RUN_TEST(test_name_with_numbers);
    RUN_TEST(test_name_with_special_chars);

    /* Modules to string */
    RUN_TEST(test_modules_str_empty);
    RUN_TEST(test_modules_str_env_only);
    RUN_TEST(test_modules_str_env_rly);
    RUN_TEST(test_modules_str_all);
    RUN_TEST(test_modules_str_sec_acc);
    RUN_TEST(test_modules_str_small_buffer);
    RUN_TEST(test_modules_str_null_buffer);

    /* Module count */
    RUN_TEST(test_count_zero);
    RUN_TEST(test_count_one);
    RUN_TEST(test_count_two);
    RUN_TEST(test_count_all);
    RUN_TEST(test_count_three);
    RUN_TEST(test_count_ignores_high_bits);

    return UNITY_END();
}
