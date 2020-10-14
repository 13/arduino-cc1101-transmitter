#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include <LowPower.h>
#include <VoltageReference.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_BME680.h>

#define INFO 
//#define DEBUG

// sensors
#define SENSOR_TYPE_1   "bme680"
#define SENSOR_TYPE_2   "ds18b20"
#define SENSOR_PIN_1    0 // sda
#define SENSOR_PIN_2    2 // sdc
#define SENSOR_PIN_3    3 // onewire
#define DS_L            5 // deepsleep min
#define DS_S            2 // deepsleep min

// CC1101
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// voltage
VoltageReference vRef;

// bme680
Adafruit_BME680 bme680 = Adafruit_BME680();
float SEALEVELPRESSURE_HPA = 1013.25;
// DS18B20
OneWire oneWire(SENSOR_PIN_3);
DallasTemperature ds18b20(&oneWire);

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

int humidity_score, gas_score;
float gas_reference = 2500;
float hum_reference = 40;
int getgasreference_count = 0;
int gas_lower_limit = 10000;  // Bad air quality limit
int gas_upper_limit = 300000; // Good air quality limit

// counter
uint16_t msgCounter = 1;

int getUniqueID();
void sleepDeep(uint8_t t);
void printHex(uint8_t num);
void GetGasReference();
String CalculateIAQ(int score);
int GetHumidityScore();
int GetGasScore();

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
  // bme680
  //bme680.begin();
  if (!bme680.begin()){
    Serial.println("[BME680]: ERROR sensor!");
    sleepDeep(DS_S);
  // DS18B20
  ds18b20.begin();
  }
  bme680.setTemperatureOversampling(BME680_OS_8X);
  bme680.setHumidityOversampling(BME680_OS_2X);
  bme680.setPressureOversampling(BME680_OS_4X);
  bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme680.setGasHeater(320, 150); // 320*C for 150 ms
  GetGasReference();
}

void loop() {
  ds18b20.requestTemperatures();
  float ds_temperature = ds18b20.getTempCByIndex(0);
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    Serial.print(SENSOR_TYPE_2);
    Serial.print(": ");
    Serial.print(ds_temperature);
    Serial.println("C");
  }
  if (! bme680.performReading()) {
    Serial.println("[BME680]: ERROR read!");
    sleepDeep(1);
  }
  float bme680_temperature = bme680.temperature; 
  float bme680_humidity = bme680.humidity;
  float bme680_pressure = bme680.pressure/100.0;
  float bme680_altitude = bme680.readAltitude(SEALEVELPRESSURE_HPA);
  float bme680_gas = bme680.gas_resistance / 1000.0;
  humidity_score = GetHumidityScore();
  gas_score      = GetGasScore();
  //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
  float air_quality_score = humidity_score + gas_score;
  if ((getgasreference_count++) % 5 == 0) GetGasReference();
  int iaq = (100 - air_quality_score) * 5;
  if (!isnan(bme680_temperature)) {
    Serial.print(SENSOR_TYPE_1);
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
    Serial.print("KOhms, ");
    Serial.print(gas_reference);
    Serial.print("ohms, ");
    Serial.print(iaq);
    Serial.println(" IAQ");
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
  str += String(getUniqueID(), HEX);
  if (!isnan(bme680_temperature)) {
    str += ",T1:";
    str += int(round(bme680_temperature*10));
    str += ",H1:";
    str += int(round(bme680_humidity*10));
    str += ",P1:";
    str += int(round(bme680_pressure*10));
    str += ",A1:";
    str += int(round(bme680_altitude));
    str += ",Q1:";
    str += int(round(iaq));
  }
  if (ds_temperature != DEVICE_DISCONNECTED_C) {
    str += ",T2:";
    str += int(round(ds_temperature*10));
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

void GetGasReference() {
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  //Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++) { // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme680.readGas();
  }
  gas_reference = gas_reference / readings;
  //Serial.println("Gas Reference = "+String(gas_reference,3));
}

String CalculateIAQ(int score) {
  String IAQ_text = "air quality is ";
  score = (100 - score) * 5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  Serial.print("IAQ Score = " + String(score) + ", ");
  return IAQ_text;
}

int GetHumidityScore() {  //Calculate humidity contribution to IAQ index
  float current_humidity = bme680.readHumidity();
  if (current_humidity >= 38 && current_humidity <= 42) // Humidity +/-5% around optimum
    humidity_score = 0.25 * 100;
  else
  { // Humidity is sub-optimal
    if (current_humidity < 38)
      humidity_score = 0.25 / hum_reference * current_humidity * 100;
    else
    {
      humidity_score = ((-0.25 / (100 - hum_reference) * current_humidity) + 0.416666) * 100;
    }
  }
  return humidity_score;
}

int GetGasScore() {
  //Calculate gas contribution to IAQ index
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00;
  if (gas_score > 75) gas_score = 75; // Sometimes gas readings can go outside of expected scale maximum
  if (gas_score <  0) gas_score = 0;  // Sometimes gas readings can go outside of expected scale minimum
  return gas_score;
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
    Serial.print(" - HEX: ");
    Serial.println(String(serialNumber, HEX));
  } else {
	  Serial.println("EEPROM SN: ERROR EMPTY!");
  }
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