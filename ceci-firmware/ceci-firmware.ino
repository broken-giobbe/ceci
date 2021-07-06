/**
 * TODO: Describe me
**/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SparkFunHTU21D.h>

#include "ConfigParser.h"

// macros used to convert minutes to other stuff
#define MINS_TO_SEC(m) ((m)*60)
#define SECS_TO_MILLIS(s) ((s)*1000)
#define MINS_TO_MILLIS(m) ((m)*60*1000)

// Baud rate for the UART port
#define UART_BAUD_RATE 115200
// Port where the heating control realy is connected
#define HEATER_PORT D7
// Rate in milliseconds at which the MQTT function loop() is called
#define MQTT_LOOP_RATE 1000

// HTU21D temperature & Humidity sensor
HTU21D myHumidity;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#define MQTT_BUFFER_SIZE 50 // buffer size for MQTT operations

// The last temperature read from the sensor
float last_temp = 0;
// The last humidity value read from the sensor
float last_humidity = 0;
// The current mode the thermostat is in: 'A' - auto, 'M' - manual
char tstat_mode = 'M';
// the target temperature to be reached
float target_temp = 0.0;

void setup()
{
  // enable WiFi and light sleep mode
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);

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
  if (thermostat_config.heater_status != -1)
  {
    pinMode(HEATER_PORT, OUTPUT);
    digitalWrite(HEATER_PORT, thermostat_config.heater_status);
  }

  // Initialize MQTT
  mqttClient.setServer(mqtt_config.server, mqtt_config.port);
  mqttClient.setCallback(mqttCallback);

  // The HTU21D temp sensor is connected as in the following code
  // https://github.com/enjoyneering/HTU21D/blob/master/examples/HTU21D_Demo/HTU21D_Demo.ino
  myHumidity.begin();
  // Reduce the resolution a bit to get faster measurements
  // (while sending the measurement, we round out the last decimal anyway)
  myHumidity.setResolution(USER_REGISTER_RESOLUTION_RH10_TEMP13);

   /*
   * Setup the various tasks, in the order of increasing priority
   * Task to run the control loop if enabled (reads temperature & publish to the heater)
   * Task to receive MQTT commands (to control the heater & settings) 
   * Task to publish data (temp/humidity and local heater state)
   * Task to control the GUI
   */
  sched_put_task(&thermostatControlLoop, SECS_TO_MILLIS(thermostat_config.sample_interval_sec));
  sched_put_task(&mqttKeepalive, MQTT_LOOP_RATE);
  sched_put_task(&mqttPublishData, SECS_TO_MILLIS(mqtt_config.pub_interval_sec));

  // All done. Let's connect to the WiFi network
  Serial.print("[WiFi] Connecting to ");
  Serial.print(node_config.wifi_ssid);
  WiFi.begin(node_config.wifi_ssid, node_config.wifi_psk);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.print("\n[WiFi] Connected. RSSI = ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  digitalWrite(LED_BUILTIN, HIGH);
}

/**
 * Publish node data using MQTT.
 * Basically we publish the sensor readings and the thermostat status (if available)
 */
void mqttPublishData()
{
  char msg[MQTT_BUFFER_SIZE];

  snprintf(msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", last_temp, last_humidity);

  Serial.print("[MQTT] publish to: ");
  Serial.println(mqtt_config.sensor_topic);
  mqttClient.publish(mqtt_config.sensor_topic, msg, 1);

  snprintf(msg, MQTT_BUFFER_SIZE, "{\"heater\":%i%c, \"target_temp\":%.1f}",
      thermostat_config.heater_status, tstat_mode, target_temp);

  Serial.print("[MQTT] publish to: ");
  Serial.println(mqtt_config.tstat_status_topic);
  mqttClient.publish(mqtt_config.tstat_status_topic, msg);
}

/**
 * MQTT utility function to (re)connect to the broker and to perform all the housekeeping stuff
 * for MQTT to work correctly
 */
void mqttKeepalive()
{
  while (!mqttClient.connected())
  {
    Serial.print("[MQTT] Attempting connection...");

    if (mqttClient.connect(node_config.name)) {
      Serial.println(" Success.");

      // (re)subscribe to the required topics
      mqttClient.subscribe(mqtt_config.tstat_enable_topic);
      mqttClient.subscribe(mqtt_config.tstat_mode_topic);
      mqttClient.subscribe(mqtt_config.tstat_target_topic);
    } else {
      Serial.print(" Fail: rc=");
      Serial.println(mqttClient.state());
      return;
    }
  }

  mqttClient.loop();
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
  char payloadStr[length + 1]; // +1 for null terminator char
  snprintf(payloadStr, length+1, "%s", payload);

  // keep only the last part of the topic, the rest does not give us any info
  topic = strrchr(topic, '/') + 1;

  Serial.print("[MQTT] Received ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payloadStr);

  switch (topic[0]) {
    case 'e': // enable
      if(thermostat_config.heater_status != -1) {
        thermostat_config.heater_status = atoi(payloadStr);
        digitalWrite(HEATER_PORT, thermostat_config.heater_status);
      }
      break;

    case 'm': // mode
      if ((payloadStr[0] == 'A') || (payloadStr[0] == 'M')) {
        tstat_mode = payloadStr[0];
        // Make sure the thermostat control loop is run next
        sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
      }
      break;
    
    case 't': // target_temp
      target_temp = fmaxf(0.0, strtof(payloadStr, NULL));
      // Make sure the thermostat control loop is run next
      sched_reschedule_taskID(sched_get_taskID(&thermostatControlLoop), 0);
      break;

    default:
      ; /* do nothing */
  }
}

/*
 * Get a temperature (&humidity) reading and compute the thermostat output if needed
 */
void thermostatControlLoop(void)
{
  // Take humidity and temperature readings.
  // Fail silently if the data is wrong
  last_humidity = myHumidity.readHumidity();
  last_temp = myHumidity.readTemperature() - node_config.temp_offset;
  Serial.print("[Sensors] Temperature: ");
  Serial.print(last_temp);
  Serial.print(" humidity: ");
  Serial.println(last_humidity);

  if ((last_temp == ERROR_BAD_CRC) || (last_temp == ERROR_I2C_TIMEOUT) ||
      (last_humidity == ERROR_BAD_CRC) || (last_humidity == ERROR_I2C_TIMEOUT))
    return;

  // Continue with the function only if the thermostat mode is set to auto ('A')
  if (tstat_mode != 'A')
    return;

  // I know it's ugly but the thermostat has to keep some state
  static float old_target_temp = target_temp;
  static float final_temp = target_temp;
  static float anticipator_temp = 0;

  // if target temperature has changed reset the state
  if (old_target_temp != target_temp) {
    final_temp = target_temp;
    anticipator_temp = 0;
    old_target_temp = target_temp;
  }
  
  Serial.print("[thermostat] final_temp = ");
  Serial.println(final_temp);
  
  if (last_temp < final_temp) // actual temp is lower than expected -> heat up
  {
    anticipator_temp += thermostat_config.anticipator;
    final_temp = target_temp + thermostat_config.hysteresis - anticipator_temp;
    mqttClient.publish(mqtt_config.tstat_output_topic, "1");
  } else {
    anticipator_temp -= thermostat_config.anticipator;
    anticipator_temp = fmaxf(0, anticipator_temp);
    final_temp = target_temp - thermostat_config.hysteresis - anticipator_temp;
    mqttClient.publish(mqtt_config.tstat_output_topic, "0");
  }
}
