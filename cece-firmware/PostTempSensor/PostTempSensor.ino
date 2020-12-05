/**
 * TODO: Describe me 
**/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
/* 
 * This is needed for using light sleep. See also:
 *    https://community.blynk.cc/t/esp8266-light-sleep/13584/16 
 *    https://github.com/esp8266/Arduino/issues/1381
 */
extern "C" {
  #include "user_interface.h"
}
#include <elapsedMillis.h>
#include "FS.h"
#include <SPIFFSIniFile.h>
#include <Wire.h>

#include "SparkFunHTU21D.h"
HTU21D myHumidity;

// Constant used to convert minutes to milliseconds
#define MINS_TO_MILLIS (60*1000)
// Baud rate for the UART port
#define UART_BAUD_RATE 115200
// Buffer length used for reading the config.ini file
#define INI_BUF_LENGTH 80
// Buffer length used for storing string values read from config.ini
#define VAL_BUF_LENGTH 50
// Builtin LED brightness when on
#define LED_ON_BRIGHTNESS_PERCENT 8

/*
 * The following variables are read from SPIFFS during init.
 */
int measurement_interval_min = 1; // Measurement interval in minutes
char node_name[VAL_BUF_LENGTH] = "A Horse with No Name"; // node name
char post_endpoint[VAL_BUF_LENGTH]; // endpoint address

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

inline void readDataAndSend() __attribute__((always_inline));
void readDataAndSend()
{
  // Take humidity and temperature reading
  float humd = myHumidity.readHumidity();
  float temp = myHumidity.readTemperature();
  
  // Send the data
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, post_endpoint);
  http.addHeader("Content-Type", "application/json");

  String post_str = (String)"{\"ID\": \"" + node_name + "\"," +
                             "\"temp\": " + temp + "," +
                             "\"rhum\": " + humd + "}";
                               
  Serial.println("[HTTP] POST: " + post_str);
  int httpCode = http.POST(post_str);

  // httpCode will be negative on error
  if (httpCode > 0) {
    Serial.printf("[HTTP] POST succeeded: %d\n", httpCode);
    // calling getString() is slow. Use it for debug only!
    //Serial.println(http.getString());
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void setup()
{
  char buffer[INI_BUF_LENGTH]; // buffer for reading config file
  char wifi_ssid[VAL_BUF_LENGTH];
  char wifi_psk[VAL_BUF_LENGTH];
   
  // initialize onboard LED as output and turn it on
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  // initialize serial
  Serial.begin(UART_BAUD_RATE);
  Serial.println("");

  // read config.ini file from SPIFFS and initialize variables
  if (!SPIFFS.begin())
  {
      Serial.println("ERROR: SPIFFS.begin() failed. Stopping.");
      while(1) ;
  }
  
  SPIFFSIniFile ini("/config.ini");
  if (!ini.open())
  {
    Serial.println("ERROR: config.ini does not exist. Stopping.");
    while (1) ;
  }

  if (!ini.validate(buffer, INI_BUF_LENGTH)) {
    Serial.print("ERROR: config.ini not valid. Error: ");
    Serial.print(ini.getError());
    Serial.println(". Stopping.");
    while (1) ;
  }
  ini.getValue("node", "name", buffer, INI_BUF_LENGTH, node_name, VAL_BUF_LENGTH);
  ini.getValue("node", "endpoint", buffer, INI_BUF_LENGTH, post_endpoint, VAL_BUF_LENGTH);
  ini.getValue("node", "meas_interval_min", buffer, INI_BUF_LENGTH, measurement_interval_min);
  ini.getValue("wifi", "ssid", buffer, INI_BUF_LENGTH, wifi_ssid, VAL_BUF_LENGTH);
  ini.getValue("wifi", "psk", buffer, INI_BUF_LENGTH, wifi_psk, VAL_BUF_LENGTH);
  
  // initialize wifi
  // Force the ESP into client-only mode (otherwhise light sleep cannot be enabled)
  WiFi.mode(WIFI_STA); 
  WiFi.hostname(node_name);

  // Enable light sleep
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  Serial.print("[WiFi] Connecting to ");
  Serial.print(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_psk);
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
    readDataAndSend();
  } else {
    Serial.println("[WiFi] Connection error");
  }
  
  setBuiltinLEDBrightness(0);
  delay((measurement_interval_min * MINS_TO_MILLIS) - elapsed); // delay uses millisecond. We'd like to sleep for minutes
}
