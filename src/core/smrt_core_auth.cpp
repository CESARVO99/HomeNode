/**
 * @file    smrt_core_auth.cpp
 * @brief   Authentication, rate limiting and WebSocket session management
 * @project HOMENODE
 * @version 0.7.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// WebSocket authenticated client tracking
//-----------------------------------------------------------------------------
static uint32_t      smrt_auth_clients[SMRT_AUTH_MAX_WS_CLIENTS];
static unsigned long smrt_auth_timestamps[SMRT_AUTH_MAX_WS_CLIENTS];  /**< Last activity per slot */
static int           smrt_auth_client_count = 0;

//-----------------------------------------------------------------------------
// PIN rate limiting state
//-----------------------------------------------------------------------------
static int           smrt_pin_fail_count = 0;
static unsigned long smrt_pin_lockout_start = 0;

//-----------------------------------------------------------------------------
// Public functions — Auth subsystem init
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes the authentication subsystem.
 * @return void
 */
void smrt_auth_init(void) {
    for (int i = 0; i < SMRT_AUTH_MAX_WS_CLIENTS; i++) {
        smrt_auth_clients[i] = 0;
        smrt_auth_timestamps[i] = 0;
    }
    smrt_auth_client_count = 0;
    smrt_pin_fail_count = 0;
    smrt_pin_lockout_start = 0;
    SMRT_DEBUG_LOG("Auth subsystem initialized");
}

//-----------------------------------------------------------------------------
// Public functions — WebSocket session tracking
//-----------------------------------------------------------------------------

/**
 * @brief  Marks a WebSocket client as authenticated.
 * @param  client_id  AsyncWebSocketClient ID
 * @return 1 on success, 0 if slots full
 */
int smrt_auth_ws_login(uint32_t client_id) {
    /* Already authenticated? */
    for (int i = 0; i < smrt_auth_client_count; i++) {
        if (smrt_auth_clients[i] == client_id) {
            return 1;
        }
    }
    /* Add to list */
    if (smrt_auth_client_count >= SMRT_AUTH_MAX_WS_CLIENTS) {
        return 0;
    }
    smrt_auth_clients[smrt_auth_client_count] = client_id;
    smrt_auth_timestamps[smrt_auth_client_count] = millis();
    smrt_auth_client_count++;
    return 1;
}

/**
 * @brief  Removes a WebSocket client from authenticated list.
 * @param  client_id  AsyncWebSocketClient ID
 * @return void
 */
void smrt_auth_ws_logout(uint32_t client_id) {
    for (int i = 0; i < smrt_auth_client_count; i++) {
        if (smrt_auth_clients[i] == client_id) {
            /* Shift remaining entries */
            for (int j = i; j < smrt_auth_client_count - 1; j++) {
                smrt_auth_clients[j] = smrt_auth_clients[j + 1];
                smrt_auth_timestamps[j] = smrt_auth_timestamps[j + 1];
            }
            smrt_auth_client_count--;
            return;
        }
    }
}

/**
 * @brief  Checks if a WebSocket client is authenticated.
 * @param  client_id  AsyncWebSocketClient ID
 * @return 1 if authenticated, 0 otherwise
 */
int smrt_auth_ws_is_authenticated(uint32_t client_id) {
    for (int i = 0; i < smrt_auth_client_count; i++) {
        if (smrt_auth_clients[i] == client_id) {
            return 1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Public functions — PIN rate limiting
//-----------------------------------------------------------------------------

/**
 * @brief  Checks if PIN attempts are currently locked out.
 * @return 1 if locked out, 0 if allowed
 */
int smrt_auth_pin_is_locked(void) {
    if (smrt_pin_fail_count < SMRT_AUTH_PIN_MAX_ATTEMPTS) {
        return 0;
    }
    /* Check if lockout period has expired */
    unsigned long elapsed = millis() - smrt_pin_lockout_start;
    if (elapsed >= SMRT_AUTH_PIN_LOCKOUT_MS) {
        /* Lockout expired, reset */
        smrt_pin_fail_count = 0;
        smrt_pin_lockout_start = 0;
        return 0;
    }
    return 1;
}

/**
 * @brief  Records a failed PIN attempt.
 * @return void
 */
void smrt_auth_pin_fail(void) {
    smrt_pin_fail_count++;
    if (smrt_pin_fail_count >= SMRT_AUTH_PIN_MAX_ATTEMPTS) {
        smrt_pin_lockout_start = millis();
        SMRT_DEBUG_PRINTF("AUTH: PIN locked out for %lus after %d failed attempts\n",
                          SMRT_AUTH_PIN_LOCKOUT_MS / 1000, smrt_pin_fail_count);
    }
}

/**
 * @brief  Resets the failed PIN attempt counter.
 * @return void
 */
void smrt_auth_pin_reset(void) {
    smrt_pin_fail_count = 0;
    smrt_pin_lockout_start = 0;
}

/**
 * @brief  Returns remaining lockout seconds, 0 if not locked.
 * @return Seconds remaining
 */
unsigned long smrt_auth_pin_lockout_remaining(void) {
    if (smrt_pin_fail_count < SMRT_AUTH_PIN_MAX_ATTEMPTS) {
        return 0;
    }
    unsigned long elapsed = millis() - smrt_pin_lockout_start;
    if (elapsed >= SMRT_AUTH_PIN_LOCKOUT_MS) {
        return 0;
    }
    return (SMRT_AUTH_PIN_LOCKOUT_MS - elapsed) / 1000;
}

//-----------------------------------------------------------------------------
// Public functions — Session timeout
//-----------------------------------------------------------------------------

/**
 * @brief  Updates last-activity timestamp for an authenticated client.
 * @param  client_id  WebSocket client ID
 * @return void
 */
void smrt_auth_ws_touch(uint32_t client_id) {
    for (int i = 0; i < smrt_auth_client_count; i++) {
        if (smrt_auth_clients[i] == client_id) {
            smrt_auth_timestamps[i] = millis();
            return;
        }
    }
}

/**
 * @brief  Logs out clients whose session has been idle longer than
 *         SMRT_AUTH_SESSION_TIMEOUT_MS. Call from main loop.
 * @return void
 */
void smrt_auth_ws_cleanup_expired(void) {
    unsigned long now = millis();
    int i = 0;
    while (i < smrt_auth_client_count) {
        if (now - smrt_auth_timestamps[i] >= SMRT_AUTH_SESSION_TIMEOUT_MS) {
            SMRT_DEBUG_PRINTF("AUTH: Session expired for client #%u\n",
                              smrt_auth_clients[i]);
            /* Shift remaining entries */
            for (int j = i; j < smrt_auth_client_count - 1; j++) {
                smrt_auth_clients[j] = smrt_auth_clients[j + 1];
                smrt_auth_timestamps[j] = smrt_auth_timestamps[j + 1];
            }
            smrt_auth_client_count--;
            /* Don't increment i — new element shifted into this position */
        } else {
            i++;
        }
    }
}

#endif // UNIT_TEST
