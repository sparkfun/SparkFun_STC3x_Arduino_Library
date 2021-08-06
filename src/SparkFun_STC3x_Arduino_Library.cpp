/*
  This is a library written for the STC3x family of CO2 sensors
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/nnnnn

  Written by Paul Clark @ SparkFun Electronics, June 11th, 2021

  The STC31 measures CO2 concentrations up to 100%

  This library handles the initialization of the STC3x and outputs
  the CO2 concentration. The readings can be compensated for
  Relative Humidity and Temperature.

  https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.13

  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for more details.
*/

#include "SparkFun_STC3x_Arduino_Library.h"

STC3x::STC3x(STC3x_sensor_product_number_e sensorType)
{
  // Constructor
  _sensorType = sensorType;
}

//Initialize the Serial port
#ifdef USE_TEENSY3_I2C_LIB
bool STC3x::begin(uint8_t i2cAddress, i2c_t3 &wirePort)
#else
bool STC3x::begin(uint8_t i2cAddress, TwoWire &wirePort)
#endif
{
  _stc3x_i2c_address = i2cAddress; //Grab which address and port the user wants us to use
  _i2cPort = &wirePort;

  bool success = true;

  char serialNumber[17]; // Serial number is 16 digits plus trailing NULL
  uint32_t productNumber;
  success &= getProductIdentifier(&productNumber, serialNumber); // Read the serial number. Return false if the CRC check fails.
  if (success == false)
    return (false);

  if (_printDebug == true)
  {
    _debugPort->print(F("STC3x::begin: got product number 0x"));
    _debugPort->println(productNumber, HEX);
    _debugPort->print(F("STC3x::begin: got serial number 0x"));
    _debugPort->println(serialNumber);
    if (productNumber != (uint32_t)_sensorType)
      _debugPort->println(F("STC3x::begin: PANIC! Unexpected product number! Are you sure this is a STC31?"));
  }

  return (success);
}

//Calling this function with nothing sets the debug port to Serial
//You can also call it with other streams like Serial1, SerialUSB, etc.
void STC3x::enableDebugging(Stream &debugPort)
{
  _debugPort = &debugPort;
  _printDebug = true;
}

//Set the binary gas. See 3.3.2
//When the system is reset, or wakes up from sleep mode, the sensor goes back to default mode, in which no binary is selected.
//This means that the binary gas must be reconfigured.
//When no binary gas is selected (default mode) the concentration measurement will return undefined results.
//This allows to detect unexpected sensor interruption (e.g. due to temporary power loss) and consequently reset the binary gas to the appropriate mixture.
bool STC3x::setBinaryGas(STC3X_binary_gas_type_e binaryGas)
{
  return (sendCommand(STC3x_COMMAND_SET_BINARY_GAS, (uint16_t)binaryGas));
}

//Set the relative humidity. See 3.3.3
//The measurement principle of the concentration measurement is dependent on the humidity of the gas.
//With the set relative humidity command, the sensor uses internal algorithms to compensate the concentration results.
bool STC3x::setRelativeHumidity(float RH)
{
  uint16_t _rh = (uint16_t)(RH * 65535.0 / 100.0); // See 3.5 Conversion to Physical Values
  bool success = sendCommand(STC3X_COMMAND_SET_RELATIVE_HUMIDITY, _rh);
  return (success);
}

//Set the temperature. See 3.3.4
//The concentration measurement requires a compensation of temperature.
//Per default, the sensor uses the internal temperature sensor to compensate the concentration results.
//However, when using the SHTxx, it is recommended to also use its temperature value, because it is more accurate.
bool STC3x::setTemperature(float temperature)
{
  // Avoid any ambiguity casting int16_t to uint16_t
  STC3x_signedUnsigned16_t signedUnsigned;
  signedUnsigned.signed16 = (int16_t)(temperature * 200.0);
  bool success = sendCommand(STC3X_COMMAND_SET_TEMPERATURE, signedUnsigned.unsigned16);
  return (success);
}

//Set the pressure. See 3.3.5
//A pressure value can be written into the sensor, for density compensation of the gas concentration measurement.
//It is recommended to set the pressure level, if it differs significantly from 1013mbar.
//Pressure compensation is valid from 600mbar to 1200mbar.
bool STC3x::setPressure(uint16_t pressure)
{
  if ((pressure < 600) || (pressure > 1200))
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("STC3x::setPressure: pressure is out of bounds. Aborting..."));
    }
    return (false);
  }

  bool success = sendCommand(STC3X_COMMAND_SET_PRESSURE, pressure);
  return (success);
}

