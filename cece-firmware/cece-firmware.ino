/**
 * TODO: Describe me 
**/
#include <PubSubClient.h>
#include <Wire.h>
#include <SparkFunHTU21D.h>

#include "ConfigParser.h"
#include "WiFiManager.h"

// Constant used to convert minutes to stuff
#define MINS_TO_SEC(x) (x*60)
#define MINS_TO_MILLIS(x) (x*60*1000)
// Baud rate for the UART port
#define UART_BAUD_RATE 115200
// Port where the heating control realy is connected
#define HEATER_PORT D7
// Rate in milliseconds at which the MQTT function loop() is called
#define MQTT_LOOP_RATE 2000

// HTU21D temperature & Humidity sensor
HTU21D myHumidity;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#define MQTT_BUFFER_SIZE 50 // buffer size for MQTT operations

// next time mqttPublish shall run
time_t mqttPublish_next_millis = 0;
// next time mqtt loop shall run
time_t mqttLoop_next_millis = 0;

// The target temperature for the thermostat mode
float target_temp = 0;
// The last temperature received from mqtt
float last_temp = 0;
// the mode the thermostat is in: A - auto, M - manual
char tstat_mode = 'M';

/*
 * Compute a new value for the thermostat output
 */
bool tstat_computeOutput(float t_temp, float a_temp)
{
  if (tstat_mode != 'A')
    return config_heater_status;
 
  bool newOutput = (t_temp - a_temp) > 0;
  
  return newOutput;
}

/**
 * Publish node data using MQTT.
 * Basically we publish the sensor readings and the thermostat status (if available)
 */
void mqttPublishData()
{
  char msg[MQTT_BUFFER_SIZE];

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
    snprintf(msg, MQTT_BUFFER_SIZE, "{\"heater\":%i%c, \"target_temp\":%.1f}",
      config_heater_status, tstat_mode, target_temp);

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

  // keep only the last part of the topic, the rest does not give us any info
  topic = strrchr(topic, '/') + 1;
  
  Serial.print("[MQTT] Received ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payloadStr);

  switch (topic[0]) {
    case 't': // target_temp
      target_temp = strtof(payloadStr, NULL);
      break;
    
    case 'm': // mode
      if(payload[0] == 'A') // auto mode
      {
        tstat_mode = 'A';
      } else { // manual mode
        config_heater_status = (payload[0] == '1');
        tstat_mode = 'M';
      }
      break;
    
    case 'a': // actual_temp
      last_temp = strtof(payloadStr, NULL);
      break;
    
    default:
      ; /* do nothing */
  }
  
  config_heater_status = tstat_computeOutput(target_temp, last_temp);
  digitalWrite(HEATER_PORT, config_heater_status);

  free(payloadStr);
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

    if (mqttClient.connect(config_node_name)) {
      Serial.println(" Success.");
      // If this node controls the heating system (re)subscribe to the required topics
      if (config_heater_status != -1)
      {
        mqttClient.subscribe(config_tstat_target_temp_topic);
        mqttClient.subscribe(config_tstat_mode_topic);
        mqttClient.subscribe(config_tstat_actual_temp_topic);
      }
    } else {
      Serial.println(" Fail: rc=");
      Serial.println(mqttClient.state());
      Serial.println("[MQTT] Will try again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  
  mqttClient.loop();
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
  // Use config_meas_interval as keepalive time for MQTT connection
  mqttClient.setKeepAlive(MINS_TO_SEC(config_meas_interval_min));
  
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
  time_t cur_millis = millis();
  
  if (cur_millis >= mqttLoop_next_millis)
  {
    withWiFiConnected(config_wifi_ssid, config_wifi_psk, &mqttKeepalive);
    mqttLoop_next_millis = cur_millis + MQTT_LOOP_RATE;
    return;
  }
  
  if (cur_millis >= mqttPublish_next_millis)
  {
    withWiFiConnected(config_wifi_ssid, config_wifi_psk, &mqttPublishData);
    mqttPublish_next_millis = cur_millis + MINS_TO_MILLIS(config_meas_interval_min);
    return;
  }
  
  delay(min(mqttLoop_next_millis, mqttPublish_next_millis) - millis());
}
