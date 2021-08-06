/*
  Reading CO2 concentration from the STC3x
  By: Paul Clark
  Based on earlier code by: Nathan Seidle
  SparkFun Electronics
  Date: June 11th, 2021
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/products/18385

  This examples shows how to compensate for pressure, temperature and humidity
  Temperature and humidity are provided by the SHTC3 on the STC31 breakout

  Hardware Connections:
  Attach RedBoard to computer using a USB cable.
  Connect STC31 to RedBoard using Qwiic cable.
  Open Serial Monitor at 115200 baud.
*/

#include <Wire.h>

#include "SparkFun_STC3x_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_STC3x
STC3x mySensor;

#include "SparkFun_SHTC3.h" //Click here to get the library: http://librarymanager/All#SparkFun_SHTC3
SHTC3 mySHTC3;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("STC3x Example"));
  Wire.begin();

  //mySensor.enableDebugging(); // Uncomment this line to get helpful debug messages on Serial

  if (mySensor.begin() == false)
  {
    Serial.println(F("STC3x not detected. Please check wiring. Freezing..."));
    while (1)
      ;
  }

  if (mySHTC3.begin() != SHTC3_Status_Nominal)
  {
    Serial.println(F("SHTC3 not detected. Please check wiring. Freezing..."));
    while (1)
      ;
  }

  //We need to tell the STC3x what binary gas and full range we are using
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

  //We can compensate for temperature and relative humidity using the readings from the SHTC3
  
  if (mySHTC3.update() != SHTC3_Status_Nominal) // Request a measurement
  {
    Serial.println(F("Could not read the RH and T from the SHTC3! Freezing..."));
    while (1)
      ;
  }
  
  //In case the ‘Set temperature command’ has been used prior to the measurement command,
  //the temperature value given out by the STC31 will be that one of the ‘Set temperature command’.
  //When the ‘Set temperature command’ has not been used, the internal temperature value can be read out. 
  float temperature = mySHTC3.toDegC(); // "toDegC" returns the temperature as a floating point number in deg C
  Serial.print(F("Setting STC3x temperature to "));
  Serial.print(temperature, 2);
  Serial.print(F("C was "));
  if (mySensor.setTemperature(temperature) == false)
    Serial.print(F("not "));
  Serial.println(F("successful"));

  float RH = mySHTC3.toPercent(); // "toPercent" returns the percent humidity as a floating point number
  Serial.print(F("Setting STC3x RH to "));
  Serial.print(RH, 2);
  Serial.print(F("% was "));
  if (mySensor.setRelativeHumidity(RH) == false)
    Serial.print(F("not "));
  Serial.println(F("successful"));

  //If we have a pressure sensor available, we can compensate for ambient pressure too.
  //As an example, let's set the pressure to 840 mbar (== SF Headquarters)
  uint16_t pressure = 840;
  Serial.print(F("Setting STC3x pressure to "));
  Serial.print(pressure);
  Serial.print(F("mbar was "));
  if (mySensor.setPressure(pressure) == false)
    Serial.print(F("not "));
  Serial.println(F("successful"));
}

void loop()
{
  if (mySensor.measureGasConcentration()) // measureGasConcentration will return true when fresh data is available
  {
    Serial.println();

    Serial.print(F("CO2(%):"));
    Serial.print(mySensor.getCO2(), 2);

    Serial.println();
  }
  else
    Serial.print(F("."));

  delay(100);
}
