# Iridium-RockBLOCK
Interfacing with the RockBLOCK and the Iridium Satellite Communication Network via an Arduino Uno. The program depends on the functions of the ___IridiumSBD___ library, which sends AT commands to the RockBLOCK.

## Sites
Link to RockBLOCK's control panel: [RockBLOCK Control Panel](https://rockblock.rock7.com/Operations) </br>
Link to Sparkfun where the RockBLOCK was purchased: [RockBLOCK on Sparkfun](https://www.sparkfun.com/products/13745)

## Hardware
- Arduino Uno
- RockBLOCK MK2
- Adafruit Ultimate GPS

## Dependencies
- ___IridiumSBD___ Arduino Library
- ___TinyGPS++___ Arduino Library

## Setup
Change the virtual TX and RX pins and the SLEEP_PIN in the beginning of the code according to how the physical wiring is done. Open the Serial Monitor and set the Baud Rate to 115200.




