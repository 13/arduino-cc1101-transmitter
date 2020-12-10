#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include <LowPower.h>
#include <VoltageReference.h>

#if __has_include("config.h")
  #include "config.h"
#else
  #error "Using Defaults: Copy config.sample.h to config.h and edit that to use your own settings"
#endif

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

// CC1101
// CS pin:    10
// GDO0 pin:  2
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

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
#ifdef INFO 
  Serial.print(F("Chip: v"));
  Serial.print(cc.getChipVersion());
  Serial.print(F(" "));
#endif
  //int state = cc.begin(868.32, 4.8, 48.0, 325.0, 0, 4);
  int state = cc.begin(CC_FREQ, 48.0, 48.0, 135.0, CC_POWER, 16);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    sleepDeep(DS_S);
  }

  // voltage
  vRef.begin();

#ifdef SENSOR_TYPE_si7021
  if (!si.begin()){
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(": ");
    Serial.println(" ERROR -1");
    sleepDeep(DS_S);
  }
#endif
	
#ifdef SENSOR_TYPE_ds18b20
  ds18b20.begin();
#endif
	
#ifdef SENSOR_TYPE_bmp280
  bmp280.begin(0x76,0x60); // fix GY-B11 module
#endif

#ifdef SENSOR_TYPE_bme680
  if (!bme680.begin()){
    Serial.print(SENSOR_TYPE_bme680);
    Serial.print(": ");
    Serial.println(" ERROR -1");
    sleepDeep(DS_S);
  }
#endif
}

void loop() {
#ifdef SENSOR_TYPE_si7021	
  float si_temperature = si.readTemperature(); 
  float si_humidity = si.readHumidity();
  if (!isnan(si_temperature)) {
    Serial.print(SENSOR_TYPE_si7021);
    Serial.print(": ");
    Serial.print(si_temperature);
    Serial.print("C, ");
    Serial.print(si_humidity);
    Serial.println("%, ");
  }
#endif
#ifdef SENSOR_TYPE_ds18b20
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    Serial.print(SENSOR_TYPE_ds18b20);
    Serial.print(": ");
    Serial.print(ds_temperature);
    Serial.println("C");
  } else {
    Serial.print(SENSOR_TYPE_ds18b20);
    Serial.print(": ");
    Serial.println("ERR");
  }
#endif
#ifdef SENSOR_TYPE_bmp280
  float bmp280_temperature = bmp280.readTemperature();
  float bmp280_pressure = bmp280.readPressure();
  float bmp280_altitude = bmp280.readAltitude(1013.25);
  if (!isnan(bmp280_pressure) || bmp280_pressure > 0) {
    Serial.print(SENSOR_TYPE_bmp280);
    Serial.print(": ");
    Serial.print(bmp280_temperature);
    Serial.print("C, ");
    Serial.print(bmp280_pressure);
    Serial.print("Pa, ");
    Serial.print(bmp280_altitude);
    Serial.println("m");
  }
#endif
#ifdef SENSOR_TYPE_bme680
  if (! bme680.performReading()) {
    Serial.println("[BME680]: ERROR read!");
    sleepDeep(1);
  }
  float bme680_temperature = bme680.temperature; 
  float bme680_humidity = bme680.humidity;
  float bme680_pressure = bme680.pressure/100.0;
  float bme680_altitude = bme680.readAltitude(1013.25);
  float bme680_gas = bme680.gas_resistance / 1000.0;
  if (!isnan(bme680_temperature)) {
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
  }
#endif
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
  str += String(getUniqueID(), HEX);
#ifdef SENSOR_TYPE_si7021	
  if (!isnan(si_temperature)) {
    str += ",T1:";
    str += int(round(si_temperature*10));
    str += ",H1:";
    str += int(round(si_humidity*10));
  }
#endif
#ifdef SENSOR_TYPE_ds18b20
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    str += ",T2:";
    str += int(round(ds_temperature*10));
  }
#endif
#ifdef SENSOR_TYPE_bmp280
  if (!isnan(bmp280_pressure) && bmp280_pressure > 0) {
    str += ",T3:";
    str += int(round(bmp280_temperature*10));
    str += ",P3:";
    str += int(round(bmp280_pressure)/10);
    str += ",A3:";
    str += int(round(bmp280_altitude));
  }
#endif
#ifdef SENSOR_TYPE_bme680
  if (!isnan(bme680_temperature)) {
    str += ",T4:";
    str += int(round(bme680_temperature*10));
    str += ",H4:";
    str += int(round(bme680_humidity*10));
    str += ",P4:";
    str += int(round(bme680_pressure*10));
    str += ",A4:";
    str += int(round(bme680_altitude));
    str += ",Q4:";
    str += int(round(bme680_gas));
  }
#endif	
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
#ifdef DEBUG
  msgCounter++;
#endif
}

// Last 4 digits of ChipID
int getUniqueID(){
  int uid = 0;
  // read EEPROM serial number
  int address = 13;
  int serialNumber;
	Serial.println();
  if (EEPROM.read(address) != 255){
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
	  Serial.print("EEPROM SN: ");
	  Serial.print(uid);
    Serial.print(" -> HEX: ");
    Serial.println(String(serialNumber, HEX));
  } else {
    Serial.println("EEPROM SN: ERROR EMPTY!");
  }
    return uid;
}

// sleep
// 1 - 254 minutes
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
