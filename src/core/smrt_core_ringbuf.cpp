/**
 * @file    smrt_core_ringbuf.cpp
 * @brief   Generic circular buffer for time-series sensor data
 * @project HOMENODE
 * @version 1.2.0
 */

#ifdef UNIT_TEST
    #include "smrt_core_ringbuf.h"
    #include <string.h>
#else
    #include "smrt_core.h"
#endif

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes a ring buffer with a pre-allocated sample array
 */
int smrt_ringbuf_init(smrt_ringbuf_t *rb, smrt_ringbuf_sample_t *samples,
                      uint16_t capacity, const char *name) {
    if (!rb || !samples || capacity == 0 || capacity > SMRT_RINGBUF_MAX_CAPACITY) return 0;

    rb->samples  = samples;
    rb->capacity = capacity;
    rb->head     = 0;
    rb->count    = 0;

    memset(rb->name, 0, SMRT_RINGBUF_NAME_MAX);
    if (name) {
        strncpy(rb->name, name, SMRT_RINGBUF_NAME_MAX - 1);
    }
    return 1;
}

/**
 * @brief  Pushes a new sample (overwrites oldest if full)
 */
int smrt_ringbuf_push(smrt_ringbuf_t *rb, float value, uint32_t timestamp) {
    if (!rb || !rb->samples) return 0;

    rb->samples[rb->head].value     = value;
    rb->samples[rb->head].timestamp = timestamp;

    rb->head = (rb->head + 1) % rb->capacity;

    if (rb->count < rb->capacity) {
        rb->count++;
    }
    return 1;
}

/**
 * @brief  Returns the number of valid samples
 */
uint16_t smrt_ringbuf_count(const smrt_ringbuf_t *rb) {
    return rb ? rb->count : 0;
}

/**
 * @brief  Gets sample by index (0=oldest, count-1=newest)
 */
int smrt_ringbuf_get(const smrt_ringbuf_t *rb, uint16_t index, smrt_ringbuf_sample_t *out) {
    if (!rb || !out || index >= rb->count) return 0;

    /* Calculate actual position: oldest sample position + offset */
    uint16_t start;
    if (rb->count < rb->capacity) {
        start = 0; /* Buffer not yet wrapped */
    } else {
        start = rb->head; /* Oldest is at head (next to be overwritten) */
    }
    uint16_t pos = (start + index) % rb->capacity;

    out->value     = rb->samples[pos].value;
    out->timestamp = rb->samples[pos].timestamp;
    return 1;
}

/**
 * @brief  Gets the most recent sample
 */
int smrt_ringbuf_latest(const smrt_ringbuf_t *rb, smrt_ringbuf_sample_t *out) {
    if (!rb || !out || rb->count == 0) return 0;

    uint16_t newest = (rb->head == 0) ? rb->capacity - 1 : rb->head - 1;
    out->value     = rb->samples[newest].value;
    out->timestamp = rb->samples[newest].timestamp;
    return 1;
}

/**
 * @brief  Clears all samples
 */
void smrt_ringbuf_clear(smrt_ringbuf_t *rb) {
    if (!rb) return;
    rb->head  = 0;
    rb->count = 0;
}

/**
 * @brief  Queries samples in a time range with downsampling
 */
int smrt_ringbuf_query(const smrt_ringbuf_t *rb, uint32_t from_ts, uint32_t to_ts,
                       smrt_ringbuf_sample_t *out, uint16_t out_max, uint16_t *out_count) {
    if (!rb || !out || !out_count || out_max == 0) return 0;

    *out_count = 0;

    /* First pass: count matching samples */
    uint16_t match_count = 0;
    for (uint16_t i = 0; i < rb->count; i++) {
        smrt_ringbuf_sample_t s;
        smrt_ringbuf_get(rb, i, &s);
        if (from_ts > 0 && s.timestamp < from_ts) continue;
        if (to_ts > 0 && s.timestamp > to_ts) continue;
        match_count++;
    }

    if (match_count == 0) return 1; /* Success with 0 results */

    /* Calculate stride for downsampling */
    uint16_t stride = 1;
    if (match_count > out_max) {
        stride = (match_count + out_max - 1) / out_max; /* Ceiling division */
    }

    /* Second pass: collect samples with stride */
    uint16_t match_idx = 0;
    uint16_t written = 0;
    for (uint16_t i = 0; i < rb->count && written < out_max; i++) {
        smrt_ringbuf_sample_t s;
        smrt_ringbuf_get(rb, i, &s);
        if (from_ts > 0 && s.timestamp < from_ts) continue;
        if (to_ts > 0 && s.timestamp > to_ts) continue;

        if (match_idx % stride == 0) {
            out[written] = s;
            written++;
        }
        match_idx++;
    }

    *out_count = written;
    return 1;
}

/**
 * @brief  Returns 1 if buffer is at capacity
 */
int smrt_ringbuf_is_full(const smrt_ringbuf_t *rb) {
    if (!rb) return 0;
    return (rb->count >= rb->capacity) ? 1 : 0;
}
