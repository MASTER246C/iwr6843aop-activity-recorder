#ifndef ORIGINAL_SPATIAL_SELFTEST_H
#define ORIGINAL_SPATIAL_SELFTEST_H
#include <stdint.h>
#define OR_FIRMWARE_VERSION "0.3.0-rc1"
#define OR_SPATIAL_SELFTEST_VERSION 1U
/* Synthetic CPU arithmetic only. No RF, peripheral or acquisition calls. */
uint32_t os_spatial_no_rf_selftest(void);
extern volatile uint32_t os_selftest_result;
#endif
