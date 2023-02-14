#include <Arduino.h>
#include <EEPROM.h>
#include <LowPower.h>
#include <VoltageReference.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <credentials.h>

// Edit credentials.h

#ifdef SENSOR_TYPE_si7021
#include <Adafruit_Si7021.h>
#endif
#ifdef SENSOR_TYPE_ds18b20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#if defined(SENSOR_TYPE_bmp280) || defined(SENSOR_TYPE_bme680)
#include <Wire.h>
#endif
#ifdef SENSOR_TYPE_bmp280
#include <Adafruit_BMP280.h>
#endif
#ifdef SENSOR_TYPE_bme680
#include <Adafruit_BME680.h>
#endif

// voltage
VoltageReference vRef;
// wakeup
boolean wakeup = false;

#ifdef SENSOR_TYPE_si7021
Adafruit_Si7021 si = Adafruit_Si7021();
#endif
#ifdef SENSOR_TYPE_ds18b20
OneWire oneWire(SENSOR_PIN_OW);
DallasTemperature ds18b20(&oneWire);
#endif
#ifdef SENSOR_TYPE_bmp280
Adafruit_BMP280 bmp280;
#endif
#ifdef SENSOR_TYPE_bme680
Adafruit_BME680 bme680 = Adafruit_BME680();
#endif
#ifdef SENSOR_TYPE_pir
boolean pir_state = false;
#endif

// counter
#ifdef DEBUG
#define PACKET_COUNT
#endif
#ifdef PACKET_COUNT
uint16_t msgCounter = 1;
#endif

int getUniqueID();
void sleepDeep();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);
void wakeInterrupt();

void setup()
{
  Serial.begin(9600);
  delay(10);
#ifdef VERBOSE
  delay(20);
#endif
  // Start Boot
  Serial.println(F("> "));
  Serial.println(F("> "));
  Serial.print(F("> Booting... Compiled: "));
  Serial.print(GIT_VERSION);
  Serial.print(" ")
  Serial.println(F(__TIMESTAMP__));
#ifdef VERBOSE
  Serial.print(("> Mode: "));
  Serial.print(F("VERBOSE "));
#endif
#ifdef DEBUG
  Serial.print(F("DEBUG "));
#endif
#ifdef GD0
  Serial.print(F("GD0"));
#endif

  // Start CC1101
#ifdef VERBOSE
  Serial.println();
  Serial.print(F("[CC1101]: Initializing... "));
#endif
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
#ifdef VERBOSE
    Serial.println(F("OK"));
#endif
    ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
#ifdef GD0
    ELECHOUSE_cc1101.setGDO0(GD0); // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
#endif
    ELECHOUSE_cc1101.setModulation(0);      // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);       // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setDeviation(47.60);   // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
    ELECHOUSE_cc1101.setChannel(0);         // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
    ELECHOUSE_cc1101.setChsp(199.95);       // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
    ELECHOUSE_cc1101.setRxBW(812.50);       // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
    ELECHOUSE_cc1101.setDRate(99.97);       // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
    ELECHOUSE_cc1101.setPA(CC_POWER);       // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setSyncMode(2);        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setSyncWord(211, 145); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
    ELECHOUSE_cc1101.setAdrChk(0);          // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    ELECHOUSE_cc1101.setAddr(0);            // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
    ELECHOUSE_cc1101.setWhiteData(0);       // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
    ELECHOUSE_cc1101.setPktFormat(0);       // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
    ELECHOUSE_cc1101.setLengthConfig(1);    // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
    ELECHOUSE_cc1101.setPacketLength(0);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
    ELECHOUSE_cc1101.setCrc(1);             // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    ELECHOUSE_cc1101.setCRC_AF(1);          // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    ELECHOUSE_cc1101.setDcFilterOff(0);     // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
    ELECHOUSE_cc1101.setManchester(0);      // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setFEC(0);             // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setPRE(0);             // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
    ELECHOUSE_cc1101.setPQT(0);             // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
    ELECHOUSE_cc1101.setAppendStatus(0);    // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("ERR "));
    Serial.println(cc_state);
#endif
    sleepDeep(DS_S);
  }

  // voltage
  vRef.begin();

#ifdef SENSOR_TYPE_si7021
  if (!si.begin())
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(": ");
    Serial.println(" ERROR -1");
#endif
    sleepDeep(DS_S);
  }
#endif

#ifdef SENSOR_TYPE_ds18b20
  ds18b20.begin();
#endif

#ifdef SENSOR_TYPE_bmp280
  bmp280.begin(0x76, 0x60); // fix GY-B11 module
#endif

#ifdef SENSOR_TYPE_bme680
  if (!bme680.begin())
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_bme680);
    Serial.print(": ");
    Serial.println(" ERROR -1");
#endif
    sleepDeep(DS_S);
  }
#endif

