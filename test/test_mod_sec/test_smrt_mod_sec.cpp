/**
 * @file    test_smrt_mod_sec.cpp
 * @brief   Unit tests for smrt_mod_sec — validation, state machine, event log, registration
 * @project HOMENODE
 * @version 0.4.0
 *
 * Tests the pure-logic (non-hardware) parts of the Security module:
 *   - Delay validation
 *   - State machine transitions
 *   - Circular event log (add, get, clear, overflow)
 *   - Entry/exit delay getter/setter
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
#include <stdio.h>

#include "../../src/modules/smrt_mod_sec.cpp"
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    /* Reset module state */
    smrt_sec_set_alarm_state(SMRT_SEC_STATE_DISARMED);
    smrt_sec_set_entry_delay(SMRT_SEC_ENTRY_DELAY_MS);
    smrt_sec_set_exit_delay(SMRT_SEC_EXIT_DELAY_MS);
    smrt_sec_clear_events();
    smrt_module_reset();
}

void tearDown(void) {}

//=============================================================================
// Test: delay validation
//=============================================================================

void test_validate_delay_minimum(void) {
    TEST_ASSERT_EQUAL(1, smrt_sec_validate_delay(SMRT_SEC_DELAY_MIN_MS));
}

void test_validate_delay_maximum(void) {
    TEST_ASSERT_EQUAL(1, smrt_sec_validate_delay(SMRT_SEC_DELAY_MAX_MS));
}

void test_validate_delay_default_entry(void) {
    TEST_ASSERT_EQUAL(1, smrt_sec_validate_delay(SMRT_SEC_ENTRY_DELAY_MS));
}

void test_validate_delay_midrange(void) {
    TEST_ASSERT_EQUAL(1, smrt_sec_validate_delay(60000));
}

void test_validate_delay_below_min(void) {
    TEST_ASSERT_EQUAL(0, smrt_sec_validate_delay(4999));
}

void test_validate_delay_zero(void) {
    TEST_ASSERT_EQUAL(0, smrt_sec_validate_delay(0));
}

void test_validate_delay_above_max(void) {
    TEST_ASSERT_EQUAL(0, smrt_sec_validate_delay(120001));
}

//=============================================================================
// Test: state machine transitions
//=============================================================================

void test_transition_disarmed_arm_goes_exit_delay(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_DISARMED, SMRT_SEC_EVT_ARM);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_EXIT_DELAY, next);
}

void test_transition_exit_delay_timeout_goes_armed(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_EXIT_DELAY, SMRT_SEC_EVT_TIMEOUT);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_ARMED, next);
}

void test_transition_armed_motion_goes_entry_delay(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_ARMED, SMRT_SEC_EVT_MOTION);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_ENTRY_DELAY, next);
}

void test_transition_armed_door_goes_entry_delay(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_ARMED, SMRT_SEC_EVT_DOOR_OPEN);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_ENTRY_DELAY, next);
}

void test_transition_armed_vibration_goes_entry_delay(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_ARMED, SMRT_SEC_EVT_VIBRATION);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_ENTRY_DELAY, next);
}

void test_transition_entry_delay_timeout_goes_triggered(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_ENTRY_DELAY, SMRT_SEC_EVT_TIMEOUT);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_TRIGGERED, next);
}

void test_transition_disarm_from_any_state(void) {
    int next;
    next = smrt_sec_transition(SMRT_SEC_STATE_ARMED, SMRT_SEC_EVT_DISARM);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, next);

    next = smrt_sec_transition(SMRT_SEC_STATE_TRIGGERED, SMRT_SEC_EVT_DISARM);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, next);

    next = smrt_sec_transition(SMRT_SEC_STATE_EXIT_DELAY, SMRT_SEC_EVT_DISARM);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, next);

    next = smrt_sec_transition(SMRT_SEC_STATE_ENTRY_DELAY, SMRT_SEC_EVT_DISARM);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, next);
}

void test_transition_disarmed_ignores_motion(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_DISARMED, SMRT_SEC_EVT_MOTION);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, next);
}

