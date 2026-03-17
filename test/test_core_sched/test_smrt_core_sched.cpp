/**
 * @file    test_smrt_core_sched.cpp
 * @brief   Unit tests for smrt_core_sched — task validation, time matching, action parsing
 * @project HOMENODE
 * @version 0.8.0
 *
 * Tests the pure-logic (non-hardware) parts of the scheduler:
 *   - Task validation (hour/minute/day bounds, action format)
 *   - Time matching (day bitmask, exact time, disabled tasks)
 *   - Action string parsing (command + arguments extraction)
 *   - Index validation
 *   - Day bitmask to string conversion
 *
 * Execute with: pio test -e native
 */

//=============================================================================
// Includes
//=============================================================================
#include <unity.h>
#include <string.h>

// Include scheduler source directly
#include "../../src/core/smrt_core_sched.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {}
void tearDown(void) {}

//=============================================================================
// Helpers
//=============================================================================

/** Returns a valid task with sensible defaults */
static smrt_sched_task_t make_task(uint8_t enabled, uint8_t hour, uint8_t minute,
                                    uint8_t days, const char *action, const char *name) {
    smrt_sched_task_t t;
    t.enabled = enabled;
    t.hour    = hour;
    t.minute  = minute;
    t.days    = days;
    strncpy(t.action, action, SMRT_SCHED_ACTION_LEN - 1);
    t.action[SMRT_SCHED_ACTION_LEN - 1] = '\0';
    strncpy(t.name, name, SMRT_SCHED_NAME_LEN - 1);
    t.name[SMRT_SCHED_NAME_LEN - 1] = '\0';
    return t;
}

//=============================================================================
// Test: task validation
//=============================================================================

void test_validate_task_valid(void) {
    smrt_sched_task_t t = make_task(1, 9, 30, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "Wake up");
    TEST_ASSERT_EQUAL(1, smrt_sched_validate_task(&t));
}

void test_validate_task_hour_max(void) {
    smrt_sched_task_t t = make_task(1, 23, 59, SMRT_SCHED_DAY_MON, "env_read", "Night");
    TEST_ASSERT_EQUAL(1, smrt_sched_validate_task(&t));
}

void test_validate_task_hour_invalid(void) {
    smrt_sched_task_t t = make_task(1, 24, 0, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "Bad");
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_task(&t));
}

void test_validate_task_minute_invalid(void) {
    smrt_sched_task_t t = make_task(1, 12, 60, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "Bad");
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_task(&t));
}

void test_validate_task_no_days(void) {
    smrt_sched_task_t t = make_task(1, 12, 0, 0, "rly_toggle:0", "Bad");
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_task(&t));
}

void test_validate_task_empty_action(void) {
    smrt_sched_task_t t = make_task(1, 12, 0, SMRT_SCHED_DAYS_ALL, "", "Bad");
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_task(&t));
}

void test_validate_task_null(void) {
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_task(NULL));
}

void test_validate_task_disabled_still_valid(void) {
    // Disabled tasks should still pass structural validation
    smrt_sched_task_t t = make_task(0, 8, 0, SMRT_SCHED_DAY_MON, "env_read", "Off");
    TEST_ASSERT_EQUAL(1, smrt_sched_validate_task(&t));
}

//=============================================================================
// Test: time matching
//=============================================================================

void test_match_time_exact(void) {
    smrt_sched_task_t t = make_task(1, 9, 30, SMRT_SCHED_DAY_MON, "rly_toggle:0", "Mon alarm");
    // Monday = dow 1
    TEST_ASSERT_EQUAL(1, smrt_sched_match_time(&t, 9, 30, 1));
}

void test_match_time_wrong_hour(void) {
    smrt_sched_task_t t = make_task(1, 9, 30, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "t");
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 10, 30, 1));
}

void test_match_time_wrong_minute(void) {
    smrt_sched_task_t t = make_task(1, 9, 30, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "t");
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 9, 31, 1));
}

void test_match_time_wrong_day(void) {
    // Only Monday
    smrt_sched_task_t t = make_task(1, 9, 30, SMRT_SCHED_DAY_MON, "rly_toggle:0", "t");
    // Tuesday = dow 2
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 9, 30, 2));
}

void test_match_time_disabled_task(void) {
    smrt_sched_task_t t = make_task(0, 9, 30, SMRT_SCHED_DAYS_ALL, "rly_toggle:0", "t");
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 9, 30, 1));
}

void test_match_time_all_days(void) {
    smrt_sched_task_t t = make_task(1, 6, 0, SMRT_SCHED_DAYS_ALL, "env_read", "t");
    // Should match any day of week 0-6
    for (int dow = 0; dow <= 6; dow++) {
        TEST_ASSERT_EQUAL(1, smrt_sched_match_time(&t, 6, 0, dow));
    }
}

void test_match_time_weekend_only(void) {
    uint8_t weekend = SMRT_SCHED_DAY_SAT | SMRT_SCHED_DAY_SUN;
    smrt_sched_task_t t = make_task(1, 10, 0, weekend, "rly_toggle:0", "Weekend");
    TEST_ASSERT_EQUAL(1, smrt_sched_match_time(&t, 10, 0, 0));  // Sunday
    TEST_ASSERT_EQUAL(1, smrt_sched_match_time(&t, 10, 0, 6));  // Saturday
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 10, 0, 1));  // Monday
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(&t, 10, 0, 5));  // Friday
}

