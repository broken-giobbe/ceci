#ifndef SYSCONFIG_H
#define SYSCONFIG_H

/**
 * Config for mod_sensors.
 * 
 * If HAS_MOD_SENSORS is defined the application will embed and load this module.
 */
#define HAS_MOD_SENSORS

/**
 * Config for mod_thermostat.
 * 
 * If HAS_MOD_THERMOSTAT is defined the application will embed and load this module.
 */
#define HAS_MOD_THERMOSTAT

/**
 * Ambient sensor driver options
 */
// Use the Bosch BME680 temperature/humidity/pressure/gas sensor
//#define BME680_TEMP_SENSOR
 
// Use the HTU2X temperature/humidity sensor
#define HTU2X_TEMP_SENSOR

// Use the LM75 temperature sensor and set the I2C address
//#define LM75_TEMP_SENSOR
#define LM75_SENSOR_ADDRESS 0x4F

/*
 * Default size in bytes for various text buffers. Set it big enough ;)
 */
#define TXT_BUF_SIZE 128

/*
 * Name for the node config file in the SPIFFS
 */
#define CONFIG_FILENAME "/config-soggiorno.ini"

#endif
