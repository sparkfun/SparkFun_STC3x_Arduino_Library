/*!
 * @file sfDevSTC3x.cpp
 *
 * This library facilitates communication with the Sensirion STC3x family of CO2 sensors over I<sup>2</sup>C.
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
 *
 */

#include "sfDevSTC3x.h"

//--------------------------------------------------------------------------------------------------
// Begin the STC3x device. Requires a bus object to communicate with the device.
//
sfTkError_t sfDevSTC3x::begin(sfTkII2C *theBus)
{
    // Nullptr check
    if (theBus == nullptr)
        return ksfTkErrBusNotInit;

    // Set bus pointer
    _theBus = theBus;

    // The STC3x sends and receives data MSB first
    _theBus->setByteOrder(sfTkByteOrder::BigEndian);

    // The measurement command should not be triggered more often than once a second
    _lastReadTimeMillis = sftk_ticks_ms();

    // Read the product identifier. Fail if the CRC check fails.
    char serialNumber[17]; // Serial number is 16 digits plus trailing NULL
    uint32_t productNumber;
    if (!getProductIdentifier(&productNumber, serialNumber))
        return ksfTkErrFail;

    return ksfTkErrOk;
}

//--------------------------------------------------------------------------------------------------
// Check if the STC3x is connected by pinging its I2C address.
//
bool sfDevSTC3x::isConnected(void)
{
    if (_theBus == nullptr)
        return false;

    return _theBus->ping() == ksfTkErrOk;
}

//--------------------------------------------------------------------------------------------------
// Set the binary gas. See 3.3.2
// When the system is reset, or wakes up from sleep mode, the sensor goes back to default mode, in which no binary is
// selected. This means that the binary gas must be reconfigured. When no binary gas is selected (default mode) the
// concentration measurement will return undefined results. This allows to detect unexpected sensor interruption (e.g.
// due to temporary power loss) and consequently reset the binary gas to the appropriate mixture.
//
bool sfDevSTC3x::setBinaryGas(STC3X_binary_gas_type_e binaryGas)
{
    return sendCommand(STC3x_COMMAND_SET_BINARY_GAS, (uint16_t)binaryGas);
}

//--------------------------------------------------------------------------------------------------
// Set the relative humidity. See 3.3.3
// The measurement principle of the concentration measurement is dependent on the humidity of the gas.
// With the set relative humidity command, the sensor uses internal algorithms to compensate the concentration results.
//
bool sfDevSTC3x::setRelativeHumidity(float RH)
{
    uint16_t rh = (uint16_t)(RH * 65535.0 / 100.0); // See 3.5 Conversion to Physical Values
    return sendCommand(STC3X_COMMAND_SET_RELATIVE_HUMIDITY, rh);
}

//--------------------------------------------------------------------------------------------------
// Set the temperature. See 3.3.4
// The concentration measurement requires a compensation of temperature.
// Per default, the sensor uses the internal temperature sensor to compensate the concentration results.
// However, when using the SHTxx, it is recommended to also use its temperature value, because it is more accurate.
//
bool sfDevSTC3x::setTemperature(float temperature)
{
    int16_t temp = (int16_t)(temperature * 200.0); // See 3.5 Conversion to Physical Values
    return sendCommand(STC3X_COMMAND_SET_TEMPERATURE, (uint16_t)temp);
}

//--------------------------------------------------------------------------------------------------
// Set the pressure. See 3.3.5
// A pressure value can be written into the sensor, for density compensation of the gas concentration measurement.
// It is recommended to set the pressure level, if it differs significantly from 1013mbar.
// Pressure compensation is valid from 600mbar to 1200mbar.
//
bool sfDevSTC3x::setPressure(uint16_t pressure)
{
    if ((pressure < 600) || (pressure > 1200))
        return false; // pressure is out of bounds

    return sendCommand(STC3X_COMMAND_SET_PRESSURE, pressure);
}

