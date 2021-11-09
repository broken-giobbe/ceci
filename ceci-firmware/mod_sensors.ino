/**
 * This module queries sensor data and publishes it using MQTT.
 * 
 * The topic and the publish interval are configured within config.ini in the following fashion:
 * 
 * [mod_sensors]
 * publish_interval_sec=XX
 * base_topic=topicname
 * 
 * The actual topic name is created by appending the node name to base_topic.
 * Therefore, let's say that the node name is 'cece-test' and base_topic=sensors/ then, this module will publish sensor readings
 * every publish_interval_sec to 'sensors/cece-test'. The readings are published in JSON format to keep accurate time correlation between the different values.
 */

#include "qd_sched.h"
#include "modules.h"

#ifdef HAS_MOD_SENSORS
// Measurement interval in seconds (max 65535s). Default is 1 minute
static uint16_t pub_interval_sec = 60;
// Topic to publish sensor readings to
static String sensors_topic;

// Publish sensor readings using MQTT. NOTE: mqttClient has been initialized in ceci_firmware.ino
void mod_sensors_publish(void)
{
  String msg = "{";
  temperature_t temp = get_temperature();
  humidity_t hmdt = get_humidity();
  //pressure_t
  //quality_t
  
  // The JSON string to be published is built according to the sensor's capabilities
  if (!temp.valid)
    return; // at least the temperature should be valid otherwhise it makes no sense to publish anything

  msg += "\"temp\":" + String(temp.value, 1);

  if (hmdt.valid)
    msg += ", \"rhum\":" + String(hmdt.value, 1);

  msg += "}";
  
  LOG("Publishing %s to %s", msg.c_str(), sensors_topic.c_str());
  mqttClient.publish(sensors_topic.c_str(), msg.c_str(), 1);
}

void mod_sensors_init(SPIFFSIniFile* conf)
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config file

  // This module needs the following config variables
  conf->getValue("mod_sensors", "pub_interval_sec", buf, TXT_BUF_SIZE, pub_interval_sec);
  sensors_topic = conf_getStr(conf, "mod_sensors", "base_topic") + node_name;

  sched_put_task(&mod_sensors_publish, SECS_TO_MILLIS(pub_interval_sec), false);
  LOG("Loaded mod_sensors.");
}

#elif
void mod_sensors_init(SPIFFSIniFile* conf){ /* do nothing */ }
#endif
