/**
 * This header file acts both as a config file for the whole project and as a global description of the system.
 * 
 * A cece can be made up of four high-level modules, all optional. At least one of them should be present to obtain any meaningful functionality.
 * Such modules are:
 *  - mod_sensors    -> Responsible for querying the ambient sensor(s) at regular intervals and publishing the measurements using MQTT
 *  - mod_thermostat -> Regularly reflects on the local temperature to decide whether the heating system should be turned on or not.
 *                      Also uses MQTT to publish its output state
 *  - mod_relay      -> Enables or disables a relay depending on data received on its relative MQTT topic
 *  - mod_UI         -> Presents the users with a screen and buttons. It allows to present the sensor readings and control the thermostat using buttons.
 *                      Requires mod_thermostat to be of any use.
 * 
 * Once a module is enabled, its parameters are read from an .ini config file. Each section of the file corresponds to the module to be cofigured.
 * There is also a global section for the WiFi network parameters and any other config variable that affects all the modules.
 * 
 * Except for mod_relay that controls the relay directly, all modules use the following low(ish)-level drivers to interact with the hardware.
 * Their role is to arbitrate the access to the respective hardware resource.
 *  - AmbientSensor  -> to read ambient temperature, humidity, pressure whatever and present them nicely
 *  - MQTTdispatcher -> to register callbacks for the various topics each module subscribes to
 *  - qd_sched       -> simple task scheduler for all the repetitive stuff each component needs to run periodically
 *  
 *  Below those drivers are the specific Arduino libraries/drivers for each component.
 *  
 *  The high-level options of the drivers, such as what is connected where is configured here, more in-depth stuff is configured in the
 *  header file for each driver. Config variables here are updated when the hardware changes.
 *  Configuration variables related to actual behaviour, not dependent on the hardware go inside their relative source files.
 */

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
 * Config for mod_relay.
 * 
 * If HAS_MOD_RELAY is defined the application will embed and load this module.
 */
#define HAS_MOD_RELAY

// Port where the heating control realy is connected
// D7 for hardware version 1.x
// D8 for hardware version 2.x
#define HEATER_PORT D8

/**
 * Config for mod_UI.
 * 
 * If HAS_MOD_UI is defined the application will embed and load this module.
 */
#define HAS_MOD_UI

// OLED Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_RESET -1

// Which pin is the UP button attached to?
#define UP_BUTTON_GPIO D5

// Which pin is the DOWN button attached to?
#define DN_BUTTON_GPIO D3

 // OLED display width, in pixels
#define SCREEN_WIDTH  128

 // OLED display height, in pixels
#define SCREEN_HEIGHT 64

/**
 * Ambient sensor driver options
 */
// Use the Bosch BME680 temperature/humidity/pressure/gas sensor
//#define BME680_TEMP_SENSOR
 
// Use the HTU2X temperature/humidity sensor
//#define HTU2X_TEMP_SENSOR

// Use the LM75 temperature sensor and set the I2C address
#define LM75_TEMP_SENSOR
#define LM75_SENSOR_ADDRESS 0x4F

/*
 * Default size in bytes for various text buffers. Set it big enough ;)
 */
#define TXT_BUF_SIZE 128

/*
 * Name for the node config file in the SPIFFS
 */
#define CONFIG_FILENAME "/config.ini"

#endif
