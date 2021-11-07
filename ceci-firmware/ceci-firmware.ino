/**
 * TODO: Describe me
**/
#include <ESP8266WiFi.h>
#include "FS.h"
#include <SPIFFSIniFile.h>

#include "ConfigParser.h"

#include "sysconfig.h"
#include "MQTTdispatcher.h"
#include "AmbientSensor.h"
#include "UIDriver.h"
#include "modules.h"

// macros used to convert minutes to other stuff
#define MINS_TO_SEC(m) ((m)*60)
#define SECS_TO_MILLIS(s) ((s)*1000)
#define MINS_TO_MILLIS(m) ((m)*60*1000)

// Facility used for logging messages to the serial port
#define LOG(fmt, ...) Serial.printf("[%lu, %s] " fmt "\n", millis(), __func__, ##__VA_ARGS__)

// This node needs a name
static String node_name;

// The current mode the thermostat is in: 'A' - auto, 'M' - manual
char tstat_mode = 'M';
// the target temperature to be reached
float target_temp = 0.0;

/*
 * Helper function to open the config file
 */
SPIFFSIniFile* conf_openFile(void)
{
  SPIFFSIniFile* ini = new SPIFFSIniFile(CONFIG_FILENAME);
   
  if (!SPIFFS.begin())
  {
    LOG("SPIFFS initialization failed. Aborting.");
    while (1) yield(); // avoid triggering the WDT: https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
  }
 
  if (!ini->open())
  {
    LOG("Cannot open config file. Aborting.");
    while (1) yield();
  }

  return ini;
}

/*
 * Facility for reading srings from the config file
 */
String conf_getStr(SPIFFSIniFile* conf, const char* section, const char* key)
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config file
  char str_buf[TXT_BUF_SIZE]; // temporary placeholder for strings

  conf->getValue(section, key, buf, TXT_BUF_SIZE, str_buf, TXT_BUF_SIZE);
  return String(str_buf);
}

/*
 * Facility for reading floats from the config file
 */
float conf_getFloat(SPIFFSIniFile* conf, const char* section, const char* key)
{
  float retval = 0;
  char buf[TXT_BUF_SIZE];
  
  conf->getValue(section, key, buf, TXT_BUF_SIZE, retval);
  return retval;
}

void setup()
{
  char buf[TXT_BUF_SIZE]; // buffer for reading config variables
  
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
  SPIFFSIniFile* conf = conf_openFile();

  // first of all we need a name
  node_name = conf_getStr(conf, "global", "node_name");
  LOG("%s booting.", node_name.c_str());
  
  parse_config(conf);
  
  // initialize the temperature sensor with the correct offset
  temp_sensor_init(conf_getFloat(conf, "global", "temp_offset"));

  // Configure the heater relay output if needed
  if (thermostat_config.heater_status != -1)
  {
    pinMode(HEATER_PORT, OUTPUT);
    digitalWrite(HEATER_PORT, thermostat_config.heater_status);
  }

  // Initialize MQTT
  mqtt_init(conf);

   /*
   * Setup the various tasks, in the order of increasing priority
   * Task to run the control loop if enabled (reads temperature & publish to the heater)
   *  This task has to run immediately after being scheduled since nobody likes a termostat that has unnecessary starup delay
   * Task to receive MQTT commands (to control the heater & settings) 
   * Task to publish data (temp/humidity and local heater state)
   * Task to control the GUI
   */
  sched_put_task(&thermostatControlLoop, SECS_TO_MILLIS(thermostat_config.sample_interval_sec), true);
  sched_put_task(&ui_task, UI_REFRESH_RATE_MS, false);

  // now that the drivers have been initialized the moules can be initialized too
  mod_sensors_init(conf);

  // Last but not least wifi conenction parameters
  String ssid = conf_getStr(conf, "global", "wifi_ssid");
  String wpsk = conf_getStr(conf, "global", "wifi_psk");
  
  // Done with the config file. Close it
  delete conf; // delete calls the destructor to also close the file
  
  // Now we are ready to connect to the WiFi network
  LOG("WiFi: connecting to %s", ssid.c_str());
  
  WiFi.hostname(node_name);
  WiFi.begin(ssid, wpsk);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  LOG("WiFi: connected. RSSI = %d dBm", WiFi.RSSI());

  // initialization complete
  digitalWrite(LED_BUILTIN, HIGH);
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
