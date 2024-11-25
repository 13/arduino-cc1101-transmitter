#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <LoRa.h>
#include <LowPower.h>
#include <VoltageReference.h>
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

// LoRa
boolean lo_state = true;
// voltage
VoltageReference vRef;
// wakeup
boolean wakeup_state = false;

#ifdef SENSOR_TYPE_button
boolean buttonDetected = false;
boolean buttonState = LOW;
#endif

#ifdef SENSOR_TYPE_pir
boolean motionDetected = false;
boolean pirState = LOW;
#endif

#ifdef SENSOR_TYPE_radar
boolean motionDetected = false;
boolean radarState = LOW;
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
  size_t hexStringLength = strlen(hexString);

  if (hexStringLength % 2 != 0 || hexStringLength / 2 != byteArrayLength)
  {
    Serial.print(F("CRYPTO: KEY INVALID"));
    // Invalid hex string length or mismatch with byte array length
    return;
  }
#ifdef DEBUG
  Serial.print(F("CRYPTO: KEY "));
#endif
  for (size_t i = 0; i < hexStringLength; i += 2)
  {
    // Convert each pair of hexadecimal characters to a byte
    sscanf(hexString + i, "%2hhx", &byteArray[i / 2]);
#ifdef DEBUG
    Serial.print(F("0x"));
    Serial.print(byteArray[i / 2], HEX);
    Serial.print(F(" "));
#endif
  }
#ifdef DEBUG
  Serial.println();
#endif
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
    Serial.print(F("EEPROM: SN "));
    Serial.print(uid);
    Serial.print(F(" -> HEX "));
    Serial.println(String(serialNumber, HEX));
#endif
  }
  else
  {
#ifdef CUSTOM_UID
#ifdef VERBOSE
  Serial.print(F("> EEPROM: CUSTOM SN "));
#endif
    long randNumber = strtol(CUSTOM_UID, NULL, 16);
#else
    randomSeed(analogRead(0));
    long randNumber = random(256, 4096);
#endif
    EEPROM.put(address, randNumber);
    delay(100);
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
#ifdef DEBUG
    Serial.print(F("EEPROM: GENERATING SN "));
    Serial.print(uid);
    Serial.print(F(" -> HEX "));
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

#ifdef CUSTOM_UID
#ifdef VERBOSE
  Serial.print(F("> EEPROM: CUSTOM SN "));
#endif
  long randNumber = strtol(CUSTOM_UID, NULL, 16);
#else
  randomSeed(analogRead(0));
  long randNumber = random(256, 4096);
#endif
  EEPROM.put(address, randNumber);
  delay(100);
  EEPROM.get(address, serialNumber);
  uid = serialNumber;
#ifdef VERBOSE
  Serial.print(F("> EEPROM: GENERATING SN "));
  Serial.print(uid);
  Serial.print(F(" -> HEX "));
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
  Serial.print(F("SleepDeep "));
  if (t < 1)
  {
    Serial.println(F("forever..."));
  }
  else if (t < 8)
  {
    t = t * 60;
    Serial.print(t);
    Serial.println(F("min..."));
  }
  else
  {
    Serial.print(t);
    Serial.println(F("s..."));
  }
  // LoRa sleep
  LoRa.sleep();
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
  Serial.println(F("Wakeup interrupt button..."));
  buttonState = digitalRead(SENSOR_PIN_BUTTON);
  buttonDetected = true;
}
#endif

#ifdef SENSOR_TYPE_pir
void wakeInterruptPir()
{
  Serial.println(F("Wakeup interrupt pir..."));
  pirState = digitalRead(SENSOR_PIN_PIR);
  motionDetected = true;
}
#endif

#ifdef SENSOR_TYPE_radar
void wakeInterruptRadar()
{
  Serial.println(F("Wakeup interrupt radar..."));
  radarState = digitalRead(SENSOR_PIN_RADAR);
  motionDetected = true;
}
#endif

#ifdef SENSOR_TYPE_switch
void wakeInterruptSwitch()
{
  Serial.println(F("Wakeup interrupt switch..."));
  switchState = digitalRead(SENSOR_PIN_SWITCH);
  switchChanged = true;
}
#endif

