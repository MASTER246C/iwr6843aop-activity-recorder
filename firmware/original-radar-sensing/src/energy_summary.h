#ifndef ORIGINAL_ENERGY_SUMMARY_H
#define ORIGINAL_ENERGY_SUMMARY_H
#include "radar_dsp.h"
/* Selected-bin sums: time-mean power, coherent static power, nonnegative variance.
 * These are uncalibrated ADC-count-squared measurements, not class probabilities. */
int or_profile_energy(const RadarDsp *state,float values[3]);
#endif