//Get 9 bytes from STC3x. See 3.3.6
//Updates global variables with floats
//Returns true if data is read successfully
bool STC3x::measureGasConcentration(void)
{
  // The measurement command should not be triggered more often than once a second.
  // Check if it is OK to trigger a new measurement
  if (millis() < (_lastReadTimeMillis + 1000))
  {
    if (_printDebug == true)
    {
      _debugPort->print(F("STC3x::measureGasConcentration: too early! Please wait another "));
      _debugPort->print(_lastReadTimeMillis + 1000 - millis());
      _debugPort->println(F("ms"));
    }
    return (false); // Too early!
  }

  STC3x_unsigned16Bytes_t tempCO2;
  tempCO2.unsigned16 = 0;
  STC3x_signed16Bytes_t  tempTemperature;
  tempTemperature.signed16 = 0;

  _i2cPort->beginTransmission(_stc3x_i2c_address);
  _i2cPort->write(STC3X_COMMAND_MEASURE_GAS_CONCENTRATION >> 8);   //MSB
  _i2cPort->write(STC3X_COMMAND_MEASURE_GAS_CONCENTRATION & 0xFF); //LSB
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK

  delay(75); //Datasheet specifies 66ms but sensor seems to need at least 70ms. 75ms provides margin.

  uint8_t receivedBytes = (uint8_t)_i2cPort->requestFrom((uint8_t)_stc3x_i2c_address, (uint8_t)9);
  bool error = false;
  if (_i2cPort->available())
  {
    byte bytesToCrc[2];
    for (byte x = 0; x < 9; x++)
    {
      byte incoming = _i2cPort->read();

      switch (x)
      {
      case 0:
      case 1:
        tempCO2.bytes[x == 0 ? 1 : 0] = incoming; // Store the two CO2 bytes in little-endian format
        bytesToCrc[x] = incoming; // Calculate the CRC on the two CO2 bytes in the order they arrive
        break;
      case 3:
      case 4:
        tempTemperature.bytes[x == 3 ? 1 : 0] = incoming; // Store the two T bytes in little-endian format
        bytesToCrc[x % 3] = incoming; // Calculate the CRC on the two T bytes in the order they arrive
        break;
      case 6:
      case 7:
        bytesToCrc[x % 3] = incoming; // Calculate the CRC on the two reserved bytes in the order they arrive
        break;
      default: // x == 2, 5, 8
        //Validate CRC
        uint8_t foundCrc = computeCRC8(bytesToCrc, 2); // Calculate what the CRC should be for these two bytes
        if (foundCrc != incoming) // Does this match the CRC byte from the sensor?
        {
          if (_printDebug == true)
          {
            _debugPort->print(F("STC3x::measureGasConcentration: found CRC in byte "));
            _debugPort->print(x);
            _debugPort->print(F(", expected 0x"));
            _debugPort->print(foundCrc, HEX);
            _debugPort->print(F(", got 0x"));
            _debugPort->println(incoming, HEX);
          }
          error = true;
        }
        break;
      }
    }
  }
  else
  {
    if (_printDebug == true)
    {
      _debugPort->print(F("STC3x::measureGasConcentration: no STC3x data found from I2C, I2C claims we should receive "));
      _debugPort->print(receivedBytes);
      _debugPort->println(F(" bytes"));
    }
    return (false);
  }

  if (error)
  {
    if (_printDebug == true)
      _debugPort->println(F("STC3x::measureGasConcentration: encountered error reading STC3x data."));
    return (false);
  }

  //Now copy the data into their associated floats
  co2 = ((((float)tempCO2.unsigned16) - 16384) / 32768) * 100; // See 3.5 Conversion to Physical Values
  temperature = ((float)tempTemperature.signed16) / 200; // See 3.5 Conversion to Physical Values

  //Mark our global variables as fresh
  co2HasBeenReported = false;
  temperatureHasBeenReported = false;
  _lastReadTimeMillis = millis();

  return (true); //Success! New data available in globals.
}

