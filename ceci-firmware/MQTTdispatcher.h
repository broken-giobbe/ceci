/**
 * Initialize MQTT PubSubClient and listen for MQTT events.
 * (server address and port are set in config.ini)
 * 
 * Once an event is received it is routed to the appropriate callback.
 */

#ifndef MQTT_DISPATCHER_H
#define MQTT_DISPATCHER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Rate in milliseconds at which the MQTT function loop() is called
// used for keepalive and to react to subscriptions.
#define MQTT_LOOP_RATE 1000

// Size of the buffer for reading/sending MQTT messages
#define MQTT_BUFFER_SIZE 64

// Maximum size for the dispatch table
#define DISPATCH_TABLE_SIZE 5

/*
 * Initialize MQTT
 */
void mqtt_init(SPIFFSIniFile* conf);

/*
 * Subscrive to a certain topic and register a callback for it.
 * 
 * Returns a value >=0 on success, -1 otherwise
 */
int mqtt_register_cb(String topic, void (*cb_func)(byte*, size_t));

/*
 * Unsubscrive to a certain topic and unregister its callback
 */
void mqtt_unregister_cb(String topic);

#endif
