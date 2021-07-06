#include "ConfigParser.h"

int parseConfig()
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  char str_buf[CONF_STRING_LENGTH]; // temporary placeholder for strings
  
  if (!SPIFFS.begin())
    return E_SPIFFSINIT;
  
  SPIFFSIniFile ini("/config.ini");
  if (!ini.open())
    return E_NOCONF;

  if (!ini.validate(buffer, INI_BUF_LENGTH))
    return E_INVCONF;

  // If we get here everything went smoothly. Lets's read the config variables
  ini.getValue("node", "name", buffer, INI_BUF_LENGTH, node_config.name, CONF_STRING_LENGTH);
  ini.getValue("node", "wifi_ssid", buffer, INI_BUF_LENGTH, node_config.wifi_ssid, CONF_STRING_LENGTH);
  ini.getValue("node", "wifi_psk", buffer, INI_BUF_LENGTH, node_config.wifi_psk, CONF_STRING_LENGTH);
  ini.getValue("node", "temp_offset", buffer, INI_BUF_LENGTH, node_config.temp_offset);

  ini.getValue("thermostat", "heater_status", buffer, INI_BUF_LENGTH, thermostat_config.heater_status);
  ini.getValue("thermostat", "sample_interval_sec", buffer, INI_BUF_LENGTH, thermostat_config.sample_interval_sec);
  ini.getValue("thermostat", "hysteresis", buffer, INI_BUF_LENGTH, thermostat_config.hysteresis);
  ini.getValue("thermostat", "anticipator", buffer, INI_BUF_LENGTH, thermostat_config.anticipator);

  ini.getValue("MQTT", "server", buffer, INI_BUF_LENGTH, mqtt_config.server, CONF_STRING_LENGTH);
  ini.getValue("MQTT", "port", buffer, INI_BUF_LENGTH, mqtt_config.port);
  ini.getValue("MQTT", "pub_interval_sec", buffer, INI_BUF_LENGTH, mqtt_config.pub_interval_sec);
  // the needed topics are created once from the config values and will be used everywhere
  ini.getValue("MQTT", "sensor_topic", buffer, INI_BUF_LENGTH, str_buf, CONF_STRING_LENGTH);
  snprintf(mqtt_config.sensor_topic, CONF_STRING_LENGTH, "%s/%s", str_buf, node_config.name);
  ini.getValue("MQTT", "tstat_topic", buffer, INI_BUF_LENGTH, str_buf, CONF_STRING_LENGTH);
  snprintf(mqtt_config.tstat_status_topic, CONF_STRING_LENGTH, "%s/%s/status", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_output_topic, CONF_STRING_LENGTH, "%s/%s/output", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_enable_topic, CONF_STRING_LENGTH, "%s/%s/enable", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_target_topic, CONF_STRING_LENGTH, "%s/%s/target_temp", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_mode_topic, CONF_STRING_LENGTH, "%s/%s/mode", str_buf, node_config.name);
  
  return 0;
}
