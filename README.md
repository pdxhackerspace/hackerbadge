# PDX Hackerspace Hackerbadge

The unofficial badge of PDX Hackerspace.

- 37 WS2812B addressable RGB LEDs
- SeeedStudio Xiao ESP32-S3 (8MB PSRAM/8MB flash)
- LSM6DSO accelerometer/gyroscope
- 18650 LiPO battery
- Qwiic/STEMMA QT I2C connector

Though we've specced this for a Xiao ESP32-S3, this should work with any Xiao board with a compatible pinout. If the board doesn't support LiPO battery charging then obviously it will only work when plugged into USB.

![front](docs/images/front-0.1.0.jpg) ![back](docs/images/back-0.1.0.jpg)

## LED power

LEDs are powered either through the VBUS or VBAT, avoiding straining the board's 3.3V regulator and providing more voltage to them for more reliable operation. We use two SS23 Schottky diodes to OR the power sources together and avoid feeding back into one another.

## Qwiic/STEMMA QT/I2C

SDA -D1
SCL - D0

## Acceleometer/Gyroscope

The board includes an LSM6DSO accelerometer/gyroscope for fancy effects, or you can just ignore it. I2C address 0x6A.

## Firmware

### WLED

### ESPHome

### CircuitPython

Install CircuitPython

## Assembly

The badge comes prebuilt except for the Xiao and the battery holder, which are mounted on the back of the board. We suggest soldering on the Xiao using solder paste and a hot air gun. You're unlikely to make a solder plate work due to the components on the front of the board. The most difficult part will be the battery connectors under the board.


