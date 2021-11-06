#include "ConfigParser.h"

void parse_config(SPIFFSIniFile* ini)
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  char str_buf[CONF_STRING_LENGTH]; // temporary placeholder for strings
  
  // If we get here everything went smoothly. Lets's read the config variables
  ini->getValue("thermostat", "heater_status", buffer, INI_BUF_LENGTH, thermostat_config.heater_status);
  ini->getValue("thermostat", "sample_interval_sec", buffer, INI_BUF_LENGTH, thermostat_config.sample_interval_sec);
  ini->getValue("thermostat", "hysteresis", buffer, INI_BUF_LENGTH, thermostat_config.hysteresis);
  ini->getValue("thermostat", "anticipator", buffer, INI_BUF_LENGTH, thermostat_config.anticipator);

  ini->getValue("MQTT", "tstat_topic", buffer, INI_BUF_LENGTH, str_buf, CONF_STRING_LENGTH);
  snprintf(mqtt_config.tstat_status_topic, CONF_STRING_LENGTH, "%s/%s/status", str_buf, node_name);
  snprintf(mqtt_config.tstat_output_topic, CONF_STRING_LENGTH, "%s/%s/output", str_buf, node_name);
  snprintf(mqtt_config.tstat_enable_topic, CONF_STRING_LENGTH, "%s/%s/enable", str_buf, node_name);
  snprintf(mqtt_config.tstat_target_topic, CONF_STRING_LENGTH, "%s/%s/target_temp", str_buf, node_name);
  snprintf(mqtt_config.tstat_mode_topic, CONF_STRING_LENGTH, "%s/%s/mode", str_buf, node_name);
}
