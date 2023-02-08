#include <Arduino.h>
#include <EEPROM.h>
#include <LowPower.h>
#include <VoltageReference.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <credentials.h>

// CC1101
#define CC_FREQ 868.32
#define CC_POWER 10
#define GD0 2

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

// counter
#ifdef DEBUG
uint16_t msgCounter = 1;
#endif

int getUniqueID();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);

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
  Serial.println(F(__TIMESTAMP__));

// Start CC1101
#ifdef VERBOSE
  Serial.print(F("[CC1101] Initializing... "));
#endif
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
#ifdef VERBOSE
    Serial.println(F("OK"));
#endif
    ELECHOUSE_cc1101.Init();           // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setGDO0(GD0);     // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
    ELECHOUSE_cc1101.setCCMode(1);     // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);  // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setSyncMode(2);   // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    // ELECHOUSE_cc1101.setPA(CC_POWER);  // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setCrc(1); // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
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
  pinMode(SENSOR_PIN_PIR, INPUT);
  sleepDeep();
#endif
}

void loop()
{

  // prepare msg string
  String str[3];
  str[0] = ",N:";
  str[0] += String(getUniqueID(), HEX);
#ifdef DEBUG
  str[0] += ",I:";
  str[0] += msgCounter;
#endif

#ifdef SENSOR_TYPE_pir
  if (digitalRead(SENSOR_PIN_PIR) == HIGH)
  {
    boolean pir_state = true;
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_pir);
    Serial.print(": ");
    Serial.println(pir_state);
#endif
    if (pir_state)
    {
      str[0] += ",M1:";
      str[0] += int(pir_state);
    }
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
    Serial.println("%, ");
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
  float bmp280_altitude = bmp280.readAltitude(1013.25);
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_bmp280);
    Serial.print(": ");
    Serial.print(bmp280_temperature);
    Serial.print("C, ");
    Serial.print(bmp280_pressure);
    Serial.print("Pa, ");
    Serial.print(bmp280_altitude);
    Serial.println("m");
#endif
    str[0] += ",T3:";
    str[0] += int(round(bmp280_temperature * 10));
    str[0] += ",P3:";
    str[0] += int(round(bmp280_pressure) / 10);
    str[0] += ",A3:";
    str[0] += int(round(bmp280_altitude));
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
  float bme680_altitude = bme680.readAltitude(1013.25);
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
    Serial.print(bme680_altitude);
    Serial.print("m, ");
    Serial.print(bme680_gas);
    Serial.println("KOhms");
#endif
    str[0] += ",T4:";
    str[0] += int(round(bme680_temperature * 10));
    str[0] += ",H4:";
    str[0] += int(round(bme680_humidity * 10));
    str[0] += ",P4:";
    str[0] += int(round(bme680_pressure * 10));
    str[0] += ",A4:";
    str[0] += int(round(bme680_altitude));
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

  int str_diff = 60 - str[0].length();

  if (str_diff < 0)
  {
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
    str[2] = str[0].substring(0, str[0].indexOf(",", str[0].indexOf(",") + 1)) + ',' + str[0].substring(str_middle + 1);
    str[0] = "";
  }

  for (uint8_t i = 0; i < 3; i++)
  {
    if (str[i].length() != 0)
    {
      /* max length is 62 because of Arduino String last byte 00
         but 62 not good better use 61
         String length here to 60, thus packet length 61 */
         // 56
      for (uint8_t j = str[i].length(); j < 55; j++)
      {
        str[i] += ".";
      }

      // Z: = +2
      str[i] = "Z:" + String(str[i].length() + String(str[i].length()).length() + 2) + str[i];
#ifdef DEBUG
      Serial.print(F("> DEBUG: "));
      Serial.println(str[i]);
#endif
      // Convert String to byte array
      // String to byte +1 string nul terminator 00 and overwrite
      byte byteArr[str[i].length() + 1];
      str[i].getBytes(byteArr, sizeof(byteArr));
      byteArr[sizeof(byteArr) / sizeof(byteArr[0]) - 1] = '0';
#ifdef VERBOSE
      Serial.print(F("> Packet Length: "));
      Serial.println(sizeof(byteArr) / sizeof(byteArr[0])); // +1
#endif
#ifdef VERBOSE
      Serial.println(F("[CC1101] Transmitting packet... "));
#endif
      int cc_tr_state = 1;
      ELECHOUSE_cc1101.SendData(byteArr, sizeof(byteArr) / sizeof(byteArr[0]));
      // ELECHOUSE_cc1101.SendData("Z:62,N:22,I:1,T2:186,T4:197,H4:350,P4:9527,A4:517,Q4:621,V1:34");
      if (cc_tr_state)
      {
#ifdef VERBOSE
        Serial.println(F("[CC1101] Transmitting packet... OK"));
#endif
        Serial.println(str[i]);
#ifdef DEBUG
        for (uint8_t k = 0; k < sizeof(byteArr); k++)
        {
          printHex(byteArr[k]);
        }
        Serial.println("");
#endif
      }
      else
      {
#ifdef VERBOSE
        // some other error occurred
        Serial.print(F("[CC1101] Transmitting packet... ERR, code "));
        Serial.println(cc_tr_state);
#endif
        sleepDeep(DS_D);
      }
      // delay multi send
      if (str[i] != 0)
      {
        delay(DS_D);
      } else {
        delay(DS_D);
      }
    }
  }
  sleepDeep(DS_L);

#ifdef DEBUG
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
#ifdef VERBOSE
    Serial.print("EEPROM SN: ");
    Serial.print(uid);
    Serial.print(" -> HEX: ");
    Serial.println(String(serialNumber, HEX));
#endif
  }
#ifdef VERBOSE
  else
  {
    Serial.println("EEPROM SN: ERROR EMPTY USING DEFAULT");
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
#ifdef VERBOSE
  Serial.print("Deep Sleep: ");
  if (t < 1)
  {
    Serial.println("forever");
  }
  else if (t > 254)
  {
    t = 1;
    m = 8;
    Serial.println("8s");
  }
  else
  {
    Serial.print(t);
    Serial.println("min");
  }
#endif
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

#ifdef DEBUG
void printHex(uint8_t num)
{
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}
#endif
