#include "ranger.hpp"
#include "i2c.hpp"
#include "vl53l0x.hpp"

#include "esp_log.h"

static VL53L0xDev dev;

static const char * TAG = "VL53L0X";

void ranger_init(void)
{
  dev.devAddr = VL53L0X_DEFAULT_ADDRESS;

  dev.io_timeout = 0;
  dev.did_timeout = 0;
  dev.timeout_start_ms = 0;
  dev.stop_variable = 0;
  dev.measurement_timing_budget_us = 0;
  dev.measurement_timing_budget_ms = 0;

  vTaskDelay(M2T(20));

  uint16_t wordData;
  wordData = vl53l0xGetModelID(&dev);
  ESP_LOGI(TAG, "%02X\n\r", wordData);

  if(wordData == VL53L0X_ID)
	{
    ESP_LOGI(TAG, "VL53L0X I2C connection [OK].\n");
  }

  if (!vl53l0xInit(&dev, false)) {
    ESP_LOGI(TAG, "failed to init VL53L0X");
  }

  vl53l0xSetVcselPulsePeriod(&dev, VcselPeriodPreRange, 18);/*长距离模式  33ms 周期*/
  vl53l0xSetVcselPulsePeriod(&dev, VcselPeriodFinalRange, 14);/*长距离模式  33ms 周期*/
  vl53l0xStartContinuous(&dev, 0);
  
//   range_last = zRangerGetMeasurementAndRestart(&dev);
//     rangeSet(rangeDown, range_last / 1000.0f);
//     DEBUG_PRINTD("ZRANGE = %f",range_last/ 1000.0f);

//     // check if range is feasible and push into the estimator
//     // the sensor should not be able to measure >3 [m], and outliers typically
//     // occur as >8 [m] measurements
//     if (range_last < RANGE_OUTLIER_LIMIT) {
//       float distance = (float)range_last * 0.001f; // Scale from [mm] to [m]
//       float stdDev = expStdA * (1.0f  + expf( expCoeff * (distance - expPointA)));
//       rangeEnqueueDownRangeInEstimator(distance, stdDev, xTaskGetTickCount());
//     }
//   }
}