// pir
#ifdef SENSOR_TYPE_pir
  Serial.print(SENSOR_TYPE_pir);
  Serial.print(": ");
  Serial.println(" OK");
  pinMode(SENSOR_PIN_PIR, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_PIR), wakeInterrupt, RISING);
  sleepDeep();
#endif
}

void loop()
{
  if (wakeup)
  {
    Serial.println("Wakeup...");
  }
  else
  {
    wakeup = true;
  }
  // prepare msg string
  String str[3];
  str[0] = ",N:";
  str[0] += String(getUniqueID(), HEX);
#ifdef PACKET_COUNT
  str[0] += ",I:";
  str[0] += msgCounter;
#endif

#ifdef SENSOR_TYPE_pir
#ifdef VERBOSE
  Serial.print(SENSOR_TYPE_pir);
  Serial.print(": ");
  Serial.println(pir_state);
#endif
  if (pir_state)
  {
    str[0] += ",M1:";
    str[0] += int(pir_state);
    pir_state = false;
  }
#endif

#ifdef SENSOR_TYPE_si7021
  float si_temperature = si.readTemperature();
  float si_humidity = si.readHumidity();
  if (!isnan(si_temperature))
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(": ");
    Serial.print(si_temperature);
    Serial.print("C, ");
    Serial.print(si_humidity);
    Serial.println("%");
#endif
    str[0] += ",T1:";
    str[0] += int(round(si_temperature * 10));
    str[0] += ",H1:";
    str[0] += int(round(si_humidity * 10));
  }

#endif
#ifdef SENSOR_TYPE_ds18b20
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
#ifdef VERBOSE
  Serial.print(SENSOR_TYPE_ds18b20);
  Serial.print(": ");
#endif
  if (ds_temperature != DEVICE_DISCONNECTED_C)
  {
#ifdef VERBOSE
    Serial.print(ds_temperature);
    Serial.println("C");
#endif
    str[0] += ",T2:";
    str[0] += int(round(ds_temperature * 10));
  }
#ifdef VERBOSE
  else
  {
    Serial.println("ERR");
  }
#endif
#endif
#ifdef SENSOR_TYPE_bmp280
  float bmp280_temperature = bmp280.readTemperature();
  float bmp280_pressure = bmp280.readPressure();
  float bmp280_temp_offset = 0;
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_bmp280);
    Serial.print(": ");
    Serial.print(bmp280_temperature - bmp280_temp_offset);
    Serial.print("C, ");
    Serial.print(bmp280_pressure);
    Serial.println("Pa");
#endif
    str[0] += ",T3:";
    str[0] += int(round((bmp280_temperature + bmp280_temp_offset) * 10));
    str[0] += ",P3:";
    str[0] += int(round(bmp280_pressure) / 10);
  }
#endif
#ifdef SENSOR_TYPE_bme680
  if (!bme680.performReading())
  {
#ifdef VERBOSE
    Serial.println("[BME680]: ERROR read!");
#endif
    sleepDeep(255);
  }
  float bme680_temperature = bme680.temperature;
  float bme680_humidity = bme680.humidity;
  float bme680_pressure = bme680.pressure / 100.0;
  float bme680_gas = bme680.gas_resistance / 1000.0;
  if (!isnan(bme680_temperature))
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_bme680);
    Serial.print(": ");
    Serial.print(bme680_temperature);
    Serial.print("C, ");
    Serial.print(bme680_humidity);
    Serial.print("%, ");
    Serial.print(bme680_pressure);
    Serial.print("hPa, ");
    Serial.print(bme680_gas);
    Serial.println("KOhms");
#endif
    str[0] += ",T4:";
    str[0] += int(round(bme680_temperature * 10));
    str[0] += ",H4:";
    str[0] += int(round(bme680_humidity * 10));
    str[0] += ",P4:";
    str[0] += int(round(bme680_pressure * 10));
    str[0] += ",Q4:";
    str[0] += int(round(bme680_gas));
  }

#endif
  float vcc = vRef.readVcc() / 1000.0;
#ifdef VERBOSE
  Serial.print("VCC: ");
  Serial.println(vcc);
#endif
  str[0] += ",V1:";
  str[0] += String(int(vcc)) + String((int)(vcc * 10) % 10);

#ifdef DEBUG
  Serial.print(F("> DEBUG: String Length "));
  Serial.println(str[0].length());
