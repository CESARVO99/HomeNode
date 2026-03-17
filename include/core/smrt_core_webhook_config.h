/**
 * @file    smrt_core_webhook_config.h
 * @brief   Webhook notification configuration defines
 * @project HOMENODE
 * @version 0.8.0
 */

#ifndef SMRT_CORE_WEBHOOK_CONFIG_H
#define SMRT_CORE_WEBHOOK_CONFIG_H

#define SMRT_WEBHOOK_MAX_HOOKS      4       /**< Max configured webhooks */
#define SMRT_WEBHOOK_URL_MAX        128     /**< Max URL string length */
#define SMRT_WEBHOOK_TIMEOUT_MS     5000    /**< HTTP request timeout */
#define SMRT_WEBHOOK_RETRY_COUNT    2       /**< Retry attempts on failure */
#define SMRT_WEBHOOK_NVS_NAMESPACE  "webhook" /**< NVS namespace */

#endif // SMRT_CORE_WEBHOOK_CONFIG_H
