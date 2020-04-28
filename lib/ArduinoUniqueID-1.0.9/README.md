# ArduinoUniqueID

This Library gets the Unique ID / Manufacture Serial Number from the Atmel AVR, SAM, SAMD, STM32, and ESP Microcontroller.

# Atmel AVR Microcontroller

## Unique Serial ID - Hidden Serial Number

The datasheet of the Atmega328pb chip has a section 'Serial Number' how explain every chip have a unique device ID with 10 bytes. <br/>

The datasheet of the Atmega328p chip does not say anything about the serial number, but I tested using the same Z-pointer Address on the datasheet of the Atmega328pb. <br/>

Apparently, the chip Atmega328p have a hidden serial number with 9 bytes, and others AVR Microcontroller maybe too, like the table below. <br/>

| Z-pointer Address | Atmega328pb | Atmega328p |
| :-------: | :------: | :------:|
| 0x000E | Byte 0 | Byte 0 |
| 0x000F | Byte 1 | Byte 1 |
| 0x0010 | Byte 2 | Byte 2 |
| 0x0011 | Byte 3 | Byte 3 |
| 0x0012 | Byte 4 | Byte 4 |
| 0x0013 | Byte 5 | Byte 5 |
| 0x0014 | Byte 6 | - |
| 0x0015 | Byte 7 | Byte 6 |
| 0x0016 | Byte 8 | Byte 7 |
| 0x0017 | Byte 9 | Byte 8 |

## Tested Microcontroller

* Atmega328pb - 10 bytes
* Atmega328p - 9 bytes
* Atmega2560 - 9 bytes
* Attiny85 - 9 bytes

# Atmel SAM ARM Microcontroller

Atmel SAM3X8E is used in Arduino Due. 

The Unique Identifier is located in the first 128 bits of the Flash memory mapping. So, at the address 0x400000-0x400003.

"Each device integrates its own 128-bit unique identifier. These bits are factory configured and cannot be changed by the user. The ERASE pin has no effect on the unique identifier." [Datasheet Section 7.2.3.7](http://ww1.microchip.com/downloads/en/devicedoc/atmel-11057-32-bit-cortex-m3-microcontroller-sam3x-sam3a_datasheet.pdf)

## Tested Microcontroller

* Atmel SAM3X8E ARM Cortex-M3 - 16 bytes

# Atmel SAMD ARM Microcontroller

Atmel SAMD21 is used in Arduino Zero / Arduino M0. 

Each device has a unique 128-bit serial number which is a concatenation of four 32-bit words contained at the following addresses: 

* Word 0: 0x0080A00C 
* Word 1: 0x0080A040
* Word 2: 0x0080A044
* Word 3: 0x0080A048

The uniqueness of the serial number is guaranteed only when using all 128 bits. [Datasheet Section 9.3.3](https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/Atmel-42181-SAM-D21_Datasheet.pdf)

## Tested Microcontroller

* Atmel SAMD21 ARM Cortex-M0 - 16 bytes

# STM32 Microcontroller

STM32 32-bit Arm Cortex MCUs has a unique 96-bit serial number which is a concatenation of three 32-bit words, the address is different depending on the microcontroller. 

The [Arduino Core STM32](https://github.com/stm32duino/Arduino_Core_STM32) has the functions HAL_GetUIDw0(), HAL_GetUIDw1() and HAL_GetUIDw2() for 96-bit UID.

## Tested Microcontroller

* STM32F103C8 (BluePill Board) - 12 bytes
* STM32L073RZ (Nucleo L073RZ) - 12 bytes

# Espressif ESP Microcontroller

ESP microcontroller has basically two versions, ESP8266 and ESP32, each one has a specific function to request the chip id. <br/>

* ESP8266 - ESP.getChipId() - 4 bytes
* ESP32 - ESP.getEfuseMac() - 6 bytes

| UniqueID | ESP8266 | ESP32 |
| :-------: | :------: | :------:|
| Byte 0| Byte 0 | Byte 5 |
| Byte 1| Byte 1 | Byte 4 |
| Byte 2| Byte 2 | Byte 3 |
| Byte 3| Byte 3 | Byte 2 |
| Byte 4| - | Byte 1 |
| Byte 5| - | Byte 0 |

To make the variable UniqueID8 to work propably the library uses the default bytes to 0x00. <br/>

| UniqueID8 | ESP8266 | ESP32 |
| :-------: | :------: | :------:|
| Byte 0| 0x00 | 0x00 |
| Byte 1| 0x00 | 0x00 |
| Byte 2| 0x00 | Byte 5 |
| Byte 3| 0x00 | Byte 4 |
| Byte 4| Byte 0 | Byte 3 |
| Byte 5| Byte 1 | Byte 2 |
| Byte 6| Byte 2 | Byte 1 |
| Byte 7| Byte 3 | Byte 0 |

## Tested Microcontroller

* ESP8266 - 4 bytes
* ESP32 - 6 bytes

# Installation

* Install the library by [Using the Library Manager](https://www.arduino.cc/en/Guide/Libraries#toc3)
* **OR** by [Importing the .zip library](https://www.arduino.cc/en/Guide/Libraries#toc4) using either the [master](https://github.com/ricaun/ArduinoUniqueID/archive/1.0.9.zip) or one of the [releases](https://github.com/ricaun/ArduinoUniqueID/releases) ZIP files.

## Examples

The library comes with [examples](examples). After installing the library you need to restart the Arduino IDE before they can be found under **File > Examples > ArduinoUniqueID**.

---

# Reference

## Include Library

```c
#include <ArduinoUniqueID.h>
```

### Variable: UniqueID & UniqueIDsize

UniqueID has UniqueIDsize bytes array of the Unique Serial ID.

```c
for(size_t i = 0; i < UniqueIDsize; i++)
  Serial.println(UniqueID[i], HEX);
```

### Method: UniqueIDdump

Print the hexadecimal bytes of the Unique Serial ID on the Stream.

```c
void UniqueIDdump(Stream);
```

### Variable: UniqueID8

UniqueID8 has the last 8 bytes array of the Unique Serial ID.

```c
for(size_t i = 0; i < 8; i++)
  Serial.println(UniqueID8[i], HEX);
```

### Method: UniqueID8dump

Print the last eight hexadecimal bytes of the Unique Serial ID on the Stream.

```c
void UniqueID8dump(Stream);
```

Do you like this library? Please [star this project on GitHub](https://github.com/ricaun/ArduinoUniqueID/stargazers)!
