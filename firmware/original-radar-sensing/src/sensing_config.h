#ifndef ORIGINAL_SENSING_CONFIG_H
#define ORIGINAL_SENSING_CONFIG_H
#include <stddef.h>
#include <stdint.h>
#define OR_ADC_SAMPLES 64U
#define OR_CHIRPS 32U
#define OR_RX 4U
#define OR_FRAME_SAMPLES (OR_ADC_SAMPLES * OR_CHIRPS * OR_RX)
#define OR_CHIRP_BYTES (OR_ADC_SAMPLES * OR_RX * 4U)
#define OR_FRAME_BYTES (OR_FRAME_SAMPLES * 4U)
#define OR_CAPTURE_IQ_REAL_FIRST 1U
#define OR_FRAME_PERIOD_MS 200U
#define OR_LINE_SIZE 192U

typedef struct {
    int32_t bracket_top_mm;
    int32_t down_tilt_mdeg;
    int32_t antenna_vertical_offset_mm;
    uint32_t minimum_power;
    uint32_t noise_multiplier;
    uint16_t first_range_bin;
    uint16_t last_range_bin;
    uint16_t session_seconds;
    uint8_t antenna_offset_known;
    uint8_t committed;
    uint8_t fields_seen;
    uint8_t reserved;
} OrConfig;
typedef enum { OR_CMD_NONE=0, OR_CMD_CHANGED=1, OR_CMD_COMMIT=2,
               OR_CMD_START=3, OR_CMD_STOP=4, OR_CMD_STATUS=5,
               OR_CMD_INVALID=-1, OR_CMD_BUSY=-2, OR_CMD_INCOMPLETE=-3 } OrCommand;
void or_config_reset(OrConfig *cfg);
OrCommand or_config_line(OrConfig *cfg, const char *line, int running);
int or_config_valid(const OrConfig *cfg);
#endif
