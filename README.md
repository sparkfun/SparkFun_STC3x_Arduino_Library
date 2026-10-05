# SparkFun STC3x CO<sub>2</sub> Sensor Arduino Library

Arduino Library for the Sensirion STC3x family of CO<sub>2</sub> sensors (STC31)

![License](https://img.shields.io/github/license/sparkfun/SparkFun_STC3x_Arduino_Library)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_STC3x_Arduino_Library)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_STC3x_Arduino_Library)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_STC3x_Arduino_Library/build-deploy-ghpages.yml?label=doc%20build)
[![Compile Test](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/actions/workflows/test-compile-sketch.yml/badge.svg)](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/actions/workflows/test-compile-sketch.yml)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_STC3x_Arduino_Library)

[![SparkX STC31 Breakout (Qwiic) (SPX-18385)](https://cdn.sparkfun.com//assets/parts/1/7/7/9/5/18385-STC31_Breakout-01.jpg)](https://www.sparkfun.com/products/18385)

An Arduino library for the STC3x family of CO<sub>2</sub> sensors from Sensirion. The STC31 uses Sensirion’s patented CMOSens® sensor technology to measure CO<sub>2</sub> concentrations up to 100%. Readings can be compensated for relative humidity, temperature and pressure.

This library can be installed through the Arduino Library Manager: search for 'SparkFun STC3x'. It depends on the [SparkFun Toolkit](https://github.com/sparkfun/SparkFun_Toolkit), which the Library Manager installs automatically.

> [!NOTE]
> Version 2.0 of this library is built on the SparkFun Toolkit. The primary class is now `SparkFunSTC3x`; the original `STC3x` class is still available for backward compatibility but is deprecated.
>
> Debug printing (`enableDebugging()`) has been removed - the method remains but does nothing. Errors are reported through the method return values. The Teensy `i2c_t3` option has also been removed.

### Supported Products

| Product | Description|
|--|--|
|[SparkX STC31 Breakout (Qwiic) (SPX-18385)](https://www.sparkfun.com/products/18385) | Breakout with the STC31 and SHTC3|
|[SparkFun MicroMod Environmental Function Board (SEN-18632)](https://www.sparkfun.com/products/18632) | MicroMod Function Board with the SGP40, STC31 and SHTC3|

## Documentation

|Reference | Description |
|---|---|
|[Library Documentation](https://docs.sparkfun.com/SparkFun_STC3x_Arduino_Library/)| The full documentation and API for this Arduino library|
|[SparkFun STC3x Arduino Library](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library)| Arduino Library - GitHub Repository|
|[STC31 Datasheet](documents/Sensirion_Thermal_Conductivity_Datasheet_STC31_D1.pdf) | Datasheet for the STC31|
|[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)| Basic information on how to install an Arduino library|

## Examples

The following examples are provided with the library

| Example | Description |
|---|---|
|[Basic Readings](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example1_BasicReadings/Example1_BasicReadings.ino)| Read the CO<sub>2</sub> concentration and temperature.|
|[PHT Compensation](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example2_PHTCompensation/Example2_PHTCompensation.ino)| Compensate readings using pressure, plus humidity and temperature from an SHTC3.|
|[Alternate I2C](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example3_AlternateI2C/Example3_AlternateI2C.ino)| Use an alternate I2C address and port.|
|[Self Test](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example4_SelfTest/Example4_SelfTest.ino)| Run the sensor self test and read the product and serial numbers.|
|[Automatic Self Calibration](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example5_AutomaticSelfCalibration/Example5_AutomaticSelfCalibration.ino)| Perform a forced recalibration and enable automatic self calibration.|
|[Production Test](https://github.com/sparkfun/SparkFun_STC3x_Arduino_Library/blob/main/examples/Example6_ProductionTest/Example6_ProductionTest.ino)| Test used during board production.|

## License Information

This product is ***open source***!

This product is licensed using the [MIT Open Source License](https://opensource.org/license/mit).

Library originally written by Paul Clark, based on the SCD30 library by Nathan Seidle ([SparkFun](http://www.sparkfun.com)).
