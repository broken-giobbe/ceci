/*
 * Handle the different temeprature sources using a common API.
 * 
 * Dependig on the hardware configuration the following sources are avaliable:
 *  - HTU2X tmperature & humidity sensor
 *  - LM75 temperatore sensor
 *  - MQTT temperature reading from a remote sensor
 *  
 * Only one temperature source has to be selected
 */
#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

// Use the HTU2X temperature/humidity sensor
//#define HTU2X_TEMP_SENSOR

// Use the LM75 temperature sensor and set the I2C address
#define LM75_TEMP_SENSOR
#define LM75_SENSOR_ADDRESS 0x48

// Receive the temperature/humidity readings from a remote sensor using MQTT
//#define MQTT_TEMP_SENSOR

// give the temperature sensor some rest between readings. Reduces self-heating.
// If two humidity/temperature requests are made within this interval the old value is returned instead
#define SAMPLE_INTERVAL_MS 1000

// The precision used to return temperature and humidity readings
#define SENSOR_PRECISION 0.5

/*
 * Datatype for temperature & humidity readings.
 * If valid is false the value is unreliable and should be discarded
 */
struct temperature_t {
  float value;
  bool valid;
};
typedef struct temperature_t temperature_t;
typedef struct temperature_t humidity_t;

/*
 * Initialize the sensor, providing an offset to correct temperature readings
 */
void temp_sensor_init(float offset);

/*
 * Get the most recent temperature reading
 */
temperature_t get_temperature(void);

/*
 * Get the most recent humidity reading
 */
humidity_t get_humidity(void);

#endif
