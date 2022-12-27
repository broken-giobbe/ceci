/*
 * AmbientSensor implementation
 */
#include "AmbientSensor.h"

#ifdef HTU2X_TEMP_SENSOR
#include <Wire.h>
#include <SparkFunHTU21D.h>
#endif

#ifdef BME680_TEMP_SENSOR
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#endif

#ifdef LM75_TEMP_SENSOR
#include <Wire.h>
#include <M2M_LM75A.h>
#endif

// macro to round sensor readings to the required precision
#define ROUND_TO_SENSOR_PRECISION(value) roundf(value/SENSOR_PRECISION) * SENSOR_PRECISION;

// offset to subtract from temperature readings
static float temp_offset = 0;

// last temperature reading
static temperature_t last_temp = { .value = 0.0, .valid = false };
// last humidity reading
static humidity_t last_hmdt = { .value = 0.0, .valid = false };
// last temperature reading
static pressure_t last_pres = { .value = 0.0, .valid = false };
// last air quality reading
static air_quality_t last_qual = { .value = 0.0, .valid = false };

// timestamp for the readings above
static unsigned long timestamp_ms = 0;

/*
 * Reads temperature and humidity (if available) at the same time and stores them
 */
void sensor_read(void);

/* 
 * Generator macro for functions to obtain sensor readings.
 * Since they all do the same thing there's no point in copy-pasting them
 */
#define GET_READING_GEN(NAME, RET_TYPE, RET_VAL) \
RET_TYPE NAME(unsigned long timelimit_ms) \
{ \
  timelimit_ms = max(MIN_SAMPLE_INTERVAL_MS, timelimit_ms); \
  if ((millis() - timestamp_ms) >= timelimit_ms) { \
    timestamp_ms = millis(); \
    sensor_read(); \
  } \
  return RET_VAL; \
} \

GET_READING_GEN(get_temperature, temperature_t, last_temp)
GET_READING_GEN(get_humidity, humidity_t, last_hmdt)
GET_READING_GEN(get_pressure, pressure_t, last_pres)
GET_READING_GEN(get_air_quality, air_quality_t, last_qual)

#ifdef HTU2X_TEMP_SENSOR
static HTU21D htu21d;

void temp_sensor_init(float offset)
{
  temp_offset = offset;
  // The HTU21D temp sensor is connected as in the following code
  // https://github.com/enjoyneering/HTU21D/blob/master/examples/HTU21D_Demo/HTU21D_Demo.ino
  htu21d.begin();
  // Reduce the resolution a bit to get faster measurements
  // (the measurement, is rounded to the last decimal anyway)
  htu21d.setResolution(USER_REGISTER_RESOLUTION_RH10_TEMP13);
}

void sensor_read()
{
  float temp = htu21d.readTemperature();
  float humd = htu21d.readHumidity();
  
  last_temp.value = ROUND_TO_SENSOR_PRECISION(temp);
  last_temp.value -= temp_offset; // subtract temperature offset
  last_hmdt.value = ROUND_TO_SENSOR_PRECISION(humd);
  
  last_temp.valid = ((temp == ERROR_BAD_CRC) || (temp == ERROR_I2C_TIMEOUT)) ? false : true;
  last_hmdt.valid = ((humd == ERROR_BAD_CRC) || (humd == ERROR_I2C_TIMEOUT)) ? false : true;
}
#endif

#ifdef LM75_TEMP_SENSOR
static M2M_LM75A lm75(LM75_SENSOR_ADDRESS);

void temp_sensor_init(float offset)
{
  temp_offset = offset;

  lm75.begin();
  lm75.wakeup();
}

void sensor_read()
{
  float temp = lm75.getTemperature();
  
  last_temp.value = ROUND_TO_SENSOR_PRECISION(temp);
  last_temp.value -= temp_offset; // subtract temperature offset
  
  last_temp.valid = (temp == LM75A_INVALID_TEMPERATURE) ? false : true;
  
  last_hmdt.value = -100.0;
  last_hmdt.valid = false;
}
#endif

#ifdef BME680_TEMP_SENSOR

static Adafruit_BME680 bme; // I2C, with default address

/*
 * Since I'm not that interest in a standard quality metric, I'd rather
 * have the sensor report an "index" capped at 100.0
 * 
 * To do so, an absolute maximum is kept for the sensor resistance readings and 
 * the quality value is normalized to 100.0 taking this maximum into account.
 * The maximum is given an initial meaningful value to speed up convergence.
 */
static float max_res_reading = 5e3;

void temp_sensor_init(float offset)
{
  temp_offset = offset;
    
  bme.begin();
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_4X);
  bme.setHumidityOversampling(BME680_OS_4X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_0);
  bme.setODR(BME68X_ODR_1000_MS);
  bme.setGasHeater(320, 150);
}

void sensor_read()
{
  if (!bme.performReading()) {
    last_temp.valid = false;
    last_hmdt.valid = false;
    last_pres.valid = false;
    last_qual.valid = false;
    return;
  }

  last_temp.value = ROUND_TO_SENSOR_PRECISION(bme.temperature);
  last_temp.value -= temp_offset; // subtract temperature offset
  last_temp.valid = true;
  
  last_hmdt.value = ROUND_TO_SENSOR_PRECISION(bme.humidity);
  last_hmdt.valid = true;

  last_pres.value = ROUND_TO_SENSOR_PRECISION(bme.pressure / 100.0);
  last_pres.valid = true;

  max_res_reading = fmaxf(max_res_reading, bme.gas_resistance);
  last_qual.value = ROUND_TO_SENSOR_PRECISION((100.0 * bme.gas_resistance) / max_res_reading);
  last_qual.valid = true;
}
#endif
