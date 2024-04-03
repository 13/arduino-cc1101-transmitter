#include <Arduino.h>
#include <EEPROM.h>
#include <LowPower.h>
#include <VoltageReference.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <version.h>
#include <credentials.h>
#ifdef USE_CRYPTO
#include <Crypto.h>
#include <AES.h>
#endif

// Edit credentials.h

#ifdef SENSOR_TYPE_si7021
#include <Adafruit_Si7021.h>
Adafruit_Si7021 si = Adafruit_Si7021();
#endif
#ifdef SENSOR_TYPE_ds18b20
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(SENSOR_PIN_OW);
DallasTemperature ds18b20(&oneWire);
#endif
#if defined(SENSOR_TYPE_bmp280) || defined(SENSOR_TYPE_bme680)
#include <Wire.h>
#endif
#ifdef SENSOR_TYPE_bmp280
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp280;
#endif
#ifdef SENSOR_TYPE_bme680
#include <Adafruit_BME680.h>
Adafruit_BME680 bme680 = Adafruit_BME680();
#endif

// cc1101
boolean cc1101_state = true;
// voltage
VoltageReference vRef;
// wakeup
boolean wakeup_state = false;

#ifdef SENSOR_TYPE_button
boolean buttonChanged = false;
boolean buttonState = LOW;
#endif

#ifdef SENSOR_TYPE_pir
boolean motionDetected = false;
boolean pirState = LOW;
#endif

#ifdef SENSOR_TYPE_switch
boolean switchChanged = true;
boolean switchState = LOW;
#endif

// counter
#ifdef VERBOSE_PC
uint16_t msgCounter = 1;
#endif
// random packet id
uint16_t pid = 0;

#ifdef USE_CRYPTO
byte key[16];
byte cipher[61];
byte decryptedText[61];
AES128 aes128;

void hexStringToByteArray(const char *hexString, byte *byteArray, size_t byteArrayLength)
{
  for (size_t i = 0; i < byteArrayLength; i++)
  {
    byteArray[i] = strtoul(hexString + 2 * i, NULL, 16);
  }
}
#endif

// supplementary functions
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
    Serial.print("EEPROM: SN ");
    Serial.print(uid);
    Serial.print(" -> HEX ");
    Serial.println(String(serialNumber, HEX));
#endif
  }
  else
  {
    randomSeed(analogRead(0));
    long randNumber = random(256, 4096);
    EEPROM.put(address, randNumber);
    delay(100);
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
#ifdef DEBUG
    Serial.print("EEPROM: GENERATING SN ");
    Serial.print(uid);
    Serial.print(" -> HEX ");
    Serial.println(String(serialNumber, HEX));
#endif
  }

  return uid;
}

int setUniqueID()
{
  int uid = 0;
  // read EEPROM serial number
  int address = 13;
  int serialNumber;

  randomSeed(analogRead(0));
  long randNumber = random(256, 4096);
  EEPROM.put(address, randNumber);
  delay(100);
  EEPROM.get(address, serialNumber);
  uid = serialNumber;
#ifdef VERBOSE
  Serial.print("> EEPROM: GENERATING SN ");
  Serial.print(uid);
  Serial.print(" -> HEX ");
  Serial.println(String(serialNumber, HEX));
#endif

  return uid;
}

