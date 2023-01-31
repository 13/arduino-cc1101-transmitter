// Disable/enable sensors
#define SENSOR_TYPE_si7021 "si7021"
// #define SENSOR_TYPE_ds18b20 "ds18b20"
// #define SENSOR_TYPE_bmp280 "bmp280"
// #define SENSOR_TYPE_bme680 "bme680"
// #define SENSOR_TYPE_pir "pir"
// #define SENSOR_TYPE_switch "switch"

// OUTPUT
// #define VERBOSE
// #define DEBUG

// Deepsleep
#define DS_L 3   // long
#define DS_S 255 // short
#define DS_D 100 // delay before sleep 70 100 '500' 1000

// Sensorpins
#define SENSOR_PIN_SDA 0
#define SENSOR_PIN_SDC 2
#define SENSOR_PIN_OW 3
#ifdef SENSOR_TYPE_pir
#define SENSOR_PIN_PIR 5
#endif