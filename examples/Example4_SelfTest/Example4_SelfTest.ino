/*
  Reading CO2 and temperature from the STC3x
  By: Paul Clark
  Based on earlier code by: Nathan Seidle
  SparkFun Electronics
  Date: June 11th, 2021
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/products/18385

  This example performs a self test and then prints the sensor product and serial numbers

  Hardware Connections:
  Attach RedBoard to computer using a USB cable.
  Connect STC31 to RedBoard using Qwiic cable.
  Open Serial Monitor at 115200 baud.
*/

#include <Wire.h>

#include "SparkFun_STC3x_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_STC3x
STC3x mySensor;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("STC3x Example"));
  Wire.begin();

  //mySensor.enableDebugging(); // Uncomment this line to get helpful debug messages on Serial

  if (mySensor.begin() == false)
  {
    Serial.println(F("Sensor not detected. Please check wiring. Freezing..."));
    while (1)
      ;
  }

  //We need to tell the sensor what binary gas and full range we are using
  //Possible values are:
  //  STC3X_BINARY_GAS_CO2_N2_100   : Set binary gas to CO2 in N2.  Range: 0 to 100 vol%
  //  STC3X_BINARY_GAS_CO2_AIR_100  : Set binary gas to CO2 in Air. Range: 0 to 100 vol%
  //  STC3X_BINARY_GAS_CO2_N2_25    : Set binary gas to CO2 in N2.  Range: 0 to 25 vol%
  //  STC3X_BINARY_GAS_CO2_AIR_25   : Set binary gas to CO2 in Air. Range: 0 to 25 vol%

  if (mySensor.setBinaryGas(STC3X_BINARY_GAS_CO2_AIR_25) == false)
  {
    Serial.println(F("Could not set the binary gas! Freezing..."));
    while (1)
      ;
  }

  //Let's perform a self test
  if (mySensor.performSelfTest() == true)
  {
    Serial.println(F("Self test passed!"));
  }
  else
  {
    Serial.println(F("Self test FAILED! Please check the debug messages for more details"));
  }

  //Let's print the sensor product and serial numbers
  uint32_t productNumber;
  char serialNumber[17]; // Serial number is 16 digits plus trailing NULL
  if (mySensor.getProductIdentifier(&productNumber, serialNumber) == true)
  {
    Serial.print(F("Sensor product number is 0x"));
    Serial.println(productNumber, HEX);
    Serial.print(F("Sensor serial number is 0x"));
    Serial.println(serialNumber);
  }
  else
  {
    Serial.println(F("getProductIdentifier FAILED! Please check the debug messages for more details"));
  }  
}

void loop()
{
  if (mySensor.measureGasConcentration()) // measureGasConcentration will return true when fresh data is available
  {
    Serial.println();

    Serial.print(F("CO2(%):"));
    Serial.print(mySensor.getCO2(), 2);

    Serial.print(F("\tTemperature(C):"));
    Serial.print(mySensor.getTemperature(), 2);

    Serial.println();
  }
  else
    Serial.print(F("."));

  delay(100);
}
