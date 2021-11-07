/**
 * Initialize MQTT PubSubClient and listen for MQTT events.
 * 
 * Once an event is received it is routed to the appropriate callback.
 */

#ifndef MQTT_DISPATCHER_H
#define MQTT_DISPATCHER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/*
 * Initialize MQTT
 */
void mqtt_init(SPIFFSIniFile* conf);

/*
 * Subscrive to a certain topic and register a callback for it
 */
void mqtt_register_cb(const char* topic, void* cb);

/*
 * Unsubscrive to a certain topic and unregister its callback
 */
void mqtt_unregister_cb(const char* topic);

#endif
