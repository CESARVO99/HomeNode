/**
 * @file    test_smrt_mod_acc.cpp
 * @brief   Unit tests for smrt_mod_acc — UID management, validation, conversion, events
 * @project HOMENODE
 * @version 0.7.0
 *
 * Tests the pure-logic parts of the Access Control module:
 *   - Pulse validation
 *   - UID format validation
 *   - UID CRUD operations (add, remove, find, count, get, clear)
 *   - UID authorization check
 *   - UID bytes <-> string conversion
 *   - Pulse getter/setter
 *   - Event log
 *   - Module descriptor
 *   - Registration
 *   - Config defines
 *
 * Execute with: pio test -e native
 */

//=============================================================================
// Includes
//=============================================================================
#include <unity.h>
#include <string.h>
#include <stdio.h>

#include "../../src/modules/smrt_mod_acc.cpp"
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    smrt_acc_uid_clear();
    smrt_acc_clear_events();
    smrt_acc_set_pulse(SMRT_ACC_PULSE_DEFAULT_MS);
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: pulse validation
//=============================================================================

void test_validate_pulse_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_pulse(SMRT_ACC_PULSE_MIN_MS));
}

void test_validate_pulse_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_pulse(SMRT_ACC_PULSE_MAX_MS));
}

void test_validate_pulse_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_pulse(SMRT_ACC_PULSE_DEFAULT_MS));
}

void test_validate_pulse_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_pulse(499));
}

void test_validate_pulse_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_pulse(15001));
}

//=============================================================================
// Test: UID format validation
//=============================================================================

void test_uid_format_valid_4byte(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_uid_format("AB:CD:EF:01"));
}

void test_uid_format_valid_7byte(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_uid_format("AB:CD:EF:01:23:45:67"));
}

void test_uid_format_valid_lowercase(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_uid_format("ab:cd:ef:01"));
}

void test_uid_format_invalid_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_uid_format(NULL));
}

void test_uid_format_invalid_empty(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_uid_format(""));
}

void test_uid_format_invalid_no_colon(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_uid_format("ABCDEF01"));
}

void test_uid_format_invalid_single_digit(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_uid_format("A:B:C:D"));
}

//=============================================================================
// Test: UID CRUD
//=============================================================================

void test_uid_add_one(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_add("AB:CD:EF:01"));
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_count());
}

void test_uid_add_duplicate(void) {
    smrt_acc_uid_add("AB:CD:EF:01");
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_add("AB:CD:EF:01"));
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_count());
}

void test_uid_add_invalid(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_add("INVALID"));
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_count());
}

void test_uid_add_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_add(NULL));
}

void test_uid_find_existing(void) {
    smrt_acc_uid_add("AB:CD:EF:01");
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_find("AB:CD:EF:01"));
}

void test_uid_find_missing(void) {
    TEST_ASSERT_EQUAL(-1, smrt_acc_uid_find("FF:FF:FF:FF"));
}

void test_uid_remove_existing(void) {
    smrt_acc_uid_add("AB:CD:EF:01");
    smrt_acc_uid_add("11:22:33:44");
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_remove("AB:CD:EF:01"));
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_count());
    /* Remaining UID shifted */
    TEST_ASSERT_EQUAL_STRING("11:22:33:44", smrt_acc_uid_get(0));
}

void test_uid_remove_missing(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_remove("FF:FF:FF:FF"));
}

void test_uid_get_by_index(void) {
    smrt_acc_uid_add("AA:BB:CC:DD");
    const char *uid = smrt_acc_uid_get(0);
    TEST_ASSERT_NOT_NULL(uid);
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD", uid);
}

void test_uid_get_out_of_range(void) {
    TEST_ASSERT_NULL(smrt_acc_uid_get(-1));
    TEST_ASSERT_NULL(smrt_acc_uid_get(0));
}

void test_uid_clear(void) {
    smrt_acc_uid_add("AB:CD:EF:01");
    smrt_acc_uid_add("11:22:33:44");
    smrt_acc_uid_clear();
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_count());
}

