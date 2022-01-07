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

// anticipator setting
static float tstat_anticipator = 0.005;
// hysteresis setting
static float tstat_hysteresis = 0.5;
// Measurement interval in seconds (max 65535s). Default is 1 minute
static uint16_t tstat_sample_interval_sec = 60;

// MQTT topic used for publishing thermostat decision
static String decision_topic;
// MQTT message struct to send decision updates
static mqtt_msg decision_msg;
// MQTT message struct to send thermostat status information in a tasklet
static String status_topic;
static mqtt_msg tstat_status_msg;

/*
 * Struct for keeping thermostat state variables
 */
struct thermostat_state_t {
  char  mode;        // The current mode the thermostat is in: 'A' - auto, 'M' - manual
  float target_temp; // The target temperature to be reached
  float final_temp;  // Temperature to be reached for the output state to change
  float anticipator_temp; // Temperature offset due to the anticipator
};
static struct thermostat_state_t state = { .mode = 'M',
                                           .target_temp = 0.0,
                                           .final_temp = 0.0,
                                           .anticipator_temp = 0.0};

/*
 * Get a temperature (&humidity) reading and compute the thermostat output if needed
 */
void thermostatControlLoop(void)
{
  // Publish status (stat is static to survive function invocation)
  static char stat[TXT_BUF_SIZE];
  snprintf(stat, TXT_BUF_SIZE, "{\"mode\":\"%c\", \"target_t\":%.2f, \"final_t\":%.2f}",
                               state.mode, state.target_temp, state.final_temp);
  tstat_status_msg.str_msg = stat;
  mqtt_pub_in_tasklet(tstat_status_msg);
  
  // Continue with the function only if the thermostat mode is set to auto ('A')
  if (state.mode != 'A')
    return;
  
  temperature_t temp = get_temperature();
  LOG("Temperature: %.2f valid: %s", temp.value, (temp.valid ? "true" : "false"));

  if (!temp.valid)
    return; // Fail silently if the data is wrong
  
  if (temp.value <= state.final_temp) // actual temp is lower than expected -> heat up
  {
    state.anticipator_temp += tstat_anticipator;
    state.final_temp = state.target_temp + tstat_hysteresis - state.anticipator_temp;
    decision_msg.str_msg = "1";
    mqtt_pub_in_tasklet(decision_msg);
  } else {
    state.anticipator_temp -= tstat_anticipator;
    state.anticipator_temp = fmaxf(0, state.anticipator_temp);
    state.final_temp = state.target_temp - tstat_hysteresis - state.anticipator_temp;
    decision_msg.str_msg = "0";
    mqtt_pub_in_tasklet(decision_msg);
  }
}

/**
 * Callbacks for MQTT messages
 */
void mode_cb(byte* payload, size_t len)
{
  if (len < 1) return;
  
  switch(payload[0])
  {
    case '0':
      state.mode = '0';
      decision_msg.str_msg = "0";
      mqtt_pub_in_tasklet(decision_msg);
      break;
    case '1':
      state.mode = '1';
      decision_msg.str_msg = "1";
      mqtt_pub_in_tasklet(decision_msg);
      break;
    case 'A':
      state.mode = 'A';
      // reset state variables
      state.final_temp = state.target_temp;
      state.anticipator_temp = 0.0;
      // reschedule thermostat control loop to run immediately
      sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
      break;
    default:
      LOG("Invalid thermostat mode received: %c", payload[0]);
  }
}

void target_temp_cb(byte* payload, size_t len)
{
  if (len < 3) return; // "0.0" Ok "20" not ok
  
  char buf[len + 1] = {0};
  memcpy(buf, payload, len);
  state.target_temp = atof(buf);
  state.final_temp = state.target_temp;
  state.anticipator_temp = 0.0;
  
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
  decision_msg.topic = decision_topic.c_str();
  decision_msg.str_msg = "0";
  decision_msg.retained = false;
  
  status_topic = base_topic + node_name + "/status";
  tstat_status_msg.topic = status_topic.c_str();
  tstat_status_msg.retained = false;
  
  String mode_topic        = base_topic + node_name + "/set_mode";
  String target_temp_topic = base_topic + node_name + "/target_temperature";
  mqtt_register_cb(mode_topic, &mode_cb);
  mqtt_register_cb(target_temp_topic, &target_temp_cb);
  
  sched_put_task(&thermostatControlLoop, SECS_TO_MILLIS(tstat_sample_interval_sec), true);
  
  // Publish the first decision, to tell that this node supports mod_thermostat
  mqtt_pub_in_tasklet(decision_msg);
  
  LOG("Loaded mod_thermostat.");
}

#else
void mod_thermostat_init(SPIFFSIniFile* conf) { /* do nothing */ }
#endif
