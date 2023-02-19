#include <Arduino.h>
#include <EEPROM.h>
#include <LowPower.h>
#include <VoltageReference.h>
#include <RadioLib.h>
#include <credentials.h>

// Edit credentials.h

// cc1101
CC1101 radio = new Module(10, GD0, RADIOLIB_NC, 3);

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
boolean wakeup_state = false;

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
#ifdef VERBOSE
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
  Serial.println(GIT_VERSION);
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
  int cc_state = radio.begin(CC_FREQ);
  if (cc_state == RADIOLIB_ERR_NONE)
  {
#ifdef VERBOSE
    Serial.println(F("OK"));
#endif
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("ERR "));
    Serial.println(cc_state);
#endif
    sleepDeep(DS_S);
  }
  // Start CC1101
#ifdef VERBOSE
  Serial.print(F("[CC1101]: Enabling Address Filtering... "));
#endif
  int cc_af_state = radio.setNodeAddress(0x01, 1);
  if (cc_af_state == RADIOLIB_ERR_NONE)
  {
#ifdef VERBOSE
    Serial.println(F("OK"));
#endif
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("ERR "));
    Serial.println(cc_af_state);
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
  if (wakeup_state)
  {
    Serial.println("Wakeup...");
  }
  else
  {
    wakeup_state = true;
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
      str[i].getBytes(byteArr, str[i].length());
      // byteArr[sizeof(byteArr) / sizeof(byteArr[0]) - 1] = '\0'; // 0 \0

      for (uint8_t i = 0; i < sizeof(byteArr); i++)
      {
        Serial.print(byteArr[i], HEX);
      }
      Serial.println();

      for (uint8_t i = 0; i < sizeof(byteArr); i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.println();

      int state = radio.transmit(byteArr, sizeof(byteArr));
#endif

#ifdef SEND_CHAR
      // Transmit char format
      char charArr[str[i].length() + 1]; // + 1
      str[i].toCharArray(charArr, str[i].length() + 1); // + 1
      // charArr[sizeof(charArr) / sizeof(charArr[0]) - 1] = '\0'; // 0 \0

      for (uint8_t i = 0; i < sizeof(charArr); i++)
      {
        Serial.print(charArr[i]);
      }
      Serial.println();

      int state = radio.transmit(charArr);
#endif
      if (state == RADIOLIB_ERR_NONE)
      {
#ifdef VERBOSE
        Serial.println(F("[CC1101]: Transmitting packet... OK"));
        Serial.print(F("> Packet Length: "));
#ifdef SEND_CHAR
        Serial.println(strlen(charArr));
#endif
#ifdef SEND_BYTE
        Serial.println(sizeof(byteArr) / sizeof(byteArr[0]));
#endif
        Serial.println(F("OK"));
#endif
        Serial.println(str[i]);
      }
      // delay multi send
      if (strCount > 1)
      {
        delay(CC_DELAY);
      }
    }
    else
    {
      Serial.println(F("ERR"));
    }
  }

#ifdef PACKET_COUNT
  if (msgCounter < 65534)
  {
    msgCounter++;
  }
  else
  {
    msgCounter = 0;
  }
#endif

#ifdef SENSOR_TYPE_pir
  sleepDeep();
#else
  sleepDeep(DS_L);
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
  // digitalWrite(13, LOW); // Fix turn LED off
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