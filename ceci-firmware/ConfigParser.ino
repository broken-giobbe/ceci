#include "ConfigParser.h"

SPIFFSIniFile* open_config_file(void)
{
  SPIFFSIniFile* ini = new SPIFFSIniFile(CONFIG_FILENAME);
  char buf[INI_BUF_LENGTH];
   
  if (!SPIFFS.begin())
    return NULL;
  
  ini->open();
  
  if (ini->getError() != SPIFFSIniFile::errorNoError)
    ini->validate(buf, INI_BUF_LENGTH);
  
  return ini;
}

float config_get_temperature_offset(SPIFFSIniFile* ini)
{
  float offset = 0;
  char buf[INI_BUF_LENGTH];
  
  ini->getValue("node", "temp_offset", buf, INI_BUF_LENGTH, offset);
  return offset;
}

void close_config_file(SPIFFSIniFile *ini)
{
  delete ini; // delete calls the destructor to also close the file
}

void parse_config(SPIFFSIniFile* ini)
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  char str_buf[CONF_STRING_LENGTH]; // temporary placeholder for strings
  
  // If we get here everything went smoothly. Lets's read the config variables
  ini->getValue("node", "name", buffer, INI_BUF_LENGTH, node_config.name, CONF_STRING_LENGTH);
  ini->getValue("node", "wifi_ssid", buffer, INI_BUF_LENGTH, node_config.wifi_ssid, CONF_STRING_LENGTH);
  ini->getValue("node", "wifi_psk", buffer, INI_BUF_LENGTH, node_config.wifi_psk, CONF_STRING_LENGTH);

  ini->getValue("thermostat", "heater_status", buffer, INI_BUF_LENGTH, thermostat_config.heater_status);
  ini->getValue("thermostat", "sample_interval_sec", buffer, INI_BUF_LENGTH, thermostat_config.sample_interval_sec);
  ini->getValue("thermostat", "hysteresis", buffer, INI_BUF_LENGTH, thermostat_config.hysteresis);
  ini->getValue("thermostat", "anticipator", buffer, INI_BUF_LENGTH, thermostat_config.anticipator);

  ini->getValue("MQTT", "server", buffer, INI_BUF_LENGTH, mqtt_config.server, CONF_STRING_LENGTH);
  ini->getValue("MQTT", "port", buffer, INI_BUF_LENGTH, mqtt_config.port);
  ini->getValue("MQTT", "pub_interval_sec", buffer, INI_BUF_LENGTH, mqtt_config.pub_interval_sec);
  // the needed topics are created once from the config values and will be used everywhere
  ini->getValue("MQTT", "sensor_topic", buffer, INI_BUF_LENGTH, str_buf, CONF_STRING_LENGTH);
  snprintf(mqtt_config.sensor_topic, CONF_STRING_LENGTH, "%s/%s", str_buf, node_config.name);
  ini->getValue("MQTT", "tstat_topic", buffer, INI_BUF_LENGTH, str_buf, CONF_STRING_LENGTH);
  snprintf(mqtt_config.tstat_status_topic, CONF_STRING_LENGTH, "%s/%s/status", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_output_topic, CONF_STRING_LENGTH, "%s/%s/output", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_enable_topic, CONF_STRING_LENGTH, "%s/%s/enable", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_target_topic, CONF_STRING_LENGTH, "%s/%s/target_temp", str_buf, node_config.name);
  snprintf(mqtt_config.tstat_mode_topic, CONF_STRING_LENGTH, "%s/%s/mode", str_buf, node_config.name);
}

const char* config_get_error_str(SPIFFSIniFile* ini)
{
  switch (ini->getError()) {
  case SPIFFSIniFile::errorNoError:
    return "no error";
  case SPIFFSIniFile::errorFileNotFound:
    return "file not found";
  case SPIFFSIniFile::errorFileNotOpen:
    return "file not open";
  case SPIFFSIniFile::errorBufferTooSmall:
    return "buffer too small";
  case SPIFFSIniFile::errorSeekError:
   return "seek error";
  case SPIFFSIniFile::errorSectionNotFound:
    return "section not found";
  case SPIFFSIniFile::errorKeyNotFound:
    return "key not found";
  case SPIFFSIniFile::errorEndOfFile:
    return "end of file";
  case SPIFFSIniFile::errorUnknownError:
    return "unknown error";
  default:
    return "unknown error value";
  }
}
