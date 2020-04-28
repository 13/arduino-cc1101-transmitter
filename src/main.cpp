#include <Arduino.h>
#include <RadioLib.h>
#include <LowPower.h>
#include <ArduinoUniqueID.h>
#include <VoltageReference.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define ENABLE_BYTE
//#define ENABLE_NODE_ADDRESS
//#define ENABLE_ENCODING
//#define ENABLE_SYNC_WORD
//#define DISABLE_SYNC_WORD_FILTERING
//#define DISABLE_CRC
#define DEBUG

// sensors
#define SENSOR_PIN 3

// CC1101
// CS pin:    10
// GDO0 pin:  2
// RST pin:   unused
// GDO2 pin:  3 (optional)
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// DS18B20
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
// BMP280
Adafruit_BMP280 bmp;

// voltage
VoltageReference vRef;

// counter
uint16_t msgCounter = 1;

// fix platformio
String getUniqueID();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);

void setup() {
  Serial.begin(9600);
  delay(10);
#ifdef DEBUG
  delay(2000);
#endif

  // Start Boot
  Serial.println("");
  Serial.println("");
  Serial.print("Booting... Compiled: ");
  Serial.println(__TIMESTAMP__);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  //int state = cc.begin();
  int state = cc.begin(868.32, 4.8, 48.0, 325.0, 0, 4);
  //int state = cc.begin(868.325, 17.240, 48.0, 325.0, 0, 4);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true);
  }
#ifdef ENABLE_ENCODING
  // set encoding 
  Serial.print("> [CC1101] Setting Encoding ... ");
  state = cc.setEncoding(0);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef ENABLE_NODE_ADDRESS
  // set node address
  Serial.print(F("[CC1101] Setting Node Address ... "));
  state = cc.setNodeAddress(0x22,0);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true);
  }
#endif
#ifdef ENABLE_SYNC_WORD 
  // 2 bytes can be set as sync word
  Serial.print(F("[CC1101] Setting Sync Word ... "));
  state = cc.setSyncWord(0xd4, 0x2d);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true);
  }
#endif
#ifdef DISABLE_SYNC_WORD_FILTERING 
  // disable sync word filtering
  Serial.print("> [CC1101] Disable Sync Word Filtering ... ");
  state = cc.disableSyncWordFiltering();
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef DISABLE_CRC 
  // disable crc 
  Serial.print(F("[CC1101] Disabling CRC ... "));
  state = cc.setCrcFiltering(false);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true);
  }
#endif
  // DS18B20
  sensors.begin();
  bmp.begin(0x76,0x60); // fix GY-B11 module
  // voltage
  vRef.begin();
}

void loop() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  if (temperature != DEVICE_DISCONNECTED_C) {
    Serial.print("DS18B20: ");
    Serial.print(temperature);
    Serial.println("C");
  }
  float bmp280_temperature = bmp.readTemperature();
  float bmp280_pressure = bmp.readPressure();
  float bmp280_altitude = bmp.readAltitude(1013.25);
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0) {
    Serial.print("BMP280: ");
    Serial.print(bmp280_temperature);
    Serial.print("C, ");
    Serial.print(bmp280_pressure);
    Serial.print("Pa, ");
    Serial.print(bmp280_altitude);
    Serial.println("m");
  }
  float vcc = vRef.readVcc()/100;
  Serial.print("VCC: ");
  Serial.print(vcc);

  // prepare msg
  //long randNum = random(0,9);
  String str = "M,I:";
  str += msgCounter;
  //str += randNum;
  str += ",N:";
  str += getUniqueID();
  if (temperature != DEVICE_DISCONNECTED_C) {
    str += ",T1:";
    str += int(round(temperature*10));
  }
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0) {
    str += ",T2:";
    str += int(round(bmp280_temperature*10));
    str += ",P1:";
    str += int(round(bmp280_pressure)/10);
    str += ",A1:";
    str += int(round(bmp280_altitude));
  }
  str += ",V1:";
  str += int(vcc);
  str += ",E:";
  // max length is 62 because of Arduino String last byte 00
  // but 62 not good better use 61
  // String length here to 60, thus packet length 61
  for (uint8_t i = str.length(); i < 60; i++){
    str += "0";
  }

  Serial.println(F("[CC1101] Transmitting packet ... "));
#ifdef ENABLE_BYTE
  // String to byte +1 string nul terminator 00 and overwrite 
  byte byteArr[str.length()+1];
  str.getBytes(byteArr,sizeof(byteArr));
  byteArr[sizeof(byteArr)/sizeof(byteArr[0])-1] = '0';
  Serial.print("Packet Length: ");
  Serial.println(sizeof(byteArr)/sizeof(byteArr[0])); // +1 
  int state = cc.transmit(byteArr,sizeof(byteArr)/sizeof(byteArr[0]));
#else
  int state = cc.transmit(str);
#endif

  if (state == ERR_NONE) {
    Serial.print(F("[CC1101] Transmitting packet "));
    Serial.println(F("OK"));
	  Serial.println(str);
#ifdef ENABLE_BYTE
    for(uint8_t i=0; i<sizeof(byteArr); i++){
      printHex(byteArr[i]);
    }
    Serial.println("");
#endif
  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 64 bytes
    Serial.println(F("ERR: too long!"));
  } else {
    // some other error occurred
    Serial.print(F("ERR, code "));
    Serial.println(state);
  }
  sleepDeep(255);
  msgCounter++;
}

String getUniqueID(){
  String uid = "";
	for (size_t i = 7; i < UniqueIDsize; i++){
		if (UniqueID[i] < 0x10){
      uid += "0";
    }
    uid += String(UniqueID[i],HEX);
	}
	Serial.println();
	return uid;
}

// sleep
// 1 - 253 minutes
// 255 = 8 seconds
// 0 = forever
void sleepDeep(uint8_t t) {
  uint8_t m = 60;
#ifdef DEBUG
  Serial.print("Deep Sleep: ");
  if (t < 1){
    Serial.println("forever");
  } else if (t > 254){
    t = 1;
    m = 8;
    Serial.println("8s");
  } else {
    Serial.print(t);
    Serial.println("min");
  }
#endif
  delay(500); // 70 100 500 1000
  if (t > 0){
    for (int8_t i = 0; i < (t * m / 8); i++) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
  } else {
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}
void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
  //Serial.print(" ");
}