// Function definitions

void handleWakeup()
{
  if (wakeup_state)
  {
    Serial.println(F("Wakeup..."));
    wakeup_state = false;
  }
  else
  {
    wakeup_state = true;
  }
}

String prepareMessage()
{
  String msg = ",N:";
  msg += String(getUniqueID(), HEX);

#ifdef VERBOSE_PC
  msg += ",C:";
  msg += msgCounter;
#endif

  // Random packet id
  pid = random(99) + 1;
  msg += ",X:";
  msg += pid;

#ifdef MQTT_RETAINED_DISABLED
  msg += ",R:0";
#endif

  return msg;
}

#ifdef SENSOR_TYPE_pir
String handleSensorPir()
{
  String msg = "";
  if (motionDetected)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_pir);
    Serial.print(F(": "));
    Serial.println(pirState == HIGH ? "HIGH" : "LOW");
#endif
    msg += ",M1:";
    msg += int(pirState == HIGH ? 1 : 0);
    motionDetected = false;
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_radar
String handleSensorRadar()
{
  String msg = "";
  // motionDetected = true;
  if (motionDetected)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_radar);
    Serial.print(F(": "));
    Serial.println(radarState == HIGH ? "HIGH" : "LOW");
#endif
    msg += ",M1:";
    msg += int(radarState == HIGH ? 1 : 0);
    motionDetected = false;
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_button
String handleSensorButton()
{
  String msg = "";
  if (buttonDetected)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_button);
    Serial.print(F(": "));
    Serial.println(buttonState == HIGH ? "LOW" : "HIGH"); // REVERSE LOGIC
#endif
    msg += ",B1:";
    if (buttonDetected){
      msg += "1";
    } else {
      msg += int(buttonState == HIGH ? 0 : 1); // REVERSE LOGIC
    }
    buttonDetected = false;
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_switch
String handleSensorSwitch()
{
  String msg = "";
  if (switchChanged)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_switch);
    Serial.print(F(": "));
    Serial.println(switchState == HIGH ? "HIGH" : "LOW");
#endif
    msg += ",S1:";
    msg += int(switchState == HIGH ? 0 : 1); // REVERSE LOGIC
    switchChanged = false;
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_si7021
String handleSensorSi7021()
{
  String msg = "";
  float si_temperature = si.readTemperature();
  float si_humidity = si.readHumidity();
  if (!isnan(si_temperature))
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(F(": "));
    Serial.print(si_temperature);
    Serial.print(F("C, "));
    Serial.print(si_humidity);
    Serial.println(F("%"));
#endif
    msg += ",T1:" + String(int(round(si_temperature * 10)));
    msg += ",H1:" + String(int(round(si_humidity * 10)));
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_ds18b20
String handleSensorDs18b20()
{
  String msg = "";
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
  if (ds_temperature != DEVICE_DISCONNECTED_C)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_ds18b20);
    Serial.print(F(": "));
    Serial.println(ds_temperature);
#endif
    msg += ",T2:" + String(int(round(ds_temperature * 10)));
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_bmp280
String handleSensorBmp280()
{
  String msg = "";
  float bmp280_temperature = bmp280.readTemperature();
  float bmp280_pressure = bmp280.readPressure();
  if (!isnan(bmp280_pressure) && bmp280_pressure > 0)
  {
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_bmp280);
    Serial.print(F(": "));
    Serial.print(bmp280_temperature);
    Serial.print(F("C, "));
    Serial.print(bmp280_pressure);
    Serial.println(F("Pa"));
#endif
    msg += ",T3:" + String(int(round(bmp280_temperature * 10)));
    msg += ",P3:" + String(int(round(bmp280_pressure) / 10));
  }
  return msg;
}
#endif

