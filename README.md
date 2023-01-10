# arduino-cc1101-transmitter

An Arduino CC1101 transmitter

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

The arduino-cc1101-transmitter emits a 61 characters string with an unique id.

```
M,N:87,T1:29,H1:817,T2:25,T3:42,P1:9260,A1:753,V1:38,E:00000

M = acknowledge character
N = node number
T = temperature
H = humidity
P = pressure
A = altitude
V = voltage
M = motion
S = switch
E = string filler until 60 chars

, = delimiter

X1 = si7021
X2 = ds18b20
X3 = bmp280
X4 = bme680
```

### Supported sensors 

* Si7021 (Temperature & humidity)
* DS18B20 (Temperature)
* BMP280 (Temperature, pressure & altitude)
* BME680 (Temperature, humidity, pressure, altitude & gas)
* PIR (Motion)
* SWITCH (Button, reed & switch)

### Built With

* [VSCode](https://github.com/microsoft/vscode)
* [PlatformIO](https://platformio.org/)
* [RadioLib](https://github.com/jgromes/RadioLib)

## Getting Started

### Prerequisites

* An Arduino with a CC1101 module as a receiver
* An Arduino with a CC1101 module as a transmitter
* VSCode
* PlatformIO

### Installation

```sh
git clone https://github.com/13/arduino-cc1101-transmitter.git
```

## Usage

* Edit `config-sample.h` and save as `config.h`
* Edit `main.cpp` to your needs
* Edit `platformio.ini` to your needs
* Build & upload to your Arduino

## Roadmap

- [ ] ...

## Release History

* 1.0.0
    * Initial release

## Contact

* **13** - *Initial work* - [13](https://github.com/13)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Thank you
