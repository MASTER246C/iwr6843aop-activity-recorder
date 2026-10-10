#ifndef ORIGINAL_JSON_OUTPUT_H
#define ORIGINAL_JSON_OUTPUT_H
#include <stddef.h>
#include <stdint.h>
#include "radar_dsp.h"
#include "original_spatial.h"
#include "sensing_config.h"
#define OR_JSON_MAX 1800U
#define OR_JSON_MAX_DETECTIONS 3U
typedef struct {
    uint32_t processing_us,previous_tx_us,deadline_misses,capture_fault,frame_events;
    float profile_energy[3];
} OrTiming;
size_t or_frame_json(char *out,size_t capacity,const OrConfig *cfg,const os_config *spatial,
                     const RadarDspResult *result,const OrTiming *timing);
#endif
