#ifndef ORIGINAL_ACQUISITION_H
#define ORIGINAL_ACQUISITION_H
#include <stdint.h>
#include <ti/drivers/soc/soc.h>
#include <ti/sysbios/knl/Semaphore.h>
#include "sensing_config.h"
#include "capture_model.h"
#include "radar_dsp.h"
int or_acquisition_init(SOC_Handle soc, Semaphore_Handle ready);
int or_acquisition_arm(void);
void or_acquisition_stop(void);
int or_acquisition_take(const RadarComplex16 **samples, RadarFrameInfo *frame,
                        unsigned *slot, uint32_t *generation);
int or_acquisition_release(unsigned slot,uint32_t generation);
uint32_t or_acquisition_fault(void);
uint32_t or_acquisition_completed(void);
uint32_t or_acquisition_frame_events(void);
uint32_t or_acquisition_last_ms(void);
#endif
