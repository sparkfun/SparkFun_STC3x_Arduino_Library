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

#ifndef __SparkFun_STC3x_ARDUINO_LIBARARY_H__
#define __SparkFun_STC3x_ARDUINO_LIBARARY_H__

// Uncomment the next #define if using an Teensy >= 3 or Teensy LC and want to use the dedicated I2C-Library for it
// Then you also have to include <i2c_t3.h> on your application instead of <Wire.h>

// #define USE_TEENSY3_I2C_LIB

#include "Arduino.h"
#ifdef USE_TEENSY3_I2C_LIB
#include <i2c_t3.h>
#else
#include <Wire.h>
#endif

//The default I2C address for the STC31 is 0x29. 0X2A, 0x2B and 0x2C can be selected via the ADDR pin.
#define STC3x_DEFAULT_ADDRESS 0x29

//Available commands

#define STC3x_COMMAND_DISABLE_CRC                             0x3768 // Note: the library does not support this!

#define STC3x_COMMAND_SET_BINARY_GAS                          0x3615

typedef enum
{
  STC3X_BINARY_GAS_CO2_N2_100 = 0x0000,                       // Set binary gas to CO2 in N2. Range: 0 to 100 vol%
  STC3X_BINARY_GAS_CO2_AIR_100,                               // Set binary gas to CO2 in Air. Range: 0 to 100 vol%
  STC3X_BINARY_GAS_CO2_N2_25,                                 // Set binary gas to CO2 in N2. Range: 0 to 25 vol%
  STC3X_BINARY_GAS_CO2_AIR_25                                 // Set binary gas to CO2 in Air. Range: 0 to 25 vol%
} STC3X_binary_gas_type_e;

#define STC3X_COMMAND_SET_RELATIVE_HUMIDITY                   0x3624
#define STC3X_COMMAND_SET_TEMPERATURE                         0x361E
#define STC3X_COMMAND_SET_PRESSURE                            0x362F
#define STC3X_COMMAND_MEASURE_GAS_CONCENTRATION               0x3639
#define STC3X_COMMAND_FORCED_RECALIBRATION                    0x3661
#define STC3X_COMMAND_AUTOMATIC_CALIBRATION_ENABLE            0x3FEF
#define STC3X_COMMAND_AUTOMATIC_CALIBRATION_DISABLE           0x3F6E
#define STC3X_COMMAND_PREPARE_READ_STATE                      0x3752
#define STC3X_COMMAND_READ_WRITE_STATE                        0xE133
#define STC3X_COMMAND_APPLY_STATE                             0x3650
#define STC3X_COMMAND_SELF_TEST                               0x365B
#define STC3X_COMMAND_ENTER_SLEEP_MODE                        0x3677
#define STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_1               0x367C
#define STC3X_COMMAND_READ_PRODUCT_IDENTIFIER_2               0xE102

typedef union
{
  int16_t signed16;
  uint16_t unsigned16;
} STC3x_signedUnsigned16_t; // Avoid any ambiguity casting int16_t to uint16_t

typedef union
{
  uint16_t unsigned16;
  uint8_t bytes[2];
} STC3x_unsigned16Bytes_t; // Make it easy to convert 2 x uint8_t to uint16_t

typedef union
{
  int16_t signed16;
  uint8_t bytes[2];
} STC3x_signed16Bytes_t; // Make it easy to convert 2 x uint8_t to int16_t

typedef union
{
  uint32_t unsigned32;
  uint8_t bytes[4];
} STC3x_unsigned32Bytes_t; // Make it easy to convert 4 x uint8_t to uint32_t

typedef enum
{
  STC3x_SENSOR_STC31 = 0x08010301,
} STC3x_sensor_product_number_e;

class STC3x
{
public:
  STC3x(STC3x_sensor_product_number_e sensorType = STC3x_SENSOR_STC31);

#ifdef USE_TEENSY3_I2C_LIB
  bool begin(uint8_t i2cAddress = STC3x_DEFAULT_ADDRESS, i2c_t3 &wirePort = Wire); //By default use Wire port
#else
  bool begin(uint8_t i2cAddress = STC3x_DEFAULT_ADDRESS, TwoWire &wirePort = Wire); //By default use Wire port
#endif

  void enableDebugging(Stream &debugPort = Serial); //Turn on debug printing. If user doesn't specify then Serial will be used

  bool setBinaryGas(STC3X_binary_gas_type_e binaryGas = STC3X_BINARY_GAS_CO2_AIR_25);

  // Set the RH
  bool setRelativeHumidity(float RH); // Returns true if I2C transfer was OK

  // Set the temperature
  bool setTemperature(float temperature); // Returns true if I2C transfer was OK

  // Set the atmospheric pressure: 600mbar to 1200mbar
  bool setPressure(uint16_t pressure); // Returns true if I2C transfer was OK

  bool measureGasConcentration(void); // Check for fresh data; store it. Returns true if fresh data is available

  float getCO2(void); // Return the CO2 concentration %. Automatically request fresh data is the data is 'stale'
  float getTemperature(void); // Return the temperature. Automatically request fresh data is the data is 'stale'

  bool forcedRecalibration(float concentration, uint16_t delayMillis = 75);

  bool enableAutomaticSelfCalibration(void) { return (sendCommand(STC3X_COMMAND_AUTOMATIC_CALIBRATION_ENABLE)); }
  bool disableAutomaticSelfCalibration(void) { return (sendCommand(STC3X_COMMAND_AUTOMATIC_CALIBRATION_DISABLE)); }

  bool performSelfTest(void); // Returns true if sensor responds with a result of 0x0000

  void softReset(uint16_t delayMillis = 12); // Writes command code 0x06 to general call address 0x00

  bool getProductIdentifier(uint32_t *productNumber, char *serialNumber); // Returns true if I2C transfer was OK

  bool enterSleepMode(void) { return (sendCommand(STC3X_COMMAND_ENTER_SLEEP_MODE)); }

  bool sendCommand(uint16_t command, uint16_t arguments);
  bool sendCommand(uint16_t command);

  bool readRegister(uint16_t registerAddress, uint16_t *response, uint16_t delayMillis = 0);

  uint8_t computeCRC8(uint8_t data[], uint8_t len);

private:
  //Variables
#ifdef USE_TEENSY3_I2C_LIB
  i2c_t3 *_i2cPort = NULL; //The generic connection to user's chosen I2C hardware
#else
  TwoWire *_i2cPort = NULL; //The generic connection to user's chosen I2C hardware
#endif

  //Sensor type - for future compatibility
  STC3x_sensor_product_number_e _sensorType;

  //I2C Address
  uint8_t _stc3x_i2c_address;

  //Global main datums
  float co2 = 0;
  float temperature = 0;

  //These track the staleness of the current data
  //This allows us to avoid calling readMeasurement() every time individual datums are requested
  bool co2HasBeenReported = true;
  bool temperatureHasBeenReported = true;

  //Keep track of the least reading
  //Datasheet says "The measurement command should not be triggered more often than once a second."
  unsigned long _lastReadTimeMillis = millis();

  //Convert serial number digit to ASCII
  char convertHexToASCII(uint8_t digit);

  //Debug
  Stream *_debugPort;          //The stream to send debug messages to if enabled. Usually Serial.
  boolean _printDebug = false; //Flag to print debugging variables
};
#endif
