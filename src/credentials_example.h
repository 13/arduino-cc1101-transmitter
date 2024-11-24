// Disable/enable sensors
// #define SENSOR_TYPE_si7021   "si7021"
// #define SENSOR_TYPE_ds18b20  "ds18b20"
// #define SENSOR_TYPE_bmp280   "bmp280"
// #define SENSOR_TYPE_bme680   "bme680"
// #define SENSOR_TYPE_pir      "pir"
// #define SENSOR_TYPE_radar    "radar"
// #define SENSOR_TYPE_switch   "switch"
#define SENSOR_TYPE_button      "button"

// OUTPUT
#define VERBOSE // Always enabled
// #define VERBOSE_FW // Firmware Version
// #define VERBOSE_PC // Packet Count
// #define DEBUG

// Deep sleep
#define DS_L 36   // long [36] 60 
#define DS_S 8 // short [8] 16
#define DS_D 100 // delay before sleep 70 [100] 500

// LoRa
#define LO_FREQ 915E6
#define LO_POWER 12
#define LO_DELAY 100 // [100]
// Type
#define SEND_CHAR
//#define SEND_BYTE

// Crypto
// openssl rand -hex 16
#define USE_CRYPTO
#ifdef USE_CRYPTO
#define AES_KEY "808639b9d210f261fefcce5a85c0cadb"
#endif

// Sensor pins
#ifdef SENSOR_TYPE_ds18b20
#define SENSOR_PIN_OW 3 // needs changes 2 & 3 only interrupt pins
#endif
#ifdef SENSOR_TYPE_pir
#define SENSOR_PIN_PIR 3
#define MQTT_RETAINED_DISABLED
#endif
#ifdef SENSOR_TYPE_radar
#define SENSOR_PIN_RADAR 3
#define MQTT_RETAINED_DISABLED
#endif
#ifdef SENSOR_TYPE_switch
#define SENSOR_PIN_SWITCH 3
#endif
#ifdef SENSOR_TYPE_button
#define SENSOR_PIN_BUTTON 3
#define MQTT_RETAINED_DISABLED
#endif

// FORCE GENERATE UID
// #define GEN_UID
// #define CUSTOM_UID "cca"
