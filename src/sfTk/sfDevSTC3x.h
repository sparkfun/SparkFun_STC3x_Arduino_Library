/*!
 * @file sfDevSTC3x.h
 *
 * SparkFun STC3x CO2 Sensor Arduino Library
 *
 * This library facilitates communication with the Sensirion STC3x family of CO2 sensors over I<sup>2</sup>C.
 * The STC31 measures CO2 concentrations up to 100%. The readings can be compensated for
 * Relative Humidity, Temperature and Pressure.
 *
 * Want to support open source hardware? Buy a board from SparkFun!
 *
 * This library was written by:
 * Paul Clark
 * SparkFun Electronics
 * June 11th, 2021
 *
 * @author SparkFun Electronics
 * @date 2021-2025
 * @copyright Copyright (c) 2021-2025, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

// include the sparkfun toolkit headers
#include <sfTk/sfToolkit.h>

// Bus interfaces
#include <sfTk/sfTkII2C.h>

/** The default I2C address for the STC31 is 0x29. 0x2A, 0x2B and 0x2C can be selected via the ADDR pin. */
#define STC3x_DEFAULT_ADDRESS 0x29

/** @name STC3x Commands
 *  Command codes supported by the STC3x
 *  @{
 */
#define STC3x_COMMAND_DISABLE_CRC 0x3768 ///< Note: the library does not support this!
#define STC3x_COMMAND_SET_BINARY_GAS 0x3615
#define STC3X_COMMAND_SET_RELATIVE_HUMIDITY 0x3624
#define STC3X_COMMAND_SET_TEMPERATURE 0x361E
#define STC3X_COMMAND_SET_PRESSURE 0x362F
#define STC3X_COMMAND_MEASURE_GAS_CONCENTRATION 0x3639
#define STC3X_COMMAND_FORCED_RECALIBRATION 0x3661
#define STC3X_COMMAND_AUTOMATIC_CALIBRATION_ENABLE 0x3FEF
#define STC3X_COMMAND_AUTOMATIC_CALIBRATION_DISABLE 0x3F6E
#define STC3X_COMMAND_PREPARE_READ_STATE 0x3752
#define STC3X_COMMAND_READ_WRITE_STATE 0xE133
#define STC3X_COMMAND_APPLY_STATE 0x3650
#define STC3X_COMMAND_SELF_TEST 0x365B
#define STC3X_COMMAND_ENTER_SLEEP_MODE 0x3677
#define STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_1 0x367C
#define STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_2 0xE102
/** @} */

/** Binary gas and measurement range selection */
typedef enum
{
    STC3X_BINARY_GAS_CO2_N2_100 = 0x0000, ///< Set binary gas to CO2 in N2. Range: 0 to 100 vol%
    STC3X_BINARY_GAS_CO2_AIR_100,         ///< Set binary gas to CO2 in Air. Range: 0 to 100 vol%
    STC3X_BINARY_GAS_CO2_N2_25,           ///< Set binary gas to CO2 in N2. Range: 0 to 25 vol%
    STC3X_BINARY_GAS_CO2_AIR_25           ///< Set binary gas to CO2 in Air. Range: 0 to 25 vol%
} STC3X_binary_gas_type_e;

/** Product numbers of the supported sensors */
typedef enum
{
    STC3x_SENSOR_STC31 = 0x08010301, ///< STC31
} STC3x_sensor_product_number_e;

/**
 * @class sfDevSTC3x
 * @brief Driver class for the Sensirion STC3x family of CO2 sensors.
 *
 * This class provides a platform independent interface to configure and read data from the STC3x sensor.
 * It uses the SparkFun Toolkit I2C bus interface (sfTkII2C) for communication.
 *
 * Usage:
 * - Call begin() with an initialized I2C bus object.
 * - Call setBinaryGas() to select the binary gas and measurement range. This must be repeated
 *   after a reset or after the sensor wakes from sleep.
 * - Optionally set the relative humidity, temperature and pressure for compensation.
 * - Call measureGasConcentration() (no more than once per second) then getCO2() and getTemperature().
 */
