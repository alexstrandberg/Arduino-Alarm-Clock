# Arduino-Alarm-Clock

The code for an alarm clock I made using an Arduino Mega, a 7-segment and a 14-segment LED Matrix, a Chronodot Real Time Clock, a VS1053 MP3 Player Breakout, a MCP9808 Temperature Sensor, Adafruit's EZ-Link Bluetooth Module, and USB-powered speakers.

- It displays and says the current time, temperature, and date and plays an MP3 file from an SD Card when an alarm goes off.  

- The Alarm Clock's settings can be changed using a Java program that communicates with the Arduino via Bluetooth.  

Check out the video for this project:
http://youtu.be/hzQA-dF6fks

The code was written by Alex Strandberg and is licensed under the MIT License, check LICENSE for more information

## Arduino Libraries:
- [Adafruit's GFX, LEDBackpack, MCP9808, and VS1053 libraries](https://github.com/adafruit)
- [Stephanie Maks' Chronodot library](https://github.com/Stephanie-Maks/Arduino-Chronodot)
- [EEPROM](http://arduino.cc/en/Reference/EEPROM)
- [SD](http://arduino.cc/en/Reference/SD)
- [SPI](http://arduino.cc/en/Reference/SPI)
- [Wire](http://arduino.cc/en/Reference/Wire)