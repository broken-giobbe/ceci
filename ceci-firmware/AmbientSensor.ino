/*
 * TempSensor implementation
 */
#include "AmbientSensor.h"

#ifdef HTU2X_TEMP_SENSOR
#include <Wire.h>
#include <SparkFunHTU21D.h>
#endif

#ifdef LM75_TEMP_SENSOR
#include <Wire.h>
#include <M2M_LM75A.h>
#endif

// offset to subtract from temperature readings
static float temp_offset = 0;
// last temperature reading
static temperature_t last_temp = { .value = 0.0, .valid = false };
// last humidity reading
static humidity_t last_hmdt = { .value = 0.0, .valid = false };
// timestamp for the readings above
static unsigned long timestamp_ms = 0;

/*
 * Reads temperature and humidity (if available) at the same time and stores them
 */
void sensor_read(void);

temperature_t get_temperature(unsigned long timelimit_ms)
{
  timelimit_ms = max(MIN_SAMPLE_INTERVAL_MS, timelimit_ms);
  
  if ((millis() - timestamp_ms) >= timelimit_ms) {
    timestamp_ms = millis();
    sensor_read();
  }
  
  return last_temp;
}

humidity_t get_humidity(unsigned long timelimit_ms)
{
  timelimit_ms = max(MIN_SAMPLE_INTERVAL_MS, timelimit_ms);
  
  if ((millis() - timestamp_ms) >= timelimit_ms) {
    timestamp_ms = millis();
    sensor_read();
  }
  
  return last_hmdt;
}

#ifdef HTU2X_TEMP_SENSOR
HTU21D htu21d;

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
  
  last_temp.value = roundf(temp/SENSOR_PRECISION) * SENSOR_PRECISION;
  last_temp.value -= temp_offset; // subtract temperature offset
  last_hmdt.value = roundf(humd/SENSOR_PRECISION) * SENSOR_PRECISION;
  
  last_temp.valid = ((temp == ERROR_BAD_CRC) || (temp == ERROR_I2C_TIMEOUT)) ? false : true;
  last_hmdt.valid = ((humd == ERROR_BAD_CRC) || (humd == ERROR_I2C_TIMEOUT)) ? false : true;
}
#endif

#ifdef LM75_TEMP_SENSOR
M2M_LM75A lm75(LM75_SENSOR_ADDRESS);

void temp_sensor_init(float offset)
{
  temp_offset = offset;

  lm75.begin();
  lm75.wakeup();
}

void sensor_read()
{
  float temp = lm75.getTemperature();
  
  last_temp.value = roundf(temp/SENSOR_PRECISION) * SENSOR_PRECISION;
  last_temp.value -= temp_offset; // subtract temperature offset
  
  last_temp.valid = (temp == LM75A_INVALID_TEMPERATURE) ? false : true;
  
  last_hmdt.value = -100.0;
  last_hmdt.valid = false;
}
#endif