#endif

  // Split packets
  // maxPacketSize (61) - leadingTupleLength ('Z:44')
  // 61 - 4 = [57]
  int str_diff = 57 - str[0].length();
  int strCount = 1;

  if (str_diff < 0)
  {
    strCount = 3;
#ifdef VERBOSE
    Serial.print(F("> String too long: "));
    Serial.println(str[0].length());
    Serial.print(F("> String diff: "));
    Serial.println(str_diff);
    Serial.print(F("> String: "));
    Serial.println(str[0]);
#endif
    int str_middle = str[0].indexOf(",", str_middle + str[0].length() / 2);
    str[1] = str[0].substring(0, str_middle);
    // Increase the packet counter in the splitted second part
#ifdef PACKET_COUNT
    str[2] = str[0].substring(0, str[0].indexOf(",", str[0].indexOf(",", str[0].indexOf(",") + 1) + 1)) +
             ',' + str[0].substring(str_middle + 1);
#else
    str[2] = str[0].substring(0, str[0].indexOf(",", str[0].indexOf(",") + 1)) +
             ',' + str[0].substring(str_middle + 1);
#endif
    str[0] = "";
  }

  for (uint8_t i = 0; i < strCount; i++)
  {
    if (str[i].length() != 0)
    {
#ifdef FILL_STRING
      // Fill to String length 57, thus total is 61
      for (uint8_t j = str[i].length(); j < 56; j++)
      {
        str[i] += ".";
      }
#endif
      // Add leadingTuple 'Z:' with length of String
      str[i] = "Z:" + String(str[i].length() + String(str[i].length()).length() + 2) + str[i];
#ifdef DEBUG
      Serial.print(F("> DEBUG: "));
      Serial.println(str[i]);
#endif

#ifdef VERBOSE
#ifdef DEBUG
      Serial.println(F("[CC1101]: Transmitting packet... "));
#else
      Serial.print(F("[CC1101]: Transmitting packet... "));
#endif
#endif

#ifdef SEND_BYTE
      // Transmit byte format
      byte byteArr[str[i].length() + 1];
      str[i].getBytes(byteArr, str[i].length() + 1);
      byteArr[sizeof(byteArr) / sizeof(byteArr[0]) - 1] = '0';
#ifdef GD0
      ELECHOUSE_cc1101.SendData(byteArr, sizeof(byteArr) / sizeof(byteArr[0]));
#else
      ELECHOUSE_cc1101.SendData(byteArr, sizeof(byteArr) / sizeof(byteArr[0]), CC_DELAY);
#endif
#endif

#ifdef SEND_CHAR
      // Transmit char format
      char charArr[str[i].length() + 1];
      str[i].toCharArray(charArr, str[i].length() + 1);
#ifdef GD0
      ELECHOUSE_cc1101.SendData(charArr);
#else
      ELECHOUSE_cc1101.SendData(charArr, CC_DELAY);
#endif
#endif

#ifdef VERBOSE
#ifdef DEBUG
      Serial.println(F("[CC1101]: Transmitting packet... OK"));
      Serial.print(F("> Packet Length: "));
#ifdef SEND_CHAR
      Serial.println(strlen(charArr));
#endif
#ifdef SEND_BYTE
      Serial.println(sizeof(byteArr) / sizeof(byteArr[0]));
#endif
#else
      Serial.println(F("OK"));
#endif
#endif
      Serial.println(str[i]);
    }
    // delay multi send
    if (strCount > 1)
    {
      delay(CC_DELAY);
    }
  }
#ifdef SENSOR_TYPE_pir
  sleepDeep();
#else
  sleepDeep(DS_L);
#endif

#ifdef PACKET_COUNT
  msgCounter++;
#endif
}

// Last 4 digits of ChipID
int getUniqueID()
{
  int uid = 0;
  // read EEPROM serial number
  int address = 13;
  int serialNumber;
  if (EEPROM.read(address) != 255)
  {
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
#ifdef DEBUG
    Serial.print("[EEPROM]: SN ");
    Serial.print(uid);
    Serial.print(" -> HEX ");
    Serial.println(String(serialNumber, HEX));
#endif
  }
#ifdef DEBUG
  else
  {
    Serial.println("[EEPROM]: SN ERROR EMPTY USING DEFAULT");
  }
#endif
  return uid;
}

/*
   sleep
   1 - 254 minutes
   255 = 8 seconds
   0, empty = forever
*/
void sleepDeep()
{
  sleepDeep(0);
}
void sleepDeep(uint8_t t)
{
  uint8_t m = 60;
  // #ifdef VERBOSE
  Serial.print("Deep Sleep ");
  if (t < 1)
  {
    Serial.println("forever...");
  }
  else if (t > 254)
  {
    t = 1;
    m = 8;
    Serial.println("8s...");
  }
  else
  {
    Serial.print(t);
    Serial.println("min...");
  }
  // endif
  digitalWrite(13, LOW); // Fix turn LED off
  delay(DS_D);
  if (t > 0)
  {
    for (int8_t i = 0; i < (t * m / 8); i++)
    {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
  }
  else
  {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}

#ifdef SENSOR_TYPE_pir
void wakeInterrupt()
{
  Serial.println("Wakeup interrupt...");
  pir_state = true;
  loop();
}
#endif