#ifdef SENSOR_TYPE_bme680
String handleSensorBme680()
{
  String msg = "";
  if (!bme680.performReading())
  {
#ifdef VERBOSE
    Serial.println(F("[BME680]: ERROR read!"));
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
    Serial.print(F(": "));
    Serial.print(bme680_temperature);
    Serial.print(F("C, "));
    Serial.print(bme680_humidity);
    Serial.print(F("%, "));
    Serial.print(bme680_pressure);
    Serial.print(F("hPa, "));
    Serial.print(bme680_gas);
    Serial.println(F("KOhms"));
#endif
    msg += ",T4:" + String(int(round(bme680_temperature * 10)));
    msg += ",H4:" + String(int(round(bme680_humidity * 10)));
    msg += ",P4:" + String(int(round(bme680_pressure * 10)));
    msg += ",Q4:" + String(int(round(bme680_gas)));
  }
  return msg;
}
#endif

String handleVcc()
{
  float vcc = vRef.readVcc() / 1000.0;
#ifdef VERBOSE
  Serial.print(F("VCC: "));
  Serial.println(vcc);
#endif
  String msg = ",V1:" + String(int(vcc)) + String((int)(vcc * 10) % 10);
  return msg;
}

void transmitData(String *str, int strCount)
{
  if (lo_state)
  {
    for (uint8_t i = 0; i < strCount; i++)
    {
      if (str[i].length() != 0)
      {
        // Add leadingTuple 'Z:' with length of String
        str[i] = "Z:" + String(str[i].length() + String(str[i].length()).length() + 2) + str[i];

#ifdef DEBUG
        Serial.print(F("> DEBUG: "));
        Serial.println(str[i]);
#endif

        // Transmission logic
#ifdef SEND_CHAR
        // Transmit char format
        char charArr[str[i].length() + 1];
        str[i].toCharArray(charArr, str[i].length() + 1);

#ifdef USE_CRYPTO
        Serial.print(F("crypto: Encrypting... "));
        // Encrypt the byte array in blocks of 16 bytes
        int blockCount = str[i].length() / 16 + 1;
        for (int i = 0; i < blockCount; ++i)
        {
          aes128.encryptBlock(&cipher[i * 16], &charArr[i * 16]);
        }
        Serial.println(F("OK"));
#ifdef DEBUG
        Serial.print(F("crypto: "));
        for (int j = 0; j < sizeof(cipher); j++)
        {
          Serial.write(cipher[j]);
        }
        Serial.println();
#endif
#endif

        // Send packet
#ifdef VERBOSE
#ifdef DEBUG
        Serial.println(F("LoRa: Transmitting packet... "));
#else
        Serial.print(F("LoRa: Transmitting packet... "));
#endif
#endif
        LoRa.beginPacket();
#ifdef USE_CRYPTO
        LoRa.print((const char*)cipher);
#else
        LoRa.print(charArr);
#endif      
#endif

#ifdef VERBOSE
#ifdef DEBUG
        Serial.println(F("LoRa: Transmitting packet... OK"));
        Serial.print(F("> Packet Length: "));
#ifdef SEND_CHAR
        Serial.println(strlen(charArr));
#endif
#else
        LoRa.endPacket();
        Serial.println(F("OK"));
#endif
#endif
        Serial.println(str[i]);
      }
      // delay multi send
      if (strCount > 1)
      {
#ifdef VERBOSE
        Serial.print(F("Delay..."));
        Serial.print(LO_DELAY);
        Serial.println(F("ms"));
#endif
        delay(LO_DELAY);
      } else {
#ifdef VERBOSE
        Serial.print(F("Delay..."));
        Serial.print(LO_DELAY);
        Serial.println(F("ms"));
#endif
        delay(LO_DELAY);
      }
    }
  }
  else
  {
    Serial.println(F("LoRa: Not transmitting"));
  }
}

void sleepDevice()
{
#ifdef SENSOR_TYPE_pir
  sleepDeep();
#elif defined(SENSOR_TYPE_radar)
  sleepDeep();
#elif defined(SENSOR_TYPE_switch)
  sleepDeep();
#elif defined(SENSOR_TYPE_button)
  sleepDeep();
#else
  sleepDeep(DS_L);
#endif
}

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
  Serial.print(F("> Mode: "));
  Serial.print(F("VERBOSE "));
#ifdef GD0
  Serial.print(F("GD0 "));
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

  // Start LoRa
