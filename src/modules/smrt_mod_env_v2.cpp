/**
 * @file    smrt_mod_env_v2.cpp
 * @brief   ENV v2 multi-sensor drivers — auto-detect I2C + UART + ADC
 * @project HOMENODE
 * @version 1.5.0
 *
 * Supports: BME280, CCS811, BH1750, MH-Z19B, PMS5003, MQ-135, LDR, MAX4466
 * Auto-detects I2C sensors at init. UART/ADC sensors enabled via config.
 *
 * This file is compiled when SMRT_MOD_ENV is defined. The ENV module's
 * env_init() calls smrt_env_v2_init() to probe sensors, and env_loop()
 * calls smrt_env_v2_read() to get latest values.
 */

#ifndef UNIT_TEST

#include "smrt_core.h"
#include "smrt_mod_env_v2_config.h"
#include <Wire.h>

//=============================================================================
// Sensor detection flags
//=============================================================================
static bool env_v2_has_bme280  = false;
static bool env_v2_has_ccs811  = false;
static bool env_v2_has_bh1750  = false;
static bool env_v2_has_mhz19b  = false;
static bool env_v2_has_pms5003 = false;
static bool env_v2_has_mq135   = false;
static bool env_v2_has_ldr     = false;
static bool env_v2_has_mic     = false;

//=============================================================================
// Latest readings
//=============================================================================
static float env_v2_pressure    = 0.0f;  /**< hPa from BME280 */
static float env_v2_bme_temp    = 0.0f;  /**< °C from BME280 (more precise than DHT22) */
static float env_v2_bme_hum     = 0.0f;  /**< % from BME280 */
static float env_v2_eco2        = 0.0f;  /**< ppm from CCS811 */
static float env_v2_tvoc        = 0.0f;  /**< ppb from CCS811 */
static float env_v2_lux         = 0.0f;  /**< lux from BH1750 */
static int   env_v2_co2_ppm     = 0;     /**< ppm from MH-Z19B */
static int   env_v2_pm10        = 0;     /**< ug/m3 from PMS5003 */
static int   env_v2_pm25        = 0;     /**< ug/m3 from PMS5003 */
static int   env_v2_pm100       = 0;     /**< ug/m3 from PMS5003 */
static float env_v2_gas         = 0.0f;  /**< MQ-135 analog (0-1.0 normalized) */
static float env_v2_light_raw   = 0.0f;  /**< LDR analog (0-1.0) */
static float env_v2_noise       = 0.0f;  /**< MAX4466 peak-to-peak (0-1.0) */

//=============================================================================
// Ring buffers for v2 sensors
//=============================================================================
static smrt_ringbuf_sample_t env_v2_rb_press_data[SMRT_RINGBUF_DEFAULT_CAPACITY];
static smrt_ringbuf_sample_t env_v2_rb_co2_data[SMRT_RINGBUF_DEFAULT_CAPACITY];
static smrt_ringbuf_sample_t env_v2_rb_lux_data[SMRT_RINGBUF_DEFAULT_CAPACITY];
static smrt_ringbuf_sample_t env_v2_rb_pm25_data[SMRT_RINGBUF_DEFAULT_CAPACITY];
static smrt_ringbuf_sample_t env_v2_rb_noise_data[SMRT_RINGBUF_DEFAULT_CAPACITY];
static smrt_ringbuf_sample_t env_v2_rb_gas_data[SMRT_RINGBUF_DEFAULT_CAPACITY];

static smrt_ringbuf_t env_v2_rb_press;
static smrt_ringbuf_t env_v2_rb_co2;
static smrt_ringbuf_t env_v2_rb_lux;
static smrt_ringbuf_t env_v2_rb_pm25;
static smrt_ringbuf_t env_v2_rb_noise;
static smrt_ringbuf_t env_v2_rb_gas;

