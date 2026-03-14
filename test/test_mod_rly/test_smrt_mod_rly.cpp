/**
 * @file    test_smrt_mod_rly.cpp
 * @brief   Unit tests for smrt_mod_rly — validation, getters, setters, registration
 * @project HOMENODE
 * @version 0.4.0
 *
 * Tests the pure-logic (non-hardware) parts of the RELAY module:
 *   - Pulse duration validation
 *   - Relay count validation
 *   - State getter/setter per index
 *   - Count and pulse getter/setter
 *   - Module descriptor fields
 *   - Module registration with platform registry
 *   - Config defines verification
 *
 * Execute with: pio test -e native
 */

//=============================================================================
// Includes
//=============================================================================
#include <unity.h>
#include <string.h>

#include "../../src/modules/smrt_mod_rly.cpp"
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    /* Reset module state */
    int i;
    for (i = 0; i < SMRT_RLY_MAX_RELAYS; i++) {
        smrt_rly_set_state(i, 0);
    }
    smrt_rly_set_count(SMRT_RLY_DEFAULT_COUNT);
    smrt_rly_set_pulse(SMRT_RLY_PULSE_DEFAULT_MS);
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: pulse validation
//=============================================================================

void test_validate_pulse_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_pulse(SMRT_RLY_PULSE_MIN_MS));
}

void test_validate_pulse_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_pulse(SMRT_RLY_PULSE_MAX_MS));
}

void test_validate_pulse_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_pulse(SMRT_RLY_PULSE_DEFAULT_MS));
}

void test_validate_pulse_midrange(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_pulse(5000));
}

void test_validate_pulse_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_pulse(99));
}

void test_validate_pulse_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_pulse(0));
}

void test_validate_pulse_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_pulse(30001));
}

//=============================================================================
// Test: count validation
//=============================================================================

void test_validate_count_one(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_count(1));
}

void test_validate_count_max(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_count(SMRT_RLY_MAX_RELAYS));
}

void test_validate_count_mid(void) {
    TEST_ASSERT_EQUAL(1, smrt_rly_validate_count(2));
}

void test_validate_count_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_count(0));
}

void test_validate_count_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_count(5));
}

void test_validate_count_negative(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_validate_count(-1));
}

//=============================================================================
// Test: state getter/setter
//=============================================================================

void test_state_initial_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_rly_get_state(0));
}

void test_state_set_on(void) {
    smrt_rly_set_state(0, 1);
    TEST_ASSERT_EQUAL(1, smrt_rly_get_state(0));
}

void test_state_set_off(void) {
    smrt_rly_set_state(0, 1);
    smrt_rly_set_state(0, 0);
    TEST_ASSERT_EQUAL(0, smrt_rly_get_state(0));
}

void test_state_multiple_relays(void) {
    smrt_rly_set_state(0, 1);
    smrt_rly_set_state(1, 0);
    smrt_rly_set_state(2, 1);
    smrt_rly_set_state(3, 0);
    TEST_ASSERT_EQUAL(1, smrt_rly_get_state(0));
    TEST_ASSERT_EQUAL(0, smrt_rly_get_state(1));
    TEST_ASSERT_EQUAL(1, smrt_rly_get_state(2));
    TEST_ASSERT_EQUAL(0, smrt_rly_get_state(3));
}

void test_state_out_of_range(void) {
    TEST_ASSERT_EQUAL(-1, smrt_rly_get_state(-1));
    TEST_ASSERT_EQUAL(-1, smrt_rly_get_state(4));
}

//=============================================================================
// Test: count getter/setter
//=============================================================================

void test_count_default(void) {
    TEST_ASSERT_EQUAL(SMRT_RLY_DEFAULT_COUNT, smrt_rly_get_count());
}

void test_count_set_and_get(void) {
    smrt_rly_set_count(3);
    TEST_ASSERT_EQUAL(3, smrt_rly_get_count());
}

void test_count_set_max(void) {
    smrt_rly_set_count(SMRT_RLY_MAX_RELAYS);
    TEST_ASSERT_EQUAL(SMRT_RLY_MAX_RELAYS, smrt_rly_get_count());
}

//=============================================================================
// Test: pulse getter/setter
//=============================================================================

void test_pulse_default(void) {
    TEST_ASSERT_EQUAL(SMRT_RLY_PULSE_DEFAULT_MS, smrt_rly_get_pulse());
}

void test_pulse_set_and_get(void) {
    smrt_rly_set_pulse(5000);
    TEST_ASSERT_EQUAL(5000, smrt_rly_get_pulse());
}

void test_pulse_set_max(void) {
    smrt_rly_set_pulse(SMRT_RLY_PULSE_MAX_MS);
    TEST_ASSERT_EQUAL(SMRT_RLY_PULSE_MAX_MS, smrt_rly_get_pulse());
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("rly", smrt_mod_rly.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Relay Control", smrt_mod_rly.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.4.0", smrt_mod_rly.version);
}

//=============================================================================
// Test: registration
//=============================================================================

void test_register_rly_module(void) {
    int ok = smrt_module_register(&smrt_mod_rly);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_rly_after_register(void) {
    smrt_module_register(&smrt_mod_rly);
    const smrt_module_t *found = smrt_module_find("rly");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Relay Control", found->name);
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_pin_1(void) {
    TEST_ASSERT_EQUAL(16, SMRT_RLY_PIN_1);
}

void test_config_pin_2(void) {
    TEST_ASSERT_EQUAL(17, SMRT_RLY_PIN_2);
}

void test_config_max_relays(void) {
    TEST_ASSERT_EQUAL(4, SMRT_RLY_MAX_RELAYS);
}

void test_config_pulse_min(void) {
    TEST_ASSERT_EQUAL(100, SMRT_RLY_PULSE_MIN_MS);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("rly", SMRT_RLY_NVS_NAMESPACE);
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
    RUN_TEST(test_validate_pulse_midrange);
    RUN_TEST(test_validate_pulse_below_min);
    RUN_TEST(test_validate_pulse_zero);
    RUN_TEST(test_validate_pulse_above_max);

    /* Count validation */
    RUN_TEST(test_validate_count_one);
    RUN_TEST(test_validate_count_max);
    RUN_TEST(test_validate_count_mid);
    RUN_TEST(test_validate_count_zero);
    RUN_TEST(test_validate_count_above_max);
    RUN_TEST(test_validate_count_negative);

    /* State getter/setter */
    RUN_TEST(test_state_initial_zero);
    RUN_TEST(test_state_set_on);
    RUN_TEST(test_state_set_off);
    RUN_TEST(test_state_multiple_relays);
    RUN_TEST(test_state_out_of_range);

    /* Count getter/setter */
    RUN_TEST(test_count_default);
    RUN_TEST(test_count_set_and_get);
    RUN_TEST(test_count_set_max);

    /* Pulse getter/setter */
    RUN_TEST(test_pulse_default);
    RUN_TEST(test_pulse_set_and_get);
    RUN_TEST(test_pulse_set_max);

    /* Descriptor */
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    /* Registration */
    RUN_TEST(test_register_rly_module);
    RUN_TEST(test_find_rly_after_register);

    /* Config defines */
    RUN_TEST(test_config_pin_1);
    RUN_TEST(test_config_pin_2);
    RUN_TEST(test_config_max_relays);
    RUN_TEST(test_config_pulse_min);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
