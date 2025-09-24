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
boolean lora_status = true;
const byte senderAddress = 0x14;
const byte receiverAddress = 0x15;
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

// -----------------------
// TLV packet builder API
// -----------------------
enum TLVType : uint8_t {
  TLV_UID     = 0x01,
  TLV_PID     = 0x02,
  TLV_VER     = 0x03,
  TLV_COUNTER = 0x04,
  TLV_VCC     = 0x05, // uint8, V
  // button
  TLV_BUTTON  = 0x06,
  TLV_SWITCH  = 0x07,
  TLV_PIR     = 0x08,
  TLV_RADAR   = 0x09,
  // radar
  // ds18b20
  TLV_T_DS    = 0x10,
  // si7021
  TLV_T_SI    = 0x11,
  TLV_H_SI    = 0x12,
  // bmp280/bme680
  TLV_T_BM    = 0x13, // int16, °C * 10
  TLV_H_BM    = 0x14, // int16, % * 10
  TLV_P_BM    = 0x15, // uint32, Pa/10
  TLV_G_BM    = 0x16  // uint16, kOhm
};

// payload constraints
const uint8_t MAX_PAYLOAD = 61; // original used 61
const uint8_t TLV_LEADING_LEN = 1; // we put 1 byte leading length
const uint8_t MAX_TLV_PAYLOAD = MAX_PAYLOAD - TLV_LEADING_LEN; // 60

uint8_t txPlain[MAX_PAYLOAD]; // [0] will hold payload length, TLVs start at 1
uint8_t txTLVLen = 0; // number of bytes used by TLVs (excluding leading length)

// Reset the packet (clear TLVs)
void resetPacket()
{
  memset(txPlain, 0, sizeof(txPlain));
  txTLVLen = 0;
}

// Add generic TLV
bool addTLV(uint8_t type, const void* data, uint8_t length)
{
  if (length == 0) return false;
  if (txTLVLen + 2 + length > MAX_TLV_PAYLOAD)
  {
#ifdef VERBOSE
    Serial.print(F("TLV: Not enough space for TLV type "));
    Serial.println(type, HEX);
#endif
    return false;
  }
  uint8_t idx = 1 + txTLVLen; // start after leading length byte
  txPlain[idx++] = type;
  txPlain[idx++] = length;
  memcpy(&txPlain[idx], data, length);
  txTLVLen += 2 + length;
  return true;
}

// Convenience helpers
bool addTLV_u8(uint8_t type, uint8_t v) { return addTLV(type, &v, 1); }
bool addTLV_u16(uint8_t type, uint16_t v) { return addTLV(type, &v, 2); }
bool addTLV_i16(uint8_t type, int16_t v) { return addTLV(type, &v, 2); }
bool addTLV_u32(uint8_t type, uint32_t v) { return addTLV(type, &v, 4); }

#ifdef SENSOR_TYPE_button
void handleSensorButton()
{
  if (buttonDetected)
  {
    uint8_t state = buttonDetected ? 1 : (buttonState == HIGH ? 0 : 1); // keep original reverse logic
    addTLV_u8(TLV_BUTTON, state);
    buttonDetected = false;
#ifdef VERBOSE
    Serial.print(F("TLV: Button state "));
    Serial.println(state);
#endif
  }
}
#endif

#ifdef SENSOR_TYPE_switch  
void handleSensorSwitch() {
  if (switchChanged) {
    uint8_t state = (switchState == HIGH) ? 1 : 0;
    addTLV_u8(TLV_SWITCH, state); 
    switchChanged = false;
  }
}
#endif

#ifdef SENSOR_TYPE_pir
void handleSensorPir()
{
  if (motionDetected)
  {
    uint8_t state = (pirState == HIGH) ? 1 : 0;
    addTLV_u8(TLV_PIR, state);
    motionDetected = false;
#ifdef VERBOSE
    Serial.print(SENSOR_TYPE_pir);
    Serial.print(F(": "));
    Serial.println(state);
#endif
  }
}
#endif

#ifdef SENSOR_TYPE_radar
void handleSensorRadar() {
  if (motionDetected) {
    uint8_t state = (radarState == HIGH) ? 1 : 0;
    addTLV_u8(TLV_RADAR, state); // Need to define TLV_RADAR
    motionDetected = false;
  }
}
#endif