//=============================================================================
// I2C probe helper
//=============================================================================
static bool i2c_probe(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

//=============================================================================
// BME280 — Simple register-based driver (no library)
//=============================================================================
static uint8_t bme280_addr = 0;

static uint8_t bme280_read8(uint8_t reg) {
    Wire.beginTransmission(bme280_addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(bme280_addr, (uint8_t)1);
    return Wire.read();
}

static void bme280_write8(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(bme280_addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static int32_t bme280_t_fine = 0;
static uint16_t bme280_dig_T1; static int16_t bme280_dig_T2, bme280_dig_T3;
static uint16_t bme280_dig_P1; static int16_t bme280_dig_P2, bme280_dig_P3, bme280_dig_P4;
static int16_t bme280_dig_P5, bme280_dig_P6, bme280_dig_P7, bme280_dig_P8, bme280_dig_P9;
static uint8_t bme280_dig_H1, bme280_dig_H3; static int16_t bme280_dig_H2, bme280_dig_H4, bme280_dig_H5;
static int8_t bme280_dig_H6;

static bool bme280_init(uint8_t addr) {
    bme280_addr = addr;
    if (bme280_read8(0xD0) != 0x60) return false; /* Chip ID */

    /* Read calibration */
    Wire.beginTransmission(addr); Wire.write(0x88); Wire.endTransmission();
    Wire.requestFrom(addr, (uint8_t)26);
    uint8_t buf[26];
    for (int i = 0; i < 26; i++) buf[i] = Wire.read();
    bme280_dig_T1 = buf[0] | (buf[1] << 8);
    bme280_dig_T2 = buf[2] | (buf[3] << 8);
    bme280_dig_T3 = buf[4] | (buf[5] << 8);
    bme280_dig_P1 = buf[6] | (buf[7] << 8);
    bme280_dig_P2 = buf[8] | (buf[9] << 8);
    bme280_dig_P3 = buf[10] | (buf[11] << 8);
    bme280_dig_P4 = buf[12] | (buf[13] << 8);
    bme280_dig_P5 = buf[14] | (buf[15] << 8);
    bme280_dig_P6 = buf[16] | (buf[17] << 8);
    bme280_dig_P7 = buf[18] | (buf[19] << 8);
    bme280_dig_P8 = buf[20] | (buf[21] << 8);
    bme280_dig_P9 = buf[22] | (buf[23] << 8);
    bme280_dig_H1 = buf[25];

    Wire.beginTransmission(addr); Wire.write(0xE1); Wire.endTransmission();
    Wire.requestFrom(addr, (uint8_t)7);
    uint8_t hbuf[7];
    for (int i = 0; i < 7; i++) hbuf[i] = Wire.read();
    bme280_dig_H2 = hbuf[0] | (hbuf[1] << 8);
    bme280_dig_H3 = hbuf[2];
    bme280_dig_H4 = (hbuf[3] << 4) | (hbuf[4] & 0x0F);
    bme280_dig_H5 = (hbuf[5] << 4) | (hbuf[4] >> 4);
    bme280_dig_H6 = hbuf[6];

    /* Config: osrs_t=x2, osrs_p=x16, osrs_h=x1, mode=normal */
    bme280_write8(0xF2, 0x01);        /* ctrl_hum */
    bme280_write8(0xF4, 0b01010111);  /* ctrl_meas: t=x2, p=x16, normal */
    bme280_write8(0xF5, 0b10010000);  /* config: standby=1000ms, filter=4 */
    return true;
}

static void bme280_read_all(float *t, float *p, float *h) {
    Wire.beginTransmission(bme280_addr); Wire.write(0xF7); Wire.endTransmission();
    Wire.requestFrom(bme280_addr, (uint8_t)8);
    uint8_t d[8];
    for (int i = 0; i < 8; i++) d[i] = Wire.read();

    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] << 8) | d[7];

    /* Temperature */
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)bme280_dig_T1 << 1))) * (int32_t)bme280_dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (int32_t)bme280_dig_T1) * ((adc_T >> 4) - (int32_t)bme280_dig_T1)) >> 12) * (int32_t)bme280_dig_T3) >> 14;
    bme280_t_fine = var1 + var2;
    *t = (float)((bme280_t_fine * 5 + 128) >> 8) / 100.0f;

    /* Pressure */
    int64_t pv1 = (int64_t)bme280_t_fine - 128000;
    int64_t pv2 = pv1 * pv1 * (int64_t)bme280_dig_P6;
    pv2 = pv2 + ((pv1 * (int64_t)bme280_dig_P5) << 17);
    pv2 = pv2 + (((int64_t)bme280_dig_P4) << 35);
    pv1 = ((pv1 * pv1 * (int64_t)bme280_dig_P3) >> 8) + ((pv1 * (int64_t)bme280_dig_P2) << 12);
    pv1 = (((((int64_t)1) << 47) + pv1)) * ((int64_t)bme280_dig_P1) >> 33;
    if (pv1 != 0) {
        int64_t pp = 1048576 - adc_P;
        pp = (((pp << 31) - pv2) * 3125) / pv1;
        pv1 = (((int64_t)bme280_dig_P9) * (pp >> 13) * (pp >> 13)) >> 25;
        pv2 = (((int64_t)bme280_dig_P8) * pp) >> 19;
        pp = ((pp + pv1 + pv2) >> 8) + (((int64_t)bme280_dig_P7) << 4);
        *p = (float)pp / 25600.0f;
    } else { *p = 0; }

    /* Humidity */
    int32_t hv = bme280_t_fine - 76800;
    hv = (((((adc_H << 14) - ((int32_t)bme280_dig_H4 << 20) - ((int32_t)bme280_dig_H5 * hv)) + 16384) >> 15) *
          (((((((hv * (int32_t)bme280_dig_H6) >> 10) * (((hv * (int32_t)bme280_dig_H3) >> 11) + 32768)) >> 10) + 2097152) * (int32_t)bme280_dig_H2 + 8192) >> 14));
    hv = hv - (((((hv >> 15) * (hv >> 15)) >> 7) * (int32_t)bme280_dig_H1) >> 4);
    hv = (hv < 0) ? 0 : hv;
    hv = (hv > 419430400) ? 419430400 : hv;
    *h = (float)(hv >> 12) / 1024.0f;
}

