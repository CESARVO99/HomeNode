/**
 * @file    smrt_core_ringbuf.h
 * @brief   Generic circular buffer for time-series sensor data
 * @project HOMENODE
 * @version 1.2.0
 *
 * Stores float+timestamp samples in a fixed-size ring buffer.
 * Used by modules to keep last 1-2h of sensor readings in RAM
 * for local charting and REST API historical data retrieval.
 *
 * Memory: 8 bytes per sample (float value + uint32_t timestamp)
 *   - 360 samples (1h at 10s) = 2.8 KB per channel
 *   - 720 samples (2h at 10s) = 5.6 KB per channel
 */

#ifndef SMRT_CORE_RINGBUF_H
#define SMRT_CORE_RINGBUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define SMRT_RINGBUF_DEFAULT_CAPACITY  360  /**< Default: 1h at 10s intervals */
#define SMRT_RINGBUF_MAX_CAPACITY      720  /**< Max: 2h at 10s intervals */
#define SMRT_RINGBUF_MAX_CHANNELS      16   /**< Max channels per registry */
#define SMRT_RINGBUF_NAME_MAX          24   /**< Max channel name length */

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

/** Single data point: value + timestamp */
typedef struct {
    float    value;      /**< Sensor value */
    uint32_t timestamp;  /**< Unix epoch seconds (or millis if no NTP) */
} smrt_ringbuf_sample_t;

/** Circular buffer instance */
typedef struct {
    smrt_ringbuf_sample_t *samples;  /**< Sample array (dynamically allocated or static) */
    uint16_t capacity;               /**< Max number of samples */
    uint16_t head;                   /**< Next write position */
    uint16_t count;                  /**< Current number of valid samples */
    char     name[SMRT_RINGBUF_NAME_MAX]; /**< Channel name (e.g., "env.temperature") */
} smrt_ringbuf_t;

//-----------------------------------------------------------------------------
// Functions (all testable on native + ESP32)
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes a ring buffer with a pre-allocated sample array
 * @param  rb        Ring buffer instance
 * @param  samples   Pre-allocated array of samples
 * @param  capacity  Number of samples the array can hold (max SMRT_RINGBUF_MAX_CAPACITY)
 * @param  name      Channel name string
 * @return 1 on success, 0 if params invalid
 */
int smrt_ringbuf_init(smrt_ringbuf_t *rb, smrt_ringbuf_sample_t *samples,
                      uint16_t capacity, const char *name);

/**
 * @brief  Pushes a new sample into the ring buffer
 * @param  rb         Ring buffer instance
 * @param  value      Sensor value
 * @param  timestamp  Unix epoch seconds
 * @return 1 on success, 0 if rb is NULL
 */
int smrt_ringbuf_push(smrt_ringbuf_t *rb, float value, uint32_t timestamp);

/**
 * @brief  Returns the number of valid samples in the buffer
 */
uint16_t smrt_ringbuf_count(const smrt_ringbuf_t *rb);

/**
 * @brief  Gets a sample by index (0 = oldest, count-1 = newest)
 * @param  rb     Ring buffer instance
 * @param  index  Index from oldest (0) to newest (count-1)
 * @param  out    Output sample pointer
 * @return 1 if sample found, 0 if out of range
 */
int smrt_ringbuf_get(const smrt_ringbuf_t *rb, uint16_t index, smrt_ringbuf_sample_t *out);

/**
 * @brief  Gets the most recent sample
 * @param  rb   Ring buffer
 * @param  out  Output sample
 * @return 1 if available, 0 if buffer is empty
 */
int smrt_ringbuf_latest(const smrt_ringbuf_t *rb, smrt_ringbuf_sample_t *out);

/**
 * @brief  Clears all samples from the buffer
 */
void smrt_ringbuf_clear(smrt_ringbuf_t *rb);

/**
 * @brief  Queries samples within a time range with downsampling
 * @param  rb          Ring buffer
 * @param  from_ts     Start timestamp (inclusive, 0 = no lower bound)
 * @param  to_ts       End timestamp (inclusive, 0 = no upper bound)
 * @param  out         Output array of samples
 * @param  out_max     Maximum number of samples to return
 * @param  out_count   Actual number of samples written
 * @return 1 on success, 0 on error
 *
 * If the number of matching samples exceeds out_max, evenly-spaced
 * samples are selected (downsampling by stride).
 */
int smrt_ringbuf_query(const smrt_ringbuf_t *rb, uint32_t from_ts, uint32_t to_ts,
                       smrt_ringbuf_sample_t *out, uint16_t out_max, uint16_t *out_count);

/**
 * @brief  Returns 1 if the buffer is full (oldest samples will be overwritten)
 */
int smrt_ringbuf_is_full(const smrt_ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif // SMRT_CORE_RINGBUF_H