void test_match_time_null_task(void) {
    TEST_ASSERT_EQUAL(0, smrt_sched_match_time(NULL, 9, 30, 1));
}

//=============================================================================
// Test: action parsing
//=============================================================================

void test_parse_action_simple(void) {
    char cmd[32] = {0};
    char args[32] = {0};
    int ok = smrt_sched_parse_action("env_read", cmd, args);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL_STRING("env_read", cmd);
    TEST_ASSERT_EQUAL_STRING("", args);
}

void test_parse_action_with_one_arg(void) {
    char cmd[32] = {0};
    char args[32] = {0};
    int ok = smrt_sched_parse_action("rly_toggle:0", cmd, args);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL_STRING("rly_toggle", cmd);
    TEST_ASSERT_EQUAL_STRING("0", args);
}

void test_parse_action_with_two_args(void) {
    char cmd[32] = {0};
    char args[32] = {0};
    int ok = smrt_sched_parse_action("rly_set:1:1", cmd, args);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL_STRING("rly_set", cmd);
    TEST_ASSERT_EQUAL_STRING("1:1", args);
}

void test_parse_action_empty(void) {
    char cmd[32] = {0};
    char args[32] = {0};
    int ok = smrt_sched_parse_action("", cmd, args);
    TEST_ASSERT_EQUAL(0, ok);
}

void test_parse_action_null(void) {
    char cmd[32] = {0};
    char args[32] = {0};
    int ok = smrt_sched_parse_action(NULL, cmd, args);
    TEST_ASSERT_EQUAL(0, ok);
}

//=============================================================================
// Test: index validation
//=============================================================================

void test_validate_index_first(void) {
    TEST_ASSERT_EQUAL(1, smrt_sched_validate_index(0));
}

void test_validate_index_last(void) {
    TEST_ASSERT_EQUAL(1, smrt_sched_validate_index(SMRT_SCHED_MAX_TASKS - 1));
}

void test_validate_index_negative(void) {
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_index(-1));
}

void test_validate_index_overflow(void) {
    TEST_ASSERT_EQUAL(0, smrt_sched_validate_index(SMRT_SCHED_MAX_TASKS));
}

//=============================================================================
// Test: days bitmask to string
//=============================================================================

void test_days_all_to_string(void) {
    char buf[32] = {0};
    smrt_sched_days_to_string(SMRT_SCHED_DAYS_ALL, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Dom,Lun,Mar,Mie,Jue,Vie,Sab", buf);
}

void test_days_weekday_only(void) {
    uint8_t weekdays = SMRT_SCHED_DAY_MON | SMRT_SCHED_DAY_TUE | SMRT_SCHED_DAY_WED
                     | SMRT_SCHED_DAY_THU | SMRT_SCHED_DAY_FRI;
    char buf[32] = {0};
    smrt_sched_days_to_string(weekdays, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Lun,Mar,Mie,Jue,Vie", buf);
}

void test_days_sunday_only(void) {
    char buf[32] = {0};
    smrt_sched_days_to_string(SMRT_SCHED_DAY_SUN, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Dom", buf);
}

void test_days_none(void) {
    char buf[32] = {0};
    smrt_sched_days_to_string(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Task validation
    RUN_TEST(test_validate_task_valid);
    RUN_TEST(test_validate_task_hour_max);
    RUN_TEST(test_validate_task_hour_invalid);
    RUN_TEST(test_validate_task_minute_invalid);
    RUN_TEST(test_validate_task_no_days);
    RUN_TEST(test_validate_task_empty_action);
    RUN_TEST(test_validate_task_null);
    RUN_TEST(test_validate_task_disabled_still_valid);

    // Time matching
    RUN_TEST(test_match_time_exact);
    RUN_TEST(test_match_time_wrong_hour);
    RUN_TEST(test_match_time_wrong_minute);
    RUN_TEST(test_match_time_wrong_day);
    RUN_TEST(test_match_time_disabled_task);
    RUN_TEST(test_match_time_all_days);
    RUN_TEST(test_match_time_weekend_only);
    RUN_TEST(test_match_time_null_task);

    // Action parsing
    RUN_TEST(test_parse_action_simple);
    RUN_TEST(test_parse_action_with_one_arg);
    RUN_TEST(test_parse_action_with_two_args);
    RUN_TEST(test_parse_action_empty);
    RUN_TEST(test_parse_action_null);

    // Index validation
    RUN_TEST(test_validate_index_first);
    RUN_TEST(test_validate_index_last);
    RUN_TEST(test_validate_index_negative);
    RUN_TEST(test_validate_index_overflow);

    // Days to string
    RUN_TEST(test_days_all_to_string);
    RUN_TEST(test_days_weekday_only);
    RUN_TEST(test_days_sunday_only);
    RUN_TEST(test_days_none);

    return UNITY_END();
}
