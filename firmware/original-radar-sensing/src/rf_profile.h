#ifndef ORIGINAL_RF_PROFILE_H
#define ORIGINAL_RF_PROFILE_H
#include <stdint.h>
typedef struct {
    double start_hz, slope_hz_per_s, carrier_hz;
    uint32_t start_code;
    int16_t slope_code;
} OrRfPhysical;
/* 60GHz only. Units and even-code rule come from TI rl_sensor.h, not77GHz scaling. */
int or_rf_physical(double device_scale_ghz,OrRfPhysical *physical);
#endif
