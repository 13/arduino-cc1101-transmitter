#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include <LowPower.h>
#include <ArduinoUniqueID.h>
#include <VoltageReference.h>
#include <Adafruit_Si7021.h>

#define DEBUG

// sensors
#define SENSOR_TYPE     "si7021"
#define SENSOR_PIN_1    0 // sda
#define SENSOR_PIN_2    2 // sdc
#define DS_LONG         2 // deepsleep min

// CC1101
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// voltage
VoltageReference vRef;

// Si7021
Adafruit_Si7021 si = Adafruit_Si7021();

// counter
uint16_t msgCounter = 1;

String getUniqueID();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);

void setup() {
  Serial.begin(9600);
  delay(10);
#ifdef DEBUG
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
    sleepDeep(1);
  }
  // voltage
  vRef.begin();
  // Si7021
  si.begin();
  /*if (!si.begin()){
    Serial.println("Did not find Si7021 sensor!");
    sleepDeep(1);
  }*/
}

void loop() {
  float temperature = si.readTemperature(); 
  float humidity = si.readHumidity();
  if (!isnan(temperature)) {
    Serial.print(SENSOR_TYPE);
    Serial.print(": ");
    Serial.print(temperature);
    Serial.print("C, ");
    Serial.print(humidity);
    Serial.println("%, ");
  }

  float vcc = vRef.readVcc()/100;
  Serial.print("VCC: ");
  Serial.print(vcc);

  // prepare msg string
  //long randNum = random(0,9);
  String str = "M,I:";
  str += msgCounter;
  //str += randNum;
  str += ",N:";
  str += getUniqueID();
  if (!isnan(temperature)) {
    str += ",T1:";
    str += int(round(temperature*10));
    str += ",H1:";
    str += int(round(humidity*10));
  }
  str += ",V1:";
  str += int(vcc);
  str += ",E:";

  if (str.length() > 60){
    Serial.println(F("> String too long"));
    sleepDeep(1);
  }

  // max length is 62 because of Arduino String last byte 00
  // but 62 not good better use 61
  // String length here to 60, thus packet length 61
  for (uint8_t i = str.length(); i < 60; i++){
    str += "0";
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
  sleepDeep(DS_LONG);
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
}