#ifdef VERBOSE
  Serial.print(F("LoRa: "));
#endif
  int lo_state = LoRa.begin(LO_FREQ);
  if (lo_state)
  {
#ifdef VERBOSE
    Serial.println(F("Detected"));
#endif
    // LoRa.setTxPower(LO_POWER);
    // LoRa.setSyncWord(0xF3);
    // LoRa.onTxDone(transmitDone);
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("Not detected "));
    Serial.println(lo_state);
    lo_state = false;
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
    Serial.print(F(": "));
    Serial.println(F("Not detected"));
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
    Serial.print(F(": "));
    Serial.println(F(" Not detected"));
#endif
  }
#endif

// button
#ifdef SENSOR_TYPE_button
  pinMode(SENSOR_PIN_BUTTON, INPUT_PULLUP);
  Serial.print(SENSOR_TYPE_button);
  Serial.print(F(": "));
  Serial.println(digitalRead(SENSOR_PIN_BUTTON) == HIGH ? "LOW" : "HIGH"); // REVERSE LOGIC
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_BUTTON), wakeInterruptButton, FALLING);
#endif

// pir
#ifdef SENSOR_TYPE_pir
  pinMode(SENSOR_PIN_PIR, INPUT);
  Serial.print(SENSOR_TYPE_pir);
  Serial.print(F(": "));
  Serial.println(digitalRead(SENSOR_PIN_PIR) == HIGH ? "HIGH" : "LOW");
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_PIR), wakeInterruptPir, RISING);
  sleepDeep();
#endif

// radar
#ifdef SENSOR_TYPE_radar
  pinMode(SENSOR_PIN_RADAR, INPUT);
  Serial.print(SENSOR_TYPE_radar);
  Serial.print(F(": "));
  Serial.println(digitalRead(SENSOR_PIN_RADAR) == HIGH ? "HIGH" : "LOW");
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_RADAR), wakeInterruptRadar, RISING);
#endif

// switch
#ifdef SENSOR_TYPE_switch
  pinMode(SENSOR_PIN_SWITCH, INPUT_PULLUP);
#ifdef DEBUG
  Serial.print(SENSOR_TYPE_switch);
  Serial.print(F(": "));
  Serial.println(digitalRead(SENSOR_PIN_SWITCH) == HIGH ? "HIGH" : "LOW");
#endif
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_SWITCH), wakeInterruptSwitch, CHANGE);
#endif
}

void loop()
{
  handleWakeup();

  // Prepare message string
  String str[3];
  str[0] = prepareMessage();

  // Append sensor readings
#ifdef SENSOR_TYPE_button
  str[0] += handleSensorButton();
#endif
#ifdef SENSOR_TYPE_pir
  str[0] += handleSensorPir();
#endif
#ifdef SENSOR_TYPE_radar
  str[0] += handleSensorRadar();
#endif
#ifdef SENSOR_TYPE_switch
  str[0] += handleSensorSwitch();
#endif
#ifdef SENSOR_TYPE_si7021
  str[0] += handleSensorSi7021();
#endif
#ifdef SENSOR_TYPE_ds18b20
  str[0] += handleSensorDs18b20();
#endif
#ifdef SENSOR_TYPE_bmp280
  str[0] += handleSensorBmp280();
#endif
#ifdef SENSOR_TYPE_bme680
  str[0] += handleSensorBme680();
#endif

  // Append voltage reading
  str[0] += handleVcc();

  // Transmission handling
  // Split packets
  // maxPacketSize (61) - leadingTupleLength ('Z:44')
  // 61 - 4 = [57]
  // With Termination char ";"
  // 61 - 5 = [56]
  int strCount = 1;
  if (str[0].length() > 57)
  {
    // Handle message splitting if too long
    strCount = 3;
    // Message splitting logic here...
    // str[1] and str[2] splitting as in the original code
#ifdef VERBOSE
    Serial.print(F("> String too long: "));
    Serial.println(str[0].length());
    Serial.print(F("> String diff: "));
    Serial.println(57 - str[0].length());
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

  transmitData(str, strCount);
  sleepDevice();
}