void test_transition_triggered_ignores_motion(void) {
    int next = smrt_sec_transition(SMRT_SEC_STATE_TRIGGERED, SMRT_SEC_EVT_MOTION);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_TRIGGERED, next);
}

//=============================================================================
// Test: event log
//=============================================================================

void test_event_log_initially_empty(void) {
    TEST_ASSERT_EQUAL(0, smrt_sec_get_event_count());
}

void test_event_add_and_get(void) {
    smrt_sec_add_event("Test event", 1000);
    TEST_ASSERT_EQUAL(1, smrt_sec_get_event_count());
    unsigned long ts = 0;
    const char *msg = smrt_sec_get_event(0, &ts);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Test event", msg);
    TEST_ASSERT_EQUAL(1000, ts);
}

void test_event_log_multiple(void) {
    smrt_sec_add_event("Event A", 100);
    smrt_sec_add_event("Event B", 200);
    smrt_sec_add_event("Event C", 300);
    TEST_ASSERT_EQUAL(3, smrt_sec_get_event_count());
    TEST_ASSERT_EQUAL_STRING("Event A", smrt_sec_get_event(0, NULL));
    TEST_ASSERT_EQUAL_STRING("Event C", smrt_sec_get_event(2, NULL));
}

void test_event_log_clear(void) {
    smrt_sec_add_event("To clear", 500);
    smrt_sec_clear_events();
    TEST_ASSERT_EQUAL(0, smrt_sec_get_event_count());
}

void test_event_log_overflow_circular(void) {
    int i;
    char buf[32];
    /* Fill beyond capacity */
    for (i = 0; i < SMRT_SEC_EVENT_LOG_SIZE + 4; i++) {
        snprintf(buf, sizeof(buf), "Evt %d", i);
        smrt_sec_add_event(buf, (unsigned long)(i * 100));
    }
    /* Count should be capped at max */
    TEST_ASSERT_EQUAL(SMRT_SEC_EVENT_LOG_SIZE, smrt_sec_get_event_count());
    /* Oldest event should be evt #4 (first 4 overwritten) */
    const char *oldest = smrt_sec_get_event(0, NULL);
    TEST_ASSERT_NOT_NULL(oldest);
    TEST_ASSERT_EQUAL_STRING("Evt 4", oldest);
}

void test_event_log_null_msg_ignored(void) {
    smrt_sec_add_event(NULL, 0);
    TEST_ASSERT_EQUAL(0, smrt_sec_get_event_count());
}

void test_event_get_out_of_range(void) {
    TEST_ASSERT_NULL(smrt_sec_get_event(-1, NULL));
    TEST_ASSERT_NULL(smrt_sec_get_event(0, NULL));
    smrt_sec_add_event("Only one", 100);
    TEST_ASSERT_NULL(smrt_sec_get_event(1, NULL));
}

//=============================================================================
// Test: delay getter/setter
//=============================================================================

void test_entry_delay_default(void) {
    TEST_ASSERT_EQUAL(SMRT_SEC_ENTRY_DELAY_MS, smrt_sec_get_entry_delay());
}

void test_entry_delay_set_and_get(void) {
    smrt_sec_set_entry_delay(15000);
    TEST_ASSERT_EQUAL(15000, smrt_sec_get_entry_delay());
}

void test_exit_delay_default(void) {
    TEST_ASSERT_EQUAL(SMRT_SEC_EXIT_DELAY_MS, smrt_sec_get_exit_delay());
}

void test_exit_delay_set_and_get(void) {
    smrt_sec_set_exit_delay(45000);
    TEST_ASSERT_EQUAL(45000, smrt_sec_get_exit_delay());
}

//=============================================================================
// Test: alarm state getter/setter
//=============================================================================

void test_alarm_state_default(void) {
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_DISARMED, smrt_sec_get_alarm_state());
}

void test_alarm_state_set_and_get(void) {
    smrt_sec_set_alarm_state(SMRT_SEC_STATE_ARMED);
    TEST_ASSERT_EQUAL(SMRT_SEC_STATE_ARMED, smrt_sec_get_alarm_state());
}