/*
   sleep
   0,empty = forever
   1-7 = minutes
   8+ = seconds
*/
void sleepDeep(uint8_t t)
{
  Serial.print("SleepDeep ");
  if (t < 1)
  {
    Serial.println("forever...");
  }
  else if (t < 8)
  {
    t = t * 60;
    Serial.print(t);
    Serial.println("min...");
  }
  else
  {
    Serial.print(t);
    Serial.println("s...");
  }
  ELECHOUSE_cc1101.goSleep();
  delay(DS_D);
  if (t > 0)
  {
    for (int8_t i = 0; i < (t / 8); i++)
    {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
  }
  else
  {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}
void sleepDeep()
{
  sleepDeep(0);
}

#ifdef SENSOR_TYPE_button
void wakeInterruptButton()
{
  Serial.println("Wakeup interrupt button...");
  buttonState = digitalRead(SENSOR_PIN_BUTTON);
  buttonChanged = true;
}
#endif

#ifdef SENSOR_TYPE_pir
void wakeInterruptPir()
{
  Serial.println("Wakeup interrupt pir...");
  pirState = digitalRead(SENSOR_PIN_PIR);
  motionDetected = true;
}
#endif

#ifdef SENSOR_TYPE_switch
void wakeInterruptSwitch()
{
  Serial.println("Wakeup interrupt switch...");
  switchState = digitalRead(SENSOR_PIN_SWITCH);
  switchChanged = true;
}
#endif

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
  Serial.println(VERSION);
#ifdef VERBOSE
  Serial.print(("> Mode: "));
  Serial.print(F("VERBOSE "));
#ifdef GD0
  Serial.print(F("GD0 "));
#endif
#ifdef SEND_CHAR
  Serial.print(F("CHAR "));
#endif
#ifdef SEND_BYTE
  Serial.print(F("BYTE "));
#endif
#ifdef USE_CRYPTO
  Serial.print(F("CRYPTO "));
#endif
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
  Serial.println();
#endif

  // print unique id
#ifdef GEN_UID
  setUniqueID();
#endif
  Serial.print(F("> Node: "));
  Serial.println(String(getUniqueID(), HEX));

  // Start CC1101
#ifdef VERBOSE
  Serial.print(F("cc1101: "));
#endif
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
#ifdef VERBOSE
    Serial.println(F("Detected"));
#endif
    ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
#ifdef GD0
    ELECHOUSE_cc1101.setGDO0(GD0);
#endif
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setModulation(0);
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);
    ELECHOUSE_cc1101.setPA(CC_POWER);
    ELECHOUSE_cc1101.setSyncMode(2);
    ELECHOUSE_cc1101.setCrc(1);
    ELECHOUSE_cc1101.setCRC_AF(1);
    // ELECHOUSE_cc1101.setAdrChk(1);
    // ELECHOUSE_cc1101.setAddr(0);
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("Not detected "));
    Serial.println(cc_state);
    cc1101_state = false;
#endif
    // sleepDeep(DS_S);
  }
  digitalWrite(13, LOW); // Fix turn LED off
  // voltage
  vRef.begin();

#ifdef USE_CRYPTO
  hexStringToByteArray(AES_KEY, key, 16);
  aes128.setKey(key, 16); // Setting Key for AES
#endif

  // Generate Random Seed
  randomSeed(analogRead(0));

#ifdef SENSOR_TYPE_si7021
  if (!si.begin())
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(": ");
    Serial.println("Not detected");
#endif
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
    Serial.println(" Not detected");
#endif
  }
#endif

// button
#ifdef SENSOR_TYPE_button
  pinMode(SENSOR_PIN_BUTTON, INPUT_PULLUP);
  Serial.print(SENSOR_TYPE_button);
  Serial.print(": ");
  Serial.println(digitalRead(SENSOR_PIN_BUTTON) == HIGH ? "HIGH" : "LOW");
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_BUTTON), wakeInterruptButton, RISING);
#endif

// pir
#ifdef SENSOR_TYPE_pir
  pinMode(SENSOR_PIN_PIR, INPUT);
  Serial.print(SENSOR_TYPE_pir);
  Serial.print(": ");
  Serial.println(digitalRead(SENSOR_PIN_PIR) == HIGH ? "HIGH" : "LOW");
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_PIR), wakeInterruptPir, RISING);
  sleepDeep();
#endif

// switch
#ifdef SENSOR_TYPE_switch
  pinMode(SENSOR_PIN_SWITCH, INPUT_PULLUP);
#ifdef DEBUG
  Serial.print(SENSOR_TYPE_switch);
  Serial.print(": ");
  Serial.println(digitalRead(SENSOR_PIN_SWITCH) == HIGH ? "HIGH" : "LOW");
#endif
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_SWITCH), wakeInterruptSwitch, CHANGE);
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
#ifdef VERBOSE_PC
  str[0] += ",C:";
  str[0] += msgCounter;
#endif

  // random packet id
  pid = random(99) + 1;
  str[0] += ",X:";
  str[0] += pid;

