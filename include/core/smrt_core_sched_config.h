/**
 * @file    smrt_core_sched_config.h
 * @brief   Scheduler configuration defines
 * @project HOMENODE
 * @version 0.8.0
 */

#ifndef SMRT_CORE_SCHED_CONFIG_H
#define SMRT_CORE_SCHED_CONFIG_H

//-----------------------------------------------------------------------------
// Scheduler limits
//-----------------------------------------------------------------------------
#define SMRT_SCHED_MAX_TASKS        8       /**< Maximum scheduled tasks */
#define SMRT_SCHED_ACTION_LEN       32      /**< Max action string length */
#define SMRT_SCHED_NAME_LEN         16      /**< Max task name length */
#define SMRT_SCHED_CHECK_INTERVAL   30000   /**< Check interval (ms) — every 30s */

//-----------------------------------------------------------------------------
// NVS configuration
//-----------------------------------------------------------------------------
#define SMRT_SCHED_NVS_NAMESPACE    "sched"
#define SMRT_SCHED_NVS_COUNT_KEY    "task_cnt"

//-----------------------------------------------------------------------------
// Day bitmask constants
//-----------------------------------------------------------------------------
#define SMRT_SCHED_DAY_SUN          0x01
#define SMRT_SCHED_DAY_MON          0x02
#define SMRT_SCHED_DAY_TUE          0x04
#define SMRT_SCHED_DAY_WED          0x08
#define SMRT_SCHED_DAY_THU          0x10
#define SMRT_SCHED_DAY_FRI          0x20
#define SMRT_SCHED_DAY_SAT          0x40
#define SMRT_SCHED_DAYS_ALL         0x7F    /**< All days of the week */
#define SMRT_SCHED_DAYS_WEEKDAYS    0x3E    /**< Mon-Fri */
#define SMRT_SCHED_DAYS_WEEKEND     0x41    /**< Sat+Sun */

#endif // SMRT_CORE_SCHED_CONFIG_H
