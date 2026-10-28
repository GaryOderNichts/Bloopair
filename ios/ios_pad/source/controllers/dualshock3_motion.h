/*
 * DualShock 3 single-axis gyro calibration and normalization.
 *
 * The DS3 must rest still during the initial calibration window
 * The only DS3 gyro measures rotation about the controller-local Y (yaw) axis.
 */
#pragma once
#include <stdint.h>

#define DS3_GYRO_Y_AXIS (1u << 1)
#define DS3_GYRO_CAL_SAMPLES 24u
#define DS3_GYRO_CAL_MAX_SPREAD 8u

typedef struct {
    uint32_t sum;
    uint32_t zeroQ8;
    uint16_t min;
    uint16_t max;
    uint8_t count;
    uint8_t ready;
} Dualshock3GyroCalibration;

/** @brief Calibrate zero while still, then return Y-axis gyro data in 16 counts/degree/s.
 *
 * Returns 0 until calibration finishes or if the sensor reading is invalid.
 * When it returns 1, x and z remain unavailable only y is a real reading.
 * A signed negative conversion matches the DRC-style Y direction used by the other Sony controllers.
 */
static inline uint8_t dualshock3_normalize_gyro(
        Dualshock3GyroCalibration* cal, uint16_t raw, int16_t* yRate)
{
    *yRate = 0;
    if (raw > 1023u) return 0;

    if (!cal->ready) {
        if (!cal->count || raw < cal->min || raw > cal->max) {
            uint16_t min = cal->count && raw > cal->min ? cal->min : raw;
            uint16_t max = cal->count && raw < cal->max ? cal->max : raw;
            if ((uint16_t)(max - min) > DS3_GYRO_CAL_MAX_SPREAD) {
                // A changing reading means the pad is moving. Start over.
                cal->count = 0;
            } else {
                cal->min = min;
                cal->max = max;
            }
        }
        if (!cal->count) {
            cal->min = cal->max = raw;
            cal->sum = 0;
        }
        cal->sum += raw;
        ++cal->count;
        if (cal->count < DS3_GYRO_CAL_SAMPLES) return 0;
        cal->zeroQ8 = (cal->sum * 256u + DS3_GYRO_CAL_SAMPLES / 2u)
                        / DS3_GYRO_CAL_SAMPLES;
        cal->ready = 1;
        return DS3_GYRO_Y_AXIS;
    }

    // DS3 nominal scale is 123 raw counts per 90 degrees/second.
    // Fixed-point Q8 avoids using floating point inside IOSU.
    const int32_t differenceQ8 = (int32_t)raw * 256 - (int32_t)cal->zeroQ8;
    const int32_t rate = -(differenceQ8 * (90 * 16)) / (123 * 256);
    *yRate = (int16_t)rate;
    return DS3_GYRO_Y_AXIS;
}