#ifdef SENSOR_TYPE_button
  if (buttonDetected)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_button);
    Serial.print(": ");
    Serial.println(buttonState == HIGH ? "HIGH" : "LOW");
#endif
    str[0] += ",B1:";
    str[0] += int(buttonState == HIGH ? 1 : 0);
    buttonDetected = false;
  }
#endif

#ifdef SENSOR_TYPE_pir
  if (motionDetected)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_pir);
    Serial.print(": ");
    Serial.println(pirState == HIGH ? "HIGH" : "LOW");
#endif
    str[0] += ",M1:";
    str[0] += int(pirState == HIGH ? 1 : 0);
    motionDetected = false;
  }
#endif

#ifdef SENSOR_TYPE_switch
  if (switchChanged)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_switch);
    Serial.print(": ");
    Serial.println(switchState == HIGH ? "HIGH" : "LOW");
#endif
    str[0] += ",S1:";
    str[0] += int(switchState == HIGH ? 0 : 1); // REVERSE LOGIC
    switchChanged = false;
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
    Serial.println("Not detected");
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

#ifdef MQTT_RETAINED_DISABLED
  str[0] += ",R:0";
#endif

#ifdef VERBOSE_FW
  // Firmware version
  str[0] += ",F:";
  str[0] += String(VERSIONTAG);
#endif

#ifdef DEBUG
  Serial.print(F("> DEBUG: String Length "));
  Serial.println(str[0].length());
#endif

  // Split packets
  // maxPacketSize (61) - leadingTupleLength ('Z:44')
  // 61 - 4 = [57]
  // With Termination char ";"
  // 61 - 5 = [56]
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
#ifdef VERBOSE_PC
    str[2] = str[0].substring(0, str[0].indexOf(",", str[0].indexOf(",", str[0].indexOf(",") + 1) + 1)) +
             ',' + str[0].substring(str_middle + 1);
#else
    str[2] = str[0].substring(0, str[0].indexOf(",", str[0].indexOf(",") + 1)) +
             ',' + str[0].substring(str_middle + 1);
#endif
    str[0] = "";
  }

  if (cc1101_state)
  {

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
// Transmission
#ifdef SEND_BYTE
        // Transmit byte format
        byte byteArr[str[i].length() + 1];
        str[i].getBytes(byteArr, str[i].length() + 1);
        byteArr[sizeof(byteArr) / sizeof(byteArr[0]) - 1] = '.'; // overwrite null byte terminator

#ifdef VERBOSE
#ifdef DEBUG
        Serial.println(F("cc1101: Transmitting packet... "));
#else
        Serial.print(F("cc1101: Transmitting packet... "));
#endif
#endif

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

#ifdef USE_CRYPTO
        Serial.println(F("crypto: Encrypting packet... "));
        // Encrypt the byte array in blocks of 16 bytes
        int blockCount = str[i].length() / 16 + 1;
        for (int i = 0; i < blockCount; ++i)
        {
          aes128.encryptBlock(&cipher[i * 16], &charArr[i * 16]);
        }
#ifdef DEBUG
        Serial.print("crypto: ");
        for (int j = 0; j < sizeof(cipher); j++)
        {
          Serial.write(cipher[j]);
        }
        Serial.println();
#endif
#endif

#ifdef VERBOSE
#ifdef DEBUG
        Serial.println(F("cc1101: Transmitting packet... "));
#else
        Serial.print(F("cc1101: Transmitting packet... "));
#endif
#endif

#ifdef GD0
#ifdef USE_CRYPTO
        ELECHOUSE_cc1101.SendData(cipher);
#else
        ELECHOUSE_cc1101.SendData(charArr);
#endif
#else
        ELECHOUSE_cc1101.SendData(charArr, CC_DELAY);

#endif
#endif

#ifdef VERBOSE
#ifdef DEBUG
        Serial.println(F("cc1101: Transmitting packet... OK"));
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
  }
  else
  {
    Serial.println(F("cc1101: Not transmitting"));
  }
#ifdef VERBOSE_PC
  if (msgCounter < UINT16_MAX)
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
#elif defined(SENSOR_TYPE_switch)
  sleepDeep();
#elif defined(SENSOR_TYPE_button)
  sleepDeep();
#else
  sleepDeep(DS_L);
#endif
}
