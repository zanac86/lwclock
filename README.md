# LW-Clock

* ESP8266 (nodemcu, wemos)
* MAX7219 (4 modules 8x8)
* DS3231 (rtc chip)

## Original project

https://github.com/Lightwell-bg/LWClock.git

## Connection to nodemcu

| nodemcu | MAX7219 |
|---------|---------|
| (14) D5 | CLK     |
| (13) D7 | DIN     |
| (15) D8 | CS      |
| +5 VIN  | VCC     |

| nodemcu | DS3231 |
|---------|--------|
| (5) D1  | SCL    |
| (4) D2  | SDA    |
| +3.3 V  | V      |

## Web interface

* Connect to wifi network `-led-clock-` with password `31415926`
* Open `http://192.168.4.1`

## Libraries

Using libraries

* https://github.com/MajicDesigns/MD_MAXPanel
* https://github.com/MajicDesigns/MD_MAX72XX
* https://github.com/MajicDesigns/MD_Parola
* https://github.com/bblanchon/ArduinoJson
* https://github.com/NorthernWidget/DS3231

Library `northernwidget/DS3231` was patched for missed function `DateTime::dayOfTheWeek()`.

Old RTC library

* https://github.com/adafruit/Adafruit_BusIO
* https://github.com/adafruit/RTClib

## Using esptool.py

```
pip install esptool
```

```
esptool --port COM4 --baud 921600 read_flash 0 0x400000 fullflash.bin`
esptool --port COM4 --baud 921600 write_flash 0 fullflash.bin`
```

```
esptool --after no_reset --baud 921600 write_flash 0x000000 .pio\build\d1_mini\firmware.bin`
esptool --after hard_reset --baud 921600 write_flash 0x200000 .pio\build\d1_mini\littlefs.bin`
```

```
esptool erase_flash
esptool run
```