//Returns the latest available CO2 level
//If the current level has already been reported, trigger a new read
float STC3x::getCO2(void)
{
  if (co2HasBeenReported == true) //Trigger a new read
    measureGasConcentration();    //Pull in new co2 and temp into global vars

  co2HasBeenReported = true;

  return (co2);
}

//Returns the latest available temperature
//If the current level has already been reported, trigger a new read
float STC3x::getTemperature(void)
{
  if (temperatureHasBeenReported == true) //Trigger a new read
    measureGasConcentration();            //Pull in new co2 and temp into global vars

  temperatureHasBeenReported = true;

  return (temperature);
}

//Force a sensor recalibration using the provided concentration
//Forced recalibration is used to improve the sensor output with a known reference value.
//See the Field Calibration Guide for more details.
//If no argument is given, the sensor will assume a default value of 0 vol%.
//This command will trigger a concentration measurement as described in 3.3.6 and therefore it will take the same measurement time.
bool STC3x::forcedRecalibration(float concentration, uint16_t delayMillis)
{
  if (concentration < 0.0) // Ignore negative concentrations
    concentration = 0.0;
  if (concentration > 100.0) // Ignore concentrations above 100%
    concentration = 100.0;
  uint16_t conc_16 = (uint16_t)(((concentration * 32768) / 100) + 16384); // See 3.5 Conversion to Physical Values
  bool success = sendCommand(STC3X_COMMAND_FORCED_RECALIBRATION, conc_16);
  if (delayMillis > 0)
    delay(delayMillis); // Allow time for the measurement to complete
  return (success);
}

//Perform self test. Takes 20ms to complete. See 3.3.10
//In case of a successful self-test the sensor returns 0x0000 with correct CRC.
bool STC3x::performSelfTest(void)
{
  uint16_t response;

  bool success = readRegister(STC3X_COMMAND_SELF_TEST, &response, 20);

  if (_printDebug == true)
  {
    _debugPort->print(F("STC3x::performSelfTest: sensor response is 0x"));
    if (response < 0x1000) _debugPort->print(F("0"));
    if (response < 0x100) _debugPort->print(F("0"));
    if (response < 0x10) _debugPort->print(F("0"));
    _debugPort->println(response, HEX);
  }

  return (success && (response == 0x0000));
}

//Peform soft reset. See 3.3.11
//Writes command code 0x06 to general call address 0x00.
//Note that the I2C address is not acknowledged.
//After the reset command the sensor will take maximum 12ms to reset.
void STC3x::softReset(uint16_t delayMillis)
{
  _i2cPort->beginTransmission(0x00);
  _i2cPort->write(0x06);
  _i2cPort->endTransmission();

  if (delayMillis > 0)
    delay(delayMillis);
}

//Get 18 bytes from STC3x. Convert 64-bit serial number to ASCII chars. See 3.3.13
//Returns true if serial number is read successfully
bool STC3x::getProductIdentifier(uint32_t *productNumber, char *serialNumber)
{
  STC3x_unsigned32Bytes_t prodNo;  // Storage for the product number
  prodNo.unsigned32 = 0;

  bool success = sendCommand(STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_1);
  success &= sendCommand(STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_2);

  if (!success)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("STC3x::getProductIdentifier: STC3x did not acknowledge STC3X_COMMAND_READ_PRODUCT_IDENTIFIER"));
    }
    return (false);
  }

  uint8_t receivedBytes = (uint8_t)_i2cPort->requestFrom((uint8_t)_stc3x_i2c_address, (uint8_t)18);
  bool error = false;
  if (_i2cPort->available())
  {
    byte bytesToCrc[2];
    int digit = 0;
    for (byte x = 0; x < 18; x++)
    {
      byte incoming = _i2cPort->read();

      switch (x)
      {
      case 0: // The product number arrives as: two bytes, CRC, two bytes, CRC
      case 1:
      case 3:
      case 4:
        prodNo.bytes[x < 3 ? (x == 0 ? 3 : 2) : (x == 3 ? 1 : 0)] = incoming;
        bytesToCrc[x % 3] = incoming;
        break;
      case 6: // The serial number arrives as: two bytes, CRC, two bytes, CRC two bytes, CRC, two bytes, CRC
      case 7:
      case 9:
      case 10:
      case 12:
      case 13:
      case 15:
      case 16:
        serialNumber[digit++] = convertHexToASCII(incoming >> 4); // Convert each nibble to ASCII
        serialNumber[digit++] = convertHexToASCII(incoming & 0x0F);
        bytesToCrc[x % 3] = incoming;
        break;
      default: // x == 2, 5, 8, etc
        //Validate CRC
        uint8_t foundCrc = computeCRC8(bytesToCrc, 2); // Calculate what the CRC should be for these two bytes
        if (foundCrc != incoming) // Does this match the CRC byte from the sensor?
        {
          if (_printDebug == true)
          {
            _debugPort->print(F("STC3x::getProductIdentifier: found CRC in byte "));
            _debugPort->print(x);
            _debugPort->print(F(", expected 0x"));
            _debugPort->print(foundCrc, HEX);
            _debugPort->print(F(", got 0x"));
            _debugPort->println(incoming, HEX);
          }
          error = true;
        }
        break;
      }
      serialNumber[digit] = 0; // NULL-terminate the string
    }
  }
  else
  {
    if (_printDebug == true)
    {
      _debugPort->print(F("STC3x::getProductIdentifier: no STC3x data found from I2C, I2C claims we should receive "));
      _debugPort->print(receivedBytes);
      _debugPort->println(F(" bytes"));
    }
    return (false);
  }

  if (error)
  {
    if (_printDebug == true)
      _debugPort->println(F("STC3x::getProductIdentifier: encountered error reading STC3x data."));
    return (false);
  }

  *productNumber = prodNo.unsigned32;

  return (true); //Success!
}