//--------------------------------------------------------------------------------------------------
// Get 9 bytes from STC3x. See 3.3.6
// Updates member variables with the CO2 and temperature
// Returns true if data is read successfully
//
bool sfDevSTC3x::measureGasConcentration(void)
{
    // The measurement command should not be triggered more often than once a second.
    // Check if it is OK to trigger a new measurement
    if (sftk_ticks_ms() - _lastReadTimeMillis < 1000)
        return false; // Too early!

    // Response is: CO2 MSB, CO2 LSB, CRC, T MSB, T LSB, CRC, reserved MSB, reserved LSB, CRC
    // Datasheet specifies 66ms but sensor seems to need at least 70ms. 75ms provides margin.
    uint8_t data[9];
    if (readCommand(STC3X_COMMAND_MEASURE_GAS_CONCENTRATION, data, sizeof(data), 75) != ksfTkErrOk)
        return false;

    if (!checkCRCs(data, sizeof(data)))
        return false;

    uint16_t rawCO2 = ((uint16_t)data[0] << 8) | data[1];
    int16_t rawTemperature = (int16_t)(((uint16_t)data[3] << 8) | data[4]);

    // Now copy the data into their associated floats
    _co2 = ((((float)rawCO2) - 16384) / 32768) * 100; // See 3.5 Conversion to Physical Values
    _temperature = ((float)rawTemperature) / 200;     // See 3.5 Conversion to Physical Values

    // Mark our member variables as fresh
    _co2HasBeenReported = false;
    _temperatureHasBeenReported = false;
    _lastReadTimeMillis = sftk_ticks_ms();

    return true; // Success! New data available.
}

//--------------------------------------------------------------------------------------------------
// Returns the latest available CO2 level
// If the current level has already been reported, trigger a new read
//
float sfDevSTC3x::getCO2(void)
{
    if (_co2HasBeenReported) // Trigger a new read
        measureGasConcentration();

    _co2HasBeenReported = true;

    return _co2;
}

//--------------------------------------------------------------------------------------------------
// Returns the latest available temperature
// If the current level has already been reported, trigger a new read
//
float sfDevSTC3x::getTemperature(void)
{
    if (_temperatureHasBeenReported) // Trigger a new read
        measureGasConcentration();

    _temperatureHasBeenReported = true;

    return _temperature;
}

//--------------------------------------------------------------------------------------------------
// Force a sensor recalibration using the provided concentration
// Forced recalibration is used to improve the sensor output with a known reference value.
// See the Field Calibration Guide for more details.
// This command will trigger a concentration measurement as described in 3.3.6 and therefore it will take the same
// measurement time.
//
bool sfDevSTC3x::forcedRecalibration(float concentration, uint16_t delayMillis)
{
    if (concentration < 0.0) // Ignore negative concentrations
        concentration = 0.0;
    if (concentration > 100.0) // Ignore concentrations above 100%
        concentration = 100.0;

    uint16_t conc16 = (uint16_t)(((concentration * 32768) / 100) + 16384); // See 3.5 Conversion to Physical Values
    bool success = sendCommand(STC3X_COMMAND_FORCED_RECALIBRATION, conc16);

    if (delayMillis > 0)
        sftk_delay_ms(delayMillis); // Allow time for the measurement to complete

    return success;
}

//--------------------------------------------------------------------------------------------------
// Perform self test. Takes 20ms to complete. See 3.3.10
// In case of a successful self-test the sensor returns 0x0000 with correct CRC.
//
bool sfDevSTC3x::performSelfTest(void)
{
    uint16_t response = 0xFFFF;

    bool success = readRegister(STC3X_COMMAND_SELF_TEST, &response, 20);

    return success && (response == 0x0000);
}

//--------------------------------------------------------------------------------------------------
// Peform soft reset. See 3.3.11
// Writes command code 0x06 to general call address 0x00.
// Note that the I2C address is not acknowledged.
// After the reset command the sensor will take maximum 12ms to reset.
//
void sfDevSTC3x::softReset(uint16_t delayMillis)
{
    if (_theBus == nullptr)
        return;

    uint8_t address = _theBus->address();
    _theBus->setAddress(0x00);
    _theBus->writeUInt8(0x06);
    _theBus->setAddress(address);

    if (delayMillis > 0)
        sftk_delay_ms(delayMillis);
}

//--------------------------------------------------------------------------------------------------
// Get 18 bytes from STC3x. Convert 64-bit serial number to ASCII chars. See 3.3.13
// Returns true if serial number is read successfully
//
bool sfDevSTC3x::getProductIdentifier(uint32_t *productNumber, char *serialNumber)
{
    if (productNumber == nullptr || serialNumber == nullptr)
        return false;

    if (!sendCommand(STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_1))
        return false;

    // The product number arrives as: two bytes, CRC, two bytes, CRC
    // The serial number arrives as: two bytes, CRC, two bytes, CRC, two bytes, CRC, two bytes, CRC
    uint8_t data[18];
    if (readCommand(STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_2, data, sizeof(data)) != ksfTkErrOk)
        return false;

    if (!checkCRCs(data, sizeof(data)))
        return false;

    *productNumber = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[3] << 8) | data[4];

    int digit = 0;
    for (size_t i = 6; i < sizeof(data); i += 3)
    {
        for (size_t j = 0; j < 2; j++)
        {
            serialNumber[digit++] = convertHexToASCII(data[i + j] >> 4); // Convert each nibble to ASCII
            serialNumber[digit++] = convertHexToASCII(data[i + j] & 0x0F);
        }
    }
    serialNumber[digit] = 0; // NULL-terminate the string

    return true; // Success!
}

