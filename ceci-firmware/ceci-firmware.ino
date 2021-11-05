/**
 * TODO: Describe me
**/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "sysconfig.h"
#include "ConfigParser.h"
#include "TempSensor.h"
#include "UIDriver.h"

// macros used to convert minutes to other stuff
#define MINS_TO_SEC(m) ((m)*60)
#define SECS_TO_MILLIS(s) ((s)*1000)
#define MINS_TO_MILLIS(m) ((m)*60*1000)

// Facility used for logging messages to the serial port
#define LOG(fmt, ...) Serial.printf("[%lu, %s] " fmt "\n", millis(), __func__, ##__VA_ARGS__)

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// The current mode the thermostat is in: 'A' - auto, 'M' - manual
char tstat_mode = 'M';
// the target temperature to be reached
float target_temp = 0.0;

void setup()
{
  // initialize onboard LED as output and turn it on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // enable WiFi and light sleep mode
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  
  // initialize serial
  Serial.begin(UART_BAUD_RATE);
  Serial.println("");

  // Initialize UI & display, to start showing something during boot
  ui_init();

  // read config.ini file from SPIFFS and initialize variables
  SPIFFSIniFile* conf = open_config_file();

  if (conf == NULL)
  {
    Serial.println("[Config] SPIFFS initialization failed. Aborting setup().");
    while (1) yield(); // avoid triggering the WDT: https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
  }
  if (conf->getError() != SPIFFSIniFile::errorNoError)
  {
    Serial.print("[Config] Error opening config file. Reason:");
    Serial.print(config_get_error_str(conf));
    Serial.println(". Aborting setup().");
    while (1) yield();
  }
  parse_config(conf);
  
  // initialize the temperature sensor with the correct offset
  temp_sensor_init(config_get_temperature_offset(conf));

  // Done with the config file. Close it
  close_config_file(conf);
  
  // Configure the heater relay output if needed
  if (thermostat_config.heater_status != -1)
  {
    pinMode(HEATER_PORT, OUTPUT);
    digitalWrite(HEATER_PORT, thermostat_config.heater_status);
  }

  // Initialize MQTT
  mqttClient.setServer(mqtt_config.server, mqtt_config.port);
  mqttClient.setCallback(mqttCallback);

   /*
   * Setup the various tasks, in the order of increasing priority
   * Task to run the control loop if enabled (reads temperature & publish to the heater)
   *  This task has to run immediately after being scheduled since nobody likes a termostat that has unnecessary starup delay
   * Task to receive MQTT commands (to control the heater & settings) 
   * Task to publish data (temp/humidity and local heater state)
   * Task to control the GUI
   */
  sched_put_task(&thermostatControlLoop, SECS_TO_MILLIS(thermostat_config.sample_interval_sec), true);
  sched_put_task(&mqttKeepalive, MQTT_LOOP_RATE, false);
  sched_put_task(&mqttPublishData, SECS_TO_MILLIS(mqtt_config.pub_interval_sec), false);
  sched_put_task(&ui_task, UI_REFRESH_RATE_MS, false);

  // All done. Let's connect to the WiFi network
  LOG("WiFi: connecting to %s", node_config.wifi_ssid);
  
  WiFi.hostname(node_config.name);
  WiFi.begin(node_config.wifi_ssid, node_config.wifi_psk);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  LOG("WiFi: connected. RSSI = %d dBm", WiFi.RSSI());

  digitalWrite(LED_BUILTIN, HIGH);
}

/**
 * Publish node data using MQTT.
 * Basically we publish the sensor readings and the thermostat status (if available)
 */
void mqttPublishData()
{
  char msg[MQTT_BUFFER_SIZE];
  temperature_t temp = get_temperature();
  humidity_t hmdt = get_humidity();

  if (temp.valid) // We don't care about humidity since some sensors do not report it
  {
    snprintf(msg, MQTT_BUFFER_SIZE, "{\"temp\":%.1f,\"rhum\":%.1f}", temp.value, hmdt.value);

    LOG("Publishing %s to %s", msg, mqtt_config.sensor_topic);
    mqttClient.publish(mqtt_config.sensor_topic, msg, 1);
  }
  
  snprintf(msg, MQTT_BUFFER_SIZE, "{\"heater\":%i%c, \"target_temp\":%.1f}",
      thermostat_config.heater_status, tstat_mode, target_temp);

  LOG("Publishing %s to %s", msg, mqtt_config.tstat_status_topic);
  mqttClient.publish(mqtt_config.tstat_status_topic, msg);
}

/**
 * MQTT utility function to (re)connect to the broker and to perform all the housekeeping stuff
 * for MQTT to work correctly
 */
void mqttKeepalive()
{
  if (!mqttClient.connected())
  {
    LOG("Connecting to %s:%d...", mqtt_config.server, mqtt_config.port);

    if (mqttClient.connect(node_config.name)) {
      LOG("Connection success.");

      // (re)subscribe to the required topics
      mqttClient.subscribe(mqtt_config.tstat_enable_topic);
      mqttClient.subscribe(mqtt_config.tstat_mode_topic);
      mqttClient.subscribe(mqtt_config.tstat_target_topic);
    } else {
      LOG("Connection failed: rc=%d", mqttClient.state());
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

  LOG("Received %s -> %s", topic, payloadStr);

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
  temperature_t temp = get_temperature();
  
  LOG("Temperature: %f valid: %s", temp.value, (temp.valid ? "true" : "false"));

  if (!temp.valid)
    return; // Fail silently if the data is wrong
  
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

  LOG("final_temp = %f", final_temp);
  
  if (temp.value < final_temp) // actual temp is lower than expected -> heat up
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
