/**
 * TODO: Describe me 
**/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <elapsedMillis.h>
#include <Wire.h>
#include "SparkFunHTU21D.h"
/* 
 * This is needed for using light sleep. See also:
 *    https://community.blynk.cc/t/esp8266-light-sleep/13584/16 
 *    https://github.com/esp8266/Arduino/issues/1381
 */
extern "C" {
#include "user_interface.h"
}

#include "ConfigParser.h"

// Constant used to convert minutes to milliseconds
#define MINS_TO_MILLIS (60*1000)
// Baud rate for the UART port
#define UART_BAUD_RATE 115200
// Builtin LED brightness when on
#define LED_ON_BRIGHTNESS_PERCENT 8

// HTU21D temperature & Humidity sensor
HTU21D myHumidity;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#define MQTT_BUFFER_SIZE 50

/* 
 *  The controls for the builtin LED are reversed.
 *  When the pin is set to HIGH the LED is off and vice-versa.
 * This subroutine makes it easy to set the brightness appropriately
 */
inline void setBuiltinLEDBrightness(int percent) __attribute__((always_inline));
void setBuiltinLEDBrightness(int percent)
{
  int pwm_val = 1023 - (1023*percent)/100;
  analogWrite(BUILTIN_LED, pwm_val);
}

/**
 * Read sensor data and send it to the broker using MQTT
 */
void readDataAndSend()
{
  char msg[MQTT_BUFFER_SIZE];
  
  // Take humidity and temperature reading
  float humd = myHumidity.readHumidity();
  float temp = myHumidity.readTemperature();

  snprintf (msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", temp, humd);
  Serial.print("[MQTT] publish to: ");
  Serial.print(config_mqtt_topic);
  Serial.print(", payload: ");
  Serial.println(msg);
                             
  mqttClient.publish(config_mqtt_topic, msg, 1);
}

/**
 * MQTT utility function to (re)connect to the broker
 */
void mqttReconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("[MQTT] Attempting connection...");

    if (mqttClient.connect(config_node_name)) {
      Serial.println(" Success.");
      // (re)subscribe to the required topics
      //client.subscribe("inTopic");
    } else {
      Serial.println(" Fail: rc=");
      Serial.println(mqttClient.state());
      Serial.println("[MQTT] Will try again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{ 
  // initialize onboard LED as output and turn it on
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  // initialize serial
  Serial.begin(UART_BAUD_RATE);
  Serial.println("");

  // read config.ini file from SPIFFS and initialize variables
  int retval = parseConfig();
  switch (retval) {
    case 0:
      Serial.println("[Config] OK.");
      break;
    case E_SPIFFSINIT:
      Serial.println("[Config] Invalid config file found. Aborting setup().");
      while (1) yield(); // avoid triggering the WDT: https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
      break;
    case E_NOCONF:
      Serial.println("[Config] No config file found. Aborting setup().");
      while (1) yield();
      break;
    case E_INVCONF:
      Serial.println("[Config] Invalid config file found. Aborting setup().");
      while (1) yield();
      break;
    default:
      Serial.println("[Config] WTF!? Something bad happened. Aborting setup().");
      while (1) yield();
  }
  
  // initialize wifi
  // Force the ESP into client-only mode (otherwhise light sleep cannot be enabled)
  WiFi.mode(WIFI_STA); 
  WiFi.hostname(config_node_name);

  // Enable light sleep
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  Serial.print("[WiFi] Connecting to ");
  Serial.print(config_wifi_ssid);
  WiFi.begin(config_wifi_ssid, config_wifi_psk);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n[WiFi] Connected. RSSI = ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize MQTT
  mqttClient.setServer(config_mqtt_server, config_mqtt_port);
  
  // initialize HTU21D temp sensor connected as in the following code
  // https://github.com/enjoyneering/HTU21D/blob/master/examples/HTU21D_Demo/HTU21D_Demo.ino
  myHumidity.begin();
}

void loop()
{
  elapsedMillis elapsed = 0; // count the time the node spent sending data for more accurate sleeping intervals
  
  setBuiltinLEDBrightness(LED_ON_BRIGHTNESS_PERCENT);

  if ((WiFi.status() == WL_CONNECTED))
  {
    mqttReconnect();
    mqttClient.loop();
    
    readDataAndSend();
  } else {
    Serial.println("[WiFi] Connection error");
  }
  
  setBuiltinLEDBrightness(0);
  delay((config_meas_interval_min * MINS_TO_MILLIS) - elapsed); // delay uses millisecond. We'd like to sleep for minutes
}
