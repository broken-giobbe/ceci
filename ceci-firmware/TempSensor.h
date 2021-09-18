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
#define LM75_SENSOR_ADDRESS 0x4F

// give the temperature sensor some rest between readings. Reduces self-heating.
// If two humidity/temperature requests are made within this interval the old value is returned instead
#define MIN_SAMPLE_INTERVAL_MS 1000UL

// The precision used to return temperature and humidity readings
#define SENSOR_PRECISION 0.5

// I want the most recent and precise temperature reading available,
// without caring if I wake the sensor too often
#define SENSOR_MOST_RECENT 0

// I just want a temperature reading, I don't care how old it is
// there's no need to bother the sensor
#define SENSOR_NOWAKE ULONG_MAX

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
temperature_t get_temperature(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

/*
 * Get the most recent humidity reading, not older than timelimit_ms
 */
humidity_t get_humidity(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

#endif