class sfDevSTC3x
{
  public:
    /**
     * @brief Constructor
     *
     * @param sensorType The expected sensor type - used to validate the product number in begin()
     */
    sfDevSTC3x(STC3x_sensor_product_number_e sensorType = STC3x_SENSOR_STC31)
        : _theBus{nullptr}, _sensorType{sensorType}, _co2{0}, _temperature{0}, _co2HasBeenReported{true},
          _temperatureHasBeenReported{true}, _lastReadTimeMillis{0}
    {
    }

    /**
     * @brief Initializes the STC3x device on the specified I2C bus.
     *
     * Sets up communication with the sensor and reads the product identifier to verify the sensor
     * is connected and responding.
     *
     * @param theBus Pointer to the I2C bus interface to use for communication.
     * @return sfTkError_t ksfTkErrOk on success, otherwise an error code.
     */
    sfTkError_t begin(sfTkII2C *theBus = nullptr);

    /**
     * @brief Checks if the STC3x sensor is connected and acknowledges its I2C address.
     *
     * @return true if the sensor is detected, false otherwise.
     */
    bool isConnected(void);

    /**
     * @brief Set the binary gas and measurement range. See datasheet section 3.3.2
     *
     * When the system is reset, or wakes up from sleep mode, the sensor goes back to default mode, in which no binary
     * gas is selected. This means that the binary gas must be reconfigured. When no binary gas is selected (default
     * mode) the concentration measurement will return undefined results.
     *
     * @param binaryGas The binary gas to select. One of:
     *      STC3X_BINARY_GAS_CO2_N2_100
     *      STC3X_BINARY_GAS_CO2_AIR_100
     *      STC3X_BINARY_GAS_CO2_N2_25
     *      STC3X_BINARY_GAS_CO2_AIR_25
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool setBinaryGas(STC3X_binary_gas_type_e binaryGas = STC3X_BINARY_GAS_CO2_AIR_25);

    /**
     * @brief Set the relative humidity used for compensation. See datasheet section 3.3.3
     *
     * @param RH The relative humidity in %RH
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool setRelativeHumidity(float RH);

    /**
     * @brief Set the temperature used for compensation. See datasheet section 3.3.4
     *
     * By default the sensor uses the internal temperature sensor to compensate the concentration results.
     * When an external (more accurate) sensor is available, it is recommended to use its temperature value.
     *
     * @param temperature The temperature in degrees C
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool setTemperature(float temperature);

    /**
     * @brief Set the atmospheric pressure used for compensation. See datasheet section 3.3.5
     *
     * It is recommended to set the pressure level if it differs significantly from 1013mbar.
     *
     * @param pressure The pressure in mbar: 600mbar to 1200mbar
     * @return true if the I2C transfer was OK, false otherwise (including if pressure is out of range).
     */
    bool setPressure(uint16_t pressure);

    /**
     * @brief Trigger a gas concentration measurement and read the results. See datasheet section 3.3.6
     *
     * The measurement command should not be triggered more often than once a second. If called sooner,
     * this method returns false without communicating with the sensor.
     *
     * @return true if fresh data is available, false otherwise.
     */
    bool measureGasConcentration(void);

    /**
     * @brief Return the CO2 concentration.
     *
     * If the current value has already been reported, a new measurement is requested.
     *
     * @return float The CO2 concentration in vol%
     */
    float getCO2(void);

    /**
     * @brief Return the temperature.
     *
     * If the current value has already been reported, a new measurement is requested.
     *
     * @return float The temperature in degrees C
     */
    float getTemperature(void);

    /**
     * @brief Force a sensor recalibration using the provided concentration.
     *
     * Forced recalibration is used to improve the sensor output with a known reference value.
     * See the Field Calibration Guide for more details.
     *
     * @param concentration The reference concentration in vol% (0.0 to 100.0)
     * @param delayMillis Time to wait for the measurement to complete
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool forcedRecalibration(float concentration, uint16_t delayMillis = 75);

    /**
     * @brief Enable automatic self calibration.
     *
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool enableAutomaticSelfCalibration(void)
    {
        return sendCommand(STC3X_COMMAND_AUTOMATIC_CALIBRATION_ENABLE);
    }

    /**
     * @brief Disable automatic self calibration.
     *
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool disableAutomaticSelfCalibration(void)
    {
        return sendCommand(STC3X_COMMAND_AUTOMATIC_CALIBRATION_DISABLE);
    }

    /**
     * @brief Perform the sensor self test. See datasheet section 3.3.10
     *
     * @return true if the sensor responds with a result of 0x0000, false otherwise.
     */
    bool performSelfTest(void);