#ifdef SENSOR_TYPE_si7021
void handleSensorSi7021()
{
  float si_temperature = si.readTemperature();
  float si_humidity = si.readHumidity();
  if (!isnan(si_temperature))
  {
    int16_t t = (int16_t)round(si_temperature * 10.0);
    int16_t h = (int16_t)round(si_humidity * 10.0);
    addTLV_i16(TLV_T_SI, t);
    addTLV_i16(TLV_H_SI, h);
#ifdef VERBOSE
    Serial.print(F("Si7021: T="));
    Serial.print(t / 10.0);
    Serial.print(F(" H="));
    Serial.println(h / 10.0);
#endif
  }
}
#endif

#ifdef SENSOR_TYPE_ds18b20
void handleSensorDs18b20()
{
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
  if (ds_temperature != DEVICE_DISCONNECTED_C)
  {
    int16_t t = (int16_t)round(ds_temperature * 10.0);
    addTLV_i16(TLV_T_DS, t);
#ifdef VERBOSE
    Serial.print(F("DS18B20 T="));
    Serial.println(t / 10.0);
#endif
  }
}
#endif

#ifdef SENSOR_TYPE_bmp280
void handleSensorBmp280()
{
  float bmp_temperature = bmp280.readTemperature();
  float bmp_pressure = bmp280.readPressure(); // Pa
  if (!isnan(bmp_pressure) && bmp_pressure > 0)
  {
    int16_t t = (int16_t)round(bmp_temperature * 10.0);
    uint32_t p_div10 = (uint32_t)round(bmp_pressure / 10.0); // Pa/10
    addTLV_i16(TLV_T_BM, t);
    addTLV_u32(TLV_P_BM, p_div10);
#ifdef VERBOSE
    Serial.print(F("TLV: BMP280 T="));
    Serial.print(t / 10.0);
    Serial.print(F(" P="));
    Serial.println(p_div10);
#endif
  }
}
#endif

#ifdef SENSOR_TYPE_bme680
void handleSensorBme680()
{
  if (!bme680.performReading())
  {
#ifdef VERBOSE
    Serial.println(F("[BME680]: ERROR read!"));
#endif
    sleepDeep(255);
    return;
  }
  float bme_temperature = bme680.temperature;
  float bme_humidity = bme680.humidity;
  float bme_pressure = bme680.pressure / 100.0; // hPa
  float bme_gas = bme680.gas_resistance / 1000.0; // kOhm

  if (!isnan(bme_temperature))
  {
    int16_t t = (int16_t)round(bme_temperature * 10.0);
    int16_t h = (int16_t)round(bme_humidity * 10.0);
    uint32_t p10 = (uint32_t)round((bme680.pressure) / 10.0); // Pa/10
    int16_t q = (int16_t)round(bme_gas);
    addTLV_i16(TLV_T_BM, t);
    addTLV_i16(TLV_H_BM, h);
    addTLV_u32(TLV_P_BM, p10);
    addTLV_u16(TLV_G_BM, (uint16_t)q);
#ifdef VERBOSE
    Serial.print(F("TLV: BME680 T="));
    Serial.print(t / 10.0);
    Serial.print(F(" H="));
    Serial.print(h / 10.0);
    Serial.print(F(" P="));
    Serial.print(p10);
    Serial.print(F(" Q="));
    Serial.println(q);
#endif
  }
}
#endif

void handleVcc()
{
  float vccf = vRef.readVcc() / 1000.0;
  uint8_t v = (uint8_t)round(vccf * 10);
  addTLV_u8(TLV_VCC, v);
#ifdef VERBOSE
  Serial.print(F("VCC: "));
  Serial.print(v / 10.0, 1);
  Serial.println(F("V"));
#endif
}

