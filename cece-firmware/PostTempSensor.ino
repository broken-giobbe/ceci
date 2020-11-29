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

#include <Wire.h>
#include "SparkFunHTU21D.h"
HTU21D myHumidity;

#define MINS_TO_MILLIS (60*1000)

/*
 * Node configuration
 */
#define POST_ADDRESS "iot.pielluzza.ts"
#define POST_ENDPOINT "http://" POST_ADDRESS "/cgi/post_data.pl"

#define STASSID ""
#define STAPSK  ""

#define NODE_ID "cece-test"
//#define NODE_ID "cece-stanza"
//#define NODE_ID "cece-fuori"

#define SLEEP_MINS 1

#define LED_ON_BRIGHTNESS_PERCENT 8

/*
 * Routines
 */

// The controls for the builtin LED are reversed.
// Wen the pin is set to HIGH the LED is off and vice-versa.
// This subroutine makes it easy to set the brightness appropriately
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
  
  http.begin(client, POST_ENDPOINT);
  http.addHeader("Content-Type", "application/json");

  String post_str = (String)"{\"ID\": \"" + NODE_ID + "\"," +
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
  // initialize onboard LED as output and turn it on
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  // initialize serial
  Serial.begin(115200);
  Serial.println("");

  // initialize wifi
  WiFi.hostname(NODE_ID);
  WiFi.begin(STASSID, STAPSK);
  // Force the ESP into client-only mode (otherwhise light sleep cannot be enabled)
  WiFi.mode(WIFI_STA); 
  // Enable light sleep
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected! RSSI = ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());
  /*
  Serial.print("[WiFi] DNS servers: ");
  WiFi.dnsIP().printTo(Serial);
  Serial.print(", ");
  WiFi.dnsIP(1).printTo(Serial);
  Serial.println();
  
  Serial.print("[WiFi] " POST_ADDRESS);
  Serial.print(" is at ");
  IPAddress resolvedIP;
  WiFi.hostByName(POST_ADDRESS, resolvedIP);
  Serial.println(resolvedIP);
  */
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
  delay((SLEEP_MINS * MINS_TO_MILLIS) - elapsed); // delay uses millisecond. We'd like to sleep for minutes
}