//=============================================================================
// BH1750 — Simple one-shot read
//=============================================================================
static uint8_t bh1750_addr = 0;

static bool bh1750_init(uint8_t addr) {
    bh1750_addr = addr;
    Wire.beginTransmission(addr);
    Wire.write(0x10); /* Continuous high-res mode */
    return (Wire.endTransmission() == 0);
}

static float bh1750_read(void) {
    Wire.requestFrom(bh1750_addr, (uint8_t)2);
    if (Wire.available() < 2) return -1;
    uint16_t raw = (Wire.read() << 8) | Wire.read();
    return (float)raw / 1.2f;
}

//=============================================================================
// MH-Z19B — UART CO2 (9600 baud on Serial2)
//=============================================================================
static void mhz19b_init(void) {
    Serial2.begin(SMRT_ENV_MHZ19B_BAUD, SERIAL_8N1, SMRT_ENV_MHZ19B_RX, SMRT_ENV_MHZ19B_TX);
}

static int mhz19b_read(void) {
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    Serial2.write(cmd, 9);
    delay(100);

    if (Serial2.available() < 9) return -1;
    uint8_t resp[9];
    Serial2.readBytes(resp, 9);

    if (resp[0] != 0xFF || resp[1] != 0x86) return -1;
    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) checksum += resp[i];
    checksum = 0xFF - checksum + 1;
    if (checksum != resp[8]) return -1;

    return (resp[2] << 8) | resp[3];
}

//=============================================================================
// Analog sensors — simple ADC reads
//=============================================================================
static float read_analog_normalized(int pin) {
    long sum = 0;
    for (int i = 0; i < SMRT_ENV_ADC_SAMPLES; i++) {
        sum += analogRead(pin);
    }
    return (float)sum / (float)(SMRT_ENV_ADC_SAMPLES * SMRT_ENV_ADC_RESOLUTION);
}

