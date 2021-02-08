/**
 * Functions to control the WiFi of the ESP8266 module
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>

// Use this LED to tell the user we're busy 
#define WIFI_LED_PIN LED_BUILTIN
// This is the brightness for such LED.
// NOTE: if using BUILTIN_LED the values are reversed! 1023 = off, 0 = full brightness
#define WIFI_LED_BRIGHTNESS 940

/**
 * Initialize the WiFi radio, turn it on, put it in station mode and set the hostname.
 * If no hostname is specified, the MAC address will be used instead.
 */
void initWiFi_sta(String hostname);

/**
 * Run the function passed as a parameter, ensuring wifi is connected
 */
void withWiFiConnected(char* ssid, char* psk, void (*todo)());

/**
 * Manually connect to the WiFi network
 */
void WiFiConnect(char* ssid, char* psk);

/**
 * Turn off wifi radio
 */
void WiFiTurnOff();

#endif