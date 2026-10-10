#ifndef ORIGINAL_RADIO_H
#define ORIGINAL_RADIO_H
#include <stdint.h>
#include <ti/drivers/soc/soc.h>
#include "sensing_config.h"
#include "rf_profile.h"
int or_radio_init(SOC_Handle soc);
void or_radio_execute(void);
int or_radio_prepare(const OrConfig *cfg,OrRfPhysical *physical);
int or_radio_start(void);
int or_radio_stop(void);
int32_t or_radio_error(void);
uint32_t or_radio_fault(void);
uint32_t or_radio_frame_end(void);
#endif