//=============================================================================
// Test: UID authorization
//=============================================================================

void test_uid_authorized(void) {
    smrt_acc_uid_add("AB:CD:EF:01");
    TEST_ASSERT_EQUAL(1, smrt_acc_uid_is_authorized("AB:CD:EF:01"));
}

void test_uid_not_authorized(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_is_authorized("FF:FF:FF:FF"));
}

//=============================================================================
// Test: UID conversion bytes <-> string
//=============================================================================

void test_bytes_to_str_4byte(void) {
    unsigned char bytes[] = {0xAB, 0xCD, 0xEF, 0x01};
    char out[SMRT_ACC_UID_STR_LEN];
    int ok = smrt_acc_uid_bytes_to_str(bytes, 4, out, sizeof(out));
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL_STRING("AB:CD:EF:01", out);
}

void test_bytes_to_str_null(void) {
    char out[SMRT_ACC_UID_STR_LEN];
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_bytes_to_str(NULL, 4, out, sizeof(out)));
}

void test_str_to_bytes_4byte(void) {
    unsigned char out[10];
    int n = smrt_acc_uid_str_to_bytes("AB:CD:EF:01", out, sizeof(out));
    TEST_ASSERT_EQUAL(4, n);
    TEST_ASSERT_EQUAL(0xAB, out[0]);
    TEST_ASSERT_EQUAL(0xCD, out[1]);
    TEST_ASSERT_EQUAL(0xEF, out[2]);
    TEST_ASSERT_EQUAL(0x01, out[3]);
}

void test_str_to_bytes_invalid(void) {
    unsigned char out[10];
    TEST_ASSERT_EQUAL(0, smrt_acc_uid_str_to_bytes("INVALID", out, sizeof(out)));
}

//=============================================================================
// Test: pulse getter/setter
//=============================================================================

void test_pulse_default(void) {
    TEST_ASSERT_EQUAL(SMRT_ACC_PULSE_DEFAULT_MS, smrt_acc_get_pulse());
}

void test_pulse_set_and_get(void) {
    smrt_acc_set_pulse(5000);
    TEST_ASSERT_EQUAL(5000, smrt_acc_get_pulse());
}

//=============================================================================
// Test: event log
//=============================================================================

void test_event_initially_empty(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_get_event_count());
}

void test_event_add_and_get(void) {
    smrt_acc_add_event("Test access", 1000);
    TEST_ASSERT_EQUAL(1, smrt_acc_get_event_count());
    unsigned long ts = 0;
    const char *msg = smrt_acc_get_event(0, &ts);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Test access", msg);
    TEST_ASSERT_EQUAL(1000, ts);
}

void test_event_clear(void) {
    smrt_acc_add_event("To clear", 100);
    smrt_acc_clear_events();
    TEST_ASSERT_EQUAL(0, smrt_acc_get_event_count());
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("acc", smrt_mod_acc.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Access Control", smrt_mod_acc.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.7.0", smrt_mod_acc.version);
}

//=============================================================================
// Test: registration
//=============================================================================

void test_register_acc_module(void) {
    int ok = smrt_module_register(&smrt_mod_acc);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_acc_after_register(void) {
    smrt_module_register(&smrt_mod_acc);
    const smrt_module_t *found = smrt_module_find("acc");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Access Control", found->name);
}

//=============================================================================
// Test: lockout validation
//=============================================================================

void test_lockout_attempts_valid(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_attempts(1));
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_attempts(5));
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_attempts(100));
}

void test_lockout_attempts_invalid(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_attempts(0));
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_attempts(-1));
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_attempts(101));
}

void test_lockout_ms_valid(void) {
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_ms(10000));
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_ms(300000));
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_ms(1800000));
}

void test_lockout_ms_invalid(void) {
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_ms(9999));
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_ms(0));
    TEST_ASSERT_EQUAL(0, smrt_acc_validate_lockout_ms(1800001));
}

