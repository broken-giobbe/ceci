#include "MQTTdispatcher.h"

/**
 * MQTT utility function to (re)connect to the broker and to perform all the housekeeping stuff
 * for MQTT to work correctly
 */
void mqttKeepalive()
{
  if (!mqttClient.connected())
  {
    LOG("Connecting to server...");

    if (mqttClient.connect(node_name.c_str())) {
      LOG("Connection success.");

      // (re)subscribe to the required topics
//      mqttClient.subscribe(mqtt_config.tstat_enable_topic);
//      mqttClient.subscribe(mqtt_config.tstat_mode_topic);
//      mqttClient.subscribe(mqtt_config.tstat_target_topic);
    } else {
      LOG("Connection failed: rc=%d", mqttClient.state());
    }
  }

  mqttClient.loop();
}

/**
 * MQTT callback receiving data about the topics we have subscribed to.
 * Parameters:
 *   topic const char[] - the topic the message arrived on
 *   payload byte[] - the message payload
 *   length unsigned int - the length of the message payload
 */
void mqttCallback(const char* topic, byte* payload, unsigned int length)
{
  char payloadStr[length + 1]; // +1 for null terminator char
  snprintf(payloadStr, length+1, "%s", payload);

  // keep only the last part of the topic, the rest does not give us any info
  topic = strrchr(topic, '/') + 1;

  LOG("Received %s -> %s", topic, payloadStr);

  switch (topic[0]) {
    case 'e': // enable
      /*if(thermostat_config.heater_status != -1) {
        thermostat_config.heater_status = atoi(payloadStr);
        digitalWrite(HEATER_PORT, thermostat_config.heater_status);
      }*/
      break;

    case 'm': // mode
      if ((payloadStr[0] == 'A') || (payloadStr[0] == 'M')) {
//        tstat_mode = payloadStr[0];
        // Make sure the thermostat control loop is run next
        sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
      }
      break;
    
    case 't': // target_temp
//      target_temp = fmaxf(0.0, strtof(payloadStr, NULL));
      // Make sure the thermostat control loop is run next
      sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
      break;

    default:
      ; /* do nothing */
  }
}

void mqtt_init(SPIFFSIniFile* conf)
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config variables
  
  // static because those variables have to survive after mqtt_init exits
  static String mqtt_server = conf_getStr(conf, "global", "mqtt_server");
  static uint16_t mqtt_port = 1883;
  conf->getValue("global", "mqtt_port", buf, TXT_BUF_SIZE, mqtt_port);
  LOG("Using MQTT server %s:%d", mqtt_server.c_str(), mqtt_port);
  
  mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
  mqttClient.setCallback(mqttCallback);

  sched_put_task(&mqttKeepalive, MQTT_LOOP_RATE, false);
}
