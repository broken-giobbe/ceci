/**
 *  Parse the node configuration file stored in SPIFFS
 */
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "FS.h"
#include <SPIFFSIniFile.h>


// Buffer length used for reading the config.ini file
#define INI_BUF_LENGTH 100
// Buffer length used for storing string values read from config.ini
#define CONF_STRING_LENGTH 80

/*
 * The following variables are read from SPIFFS during init.
 * Each struct corresponds to a section within the .ini config file
 */

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

/*
 * Parse the configuration file.
 * 
 * If some of the config keys are not prtesent in the file, the default value is used
 */
void parse_config(SPIFFSIniFile* ini);

#endif