void test_lockout_config_defaults(void) {
    TEST_ASSERT_EQUAL(5, SMRT_ACC_MAX_FAILED_ATTEMPTS);
    TEST_ASSERT_EQUAL(300000, SMRT_ACC_LOCKOUT_MS);
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_attempts(SMRT_ACC_MAX_FAILED_ATTEMPTS));
    TEST_ASSERT_EQUAL(1, smrt_acc_validate_lockout_ms(SMRT_ACC_LOCKOUT_MS));
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_spi_ss_pin(void) {
    TEST_ASSERT_EQUAL(5, SMRT_ACC_SPI_SS);
}

void test_config_lock_pin(void) {
    TEST_ASSERT_EQUAL(2, SMRT_ACC_LOCK_PIN);
}

void test_config_max_uids(void) {
    TEST_ASSERT_EQUAL(20, SMRT_ACC_MAX_UIDS);
}

void test_config_event_log_size(void) {
    TEST_ASSERT_EQUAL(16, SMRT_ACC_EVENT_LOG_SIZE);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("acc", SMRT_ACC_NVS_NAMESPACE);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    /* Pulse validation */
    RUN_TEST(test_validate_pulse_minimum);
    RUN_TEST(test_validate_pulse_maximum);
    RUN_TEST(test_validate_pulse_default);
    RUN_TEST(test_validate_pulse_below_min);
    RUN_TEST(test_validate_pulse_above_max);

    /* UID format */
    RUN_TEST(test_uid_format_valid_4byte);
    RUN_TEST(test_uid_format_valid_7byte);
    RUN_TEST(test_uid_format_valid_lowercase);
    RUN_TEST(test_uid_format_invalid_null);
    RUN_TEST(test_uid_format_invalid_empty);
    RUN_TEST(test_uid_format_invalid_no_colon);
    RUN_TEST(test_uid_format_invalid_single_digit);

    /* UID CRUD */
    RUN_TEST(test_uid_add_one);
    RUN_TEST(test_uid_add_duplicate);
    RUN_TEST(test_uid_add_invalid);
    RUN_TEST(test_uid_add_null);
    RUN_TEST(test_uid_find_existing);
    RUN_TEST(test_uid_find_missing);
    RUN_TEST(test_uid_remove_existing);
    RUN_TEST(test_uid_remove_missing);
    RUN_TEST(test_uid_get_by_index);
    RUN_TEST(test_uid_get_out_of_range);
    RUN_TEST(test_uid_clear);

    /* Authorization */
    RUN_TEST(test_uid_authorized);
    RUN_TEST(test_uid_not_authorized);

    /* Byte/string conversion */
    RUN_TEST(test_bytes_to_str_4byte);
    RUN_TEST(test_bytes_to_str_null);
    RUN_TEST(test_str_to_bytes_4byte);
    RUN_TEST(test_str_to_bytes_invalid);

    /* Pulse getter/setter */
    RUN_TEST(test_pulse_default);
    RUN_TEST(test_pulse_set_and_get);

    /* Event log */
    RUN_TEST(test_event_initially_empty);
    RUN_TEST(test_event_add_and_get);
    RUN_TEST(test_event_clear);

    /* Descriptor */
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    /* Registration */
    RUN_TEST(test_register_acc_module);
    RUN_TEST(test_find_acc_after_register);

    /* Lockout validation */
    RUN_TEST(test_lockout_attempts_valid);
    RUN_TEST(test_lockout_attempts_invalid);
    RUN_TEST(test_lockout_ms_valid);
    RUN_TEST(test_lockout_ms_invalid);
    RUN_TEST(test_lockout_config_defaults);

    /* Config defines */
    RUN_TEST(test_config_spi_ss_pin);
    RUN_TEST(test_config_lock_pin);
    RUN_TEST(test_config_max_uids);
    RUN_TEST(test_config_event_log_size);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