    /**
     * @brief Perform a soft reset. See datasheet section 3.3.11
     *
     * Writes command code 0x06 to the general call address 0x00. Note: this resets all devices on the bus
     * that support the general call reset.
     *
     * @param delayMillis Time to wait for the reset to complete. The sensor takes a maximum of 12ms to reset.
     */
    void softReset(uint16_t delayMillis = 12);

    /**
     * @brief Read the product number and serial number. See datasheet section 3.3.13
     *
     * @param[out] productNumber The 32-bit product number
     * @param[out] serialNumber Buffer for the serial number as ASCII hex. Must be at least 17 chars
     *                          (16 digits plus trailing NULL).
     * @return true if the I2C transfer and CRC checks were OK, false otherwise.
     */
    bool getProductIdentifier(uint32_t *productNumber, char *serialNumber);

    /**
     * @brief Put the sensor into sleep mode.
     *
     * @return true if the I2C transfer was OK, false otherwise.
     */
    bool enterSleepMode(void)
    {
        return sendCommand(STC3X_COMMAND_ENTER_SLEEP_MODE);
    }

    /**
     * @brief Send a command with a 16-bit argument and CRC.
     *
     * @param command The command code
     * @param arguments The 16-bit argument
     * @return true if the sensor acknowledged, false otherwise.
     */
    bool sendCommand(uint16_t command, uint16_t arguments);

    /**
     * @brief Send a command with no arguments.
     *
     * @param command The command code
     * @return true if the sensor acknowledged, false otherwise.
     */
    bool sendCommand(uint16_t command);

    /**
     * @brief Send a command and read back a 16-bit response plus CRC.
     *
     * @param registerAddress The command code
     * @param[out] response The 16-bit response
     * @param delayMillis Time to wait between sending the command and reading the response
     * @return true if the sensor acknowledged and the CRC check is valid, false otherwise.
     */
    bool readRegister(uint16_t registerAddress, uint16_t *response, uint16_t delayMillis = 0);

    /**
     * @brief Compute the Sensirion CRC8 (polynomial 0x31, init 0xFF) for the given data.
     *
     * @param data The data bytes
     * @param len The number of bytes
     * @return uint8_t The CRC
     */
    uint8_t computeCRC8(uint8_t data[], uint8_t len);

  private:
    /**
     * @brief Send a command, optionally wait, then read numBytes from the sensor.
     *
     * @param command The command code
     * @param[out] data Buffer to read into
     * @param numBytes Number of bytes to read
     * @param delayMillis Time to wait between sending the command and reading the response
     * @return sfTkError_t ksfTkErrOk on success, otherwise an error code.
     */
    sfTkError_t readCommand(uint16_t command, uint8_t *data, size_t numBytes, uint32_t delayMillis = 0);

    /**
     * @brief Validate the CRC of each 2-byte word in a buffer of [MSB, LSB, CRC] triplets.
     *
     * @param data The buffer
     * @param numBytes The buffer length - a multiple of 3
     * @return true if all CRCs are valid, false otherwise.
     */
    bool checkCRCs(uint8_t *data, size_t numBytes);

    /** Convert a serial number nibble to an ASCII hex digit */
    char convertHexToASCII(uint8_t digit);

    /** Pointer to the I2C bus interface used for communication with the sensor */
    sfTkII2C *_theBus;

    /** Sensor type - for future compatibility */
    STC3x_sensor_product_number_e _sensorType;

    /** Latest CO2 reading */
    float _co2;
    /** Latest temperature reading */
    float _temperature;

    /** Track the staleness of the current data, to avoid a new measurement each time an individual datum is requested
     */
    bool _co2HasBeenReported;
    /** Track the staleness of the current data */
    bool _temperatureHasBeenReported;

    /** Time of the last measurement. The measurement command should not be triggered more often than once a second */
    uint32_t _lastReadTimeMillis;
};
