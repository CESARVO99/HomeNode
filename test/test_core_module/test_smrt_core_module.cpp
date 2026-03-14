/**
 * @file    test_smrt_core_module.cpp
 * @brief   Unit tests for smrt_core_module — registry, dispatch, lifecycle
 * @project HOMENODE
 * @version 0.2.0
 *
 * Tests run natively on the PC using PlatformIO's Unity framework.
 * Execute with: pio test -e native
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <unity.h>
#include <string.h>

#include "smrt_core_module.h"

// Include the source file directly (same pattern as format tests)
#include "../../src/core/smrt_core_module.cpp"

//=============================================================================
// Unity required setup/teardown
//=============================================================================
void setUp(void) {
    smrt_module_reset();  // Clean slate for each test
}

void tearDown(void) {}

//=============================================================================
// Test helper: mock module callbacks and state
//=============================================================================
static int mock_init_called = 0;
static int mock_loop_called = 0;
static const char *mock_ws_last_cmd = NULL;
static int mock_telemetry_called = 0;

static void mock_init(void) { mock_init_called++; }
static void mock_loop(void) { mock_loop_called++; }
static void mock_ws_handler(const char *cmd, void *doc, void *client) {
    mock_ws_last_cmd = cmd;
}
static void mock_get_telemetry(void *data) { mock_telemetry_called++; }

// Module descriptors
static const smrt_module_t mod_env = {
    "env", "Environmental", "1.0.0",
    mock_init, mock_loop, mock_ws_handler, mock_get_telemetry
};

static const smrt_module_t mod_sec = {
    "sec", "Security", "1.0.0",
    mock_init, mock_loop, mock_ws_handler, mock_get_telemetry
};

static const smrt_module_t mod_rly = {
    "rly", "Relay", "1.0.0",
    NULL, NULL, NULL, NULL  // All callbacks NULL
};

//=============================================================================
// Test: register and count
//=============================================================================

void test_register_single_module(void) {
    TEST_ASSERT_EQUAL(0, smrt_module_count());
    int ok = smrt_module_register(&mod_env);
    TEST_ASSERT_EQUAL(1, ok);
    TEST_ASSERT_EQUAL(1, smrt_module_count());
}

void test_register_multiple_modules(void) {
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);
    TEST_ASSERT_EQUAL(2, smrt_module_count());
}

void test_register_null_returns_zero(void) {
    int ok = smrt_module_register(NULL);
    TEST_ASSERT_EQUAL(0, ok);
    TEST_ASSERT_EQUAL(0, smrt_module_count());
}

void test_register_overflow(void) {
    // Fill up the registry to max
    smrt_module_t dummy[SMRT_MAX_MODULES];
    char ids[SMRT_MAX_MODULES][4];
    int i;
    for (i = 0; i < SMRT_MAX_MODULES; i++) {
        ids[i][0] = 'm';
        ids[i][1] = '0' + (char)i;
        ids[i][2] = '\0';
        dummy[i].id = ids[i];
        dummy[i].name = "Dummy";
        dummy[i].version = "0.0.0";
        dummy[i].init = NULL;
        dummy[i].loop = NULL;
        dummy[i].ws_handler = NULL;
        dummy[i].get_telemetry = NULL;
        TEST_ASSERT_EQUAL(1, smrt_module_register(&dummy[i]));
    }
    TEST_ASSERT_EQUAL(SMRT_MAX_MODULES, smrt_module_count());

    // Next registration should fail
    int ok = smrt_module_register(&mod_env);
    TEST_ASSERT_EQUAL(0, ok);
    TEST_ASSERT_EQUAL(SMRT_MAX_MODULES, smrt_module_count());
}

//=============================================================================
// Test: get and find
//=============================================================================

void test_get_by_index(void) {
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);

    const smrt_module_t *m0 = smrt_module_get(0);
    TEST_ASSERT_NOT_NULL(m0);
    TEST_ASSERT_EQUAL_STRING("env", m0->id);

    const smrt_module_t *m1 = smrt_module_get(1);
    TEST_ASSERT_NOT_NULL(m1);
    TEST_ASSERT_EQUAL_STRING("sec", m1->id);
}

void test_get_out_of_range(void) {
    smrt_module_register(&mod_env);
    TEST_ASSERT_NULL(smrt_module_get(1));
    TEST_ASSERT_NULL(smrt_module_get(-1));
}

void test_find_by_id(void) {
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);

    const smrt_module_t *found = smrt_module_find("sec");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Security", found->name);
}

void test_find_not_found(void) {
    smrt_module_register(&mod_env);
    TEST_ASSERT_NULL(smrt_module_find("xyz"));
    TEST_ASSERT_NULL(smrt_module_find(NULL));
}

//=============================================================================
// Test: init_all and loop_all
//=============================================================================

void test_init_all(void) {
    mock_init_called = 0;
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);
    smrt_module_register(&mod_rly);  // NULL init

    smrt_module_init_all();
    TEST_ASSERT_EQUAL(2, mock_init_called);  // mod_rly has NULL init
}

void test_loop_all(void) {
    mock_loop_called = 0;
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_rly);  // NULL loop

    smrt_module_loop_all();
    TEST_ASSERT_EQUAL(1, mock_loop_called);
}

//=============================================================================
// Test: dispatch with prefix stripping
//=============================================================================

void test_dispatch_env_read(void) {
    mock_ws_last_cmd = NULL;
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);

    int handled = smrt_module_dispatch("env_read", NULL, NULL);
    TEST_ASSERT_EQUAL(1, handled);
    TEST_ASSERT_NOT_NULL(mock_ws_last_cmd);
    TEST_ASSERT_EQUAL_STRING("read", mock_ws_last_cmd);
}

void test_dispatch_sec_arm(void) {
    mock_ws_last_cmd = NULL;
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);

    int handled = smrt_module_dispatch("sec_arm", NULL, NULL);
    TEST_ASSERT_EQUAL(1, handled);
    TEST_ASSERT_EQUAL_STRING("arm", mock_ws_last_cmd);
}

void test_dispatch_unknown_command(void) {
    smrt_module_register(&mod_env);

    int handled = smrt_module_dispatch("xyz_test", NULL, NULL);
    TEST_ASSERT_EQUAL(0, handled);
}

void test_dispatch_null_command(void) {
    smrt_module_register(&mod_env);
    int handled = smrt_module_dispatch(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, handled);
}

void test_dispatch_no_underscore(void) {
    smrt_module_register(&mod_env);
    int handled = smrt_module_dispatch("envread", NULL, NULL);
    TEST_ASSERT_EQUAL(0, handled);
}

void test_dispatch_null_handler(void) {
    smrt_module_register(&mod_rly);  // ws_handler is NULL

    int handled = smrt_module_dispatch("rly_toggle", NULL, NULL);
    TEST_ASSERT_EQUAL(1, handled);  // Module matched, but handler is NULL (no crash)
}

//=============================================================================
// Test: telemetry aggregation
//=============================================================================

void test_telemetry_all(void) {
    mock_telemetry_called = 0;
    smrt_module_register(&mod_env);
    smrt_module_register(&mod_sec);
    smrt_module_register(&mod_rly);  // NULL get_telemetry

    smrt_module_get_telemetry_all(NULL);
    TEST_ASSERT_EQUAL(2, mock_telemetry_called);
}

//=============================================================================
// Test runner
//=============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Register and count
    RUN_TEST(test_register_single_module);
    RUN_TEST(test_register_multiple_modules);
    RUN_TEST(test_register_null_returns_zero);
    RUN_TEST(test_register_overflow);

    // Get and find
    RUN_TEST(test_get_by_index);
    RUN_TEST(test_get_out_of_range);
    RUN_TEST(test_find_by_id);
    RUN_TEST(test_find_not_found);

    // Lifecycle
    RUN_TEST(test_init_all);
    RUN_TEST(test_loop_all);

    // Dispatch
    RUN_TEST(test_dispatch_env_read);
    RUN_TEST(test_dispatch_sec_arm);
    RUN_TEST(test_dispatch_unknown_command);
    RUN_TEST(test_dispatch_null_command);
    RUN_TEST(test_dispatch_no_underscore);
    RUN_TEST(test_dispatch_null_handler);

    // Telemetry
    RUN_TEST(test_telemetry_all);

    return UNITY_END();
}
