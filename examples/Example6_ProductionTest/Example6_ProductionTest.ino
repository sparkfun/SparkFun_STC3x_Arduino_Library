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

  Production test: LED_BUILTIN will light up if the STC31 and SHTC3 are detected correctly
  and the CO2, humidity and temperature are within bounds.

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
  Serial.println(F("STC31 & SHTC3 Production Test"));
  Wire.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  delay(250); // Try every 0.25 seconds

  Wire.begin();

  Wire.beginTransmission(0x29); // Detect STC31 on address 0x29
  if (Wire.endTransmission() != 0)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: nothing detected on address 0x29"));
    return;
  }
  
  //Initialize sensor
  if (mySensor.begin() == false)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: STC31 .begin failed"));
    return;
  }

  //Tell the sensor what binary gas and full range we are using

  if (mySensor.setBinaryGas(STC3X_BINARY_GAS_CO2_AIR_25) == false)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: .setBinaryGas failed"));
    return;
  }

  delay(1001);

  if (mySensor.measureGasConcentration() == false)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: .measureGasConcentration failed"));
    return;
  }

  float myCO2 = mySensor.getCO2();
  if ((myCO2 < -10) || (myCO2 > 20))
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: CO2 out of bounds"));
    return;
  }
  
  float myTemp = mySensor.getTemperature();
  if ((myTemp < 10) || (myTemp > 30))
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: STC31 temperature out of bounds"));
    return;
  }
  

  Wire.beginTransmission(0x70); // Detect SHTC3 on address 0x70
  if (Wire.endTransmission() != 0)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: nothing detected on address 0x70"));
    return;
  }
  
  //Initialize sensor
  if (mySHTC3.begin() != SHTC3_Status_Nominal)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: SHTC3 .begin failed"));
    return;
  }

  if (mySHTC3.update() != SHTC3_Status_Nominal) // Request a measurement
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: Could not read the RH and T from the SHTC3"));
    return;
  }
  
  myTemp = mySHTC3.toDegC();
  if ((myTemp < 10) || (myTemp > 30))
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: SHTC3 temperature out of bounds"));
    return;
  }

  float myRH = mySHTC3.toPercent();
  if ((myRH < 10) || (myRH > 90))
  {
    digitalWrite(LED_BUILTIN, LOW);
    Wire.end();
    Serial.println(F("Error: SHTC3 humidity out of bounds"));
    return;
  }


  // All good!
  Serial.println(F("Test passed!"));
  digitalWrite(LED_BUILTIN, HIGH);
  Wire.end();  
}
