/**
 *  Parse the node configuration file stored in SPIFFS
 */
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "FS.h"
#include <SPIFFSIniFile.h>

#define CONFIG_FILENAME "/config.ini"
// Buffer length used for reading the config.ini file
#define INI_BUF_LENGTH 80
// Buffer length used for storing string values read from config.ini
#define VAL_BUF_LENGTH 50

/*
 * The following variables are read from SPIFFS during init.
 */
uint16_t config_meas_interval_min = 1; // Measurement interval in minutes (max 65535' = 18.2h)
char  config_node_name[VAL_BUF_LENGTH] = "A Horse with No Name";
float config_temp_offset = 0.0;
char  config_sensor_topic[VAL_BUF_LENGTH];

int   config_heater_status = -1; // -1 = No heater, 0 = off, 1 = on
float config_tstat_hysteresis = 0.5;
float config_tstat_anticipator = 0.005;
char  config_tstat_status_topic[VAL_BUF_LENGTH];
char  config_tstat_target_temp_topic[VAL_BUF_LENGTH];
char  config_tstat_mode_topic[VAL_BUF_LENGTH];
char  config_tstat_actual_temp_topic[VAL_BUF_LENGTH];

char  config_wifi_ssid[VAL_BUF_LENGTH];
char  config_wifi_psk[VAL_BUF_LENGTH];

char  config_mqtt_server[VAL_BUF_LENGTH];
int   config_mqtt_port = 1883;

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