//PRIVATE: Convert serial number digit to ASCII
char STC3x::convertHexToASCII(uint8_t digit)
{
  if (digit <= 9)
    return (char(digit + 0x30));
  else
    return (char(digit + 0x41 - 10)); // Use upper case for A-F
}

//Sends a command along with arguments and CRC
bool STC3x::sendCommand(uint16_t command, uint16_t arguments)
{
  uint8_t data[2];
  data[0] = arguments >> 8;
  data[1] = arguments & 0xFF;
  uint8_t crc = computeCRC8(data, 2); //Calc CRC on the arguments only, not the command

  _i2cPort->beginTransmission(_stc3x_i2c_address);
  _i2cPort->write(command >> 8);     //MSB
  _i2cPort->write(command & 0xFF);   //LSB
  _i2cPort->write(arguments >> 8);   //MSB
  _i2cPort->write(arguments & 0xFF); //LSB
  _i2cPort->write(crc);
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Sends just a command, no arguments, no CRC
bool STC3x::sendCommand(uint16_t command)
{
  _i2cPort->beginTransmission(_stc3x_i2c_address);
  _i2cPort->write(command >> 8);   //MSB
  _i2cPort->write(command & 0xFF); //LSB
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Gets two bytes from STC3x plus CRC.
//Returns true if endTransmission returns zero _and_ the CRC check is valid
bool STC3x::readRegister(uint16_t registerAddress, uint16_t *response, uint16_t delayMillis)
{
  _i2cPort->beginTransmission(_stc3x_i2c_address);
  _i2cPort->write(registerAddress >> 8);   //MSB
  _i2cPort->write(registerAddress & 0xFF); //LSB
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK

  delay(delayMillis);

  _i2cPort->requestFrom(_stc3x_i2c_address, (uint8_t)3); // Request data and CRC
  if (_i2cPort->available())
  {
    uint8_t data[2];
    data[0] = _i2cPort->read();
    data[1] = _i2cPort->read();
    uint8_t crc = _i2cPort->read();
    *response = (uint16_t)data[0] << 8 | data[1];
    uint8_t expectedCRC = computeCRC8(data, 2);
    if (crc == expectedCRC) // Return true if CRC check is OK
      return (true);
    if (_printDebug == true)
    {
      _debugPort->print(F("STC3x::readRegister: CRC fail: expected 0x"));
      _debugPort->print(expectedCRC, HEX);
      _debugPort->print(F(", got 0x"));
      _debugPort->println(crc, HEX);
    }
  }
  return (false);
}

//Given an array and a number of bytes, this calculate CRC8 for those bytes
//CRC is only calc'd on the data portion (two bytes) of the four bytes being sent
//From: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
//Tested with: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
//x^8+x^5+x^4+1 = 0x31
uint8_t STC3x::computeCRC8(uint8_t data[], uint8_t len)
{
  uint8_t crc = 0xFF; //Init with 0xFF

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

  return crc; //No output reflection
}
