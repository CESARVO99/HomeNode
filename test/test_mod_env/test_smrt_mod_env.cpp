/**
 * @file    test_smrt_mod_env.cpp
 * @brief   Unit tests for smrt_mod_env — interval validation, getters, registration
 * @project HOMENODE
 * @version 0.3.0
 *
 * Tests the pure-logic (non-hardware) parts of the ENV module:
 *   - Interval validation (bounds checking)
 *   - Getter/setter functions for interval, temp, humidity, status
 *   - Module descriptor fields
 *   - Module registration with the platform registry
 *
 * Execute with: pio test -e native
 */

//=============================================================================
// Includes
//=============================================================================
#include <unity.h>
#include <string.h>

// Include module source directly (compiles testable functions only under UNIT_TEST)
#include "../../src/modules/smrt_mod_env.cpp"

// Include module registry for registration tests
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    // Reset module state to defaults before each test
    smrt_env_set_interval(SMRT_ENV_READ_INTERVAL_MS);
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: interval validation
//=============================================================================

void test_validate_interval_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_env_validate_interval(SMRT_ENV_READ_MIN_MS));
}

void test_validate_interval_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_env_validate_interval(SMRT_ENV_READ_MAX_MS));
}

void test_validate_interval_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_env_validate_interval(SMRT_ENV_READ_INTERVAL_MS));
}

void test_validate_interval_midrange(void) {
    TEST_ASSERT_EQUAL(1, smrt_env_validate_interval(10000));
}

void test_validate_interval_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_env_validate_interval(1999));
}

void test_validate_interval_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_env_validate_interval(0));
}

void test_validate_interval_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_env_validate_interval(60001));
}

void test_validate_interval_very_large(void) {
    TEST_ASSERT_EQUAL(0, smrt_env_validate_interval(1000000));
}

//=============================================================================
// Test: interval getter/setter
//=============================================================================

void test_get_default_interval(void) {
    TEST_ASSERT_EQUAL(SMRT_ENV_READ_INTERVAL_MS, smrt_env_get_interval());
}

void test_set_and_get_interval(void) {
    smrt_env_set_interval(3000);
    TEST_ASSERT_EQUAL(3000, smrt_env_get_interval());
}

void test_set_interval_max(void) {
    smrt_env_set_interval(SMRT_ENV_READ_MAX_MS);
    TEST_ASSERT_EQUAL(SMRT_ENV_READ_MAX_MS, smrt_env_get_interval());
}

//=============================================================================
// Test: sensor data getters (initial state)
//=============================================================================

void test_initial_temperature(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, smrt_env_get_temperature());
}

void test_initial_humidity(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, smrt_env_get_humidity());
}

void test_initial_status(void) {
    TEST_ASSERT_EQUAL(0, smrt_env_get_status());
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("env", smrt_mod_env.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Environmental", smrt_mod_env.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.3.0", smrt_mod_env.version);
}

//=============================================================================
// Test: registration with platform registry
//=============================================================================

void test_register_env_module(void) {
    int ok = smrt_module_register(&smrt_mod_env);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_env_after_register(void) {
    smrt_module_register(&smrt_mod_env);
    const smrt_module_t *found = smrt_module_find("env");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Environmental", found->name);
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_dht_pin(void) {
    TEST_ASSERT_EQUAL(4, SMRT_ENV_DHT_PIN);
}

void test_config_dht_type(void) {
    TEST_ASSERT_EQUAL(22, SMRT_ENV_DHT_TYPE);
}

void test_config_read_min_ms(void) {
    TEST_ASSERT_EQUAL(2000, SMRT_ENV_READ_MIN_MS);
}

void test_config_read_max_ms(void) {
    TEST_ASSERT_EQUAL(60000, SMRT_ENV_READ_MAX_MS);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("env", SMRT_ENV_NVS_NAMESPACE);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Interval validation
    RUN_TEST(test_validate_interval_minimum);
    RUN_TEST(test_validate_interval_maximum);
    RUN_TEST(test_validate_interval_default);
    RUN_TEST(test_validate_interval_midrange);
    RUN_TEST(test_validate_interval_below_min);
    RUN_TEST(test_validate_interval_zero);
    RUN_TEST(test_validate_interval_above_max);
    RUN_TEST(test_validate_interval_very_large);

    // Interval getter/setter
    RUN_TEST(test_get_default_interval);
    RUN_TEST(test_set_and_get_interval);
    RUN_TEST(test_set_interval_max);

    // Sensor data getters
    RUN_TEST(test_initial_temperature);
    RUN_TEST(test_initial_humidity);
    RUN_TEST(test_initial_status);

    // Module descriptor
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    // Registration
    RUN_TEST(test_register_env_module);
    RUN_TEST(test_find_env_after_register);

    // Config defines
    RUN_TEST(test_config_dht_pin);
    RUN_TEST(test_config_dht_type);
    RUN_TEST(test_config_read_min_ms);
    RUN_TEST(test_config_read_max_ms);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
