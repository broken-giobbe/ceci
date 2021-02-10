#include "ConfigParser.h"

int parseConfig()
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  
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
  
  ini.getValue("wifi", "ssid", buffer, INI_BUF_LENGTH, config_wifi_ssid, VAL_BUF_LENGTH);
  ini.getValue("wifi", "psk", buffer, INI_BUF_LENGTH, config_wifi_psk, VAL_BUF_LENGTH);
  
  ini.getValue("MQTT", "server", buffer, INI_BUF_LENGTH, config_mqtt_server, VAL_BUF_LENGTH);
  ini.getValue("MQTT", "port", buffer, INI_BUF_LENGTH, config_mqtt_port);
  ini.getValue("MQTT", "topic", buffer, INI_BUF_LENGTH, config_mqtt_topic, VAL_BUF_LENGTH);
  
  return 0;
}
