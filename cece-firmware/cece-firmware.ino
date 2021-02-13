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
// Port where the heating control realy is connected
#define HEATER_PORT D7

// HTU21D temperature & Humidity sensor
HTU21D myHumidity;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#define MQTT_BUFFER_SIZE 50 // buffer size for MQTT operations

/**
 * Publish node data using MQTT.
 * Basically we publish the sensor readings and the thermostat status (if available)
 * Make also sure that we call the loop() function and reconnect to the broker if needed
 */
void mqttPublishData()
{
  char msg[MQTT_BUFFER_SIZE];

  mqttReconnect();
  mqttClient.loop();

  // Take humidity and temperature readings.
  // Fail silently if the data is wrong
  float humd = myHumidity.readHumidity();
  float temp = myHumidity.readTemperature() - config_temp_offset;
  Serial.print("[Sensors] Temperature: ");
  Serial.print(temp);
  Serial.print(" humidity: ") ;
  Serial.println(humd);
  
  if ((temp != ERROR_BAD_CRC) && (temp != ERROR_I2C_TIMEOUT) &&
      (humd != ERROR_BAD_CRC) && (humd != ERROR_I2C_TIMEOUT))
  {
    snprintf(msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", temp, humd);
    
    Serial.print("[MQTT] publish to: ");
    Serial.println(config_sensor_topic);
    mqttClient.publish(config_sensor_topic, msg, 1);
  }

  // If this node can control some heating system then tell the world so
  if (config_heater_status != -1)
  {
    snprintf(msg, MQTT_BUFFER_SIZE, "{\"heater\":%i,\"hysteresis\":%.1f,\"anticipator\":%.1f}",
      config_heater_status, config_tstat_hysteresis, config_tstat_anticipator);

    Serial.print("[MQTT] publish to: ");
    Serial.println(config_tstat_status_topic);
    mqttClient.publish(config_tstat_status_topic, msg);
  }
}

/**
 * MQTT callback receiving data about the topics we have subscribed to.
 * Parameters:
 *   topic const char[] - the topic the message arrived on
 *   payload byte[] - the message payload
 *   length unsigned int - the length of the message payload
 */
void mqttCallback(const char* topic, byte* payload, unsigned int length)
{
  char* payloadStr = (char*) malloc(length + 1); // for null terminator char
  snprintf(payloadStr, length+1, "%s", payload);

  Serial.print("[MQTT] got: ");
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(payloadStr);
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
      // If this node controls the heating system (re)subscribe to the required topics
      if (config_heater_status != -1)
      {
        mqttClient.subscribe(config_tstat_temp_topic);
        mqttClient.subscribe(config_tstat_mode_topic);
        mqttClient.subscribe(config_tstat_sensor_topic);
      }
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

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

  // Configure the heater relay output if needed
  if (config_heater_status != -1)
  {
    pinMode(HEATER_PORT, OUTPUT);
    digitalWrite(HEATER_PORT, config_heater_status);
  }

  // Initialize MQTT
  mqttClient.setServer(config_mqtt_server, config_mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  // The HTU21D temp sensor is connected as in the following code
  // https://github.com/enjoyneering/HTU21D/blob/master/examples/HTU21D_Demo/HTU21D_Demo.ino
  myHumidity.begin();
  // Reduce the resolution a bit to get faster measurements
  // (while sending the measurement, we round out the last decimal anyway)
  myHumidity.setResolution(USER_REGISTER_RESOLUTION_RH10_TEMP13);

  // initialize wifi, putting the ESP into STA(client)-only mode
  initWiFi_sta(config_node_name);

  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  elapsedMillis elapsed = 0; // count the time the node spent sending data for more accurate sleeping intervals

  withWiFiConnected(config_wifi_ssid, config_wifi_psk, &mqttPublishData);

  delay((config_meas_interval_min * MINS_TO_MILLIS) - elapsed); // delay uses millisecond. We'd like to sleep for minutes
}
