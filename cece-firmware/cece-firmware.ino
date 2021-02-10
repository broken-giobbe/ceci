/**
 * TODO: Describe me 
**/
#include <PubSubClient.h>
#include <elapsedMillis.h>
#include <Wire.h>
#include <SparkFunHTU21D.h>

#include "ConfigParser.h"
#include "WiFiManager.h"

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
 * Read sensor data and send it to the broker using MQTT.
 * Make also sure that we call the loop() function and reconnect to the broker if needed
 */
void mqttReadDataAndSend()
{
  char msg[MQTT_BUFFER_SIZE];

  mqttReconnect();
  mqttClient.loop();

  // Take humidity and temperature reading
  float humd = myHumidity.readHumidity();
  float temp = myHumidity.readTemperature() - config_temp_offset;

  snprintf(msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", temp, humd);
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
  // Shutdown the WiFi radio until everything has been set up properly
  WiFiTurnOff();

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

  // Initialize MQTT
  mqttClient.setServer(config_mqtt_server, config_mqtt_port);
  
  // The HTU21D temp sensor is connected as in the following code
  // https://github.com/enjoyneering/HTU21D/blob/master/examples/HTU21D_Demo/HTU21D_Demo.ino
  myHumidity.begin();
  // Reduce the resolution a bit to get faster measurements
  // (while sending the measurement, we round out the last decimal anyway)
  myHumidity.setResolution(USER_REGISTER_RESOLUTION_RH10_TEMP13);

  // initialize wifi, putting the ESP into STA(client)-only mode
  initWiFi_sta(config_node_name);

  digitalWrite(BUILTIN_LED, HIGH);
}

void loop()
{
  elapsedMillis elapsed = 0; // count the time the node spent sending data for more accurate sleeping intervals

  withWiFiConnected(config_wifi_ssid, config_wifi_psk, &mqttReadDataAndSend);

  delay((config_meas_interval_min * MINS_TO_MILLIS) - elapsed); // delay uses millisecond. We'd like to sleep for minutes
}
