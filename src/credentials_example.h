// Disable/enable sensors
// #define SENSOR_TYPE_si7021   "si7021"
// #define SENSOR_TYPE_ds18b20  "ds18b20"
// #define SENSOR_TYPE_bmp280   "bmp280"
// #define SENSOR_TYPE_bme680   "bme680"
// #define SENSOR_TYPE_pir      "pir"
#define SENSOR_TYPE_switch "switch"

// OUTPUT
#define VERBOSE // Always enabled
// #define VERBOSE_FW // Firmware Version
// #define VERBOSE_PC // Packet Count
#define DEBUG

// Deep sleep
#define DS_L 36   // long [36] 60 
#define DS_S 8 // short [8] 16
#define DS_D 100 // delay before sleep 70 [100] 500

// CC1101
#define GD0 2
#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 100 // [100]
// Type
#define SEND_CHAR
// #define SEND_BYTE

// Sensor pins
#ifdef SENSOR_TYPE_ds18b20
#define SENSOR_PIN_OW 3 // needs changes 2 & 3 only interrupt pins
#endif
#ifdef SENSOR_TYPE_pir
#define SENSOR_PIN_PIR 3
#endif
#ifdef SENSOR_TYPE_switch
#define SENSOR_PIN_SWITCH 3
#define MQTT_RETAINED
#endif

// FORCE GENERATE UID
// #define GEN_UID