/**
 * @file    test_smrt_mod_plg.cpp
 * @brief   Unit tests for smrt_mod_plg — validation, calculations, getters, registration
 * @project HOMENODE
 * @version 0.4.0
 *
 * Tests the pure-logic (non-hardware) parts of the Smart Plug module:
 *   - Interval validation
 *   - Overload validation
 *   - Power calculation
 *   - Energy calculation
 *   - RMS calculation
 *   - State/voltage/current/power/energy getters/setters
 *   - Module descriptor fields
 *   - Module registration
 *   - Config defines
 *
 * Execute with: pio test -e native
 */

//=============================================================================
// Includes
//=============================================================================
#include <unity.h>
#include <string.h>
#include <math.h>

#include "../../src/modules/smrt_mod_plg.cpp"
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    smrt_plg_set_state(0);
    smrt_plg_set_energy(0.0f);
    smrt_plg_set_overload(SMRT_PLG_OVERLOAD_DEFAULT_A);
    smrt_plg_set_interval(SMRT_PLG_INTERVAL_DEFAULT_MS);
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: interval validation
//=============================================================================

void test_validate_interval_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_interval(SMRT_PLG_INTERVAL_MIN_MS));
}

void test_validate_interval_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_interval(SMRT_PLG_INTERVAL_MAX_MS));
}

void test_validate_interval_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_interval(SMRT_PLG_INTERVAL_DEFAULT_MS));
}

void test_validate_interval_midrange(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_interval(10000));
}

void test_validate_interval_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_validate_interval(999));
}

void test_validate_interval_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_validate_interval(60001));
}

void test_validate_interval_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_validate_interval(0));
}

//=============================================================================
// Test: overload validation
//=============================================================================

void test_validate_overload_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_overload(SMRT_PLG_OVERLOAD_MIN_A));
}

void test_validate_overload_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_overload(SMRT_PLG_OVERLOAD_MAX_A));
}

void test_validate_overload_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_plg_validate_overload(SMRT_PLG_OVERLOAD_DEFAULT_A));
}

void test_validate_overload_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_validate_overload(0.5f));
}

void test_validate_overload_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_validate_overload(31.0f));
}

//=============================================================================
// Test: power calculation
//=============================================================================

void test_calc_power_basic(void) {
    float p = smrt_plg_calc_power(220.0f, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1100.0f, p);
}

void test_calc_power_zero_voltage(void) {
    float p = smrt_plg_calc_power(0.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, p);
}

void test_calc_power_zero_current(void) {
    float p = smrt_plg_calc_power(220.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, p);
}

void test_calc_power_fractional(void) {
    float p = smrt_plg_calc_power(120.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, p);
}

//=============================================================================
// Test: energy calculation
//=============================================================================

void test_calc_energy_one_hour(void) {
    /* 100W for 1 hour = 100 Wh */
    float e = smrt_plg_calc_energy(100.0f, 3600000UL);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, e);
}

void test_calc_energy_one_second(void) {
    /* 3600W for 1 second = 1 Wh */
    float e = smrt_plg_calc_energy(3600.0f, 1000UL);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, e);
}

void test_calc_energy_zero_time(void) {
    float e = smrt_plg_calc_energy(1000.0f, 0UL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, e);
}

//=============================================================================
// Test: RMS calculation
//=============================================================================

void test_calc_rms_null_samples(void) {
    float rms = smrt_plg_calc_rms(NULL, 10, 2048, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, rms);
}

void test_calc_rms_zero_count(void) {
    int samples[] = {2048};
    float rms = smrt_plg_calc_rms(samples, 0, 2048, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, rms);
}

void test_calc_rms_dc_offset_only(void) {
    /* All samples at midpoint => RMS = 0 */
    int samples[4] = {2048, 2048, 2048, 2048};
    float rms = smrt_plg_calc_rms(samples, 4, 2048, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, rms);
}

void test_calc_rms_known_values(void) {
    /* Samples: +100 and -100 around midpoint, scale=1 */
    int samples[4] = {2148, 1948, 2148, 1948};
    float rms = smrt_plg_calc_rms(samples, 4, 2048, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, rms);
}

//=============================================================================
// Test: state getter/setter
//=============================================================================

void test_state_initial_off(void) {
    TEST_ASSERT_EQUAL(0, smrt_plg_get_state());
}

void test_state_set_on(void) {
    smrt_plg_set_state(1);
    TEST_ASSERT_EQUAL(1, smrt_plg_get_state());
}

void test_state_set_off(void) {
    smrt_plg_set_state(1);
    smrt_plg_set_state(0);
    TEST_ASSERT_EQUAL(0, smrt_plg_get_state());
}

