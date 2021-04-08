#include "ConfigParser.h"

int parseConfig()
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  char str_buf[VAL_BUF_LENGTH]; // temporary placeholder for strings

  if (!SPIFFS.begin())
    return E_SPIFFSINIT;
  
  SPIFFSIniFile ini("/config.ini");
  if (!ini.open())
    return E_NOCONF;

  if (!ini.validate(buffer, INI_BUF_LENGTH))
    return E_INVCONF;

  // If we get here everything went smoothly. Lets's read the config variables
  ini.getValue("node", "name", buffer, INI_BUF_LENGTH, config_node_name, VAL_BUF_LENGTH);
  ini.getValue("node", "meas_interval_min", buffer, INI_BUF_LENGTH, config_meas_interval_min);
  ini.getValue("node", "temp_offset", buffer, INI_BUF_LENGTH, config_temp_offset);
  ini.getValue("node", "heater_status", buffer, INI_BUF_LENGTH, config_heater_status);
  ini.getValue("node", "tstat_hysteresis", buffer, INI_BUF_LENGTH, config_tstat_hysteresis);
  ini.getValue("node", "tstat_anticipator", buffer, INI_BUF_LENGTH, config_tstat_anticipator);

  ini.getValue("wifi", "ssid", buffer, INI_BUF_LENGTH, config_wifi_ssid, VAL_BUF_LENGTH);
  ini.getValue("wifi", "psk", buffer, INI_BUF_LENGTH, config_wifi_psk, VAL_BUF_LENGTH);
  
  ini.getValue("MQTT", "server", buffer, INI_BUF_LENGTH, config_mqtt_server, VAL_BUF_LENGTH);
  ini.getValue("MQTT", "port", buffer, INI_BUF_LENGTH, config_mqtt_port);
  
  // the needed topics are created once from the config values and will be used everywhere
  ini.getValue("MQTT", "sensor_topic", buffer, INI_BUF_LENGTH, str_buf, VAL_BUF_LENGTH);
  snprintf(config_sensor_topic, VAL_BUF_LENGTH, "%s/%s", str_buf, config_node_name);
  
  if (config_heater_status != -1)
  {
    ini.getValue("MQTT", "tstat_topic", buffer, INI_BUF_LENGTH, str_buf, VAL_BUF_LENGTH);
    snprintf(config_tstat_status_topic, VAL_BUF_LENGTH, "%s/%s/status", str_buf, config_node_name);
    snprintf(config_tstat_target_temp_topic, VAL_BUF_LENGTH, "%s/%s/target_temp", str_buf, config_node_name);
    snprintf(config_tstat_mode_topic, VAL_BUF_LENGTH, "%s/%s/mode", str_buf, config_node_name);
    snprintf(config_tstat_actual_temp_topic, VAL_BUF_LENGTH, "%s/%s/actual_temp", str_buf, config_node_name);
  }
  return 0;
}
