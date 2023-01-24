/**
 * Entry point for cece firmware
**/
#include <ESP8266WiFi.h>
#include "FS.h"
#include <SPIFFSIniFile.h>

#include "sysconfig.h"
#include "MQTTdispatcher.h"
#include "AmbientSensor.h"
#include "modules.h"

// Build a version string from __DATE__
// See: https://stackoverflow.com/questions/11697820/how-to-use-date-and-time-predefined-macros-in-as-two-integers-then-stri
#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1 \
    ( (BUILD_MONTH_IS_JAN) ? '1' : \
      (BUILD_MONTH_IS_FEB) ? '2' : \
      (BUILD_MONTH_IS_MAR) ? '3' : \
      (BUILD_MONTH_IS_APR) ? '4' : \
      (BUILD_MONTH_IS_MAY) ? '5' : \
      (BUILD_MONTH_IS_JUN) ? '6' : \
      (BUILD_MONTH_IS_JUL) ? '7' : \
      (BUILD_MONTH_IS_AUG) ? '8' : \
      (BUILD_MONTH_IS_SEP) ? '9' : \
      (BUILD_MONTH_IS_OCT) ? '0' : \
      (BUILD_MONTH_IS_NOV) ? '1' : \
      (BUILD_MONTH_IS_DEC) ? '2' : \
      /* error default */    '?' \
    )

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])

const unsigned char buildver[] =
{
  BUILD_YEAR_CH2, BUILD_YEAR_CH3,
  '.',
  BUILD_MONTH_CH0, BUILD_MONTH_CH1,
  '.',
  BUILD_DAY_CH0, BUILD_DAY_CH1, '\0'
};

// macros used to convert minutes to other stuff
#define MINS_TO_SEC(m) ((m)*60)
#define SECS_TO_MILLIS(s) ((s)*1000)
#define MINS_TO_MILLIS(m) ((m)*60*1000)

// Facility used for logging messages to the serial port
#define LOG(fmt, ...) Serial.printf("[%lu, %s] " fmt "\n", millis(), __func__, ##__VA_ARGS__)

//Serial port baud rate
#define UART_BAUD_RATE 115200

// This node needs a name
static String node_name;

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
  // initialize onboard LED as output and turn it on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // enable WiFi and light sleep mode
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  
  // initialize serial
  Serial.begin(UART_BAUD_RATE);
  Serial.println("");
  LOG("Firmware %s booting...", buildver);

  // read config.ini file from SPIFFS and initialize variables
  SPIFFSIniFile* conf = conf_openFile();

  // first of all we need a name
  node_name = conf_getStr(conf, "global", "node_name");
  LOG("Node name is '%s'.", node_name.c_str());
    
  // Initialize UI & display, to start showing something during boot
  mod_ui_init();
  
  // initialize the temperature sensor with the correct offset
  temp_sensor_init(conf_getFloat(conf, "global", "temp_offset"));

  // Initialize MQTT
  mqtt_init(conf);

  // now that the drivers have been initialized the moules can be initialized too
  mod_thermostat_init(conf);
  mod_sensors_init(conf);
  mod_relay_init(conf);
  
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
      delay(200);

  LOG("WiFi: connected. RSSI = %d dBm", WiFi.RSSI());

  // initialization complete
  digitalWrite(LED_BUILTIN, HIGH);
  LOG("Boot complete.");
}