//--------------------------------------------------------------------------------------------------
// PRIVATE: Convert serial number digit to ASCII
//
char sfDevSTC3x::convertHexToASCII(uint8_t digit)
{
    if (digit <= 9)
        return (char)(digit + '0');
    else
        return (char)(digit + 'A' - 10); // Use upper case for A-F
}

//--------------------------------------------------------------------------------------------------
// Sends a command along with arguments and CRC
//
bool sfDevSTC3x::sendCommand(uint16_t command, uint16_t arguments)
{
    if (_theBus == nullptr)
        return false;

    uint8_t data[5];
    data[0] = command >> 8;     // MSB
    data[1] = command & 0xFF;   // LSB
    data[2] = arguments >> 8;   // MSB
    data[3] = arguments & 0xFF; // LSB
    data[4] = computeCRC8(&data[2], 2); // Calc CRC on the arguments only, not the command

    return _theBus->writeData(data, sizeof(data)) == ksfTkErrOk;
}

//--------------------------------------------------------------------------------------------------
// Sends just a command, no arguments, no CRC
//
bool sfDevSTC3x::sendCommand(uint16_t command)
{
    if (_theBus == nullptr)
        return false;

    uint8_t data[2];
    data[0] = command >> 8;   // MSB
    data[1] = command & 0xFF; // LSB

    return _theBus->writeData(data, sizeof(data)) == ksfTkErrOk;
}

//--------------------------------------------------------------------------------------------------
// Gets two bytes from STC3x plus CRC.
// Returns true if the sensor acknowledged _and_ the CRC check is valid
//
bool sfDevSTC3x::readRegister(uint16_t registerAddress, uint16_t *response, uint16_t delayMillis)
{
    if (response == nullptr)
        return false;

    uint8_t data[3]; // Data and CRC
    if (readCommand(registerAddress, data, sizeof(data), delayMillis) != ksfTkErrOk)
        return false;

    *response = ((uint16_t)data[0] << 8) | data[1];

    return checkCRCs(data, sizeof(data));
}

//--------------------------------------------------------------------------------------------------
// PRIVATE: Send a command, wait, then read numBytes from the sensor
//
sfTkError_t sfDevSTC3x::readCommand(uint16_t command, uint8_t *data, size_t numBytes, uint32_t delayMillis)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    uint8_t cmd[2];
    cmd[0] = command >> 8;   // MSB
    cmd[1] = command & 0xFF; // LSB

    size_t readBytes = 0;
    sfTkError_t rc = _theBus->readRegister(cmd, sizeof(cmd), data, numBytes, readBytes, delayMillis);
    if (rc != ksfTkErrOk)
        return rc;

    return readBytes == numBytes ? ksfTkErrOk : ksfTkErrBusUnderRead;
}

//--------------------------------------------------------------------------------------------------
// PRIVATE: Validate the CRC of each [MSB, LSB, CRC] triplet in the buffer
//
bool sfDevSTC3x::checkCRCs(uint8_t *data, size_t numBytes)
{
    for (size_t i = 0; i + 2 < numBytes; i += 3)
    {
        if (computeCRC8(&data[i], 2) != data[i + 2])
            return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
// Given an array and a number of bytes, this calculate CRC8 for those bytes
// CRC is only calc'd on the data portion (two bytes) of the four bytes being sent
// From: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
// Tested with: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
// x^8+x^5+x^4+1 = 0x31
//
uint8_t sfDevSTC3x::computeCRC8(uint8_t data[], uint8_t len)
{
    uint8_t crc = 0xFF; // Init with 0xFF

    for (uint8_t x = 0; x < len; x++)
    {
        crc ^= data[x]; // XOR-in the next input byte

        for (uint8_t i = 0; i < 8; i++)
        {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }

    return crc; // No output reflection
}
