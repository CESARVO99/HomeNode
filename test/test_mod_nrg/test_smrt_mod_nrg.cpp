/**
 * @file    test_smrt_mod_nrg.cpp
 * @brief   Unit tests for smrt_mod_nrg — validation, calculations, moving avg, registration
 * @project HOMENODE
 * @version 0.4.0
 *
 * Tests the pure-logic parts of the Energy module:
 *   - Interval validation
 *   - Alert threshold validation
 *   - Channel count validation
 *   - Power calculation
 *   - Apparent power calculation
 *   - Power factor calculation
 *   - Energy calculation
 *   - Moving average
 *   - Getters/setters
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
#include <math.h>

#include "../../src/modules/smrt_mod_nrg.cpp"
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    smrt_nrg_set_channels(SMRT_NRG_DEFAULT_CHANNELS);
    smrt_nrg_set_interval(SMRT_NRG_INTERVAL_DEFAULT_MS);
    smrt_nrg_set_alert(SMRT_NRG_ALERT_DEFAULT_W);
    int i;
    for (i = 0; i < SMRT_NRG_MAX_CHANNELS; i++) {
        smrt_nrg_set_energy(i, 0.0f);
    }
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: interval validation
//=============================================================================

void test_validate_interval_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_interval(SMRT_NRG_INTERVAL_MIN_MS));
}

void test_validate_interval_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_interval(SMRT_NRG_INTERVAL_MAX_MS));
}

void test_validate_interval_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_interval(SMRT_NRG_INTERVAL_DEFAULT_MS));
}

void test_validate_interval_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_interval(999));
}

void test_validate_interval_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_interval(60001));
}

void test_validate_interval_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_interval(0));
}

//=============================================================================
// Test: alert threshold validation
//=============================================================================

void test_validate_alert_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_alert(SMRT_NRG_ALERT_MIN_W));
}

void test_validate_alert_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_alert(SMRT_NRG_ALERT_MAX_W));
}

void test_validate_alert_default(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_alert(SMRT_NRG_ALERT_DEFAULT_W));
}

void test_validate_alert_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_alert(50.0f));
}

void test_validate_alert_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_alert(10001.0f));
}

//=============================================================================
// Test: channel count validation
//=============================================================================

void test_validate_channels_one(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_channels(1));
}

void test_validate_channels_max(void) {
    TEST_ASSERT_EQUAL(1, smrt_nrg_validate_channels(SMRT_NRG_MAX_CHANNELS));
}

void test_validate_channels_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_channels(0));
}

void test_validate_channels_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_nrg_validate_channels(5));
}

//=============================================================================
// Test: power calculation
//=============================================================================

void test_calc_power_basic(void) {
    float p = smrt_nrg_calc_power(220.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2200.0f, p);
}

void test_calc_power_zero(void) {
    float p = smrt_nrg_calc_power(220.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, p);
}

void test_calc_apparent_basic(void) {
    float va = smrt_nrg_calc_apparent_power(220.0f, 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1100.0f, va);
}

//=============================================================================
// Test: power factor
//=============================================================================

void test_pf_unity(void) {
    float pf = smrt_nrg_calc_power_factor(1000.0f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pf);
}

void test_pf_half(void) {
    float pf = smrt_nrg_calc_power_factor(500.0f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, pf);
}

void test_pf_zero_apparent(void) {
    float pf = smrt_nrg_calc_power_factor(100.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pf);
}

//=============================================================================
// Test: energy calculation
//=============================================================================

void test_calc_energy_one_hour(void) {
    float e = smrt_nrg_calc_energy(1000.0f, 3600000UL);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, e);
}

void test_calc_energy_zero_time(void) {
    float e = smrt_nrg_calc_energy(500.0f, 0UL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, e);
}

void test_calc_energy_short_interval(void) {
    /* 1000W for 5 seconds = 1000 * 5/3600 = 1.389 Wh */
    float e = smrt_nrg_calc_energy(1000.0f, 5000UL);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.389f, e);
}

//=============================================================================
// Test: moving average
//=============================================================================

void test_moving_avg_null(void) {
    float avg = smrt_nrg_moving_avg(NULL, 5, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, avg);
}

void test_moving_avg_zero_count(void) {
    float buf[] = {1.0f, 2.0f, 3.0f};
    float avg = smrt_nrg_moving_avg(buf, 3, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, avg);
}

void test_moving_avg_basic(void) {
    float buf[] = {100.0f, 200.0f, 300.0f};
    float avg = smrt_nrg_moving_avg(buf, 3, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, avg);
}

void test_moving_avg_partial(void) {
    float buf[5] = {100.0f, 200.0f, 0.0f, 0.0f, 0.0f};
    float avg = smrt_nrg_moving_avg(buf, 5, 2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, avg);
}

