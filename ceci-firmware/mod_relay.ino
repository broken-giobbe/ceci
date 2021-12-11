/*
 * 
 */
#include "MQTTdispatcher.h"
#include "modules.h"

#ifdef HAS_MOD_RELAY

// Topic to publish & receive relay status changes
static String relay_pub_topic;

/**
 * Callbacks for MQTT messages
 */
void relay_enable_cb(byte* payload, size_t len)
{
  if (len < 1) return;

  switch (payload[0])
  {
    case '0':
      digitalWrite(HEATER_PORT, LOW);
      mqttClient.publish(relay_pub_topic.c_str(), "0");
      break;
    case '1':
      digitalWrite(HEATER_PORT, HIGH);
      mqttClient.publish(relay_pub_topic.c_str(), "1");
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
  relay_pub_topic = relay_topic + "/status";
  
  String relay_sub_topic = relay_topic + "/enable";
  mqtt_register_cb(relay_sub_topic, &relay_enable_cb);
  
  // Publish the first status, to tell that this node supports the relay
  mqttClient.publish(relay_pub_topic.c_str(), "0");
  
  LOG("Loaded mod_relay.");
}

#else
void mod_relay_init(SPIFFSIniFile* conf){ /* do nothing */ }
#endif