void transmitPacket()
{
  if (!lora_status || txTLVLen == 0) {
#ifdef VERBOSE
    if (!lora_status) Serial.println(F("LoRa: Not initialized"));
    if (txTLVLen == 0) Serial.println(F("LoRa: No data to send"));
#endif
    return;
  }

  // Put leading length byte (number of TLV payload bytes)
  txPlain[0] = txTLVLen;
  uint16_t plainLen = txTLVLen + 1; // include length byte

  LoRa.beginPacket();
  LoRa.write(receiverAddress);
  LoRa.write(senderAddress);

#ifdef USE_CRYPTO
  // PKCS#7 padding length
  uint8_t padLen = 16 - (plainLen % 16);
  if (padLen == 0) padLen = 16;  // always add padding
  uint16_t cipherLen = plainLen + padLen;

  // Fill plaintext buffer with padding
  memcpy(cipher, txPlain, plainLen);
  memset(cipher + plainLen, padLen, padLen);

  // Encrypt each block
  for (uint16_t b = 0; b < cipherLen; b += 16) {
    aes128.encryptBlock(&cipher[b], &cipher[b]);
  }

  // Send encrypted payload (binary safe)
  LoRa.write(cipher, cipherLen);

#ifdef VERBOSE
  Serial.println(F("LoRa: Transmitting AES packet... OK"));
  Serial.print(F("Packet Length: "));
  Serial.print(cipherLen);
  Serial.println(F(" bytes"));

  // Hex dump
  Serial.print(F("Packet (hex): "));
  for (uint16_t i = 0; i < cipherLen; i++) {
    if (cipher[i] < 0x10) Serial.print('0');  // leading zero
    Serial.print(cipher[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif

#else // no crypto
  // Send plain payload (binary safe)
  LoRa.write(txPlain, plainLen);

#ifdef VERBOSE
  Serial.println(F("LoRa: Transmitting packet... OK"));
  Serial.print(F("Packet Length: "));
  Serial.print(plainLen);
  Serial.println(F(" bytes"));

  // Hex dump
  Serial.print(F("Packet (hex): "));
  for (uint16_t i = 0; i < plainLen; i++) {
    if (txPlain[i] < 0x10) Serial.print('0');  // leading zero
    Serial.print(txPlain[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif

#endif // USE_CRYPTO

  if (LoRa.endPacket() == 0) {
#ifdef VERBOSE
    Serial.println(F("LoRa: Transmission failed"));
#endif
  }

#ifdef VERBOSE
  // Print TLV summary for debugging
  Serial.print(F("TLV bytes: "));
  Serial.println(txTLVLen);
  Serial.print(F("Packet length (sent): "));
#ifdef USE_CRYPTO
  Serial.println(cipherLen);
#else
  Serial.println(plainLen);
#endif
#endif
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

void printTLVDebug() {
  Serial.println(F("TLV Packet:"));
  for (uint8_t i = 1; i <= txTLVLen; ) {
    uint8_t type = txPlain[i++];
    uint8_t len  = txPlain[i++];
    Serial.print(F("Type 0x"));
    Serial.print(type, HEX);
    Serial.print(F(" Len "));
    Serial.print(len);
    Serial.print(F(" Value: "));
    for (uint8_t j = 0; j < len; j++) {
      if (txPlain[i+j] < 0x10) Serial.print('0');
      Serial.print(txPlain[i+j], HEX);
      Serial.print(' ');
    }
    Serial.println();
    i += len;
  }
}

// -----------------------
// Setup & Loop
// -----------------------

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

  // Start LoRa
#ifdef VERBOSE
  Serial.print(F("> LoRa: "));
#endif
  int lora_status = LoRa.begin(LO_FREQ);
  if (lora_status)
  {
#ifdef VERBOSE
    Serial.println(F("Initialized"));
#endif
    // LoRa.setTxPower(LO_POWER);
    // LoRa.onTxDone(transmitDone);
    LoRa.setSpreadingFactor(10);
    LoRa.setSyncWord(0x13);
    LoRa.enableCrc();
  }
  else
  {
#ifdef VERBOSE
    Serial.print(F("Not detected "));
    Serial.println(lora_status);
    lora_status= false;
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

  // Build TLV packet
  resetPacket();

  // Core identifying TLVs
  uint16_t uid = (uint16_t)getUniqueID();
  addTLV_u16(TLV_UID, uid);

#ifdef VERBOSE_PC
  addTLV_u16(TLV_COUNTER, msgCounter++);
#endif

  pid = random(99) + 1;
  addTLV_u8(TLV_PID, (uint8_t)pid);

  // Append sensor readings
#ifdef SENSOR_TYPE_button
  handleSensorButton();
#endif
#ifdef SENSOR_TYPE_pir
  handleSensorPir();
#endif
#ifdef SENSOR_TYPE_radar
  handleSensorRadar();
#endif
#ifdef SENSOR_TYPE_switch
  handleSensorSwitch();
#endif
#ifdef SENSOR_TYPE_si7021
  handleSensorSi7021();
#endif
#ifdef SENSOR_TYPE_ds18b20
  handleSensorDs18b20();
#endif
#ifdef SENSOR_TYPE_bmp280
  handleSensorBmp280();
#endif
#ifdef SENSOR_TYPE_bme680
  handleSensorBme680();
#endif

  handleVcc();

  // printTLVDebug();

  transmitPacket();
  sleepDevice();
}