//=============================================================================
// Test: getters/setters
//=============================================================================

void test_channels_default(void) {
    TEST_ASSERT_EQUAL(SMRT_NRG_DEFAULT_CHANNELS, smrt_nrg_get_channels());
}

void test_channels_set_and_get(void) {
    smrt_nrg_set_channels(3);
    TEST_ASSERT_EQUAL(3, smrt_nrg_get_channels());
}

void test_interval_default(void) {
    TEST_ASSERT_EQUAL(SMRT_NRG_INTERVAL_DEFAULT_MS, smrt_nrg_get_interval());
}

void test_interval_set_and_get(void) {
    smrt_nrg_set_interval(10000);
    TEST_ASSERT_EQUAL(10000, smrt_nrg_get_interval());
}

void test_alert_default(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, SMRT_NRG_ALERT_DEFAULT_W, smrt_nrg_get_alert());
}

void test_alert_set_and_get(void) {
    smrt_nrg_set_alert(5000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5000.0f, smrt_nrg_get_alert());
}

void test_energy_per_channel(void) {
    smrt_nrg_set_energy(0, 100.0f);
    smrt_nrg_set_energy(1, 200.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, smrt_nrg_get_energy(0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, smrt_nrg_get_energy(1));
}

void test_energy_out_of_range(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, smrt_nrg_get_energy(-1));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, smrt_nrg_get_energy(4));
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("nrg", smrt_mod_nrg.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Energy Monitor", smrt_mod_nrg.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.5.0", smrt_mod_nrg.version);
}

//=============================================================================
// Test: registration
//=============================================================================

void test_register_nrg_module(void) {
    int ok = smrt_module_register(&smrt_mod_nrg);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_nrg_after_register(void) {
    smrt_module_register(&smrt_mod_nrg);
    const smrt_module_t *found = smrt_module_find("nrg");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Energy Monitor", found->name);
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_max_channels(void) {
    TEST_ASSERT_EQUAL(4, SMRT_NRG_MAX_CHANNELS);
}

void test_config_adc_samples(void) {
    TEST_ASSERT_EQUAL(200, SMRT_NRG_ADC_SAMPLES);
}

void test_config_avg_window(void) {
    TEST_ASSERT_EQUAL(5, SMRT_NRG_AVG_WINDOW);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("nrg", SMRT_NRG_NVS_NAMESPACE);
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
    RUN_TEST(test_validate_interval_below_min);
    RUN_TEST(test_validate_interval_above_max);
    RUN_TEST(test_validate_interval_zero);

    /* Alert validation */
    RUN_TEST(test_validate_alert_minimum);
    RUN_TEST(test_validate_alert_maximum);
    RUN_TEST(test_validate_alert_default);
    RUN_TEST(test_validate_alert_below_min);
    RUN_TEST(test_validate_alert_above_max);

    /* Channel validation */
    RUN_TEST(test_validate_channels_one);
    RUN_TEST(test_validate_channels_max);
    RUN_TEST(test_validate_channels_zero);
    RUN_TEST(test_validate_channels_above_max);

    /* Power calc */
    RUN_TEST(test_calc_power_basic);
    RUN_TEST(test_calc_power_zero);
    RUN_TEST(test_calc_apparent_basic);

    /* Power factor */
    RUN_TEST(test_pf_unity);
    RUN_TEST(test_pf_half);
    RUN_TEST(test_pf_zero_apparent);

    /* Energy calc */
    RUN_TEST(test_calc_energy_one_hour);
    RUN_TEST(test_calc_energy_zero_time);
    RUN_TEST(test_calc_energy_short_interval);

    /* Moving average */
    RUN_TEST(test_moving_avg_null);
    RUN_TEST(test_moving_avg_zero_count);
    RUN_TEST(test_moving_avg_basic);
    RUN_TEST(test_moving_avg_partial);

    /* Getters/setters */
    RUN_TEST(test_channels_default);
    RUN_TEST(test_channels_set_and_get);
    RUN_TEST(test_interval_default);
    RUN_TEST(test_interval_set_and_get);
    RUN_TEST(test_alert_default);
    RUN_TEST(test_alert_set_and_get);
    RUN_TEST(test_energy_per_channel);
    RUN_TEST(test_energy_out_of_range);

    /* Descriptor */
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    /* Registration */
    RUN_TEST(test_register_nrg_module);
    RUN_TEST(test_find_nrg_after_register);

    /* Config defines */
    RUN_TEST(test_config_max_channels);
    RUN_TEST(test_config_adc_samples);
    RUN_TEST(test_config_avg_window);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
