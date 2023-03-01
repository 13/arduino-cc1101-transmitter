// Disable/enable sensors
// #define SENSOR_TYPE_si7021 "si7021"
// #define SENSOR_TYPE_ds18b20 "ds18b20"
// #define SENSOR_TYPE_bmp280 "bmp280"
// #define SENSOR_TYPE_bme680 "bme680"
// #define SENSOR_TYPE_pir "pir"
// #define SENSOR_TYPE_switch "switch"

// OUTPUT
#define VERBOSE // Always enabled
// #define VERBOSE_FW // Firmware Version
// #define VERBOSE_PC // Packet Count
// #define DEBUG

#define GD0 2
#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 100 // [100]

// Type
#define SEND_CHAR
// #define SEND_BYTE

// Deep sleep
#define DS_L 5   // long [3] 5 
#define DS_S 255 // short [255] 3
#define DS_D 100 // delay before sleep 70 [100] 500

// Sensor pins
#define SENSOR_PIN_SDA 0
#define SENSOR_PIN_SDC 2
#ifdef SENSOR_TYPE_ds18b20
#define SENSOR_PIN_OW 3
#endif
#ifdef SENSOR_TYPE_pir
#define SENSOR_PIN_PIR 3
#endif

// GIT
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif
#ifndef GIT_VERSION_SHORT
#define GIT_VERSION_SHORT "0"
#endif