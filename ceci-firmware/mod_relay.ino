/*
 * 
 */
#include "qd_sched.h"
#include "MQTTdispatcher.h"
#include "modules.h"

#ifdef HAS_MOD_RELAY

// Topic to publish & receive relay status changes
static String relay_pub_topic;

/**
 * Callback for MQTT messages
 */
void relay_enable_cb(byte* payload, size_t len)
{
  if (len < 1) return;

  switch (payload[0])
  {
    case '0':
      digitalWrite(HEATER_PORT, LOW);
      sched_put_rt_tasklet(&relay_status_pub, (void*)"0", 0L);
      break;
    case '1':
      digitalWrite(HEATER_PORT, HIGH);
      sched_put_rt_tasklet(&relay_status_pub, (void*)"1", 0L);
      break;
    default:
      LOG("Invalid relay command received: %c", payload[0]);
  }
}

/*
 * Tasklet to publish mqtt data
 */
void relay_status_pub(void* state)
{
  if(!mqttClient.connected())
  {
    sched_put_rt_tasklet(&relay_status_pub, state, 1000L); // retry in 1s
    return;
  }
  
  mqttClient.publish(relay_pub_topic.c_str(), (char*)state);
}

void mod_relay_init(SPIFFSIniFile* conf)
{
  String relay_topic;
  // initialize relay output pin and keep it low
  pinMode(HEATER_PORT, OUTPUT);
  digitalWrite(HEATER_PORT, LOW);
  
  // This module needs the following config variables
  relay_topic = conf_getStr(conf, "mod_relay", "base_topic") + node_name;
  relay_pub_topic = relay_topic + "/status";
  
  String relay_sub_topic = relay_topic + "/enable";
  mqtt_register_cb(relay_sub_topic, &relay_enable_cb);
  
  // Publish the first status, to tell that this node supports the relay
  sched_put_rt_tasklet(&relay_status_pub, (void*)"0", 0L);
  
  LOG("Loaded mod_relay.");
}

#else
void mod_relay_init(SPIFFSIniFile* conf){ /* do nothing */ }
#endif
