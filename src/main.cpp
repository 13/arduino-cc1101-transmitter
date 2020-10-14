#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include <LowPower.h>
#include <ArduinoUniqueID.h>
#include <VoltageReference.h>
#include <Adafruit_Si7021.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define INFO 
//#define DEBUG

// sensors
#define SENSOR_TYPE_1   "si7021"
#define SENSOR_TYPE_2   "ds18b20"
#define SENSOR_TYPE_3   "bmp280"
#define SENSOR_PIN_1    0 // sda
#define SENSOR_PIN_2    2 // sdc
#define SENSOR_PIN_3    3 // onewire
#define DS_L           10 // deepsleep min
#define DS_S            2 // deepsleep min

// CC1101
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// voltage
VoltageReference vRef;

// Si7021
Adafruit_Si7021 si = Adafruit_Si7021();
// DS18B20
OneWire oneWire(SENSOR_PIN_3);
DallasTemperature ds18b20(&oneWire);
// BMP280
Adafruit_BMP280 bmp280;

// counter
uint16_t msgCounter = 1;

String getUniqueID();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);

void setup() {
  Serial.begin(9600);
  delay(10);
#ifdef INFO
  delay(20);
#endif
  // Start Boot
  Serial.println(F("> "));
  Serial.println(F("> "));
  Serial.print(F("> Booting... Compiled: "));
  Serial.println(F(__TIMESTAMP__));
  // Start CC1101
  Serial.print(F("[CC1101] Initializing... "));
  int state = cc.begin(868.32, 4.8, 48.0, 325.0, 0, 4);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    sleepDeep(DS_S);
  }
  // voltage
  vRef.begin();
  // Si7021
  si.begin();
  /*if (!si.begin()){
    Serial.println("Did not find Si7021 sensor!");
    sleepDeep(DS_S);
  }*/
  // DS18B20
  ds18b20.begin();
  // BMP280
  bmp280.begin(0x76,0x60); // fix GY-B11 module
}

void loop() {
  float si_temperature = si.readTemperature(); 
  float si_humidity = si.readHumidity();
  if (!isnan(si_temperature)) {
    Serial.print(SENSOR_TYPE_1);
    Serial.print(": ");
    Serial.print(si_temperature);
    Serial.print("C, ");
    Serial.print(si_humidity);
    Serial.println("%, ");
  }
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    Serial.print(SENSOR_TYPE_2);
    Serial.print(": ");
    Serial.print(ds_temperature);
    Serial.println("C");
  }
  float bmp280_temperature = bmp280.readTemperature();
  float bmp280_pressure = bmp280.readPressure();
  float bmp280_altitude = bmp280.readAltitude(1013.25);
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0) {
    Serial.print(SENSOR_TYPE_3);
    Serial.print(": ");
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

  // prepare msg string
  String str = "M";
#ifdef DEBUG
  str += ",I:";
  str += msgCounter;
#endif
  str += ",N:";
  str += getUniqueID();
  if (!isnan(si_temperature)) {
    str += ",T1:";
    str += int(round(si_temperature*10));
    str += ",H1:";
    str += int(round(si_humidity*10));
  }
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    str += ",T2:";
    str += int(round(ds_temperature*10));
  }
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0) {
    str += ",T3:";
    str += int(round(bmp280_temperature*10));
    str += ",P1:";
    str += int(round(bmp280_pressure)/10);
    str += ",A1:";
    str += int(round(bmp280_altitude));
  }
  str += ",V1:";
  str += int(vcc);

  if (str.length() > 60){
    Serial.print(F("> String too long: "));
    Serial.println(str.length());
    sleepDeep(DS_S);
  } else {
    Serial.print(F("> String length: "));
    Serial.println(str.length());
    //str += ",E:";
    int str_diff = 60 - str.length();
    if (str_diff >= 1){ str += ",";}
    if (str_diff >= 2){ str += "E";}
    if (str_diff >= 3){ str += ":";}

    // max length is 62 because of Arduino String last byte 00
    // but 62 not good better use 61
    // String length here to 60, thus packet length 61
    for (uint8_t i = str.length(); i < 60; i++){
      str += "0";
    }
  }
  Serial.print(F("[CC1101] Transmitting packet... "));
  // String to byte +1 string nul terminator 00 and overwrite 
  byte byteArr[str.length()+1];
  str.getBytes(byteArr,sizeof(byteArr));
  byteArr[sizeof(byteArr)/sizeof(byteArr[0])-1] = '0';
  //Serial.print("Packet Length: ");
  //Serial.println(sizeof(byteArr)/sizeof(byteArr[0])); // +1 
  int state = cc.transmit(byteArr,sizeof(byteArr)/sizeof(byteArr[0]));

  if (state == ERR_NONE) {
    Serial.println(F("OK"));
	  Serial.println(str);
    for(uint8_t i=0; i<sizeof(byteArr); i++){
      printHex(byteArr[i]);
    }
    Serial.println("");
  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 64 bytes
    Serial.println(F("ERR: too long!"));
  } else {
    // some other error occurred
    Serial.print(F("ERR, code "));
    Serial.println(state);
  }
  sleepDeep(DS_L);
  msgCounter++;
}

// Last 4 digits of ChipID
String getUniqueID(){
//#ifdef DEBUG
//	Serial.println();
//  UniqueIDdump(Serial);
//#endif
  String uid = "";
	for (size_t i = 7; i < UniqueIDsize; i++){
		if (UniqueID[i] < 0x10){
      uid += "0";
    }
    uid += String(UniqueID[i],HEX);
	}
	Serial.println();
  // read EEPROM serial number
  if (uid == "ffff"){
    int address = 13;
    int serialNumber;
    if (EEPROM.read(address) != 255){
      EEPROM.get(address, serialNumber);
      uid = serialNumber;
	    Serial.print("EEPROM SN: ");
    } else {
	    Serial.print("EEPROM SN: ERROR EMPTY!");
      uid = "0000";
    }
  } else {
	  Serial.print("CHIP SN: ");
  }
	Serial.println(uid);
	return uid;
}

// sleep
// 1 - 253 minutes
// 255 = 8 seconds
// 0 = forever
void sleepDeep(uint8_t t) {
  uint8_t m = 60;
#ifdef INFO
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
}