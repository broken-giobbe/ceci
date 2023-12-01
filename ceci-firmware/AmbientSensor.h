/*
 * Handle the different temeprature sources using a common API.
 * 
 * Dependig on the hardware configuration the following sources are avaliable:
 *  - HTU2X tmperature & humidity sensor
 *  - LM75 temperatore sensor
 *  
 * Only one temperature source has to be selected
 */
#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

// Give the temperature sensor some rest between readings. Reduces self-heating.
// If two humidity/temperature requests are made within this interval an old value is returned instead
#define MIN_SAMPLE_INTERVAL_MS 1000UL

// I want the most recent and precise temperature reading available,
// without caring if I wake the sensor too often
#define SENSOR_MOST_RECENT 0

// I just want a temperature reading, I don't care how old it is
// there's no need to bother the sensor
#define SENSOR_NOWAKE ULONG_MAX

// The precision used to return temperature and humidity readings
#ifdef LM75_TEMP_SENSOR
#define SENSOR_PRECISION 0.5
#else
#define SENSOR_PRECISION 0.1
#endif

/*
 * Datatype for sensor readings.
 * If valid is false the value is unreliable and should be discarded
 */
struct sensor_reading_t {
  float value;
  bool valid;
};
typedef struct sensor_reading_t temperature_t;
typedef struct sensor_reading_t humidity_t;
typedef struct sensor_reading_t pressure_t;
typedef struct sensor_reading_t air_quality_t;

/*
 * Initialize the sensor, providing an offset to correct temperature readings
 */
void temp_sensor_init(float offset);

/*
 * Get the most recent temperature reading, not older than timelimit_ms
 */
temperature_t get_temperature(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

/*
 * Get the most recent humidity reading, not older than timelimit_ms
 */
humidity_t get_humidity(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

/*
 * Get the most recent ambient pressure reading, not older than timelimit_ms
 */
pressure_t get_pressure(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

/*
 * Get the most recent air quality reading, not older than timelimit_ms
 */
air_quality_t get_air_quality(unsigned long timelimit_ms = SENSOR_MOST_RECENT);

#endif