//=============================================================================
// Test: energy getter/setter
//=============================================================================

void test_energy_initial_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, smrt_plg_get_energy());
}

void test_energy_set_and_get(void) {
    smrt_plg_set_energy(123.45f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 123.45f, smrt_plg_get_energy());
}

void test_energy_reset(void) {
    smrt_plg_set_energy(500.0f);
    smrt_plg_set_energy(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, smrt_plg_get_energy());
}

//=============================================================================
// Test: overload getter/setter
//=============================================================================

void test_overload_default(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, SMRT_PLG_OVERLOAD_DEFAULT_A,
                             smrt_plg_get_overload());
}

void test_overload_set_and_get(void) {
    smrt_plg_set_overload(20.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, smrt_plg_get_overload());
}

//=============================================================================
// Test: interval getter/setter
//=============================================================================

void test_interval_default(void) {
    TEST_ASSERT_EQUAL(SMRT_PLG_INTERVAL_DEFAULT_MS, smrt_plg_get_interval());
}

void test_interval_set_and_get(void) {
    smrt_plg_set_interval(5000);
    TEST_ASSERT_EQUAL(5000, smrt_plg_get_interval());
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("plg", smrt_mod_plg.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Smart Plug", smrt_mod_plg.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.4.0", smrt_mod_plg.version);
}

//=============================================================================
// Test: registration
//=============================================================================

void test_register_plg_module(void) {
    int ok = smrt_module_register(&smrt_mod_plg);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_plg_after_register(void) {
    smrt_module_register(&smrt_mod_plg);
    const smrt_module_t *found = smrt_module_find("plg");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Smart Plug", found->name);
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_relay_pin(void) {
    TEST_ASSERT_EQUAL(2, SMRT_PLG_RELAY_PIN);
}

void test_config_current_pin(void) {
    TEST_ASSERT_EQUAL(34, SMRT_PLG_CURRENT_PIN);
}

void test_config_voltage_pin(void) {
    TEST_ASSERT_EQUAL(35, SMRT_PLG_VOLTAGE_PIN);
}

void test_config_adc_samples(void) {
    TEST_ASSERT_EQUAL(100, SMRT_PLG_ADC_SAMPLES);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("plg", SMRT_PLG_NVS_NAMESPACE);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    /* Interval validation */
    RUN_TEST(test_validate_interval_minimum);
    RUN_TEST(test_validate_interval_maximum);
    RUN_TEST(test_validate_interval_default);
    RUN_TEST(test_validate_interval_midrange);
    RUN_TEST(test_validate_interval_below_min);
    RUN_TEST(test_validate_interval_above_max);
    RUN_TEST(test_validate_interval_zero);

    /* Overload validation */
    RUN_TEST(test_validate_overload_minimum);
    RUN_TEST(test_validate_overload_maximum);
    RUN_TEST(test_validate_overload_default);
    RUN_TEST(test_validate_overload_below_min);
    RUN_TEST(test_validate_overload_above_max);

    /* Power calculation */
    RUN_TEST(test_calc_power_basic);
    RUN_TEST(test_calc_power_zero_voltage);
    RUN_TEST(test_calc_power_zero_current);
    RUN_TEST(test_calc_power_fractional);

    /* Energy calculation */
    RUN_TEST(test_calc_energy_one_hour);
    RUN_TEST(test_calc_energy_one_second);
    RUN_TEST(test_calc_energy_zero_time);

    /* RMS calculation */
    RUN_TEST(test_calc_rms_null_samples);
    RUN_TEST(test_calc_rms_zero_count);
    RUN_TEST(test_calc_rms_dc_offset_only);
    RUN_TEST(test_calc_rms_known_values);

    /* State */
    RUN_TEST(test_state_initial_off);
    RUN_TEST(test_state_set_on);
    RUN_TEST(test_state_set_off);

    /* Energy getter/setter */
    RUN_TEST(test_energy_initial_zero);
    RUN_TEST(test_energy_set_and_get);
    RUN_TEST(test_energy_reset);

    /* Overload getter/setter */
    RUN_TEST(test_overload_default);
    RUN_TEST(test_overload_set_and_get);

    /* Interval getter/setter */
    RUN_TEST(test_interval_default);
    RUN_TEST(test_interval_set_and_get);

    /* Descriptor */
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    /* Registration */
    RUN_TEST(test_register_plg_module);
    RUN_TEST(test_find_plg_after_register);

    /* Config defines */
    RUN_TEST(test_config_relay_pin);
    RUN_TEST(test_config_current_pin);
    RUN_TEST(test_config_voltage_pin);
    RUN_TEST(test_config_adc_samples);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
