/**
 * This module queries sensor data and publishes it using MQTT.
 * 
 * The topic and the publish interval are configured within config.ini in the following fashion:
 * 
 * [mod_sensors]
 * publish_interval_sec=XX
 * publish_topic=topicname
 * 
 * The actual topic name is created by appending the node name to publish_topic.
 * Therefore, let's say that the node name is 'cece-test' and publish_topic=/sensors then, this module will publish sensor readings
 * every publish_interval_sec to '/cece-test/sensors'. The readings are published in JSON format to keep accurate time correlation between the different values.
 */

#include "qd_sched.h"
#include "modules.h"

#ifdef HAS_MOD_SENSORS
// Measurement interval in seconds (max 65535s). Default is 1 minute
static uint16_t pub_interval_sec = 60;
// Topic to publish sensor readings to
static String sensors_topic = node_name + "/sensors";

// Publish sensor readings using MQTT. NOTE: mqttClient has been initialized in ceci_firmware.ino
void mod_sensors_publish(void)
{
  char msg[MQTT_BUFFER_SIZE];
  temperature_t temp = get_temperature();
  humidity_t hmdt = get_humidity();

  if (temp.valid) // We don't care about humidity since some sensors do not report it
  {
    snprintf(msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", temp.value, hmdt.value);

    LOG("Publishing %s to %s", msg, sensors_topic.c_str());
    mqttClient.publish(sensors_topic.c_str(), msg, 1);
  }
}

void mod_sensors_init(SPIFFSIniFile* conf)
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config file

  // This module needs the following config variables
  conf->getValue("mod_sensors", "pub_interval_sec", buf, TXT_BUF_SIZE, pub_interval_sec);
  sensors_topic = conf_getStr(conf, "mod_sensors", "base_topic") + node_name;

  sched_put_task(&mod_sensors_publish, SECS_TO_MILLIS(pub_interval_sec), false);
}

#elif
void mod_sensors_init(SPIFFSIniFile* conf){ // do nothing }
#endif
