/**
 *  Parse the node configuration file stored in SPIFFS
 */
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "FS.h"
#include <SPIFFSIniFile.h>

#define CONFIG_FILENAME "/config.ini"
// Buffer length used for reading the config.ini file
#define INI_BUF_LENGTH 100
// Buffer length used for storing string values read from config.ini
#define CONF_STRING_LENGTH 80

/*
 * The following variables are read from SPIFFS during init.
 * Each struct corresponds to a section within the .ini config file
 */
// Node configuration parameters
struct node_config_t {
    // Name of the node. Will be also used to generate the MQTT topic names
    char  name[CONF_STRING_LENGTH] = "A Horse with No Name";
    // compensate for measurement offset of temperature readings
    float temp_offset = 0.0;
    // SSID of the WiFi network the node will connect to on boot
    char  wifi_ssid[CONF_STRING_LENGTH];
    // Password of the WiFi network the node will connect to on boot
    char  wifi_psk[CONF_STRING_LENGTH];
} node_config;

// Termostat configuration parameters
struct thermostat_config_t {
    // Initial status for the heating element. If set to -1 then, no heating element exists
    int  heater_status = -1;
    // Controls how often the thermostat routine is run
    uint16_t sample_interval_sec = 60;
    // Hysteresis in degrees celsius applied by the control algorithm
    float    hysteresis = 0.5;
    // Dampens temperature variations due to hysteresis and thermal mass
    float    anticipator = 0.005;
} thermostat_config;

// MQTT-related configuration parameters
struct mqtt_config_t {
    // MQTT server hostname or IP address
    char     server[CONF_STRING_LENGTH];
    // MQTT server port
    int      port = 1883;
    // Measurement interval in seconds (max 65535s)
    uint16_t pub_interval_sec = 60;
    // Topic used to publish sensor readings
    char     sensor_topic[CONF_STRING_LENGTH];
    // Topic used to publish heater status
    char     tstat_status_topic[CONF_STRING_LENGTH];
    // Topic to listen for remote heating element control
    char     tstat_enable_topic[CONF_STRING_LENGTH];
    // Topic to output the outcome of the thermostat control routine
    char     tstat_output_topic[CONF_STRING_LENGTH];
    // Topic to listen for setting the mode of the thermostat: 'A' = auto, 'M' = manual
    char     tstat_mode_topic[CONF_STRING_LENGTH];
    // Topic to listen for target temperature setting
    char     tstat_target_topic[CONF_STRING_LENGTH];
} mqtt_config;

// Error initializing the SPIFFS partition
#define E_SPIFFSINIT -1
// No config file found
#define E_NOCONF -2
// Invalid config file found
#define E_INVCONF -3

/*
 * Parse the configuration file.
 * 
 * Returns 0 on success or one of the errors above on failure
 */
int parseConfig();

#endif
