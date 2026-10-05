/******************************************************************************
 * @file SparkFun_STC3x_Arduino_Library.h
 * @brief SparkFun STC3x Library header file
 *
 * This file implements the SparkFunSTC3x class
 * for use with the Sensirion STC3x family of CO2 sensors (STC31)
 *
 * @author SparkFun Electronics
 * @date 2021-2025
 * @version 2.0.0
 * @copyright (c) 2021-2025 SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 ******************************************************************************/

#pragma once

// helps to keep the Toolkit header before the tk calls
// clang-format off
#include <SparkFun_Toolkit.h>
#include "sfTk/sfDevSTC3x.h"
// clang-format on

/**
 * @brief Class for interfacing with the STC3x sensor using I2C communication
 *
 * This class provides methods to initialize and communicate with the STC3x sensor
 * over an I2C bus. It inherits from the sfDevSTC3x class and uses the SparkFun
 * Toolkit for I2C communication.
 *
 * @see sfDevSTC3x
 */
class SparkFunSTC3x : public sfDevSTC3x
{
  public:
    /**
     * @brief Constructor
     *
     * @param sensorType The expected sensor type
     */
    SparkFunSTC3x(STC3x_sensor_product_number_e sensorType = STC3x_SENSOR_STC31) : sfDevSTC3x(sensorType)
    {
    }

    /**
     * @brief Begins the Device with I2C as the communication bus
     *
     * This method initializes the I2C bus and sets up communication with the STC3x sensor.
     *
     * @param i2cAddress I2C device address to use for the sensor
     * @param wirePort Wire port to use for I2C communication
     * @return True if successful, false otherwise
     */
    bool begin(const uint8_t i2cAddress = STC3x_DEFAULT_ADDRESS, TwoWire &wirePort = Wire)
    {
        // Setup Arduino I2C bus
        _theI2CBus.init(wirePort, i2cAddress);

        // Begin the sensor
        return sfDevSTC3x::begin(&_theI2CBus) == ksfTkErrOk;
    }

    /**
     * @brief Debug printing is no longer supported - errors are reported via the method return values.
     *
     * @deprecated since version 2.0.0. Retained for backward compatibility; this method does nothing.
     */
    void enableDebugging(Stream &debugPort = Serial)
    {
        (void)debugPort;
    }

  private:
    sfTkArdI2C _theI2CBus;
};

// for backwards compatibility
/**
 * @brief Deprecated class for interfacing with the STC3x sensor - supports version 1.x of the library
 *
 * @deprecated This class is deprecated for version 2.0 of this library. Use SparkFunSTC3x instead.
 */
class STC3x : public SparkFunSTC3x
{
  public:
    /**
     * @brief Constructor
     *
     * @param sensorType The expected sensor type
     */
    STC3x(STC3x_sensor_product_number_e sensorType = STC3x_SENSOR_STC31) : SparkFunSTC3x(sensorType)
    {
    }
};
