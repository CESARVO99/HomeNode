/**
 * @file    smrt_mod_env_v2_config.h
 * @brief   ENV v2 multi-sensor configuration — pin assignments and I2C addresses
 * @project HOMENODE
 * @version 1.5.0
 */

#ifndef SMRT_MOD_ENV_V2_CONFIG_H
#define SMRT_MOD_ENV_V2_CONFIG_H

//-----------------------------------------------------------------------------
// I2C Sensor Addresses (auto-detected at init)
//-----------------------------------------------------------------------------
#define SMRT_ENV_BME280_ADDR        0x76    /**< BME280 I2C address */
#define SMRT_ENV_BME280_ADDR_ALT    0x77    /**< BME280 alternate address */
#define SMRT_ENV_CCS811_ADDR        0x5A    /**< CCS811 I2C address */
#define SMRT_ENV_CCS811_ADDR_ALT    0x5B    /**< CCS811 alternate address */
#define SMRT_ENV_BH1750_ADDR        0x23    /**< BH1750 I2C address */
#define SMRT_ENV_BH1750_ADDR_ALT    0x5C    /**< BH1750 alternate address */
#define SMRT_ENV_VEML6075_ADDR      0x10    /**< VEML6075 I2C address */

//-----------------------------------------------------------------------------
// UART Sensors (mutual exclusive — share UART2 or use different UARTs)
//-----------------------------------------------------------------------------
#define SMRT_ENV_MHZ19B_TX          17      /**< MH-Z19B TX → ESP32 RX2 (GPIO17) */
#define SMRT_ENV_MHZ19B_RX          16      /**< MH-Z19B RX ← ESP32 TX2 (GPIO16) */
#define SMRT_ENV_MHZ19B_BAUD        9600
#define SMRT_ENV_PMS5003_TX         26      /**< PMS5003 TX → ESP32 (GPIO26) */
#define SMRT_ENV_PMS5003_RX         27      /**< PMS5003 RX ← ESP32 (GPIO27) */
#define SMRT_ENV_PMS5003_BAUD       9600

//-----------------------------------------------------------------------------
// Analog Sensors (ADC pins)
//-----------------------------------------------------------------------------
#define SMRT_ENV_MQ135_PIN          34      /**< MQ-135 analog output → ADC (GPIO34) */
#define SMRT_ENV_LDR_PIN            35      /**< LDR voltage divider → ADC (GPIO35) */
#define SMRT_ENV_MIC_PIN            36      /**< MAX4466 analog output → ADC (GPIO36) */

//-----------------------------------------------------------------------------
// ADC Configuration
//-----------------------------------------------------------------------------
#define SMRT_ENV_ADC_SAMPLES        64      /**< Samples for analog RMS/average */
#define SMRT_ENV_ADC_RESOLUTION     4095    /**< 12-bit ADC */
#define SMRT_ENV_ADC_VREF           3.3f    /**< ADC reference voltage */

//-----------------------------------------------------------------------------
// Sensor Enable Flags (set in NVS, default all false until detected)
//-----------------------------------------------------------------------------
#define SMRT_ENV_V2_NVS_NAMESPACE   "env_v2"

//-----------------------------------------------------------------------------
// MH-Z19B CO2 Protocol
//-----------------------------------------------------------------------------
#define SMRT_ENV_MHZ19B_CMD_READ    {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}
#define SMRT_ENV_MHZ19B_RESP_LEN    9

//-----------------------------------------------------------------------------
// PMS5003 Protocol
//-----------------------------------------------------------------------------
#define SMRT_ENV_PMS5003_START1     0x42
#define SMRT_ENV_PMS5003_START2     0x4D
#define SMRT_ENV_PMS5003_FRAME_LEN  32

#endif // SMRT_MOD_ENV_V2_CONFIG_H
