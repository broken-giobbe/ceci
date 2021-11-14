#include "MQTTdispatcher.h"

/**
 * This structure represents a topic-callback association.
 */
struct dispatch_table_t {
  String topic;  // string containing the topic name the callback refers to
  void (*cb_func)(byte*, size_t); // callback function: byte* is for the payload, size_t for the size of received data
}dispatchTable[DISPATCH_TABLE_SIZE] = {"", 0};

/**
 * Since the topics to subscribe to can change at any time, we keep track wether we should
 * (re)subscribe to the topics in the dispatch_table by means of this variable.
 * 
 * Set to true to force subscribe on first run.
 */
bool needs_resubscribe = true;

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
      needs_resubscribe = true; // Upon connection to the server we always need to resubscribe
    } else {
      LOG("Connection failed: rc=%d", mqttClient.state());
      return; // retry next time, there's no point in continuing
    }
  }

  if(needs_resubscribe == true)
  {
    mqttClient.unsubscribe("#");
    
    for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++)
    {
      if(dispatchTable[i].cb_func != 0) {
        mqttClient.subscribe(dispatchTable[i].topic.c_str());
        LOG("Subscribed to: %s", dispatchTable[i].topic.c_str());
      }
    }
    
    needs_resubscribe = false;
  }
 
  mqttClient.loop();
}

int mqtt_register_cb(String topic, void (*cb_func)(byte*, size_t))
{
  size_t emptyIdx = 0;
  
  // search for the first empty entry in the dispatch table
  while(dispatchTable[emptyIdx].cb_func != 0)
  {
    emptyIdx++;
    if(emptyIdx == DISPATCH_TABLE_SIZE)
      return -1;
  }

  dispatchTable[emptyIdx].topic = topic;
  dispatchTable[emptyIdx].cb_func = cb_func;
  needs_resubscribe = true;
  return emptyIdx;
}

/**
 * MQTT callback receiving data about the topics we have subscribed to.
 * Parameters:
 *   topic const char[] - the topic the message arrived on
 *   payload byte[]     - the message payload
 *   length size_t      - the length of the message payload
 */
void mqttCallback(const char* topic, byte* payload, size_t len)
{
  String topicStr = topic;
  LOG("Received %s -> %.*s", topic, len, payload);

  for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++)
  {
    if ((dispatchTable[i].cb_func != 0) && (dispatchTable[i].topic == topicStr))
      (*dispatchTable[i].cb_func)(payload, len);
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
