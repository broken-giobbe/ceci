/**
 * This module is responsible for handling the thermostat control loop.
 * 
 * Once every sample_interval_sec the temperature is requested to AmbientSensor and a new decision is made 
 * whether or not the heating system has to be turned on or off.
 * 
 * Its parameters are configured from config.ini in the module filesystem. The parameters are:
 *  [mod_thermostat]
 *  sample_interval_sec=XX
 *  base_topic=topicname
 *  hysteresis=XX
 *  anticipator=XX
 *  
 * The actual topic name is created by appending the node name to base_topic.
 * Therefore, let's say that the node name is 'cece-test' and base_topic=heating/ then, this module will listen to the folowing topics:
 *  heating/cece-test/set_mode           -> To turn the thermostat to always on, always off or auto
 *  heating/cece-test/target_temperature -> To set the target temperature for the room
 * This module will also publish to:
 *  heating/cece-test/status   -> Summary of thermostat status in json format (updated when data is received to the topics above)
 *  heating/cece-test/decision -> 1 if the heater must be turned on, 0 if turned off
 */
#include "qd_sched.h"
#include "MQTTdispatcher.h"
#include "modules.h"

#ifdef HAS_MOD_THERMOSTAT

// The current mode the thermostat is in: 'A' - auto, 'M' - manual
static char tstat_mode = 'M';
// the target temperature to be reached
static float tstat_target = 0.0;

// anticipator setting
static float tstat_anticipator = 0.005;
// hysteresis setting
static float tstat_hysteresis = 0.5;
// Measurement interval in seconds (max 65535s). Default is 1 minute
static uint16_t tstat_sample_interval_sec = 60;

// MQTT topic used for publishing thermostat decision
String decision_topic;
// MQTT topic used for publishing thermostat status information
String status_topic;

/*
 * Get a temperature (&humidity) reading and compute the thermostat output if needed
 */
void thermostatControlLoop(void)
{
  // I know it's ugly but the thermostat has to keep some state
  static float old_target_temp = tstat_target;
  static float final_temp = tstat_target;
  static float anticipator_temp = 0;
  
  // Publish status
  String stat = "{\"mode\":" + String(tstat_mode); 
  stat +=      ", \"target_t\":" + String(tstat_target);
  stat +=      ", \"final_t\":" + String(final_temp) + "}";
  mqttClient.publish(status_topic.c_str(), stat.c_str());
  
  // Continue with the function only if the thermostat mode is set to auto ('A')
  if (tstat_mode != 'A')
    return;
  
  temperature_t temp = get_temperature();
  
  LOG("Temperature: %.2f valid: %s", temp.value, (temp.valid ? "true" : "false"));

  if (!temp.valid)
    return; // Fail silently if the data is wrong

  // if target temperature has changed reset the state
  if (old_target_temp != tstat_target) {
    final_temp = tstat_target;
    anticipator_temp = 0;
    old_target_temp = tstat_target;
  }

  LOG("final_temp = %f", final_temp);
  
  if (temp.value < final_temp) // actual temp is lower than expected -> heat up
  {
    anticipator_temp += tstat_anticipator;
    final_temp = tstat_target + tstat_hysteresis - anticipator_temp;
    mqttClient.publish(decision_topic.c_str(), "1");
  } else {
    anticipator_temp -= tstat_anticipator;
    anticipator_temp = fmaxf(0, anticipator_temp);
    final_temp = tstat_target - tstat_hysteresis - anticipator_temp;
    mqttClient.publish(decision_topic.c_str(), "0");
  }
}

/**
 * Callbacks for MQTT messages
 */
void mode_cb(byte* payload, size_t len)
{
  if (len < 1) return;
  tstat_mode = payload[0]; // ignore the rest

  // reschedule thermostat control loop to run immediately
  sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
}

void target_temp_cb(byte* payload, size_t len)
{
  if (len < 3) return; // "0.0" Ok "20" not ok
  
  char buf[len + 1] = {0};
  memcpy(buf, payload, len);
  tstat_target = atof(buf);

  // reschedule thermostat control loop to run immediately
  sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
}

void mod_thermostat_init(SPIFFSIniFile* conf)
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config file
  
  conf->getValue("mod_thermostat", "sample_interval_sec", buf, TXT_BUF_SIZE, tstat_sample_interval_sec);
  conf->getValue("mod_thermostat", "hysteresis", buf, TXT_BUF_SIZE, tstat_hysteresis);
  conf->getValue("mod_thermostat", "anticipator", buf, TXT_BUF_SIZE, tstat_anticipator);

  String base_topic = conf_getStr(conf, "mod_thermostat", "base_topic");

  decision_topic = base_topic + node_name + "/decision";
  status_topic   = base_topic + node_name + "/status";
  
  String mode_topic        = base_topic + node_name + "/set_mode";
  String target_temp_topic = base_topic + node_name + "/target_temperature";
  mqtt_register_cb(mode_topic, &mode_cb);
  mqtt_register_cb(target_temp_topic, &target_temp_cb);
  
  sched_put_task(&thermostatControlLoop, SECS_TO_MILLIS(tstat_sample_interval_sec), true);
  LOG("Loaded mod_thermostat.");
}

#else
void mod_thermostat_init(SPIFFSIniFile* conf) { /* do nothing */ }
#endif