static float read_mic_peak(int pin) {
    int mn = 4095, mx = 0;
    unsigned long start = millis();
    while (millis() - start < 50) { /* 50ms window */
        int v = analogRead(pin);
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    return (float)(mx - mn) / (float)SMRT_ENV_ADC_RESOLUTION;
}

//=============================================================================
// Public API — called from ENV module
//=============================================================================

/**
 * @brief  Probes I2C bus and initializes detected sensors
 */
void smrt_env_v2_init(void) {
    Wire.begin(SMRT_I2C_SDA, SMRT_I2C_SCL);

    /* BME280 */
    if (i2c_probe(SMRT_ENV_BME280_ADDR) && bme280_init(SMRT_ENV_BME280_ADDR)) {
        env_v2_has_bme280 = true;
        Serial.println("[ENV-V2] BME280 detected at 0x76");
    } else if (i2c_probe(SMRT_ENV_BME280_ADDR_ALT) && bme280_init(SMRT_ENV_BME280_ADDR_ALT)) {
        env_v2_has_bme280 = true;
        Serial.println("[ENV-V2] BME280 detected at 0x77");
    }

    /* BH1750 */
    if (i2c_probe(SMRT_ENV_BH1750_ADDR) && bh1750_init(SMRT_ENV_BH1750_ADDR)) {
        env_v2_has_bh1750 = true;
        Serial.println("[ENV-V2] BH1750 detected at 0x23");
    }

    /* CCS811 — just probe, library init needed for full use */
    if (i2c_probe(SMRT_ENV_CCS811_ADDR)) {
        env_v2_has_ccs811 = true;
        /* Wake pin LOW, app start */
        Wire.beginTransmission(SMRT_ENV_CCS811_ADDR);
        Wire.write(0xF4); /* APP_START */
        Wire.endTransmission();
        delay(100);
        /* Set mode 1 (1s measurement) */
        Wire.beginTransmission(SMRT_ENV_CCS811_ADDR);
        Wire.write(0x01); /* MEAS_MODE register */
        Wire.write(0x10); /* Mode 1 */
        Wire.endTransmission();
        Serial.println("[ENV-V2] CCS811 detected at 0x5A");
    }

    /* MH-Z19B — check if UART pins available (not used by RLY) */
    if (smrt_node_has_module(SMRT_NODE_MOD_ENV) && !smrt_node_has_module(SMRT_NODE_MOD_RLY)) {
        mhz19b_init();
        env_v2_has_mhz19b = true;
        Serial.println("[ENV-V2] MH-Z19B UART initialized");
    }

    /* Analog sensors — always available on ADC pins if ENV is active */
    env_v2_has_mq135 = true;
    env_v2_has_ldr   = true;
    env_v2_has_mic   = true;

    /* Init ring buffers */
    smrt_ringbuf_init(&env_v2_rb_press, env_v2_rb_press_data, SMRT_RINGBUF_DEFAULT_CAPACITY, "env.pressure");
    smrt_ringbuf_init(&env_v2_rb_co2,   env_v2_rb_co2_data,   SMRT_RINGBUF_DEFAULT_CAPACITY, "env.co2");
    smrt_ringbuf_init(&env_v2_rb_lux,   env_v2_rb_lux_data,   SMRT_RINGBUF_DEFAULT_CAPACITY, "env.lux");
    smrt_ringbuf_init(&env_v2_rb_pm25,  env_v2_rb_pm25_data,  SMRT_RINGBUF_DEFAULT_CAPACITY, "env.pm25");
    smrt_ringbuf_init(&env_v2_rb_noise, env_v2_rb_noise_data,  SMRT_RINGBUF_DEFAULT_CAPACITY, "env.noise");
    smrt_ringbuf_init(&env_v2_rb_gas,   env_v2_rb_gas_data,   SMRT_RINGBUF_DEFAULT_CAPACITY, "env.gas");

    int count = (int)env_v2_has_bme280 + (int)env_v2_has_ccs811 + (int)env_v2_has_bh1750
              + (int)env_v2_has_mhz19b + 3; /* +3 for analog always */
    Serial.printf("[ENV-V2] %d sensors active\n", count);
}

/**
 * @brief  Reads all detected v2 sensors and pushes to ring buffers
 */
void smrt_env_v2_read(uint32_t timestamp) {
    if (env_v2_has_bme280) {
        bme280_read_all(&env_v2_bme_temp, &env_v2_pressure, &env_v2_bme_hum);
        smrt_ringbuf_push(&env_v2_rb_press, env_v2_pressure, timestamp);
        smrt_auto_check_value("env.pressure", env_v2_pressure);
    }

    if (env_v2_has_bh1750) {
        env_v2_lux = bh1750_read();
        if (env_v2_lux >= 0) {
            smrt_ringbuf_push(&env_v2_rb_lux, env_v2_lux, timestamp);
            smrt_auto_check_value("env.lux", env_v2_lux);
        }
    }

    if (env_v2_has_ccs811) {
        /* Read CCS811 data register */
        Wire.beginTransmission(SMRT_ENV_CCS811_ADDR);
        Wire.write(0x02); /* ALG_RESULT_DATA */
        Wire.endTransmission();
        Wire.requestFrom((uint8_t)SMRT_ENV_CCS811_ADDR, (uint8_t)4);
        if (Wire.available() >= 4) {
            env_v2_eco2 = (float)((Wire.read() << 8) | Wire.read());
            env_v2_tvoc = (float)((Wire.read() << 8) | Wire.read());
            smrt_ringbuf_push(&env_v2_rb_co2, env_v2_eco2, timestamp);
            smrt_auto_check_value("env.co2", env_v2_eco2);
        }
    }

    if (env_v2_has_mhz19b) {
        int ppm = mhz19b_read();
        if (ppm > 0) {
            env_v2_co2_ppm = ppm;
            smrt_ringbuf_push(&env_v2_rb_co2, (float)ppm, timestamp);
            smrt_auto_check_value("env.co2", (float)ppm);
        }
    }

    /* Analog sensors */
    if (env_v2_has_mq135) {
        env_v2_gas = read_analog_normalized(SMRT_ENV_MQ135_PIN);
        smrt_ringbuf_push(&env_v2_rb_gas, env_v2_gas, timestamp);
        smrt_auto_check_value("env.gas", env_v2_gas);
    }

    if (env_v2_has_ldr) {
        env_v2_light_raw = read_analog_normalized(SMRT_ENV_LDR_PIN);
        smrt_auto_check_value("env.light", env_v2_light_raw);
    }

    if (env_v2_has_mic) {
        env_v2_noise = read_mic_peak(SMRT_ENV_MIC_PIN);
        smrt_ringbuf_push(&env_v2_rb_noise, env_v2_noise, timestamp);
        smrt_auto_check_value("env.noise", env_v2_noise);
    }
}

/**
 * @brief  Adds v2 sensor data to telemetry JSON
 */
void smrt_env_v2_get_telemetry(void *json_obj) {
    JsonObject &obj = *(JsonObject *)json_obj;

    if (env_v2_has_bme280) {
        obj["bme_temp"]  = env_v2_bme_temp;
        obj["bme_hum"]   = env_v2_bme_hum;
        obj["pressure"]  = env_v2_pressure;
    }
    if (env_v2_has_ccs811) {
        obj["eco2"] = env_v2_eco2;
        obj["tvoc"] = env_v2_tvoc;
    }
    if (env_v2_has_bh1750) {
        obj["lux"] = env_v2_lux;
    }
    if (env_v2_has_mhz19b) {
        obj["co2"] = env_v2_co2_ppm;
    }
    if (env_v2_has_mq135)  obj["gas"]   = env_v2_gas;
    if (env_v2_has_ldr)    obj["light"] = env_v2_light_raw;
    if (env_v2_has_mic)    obj["noise"] = env_v2_noise;
}

/**
 * @brief  Returns ring buffer getters for REST API
 */
smrt_ringbuf_t *smrt_env_v2_get_ringbuf(const char *channel) {
    if (strcmp(channel, "env.pressure") == 0) return &env_v2_rb_press;
    if (strcmp(channel, "env.co2") == 0)      return &env_v2_rb_co2;
    if (strcmp(channel, "env.lux") == 0)      return &env_v2_rb_lux;
    if (strcmp(channel, "env.pm25") == 0)     return &env_v2_rb_pm25;
    if (strcmp(channel, "env.noise") == 0)    return &env_v2_rb_noise;
    if (strcmp(channel, "env.gas") == 0)      return &env_v2_rb_gas;
    return NULL;
}

#endif // UNIT_TEST
