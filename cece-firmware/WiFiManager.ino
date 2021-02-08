#include "WiFiManager.h"

void initWiFi_sta(String hostname = WiFi.macAddress())
{
    /*
     * Disable the WiFi persistence.
     * The ESP8266 will not load and save WiFi settings in the flash memory.
     * See:
     * https://www.bakke.online/index.php/2017/05/22/reducing-wifi-power-consumption-on-esp8266-part-3/
     */
    WiFi.persistent(false);
    WiFi.hostname(config_node_name);    
    // turn on the radio
    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    yield();
}

void withWiFiConnected(char* ssid, char* pswd, void (*todo)())
{
    if (WiFi.status() != WL_CONNECTED)
        WiFiConnect(ssid, pswd);

    analogWrite(WIFI_LED_PIN, WIFI_LED_BRIGHTNESS);
    (*todo)();
    digitalWrite(WIFI_LED_PIN, 1);
}

void WiFiConnect(char* ssid, char* psk)
{
    digitalWrite(WIFI_LED_PIN, 0);
    Serial.print("[WiFi] Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, psk);
    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(WIFI_LED_PIN, !digitalRead(WIFI_LED_PIN)); // let's have some bliking action!
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n[WiFi] Connected. RSSI = ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(WIFI_LED_PIN, 1);
}

void WiFiTurnOff()
{
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    yield(); // return control to the ESP tom to shutdown the WiFi
} 