//=============================================================================
// Test: module descriptor
//=============================================================================

void test_descriptor_id(void) {
    TEST_ASSERT_EQUAL_STRING("sec", smrt_mod_sec.id);
}

void test_descriptor_name(void) {
    TEST_ASSERT_EQUAL_STRING("Security", smrt_mod_sec.name);
}

void test_descriptor_version(void) {
    TEST_ASSERT_EQUAL_STRING("0.4.0", smrt_mod_sec.version);
}

//=============================================================================
// Test: registration
//=============================================================================

void test_register_sec_module(void) {
    int ok = smrt_module_register(&smrt_mod_sec);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_find_sec_after_register(void) {
    smrt_module_register(&smrt_mod_sec);
    const smrt_module_t *found = smrt_module_find("sec");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Security", found->name);
}

//=============================================================================
// Test: config defines
//=============================================================================

void test_config_pir_pin(void) {
    TEST_ASSERT_EQUAL(12, SMRT_SEC_PIR_PIN);
}

void test_config_reed_pin(void) {
    TEST_ASSERT_EQUAL(13, SMRT_SEC_REED_PIN);
}

void test_config_event_log_size(void) {
    TEST_ASSERT_EQUAL(16, SMRT_SEC_EVENT_LOG_SIZE);
}

void test_config_debounce(void) {
    TEST_ASSERT_EQUAL(200, SMRT_SEC_DEBOUNCE_MS);
}

void test_config_nvs_namespace(void) {
    TEST_ASSERT_EQUAL_STRING("sec", SMRT_SEC_NVS_NAMESPACE);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    /* Delay validation */
    RUN_TEST(test_validate_delay_minimum);
    RUN_TEST(test_validate_delay_maximum);
    RUN_TEST(test_validate_delay_default_entry);
    RUN_TEST(test_validate_delay_midrange);
    RUN_TEST(test_validate_delay_below_min);
    RUN_TEST(test_validate_delay_zero);
    RUN_TEST(test_validate_delay_above_max);

    /* State machine */
    RUN_TEST(test_transition_disarmed_arm_goes_exit_delay);
    RUN_TEST(test_transition_exit_delay_timeout_goes_armed);
    RUN_TEST(test_transition_armed_motion_goes_entry_delay);
    RUN_TEST(test_transition_armed_door_goes_entry_delay);
    RUN_TEST(test_transition_armed_vibration_goes_entry_delay);
    RUN_TEST(test_transition_entry_delay_timeout_goes_triggered);
    RUN_TEST(test_transition_disarm_from_any_state);
    RUN_TEST(test_transition_disarmed_ignores_motion);
    RUN_TEST(test_transition_triggered_ignores_motion);

    /* Event log */
    RUN_TEST(test_event_log_initially_empty);
    RUN_TEST(test_event_add_and_get);
    RUN_TEST(test_event_log_multiple);
    RUN_TEST(test_event_log_clear);
    RUN_TEST(test_event_log_overflow_circular);
    RUN_TEST(test_event_log_null_msg_ignored);
    RUN_TEST(test_event_get_out_of_range);

    /* Delay getter/setter */
    RUN_TEST(test_entry_delay_default);
    RUN_TEST(test_entry_delay_set_and_get);
    RUN_TEST(test_exit_delay_default);
    RUN_TEST(test_exit_delay_set_and_get);

    /* Alarm state */
    RUN_TEST(test_alarm_state_default);
    RUN_TEST(test_alarm_state_set_and_get);

    /* Descriptor */
    RUN_TEST(test_descriptor_id);
    RUN_TEST(test_descriptor_name);
    RUN_TEST(test_descriptor_version);

    /* Registration */
    RUN_TEST(test_register_sec_module);
    RUN_TEST(test_find_sec_after_register);

    /* Config defines */
    RUN_TEST(test_config_pir_pin);
    RUN_TEST(test_config_reed_pin);
    RUN_TEST(test_config_event_log_size);
    RUN_TEST(test_config_debounce);
    RUN_TEST(test_config_nvs_namespace);

    return UNITY_END();
}
