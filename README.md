# arduino-lora-transmitter

An Arduino LoRa transmitter

## Contents

 * [About](#about)
   * [Built With](#built-with)
 * [Getting Started](#getting-started)
   * [Prerequisites](#prerequisites)
   * [Installation](#installation)
 * [Usage](#usage)
 * [Roadmap](#roadmap)
 * [Release History](#release-history)
 * [License](#license)
 * [Contact](#contact)
 * [Acknowledgements](#acknowledgements)

## About

The arduino-lora-transmitter emits a 61 characters string with an unique id.

```
Z:60,N:87,T1:29,H1:817,T2:25,T3:42,P1:9260,A1:753,V1:38

Z = acknowledge character & packet length
N = node number
X = random packet id
C = packet counter
R = retained (default 1)

T = temperature
H = humidity
P = pressure
A = altitude
V = voltage
M = motion
S = switch
B = button

, = delimiter

T1 = si7021
T2 = ds18b20
T3 = bmp280
T4 = bme680
```

### Supported sensors 

* Si7021 (Temperature & humidity)
* DS18B20 (Temperature)
* BMP280 (Temperature, pressure & altitude)
* BME680 (Temperature, humidity, pressure, altitude & gas)
* PIR (Motion)
* SWITCH (reed & switch)
* BUTTON 

### Built With

* [VSCode](https://github.com/microsoft/vscode)
* [PlatformIO](https://platformio.org/)
* [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)

## Getting Started

### Prerequisites

* An Arduino with a RFM95W/SX1276 module as a receiver
* An Arduino with a RFM95W/SX1276 module as a transmitter
* VSCode
* PlatformIO

### Hardware

| Arduino Pro Mini | RFM95W (SX1276) |
|---| ---|
| VCC | 3V |
| GND | GND |
| 11 | MOSI |
| 12 | MISO |
| 13 | SCK |
| 10 | NSS |
| 9 | RST |
| 2 | DIO0 |

### Installation

```sh
git clone https://github.com/13/arduino-lora-transmitter.git
```

## Usage

* Edit `config-sample.h` and save as `credentials.h`
* Edit `main.cpp` to your needs
* Edit `platformio.ini` to your needs
* Build & upload to your Arduino

## Roadmap

- [ ] ...

## Release History

* v10
    * Initial release

## Contact

* **13** - *Initial work* - [13](https://github.com/13)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Thank you
