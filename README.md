# 25-fall-payload

Sensors files for the ERAU ASCEND! team.

> **Note:** This repository should live under the [AscendERAU1](https://github.com/AscendERAU1) organization, not a personal account.

## Status

| Component         | Status             | Notes                                                                                                                                            |
| ----------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Main sensor stack | Working (Tested)   | Been tested and works well                                                                                                                       |
| Transceiver       | Working (Untested) | Has not been tested but shown to build and compile. Lives on [GS_TX](https://github.com/AscendERAU1/25-fall-payload/tree/GS_TX) Branch currently |
| Motor             | Not working        | Untested and Not working                                                                                                                         |


## Overview

Code behind the payload on the weather balloon. Currently all sensors work and TX has been created and built but not tested.


## Sensors

| Component         | Status             | Notes                                                                                                                                                                                                                                                                                                         |
| ----------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Main sensor stack | Working (Tested)   | Been tested and works well.<br><br>**Sensors:**<br>- BNO055 (Accelerometer)<br>- ISM3330DLC (Accelerometer, Temp, Gyroscope)<br>- SCD41 (CO2 detector, Temp, Humidity)<br>- BMP390 (Temp, Pressure, Altitude)<br>- LTR390 (UV, Ambient Light)<br>- AS7341 (Light Wavelength Detector)<br>- Adafruit GPS (GPS) |
| Transceiver       | Working (Untested) | Has not been tested but shown to build and compile. Lives on [GS_TX](https://github.com/AscendERAU1/25-fall-payload/tree/GS_TX) Branch currently.<br><br>**Ground Station (GS) addition:**<br>- Transmits GPS and data packets<br>- Currently set to 909 MHz                                                  |
| Motor             | Not working        | Untested and Not working                                                                                                                                                                                                                                                                                      |

## Contributing

This repo belongs to the ERAU ASCEND! team. Please open PRs against the appropriate component folder and note in your PR description what subsystem it affects.



