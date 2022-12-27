/*
 * 
 */
#include "qd_sched.h"
#include "MQTTdispatcher.h"
#include "modules.h"

#ifdef HAS_MOD_RELAY

// Topic to publish & receive relay status changes
static String relay_pub_topic;
// MQTT message struct to send relay status updates
static mqtt_msg relay_msg;

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
      relay_msg.str_msg = "0";
      mqtt_pub_in_tasklet(relay_msg);
      break;
    case '1':
      digitalWrite(HEATER_PORT, HIGH);
      relay_msg.str_msg = "1";
      mqtt_pub_in_tasklet(relay_msg);
      break;
    default:
      LOG("Invalid relay command received: %c", payload[0]);
  }
}

void mod_relay_init(SPIFFSIniFile* conf)
{
  String relay_topic;
  // initialize relay output pin and keep it low
  pinMode(HEATER_PORT, OUTPUT);
  digitalWrite(HEATER_PORT, LOW);
  
  // This module needs the following config variables
  relay_topic = conf_getStr(conf, "mod_relay", "base_topic") + node_name;
  
  String relay_sub_topic = relay_topic + "/enable";
  mqtt_register_cb(relay_sub_topic, &relay_enable_cb);
  
  relay_pub_topic = relay_topic + "/status";
  relay_msg.topic = relay_pub_topic.c_str();
  relay_msg.str_msg = "0";
  relay_msg.retained = false;
  
  // Publish the first status, to tell that this node supports the relay
  mqtt_pub_in_tasklet(relay_msg);
  
  LOG("Loaded mod_relay.");
}

#else
void mod_relay_init(SPIFFSIniFile* conf){ /* do nothing */ }
#endif
