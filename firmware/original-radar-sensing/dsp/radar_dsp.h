#ifndef ORIGINAL_RADAR_DSP_H
#define ORIGINAL_RADAR_DSP_H

#include <stddef.h>
#include <stdint.h>

/* Independently authored, allocation-free range/Doppler measurements.
 * No TI demo signal processing, CFAR, tracker, beamformer or classifier. */
#ifndef RADAR_DSP_MAX_SAMPLES
#define RADAR_DSP_MAX_SAMPLES 64u
#endif
#ifndef RADAR_DSP_MAX_CHIRPS
#define RADAR_DSP_MAX_CHIRPS 32u
#endif
#define RADAR_DSP_MAX_RX 4u
#define RADAR_DSP_MAX_DETECTIONS 8u
#define RADAR_DSP_RANGE_BINS (RADAR_DSP_MAX_SAMPLES / 2u)

typedef struct { float real, imag; } RadarComplex;
/* TI ADCBUF sampleInterleave=0, channelInterleave=1; little-endian target.
 * Capture order: [chirp][receiver][sample]. No reinterpretation of Q/I. */
typedef struct { int16_t real, imag; } RadarComplex16;

typedef enum {
    RADAR_DSP_OK = 0,
    RADAR_DSP_BAD_CONFIG = -1,
    RADAR_DSP_BAD_FRAME = -2,
    RADAR_DSP_STALE_FRAME = -3
} RadarDspStatus;

enum {
    RADAR_QUALITY_SEQUENCE_GAP = 1u << 0,
    RADAR_QUALITY_ADC_CLIPPED = 1u << 1,
    RADAR_QUALITY_RX_PHASE_UNCALIBRATED = 1u << 2,
    RADAR_QUALITY_POWER_UNCALIBRATED = 1u << 3,
    RADAR_QUALITY_INVALID_FRAME = 1u << 4,
    RADAR_QUALITY_MOTION_SIGN_UNVERIFIED = 1u << 5
};

typedef struct {
    uint16_t adc_samples;              /* power of two, 16..MAX_SAMPLES */
    uint16_t chirps;                   /* power of two, 8..MAX_CHIRPS */
    uint16_t receivers;                /* 1..4 */
    uint16_t first_range_bin;          /* inclusive, >=1 */
    uint16_t last_range_bin;           /* inclusive, <adc_samples/2 */
    uint8_t physical_rx[RADAR_DSP_MAX_RX]; /* logical -> hardware RX index */
    uint8_t motion_sign_verified;      /* defaults false; hardware validation */
    uint8_t reserved[3];
    float sample_rate_hz;
    float chirp_slope_hz_per_s;         /* positive slope only */
    float chirp_period_s;              /* same TX, uniform chirp separation */
    float carrier_hz;                  /* RF at ADC-window center preferred */
    float minimum_power;               /* ADC-count squared after FFT normalization */
    float noise_multiplier;            /* >=1, global median background gate */
} RadarDspConfig;

typedef struct {
    uint32_t sequence;
    uint64_t timestamp_us;
    uint16_t valid_chirps;             /* MUST equal configured chirps */
    uint8_t capture_complete;          /* nonzero only after full coherent frame */
    uint8_t reserved;
} RadarFrameInfo;

typedef struct {
    uint16_t range_bin;
    int16_t doppler_bin;               /* [-chirps/2, chirps/2-1] */
    float range_m;                      /* bin center, no range-bias calibration */
    float radial_velocity_mps;         /* mathematical signed IQ convention */
    float power;                       /* RX-mean |range/Doppler coefficient|^2 */
    float background_power;            /* median of evaluated 2-D power bins */
    float range_total_power;            /* pre-Doppler time-mean RX-mean power */
    float range_static_power;           /* |unwindowed slow-time mean|^2 */
    float range_dynamic_power;          /* time variance, stationary energy retained */
    RadarComplex rx_phasor[RADAR_DSP_MAX_RX]; /* raw uncalibrated coherent coefficient */
} RadarDetection;

typedef struct {
    uint32_t sequence;
    uint64_t timestamp_us;
    uint32_t quality_flags;
    uint32_t missed_frames;             /* sequence gap only, not inferred elapsed time */
    uint32_t clipped_components;
    uint16_t detection_count;
    uint16_t reserved;
    float range_bin_m;
    float velocity_bin_mps;
    float background_power;
    float detection_threshold;
    RadarDetection detections[RADAR_DSP_MAX_DETECTIONS];
} RadarDspResult;

/* Workspace must have static/heap lifetime; do not put this on an MSS task stack.
 * The fixed upper bounds make runtime and RAM predictable; no heap is used. */
typedef struct {
    RadarDspConfig config;
    uint32_t magic;
    uint32_t last_sequence;
    uint8_t have_sequence;
    uint8_t padding[3];
    float range_window[RADAR_DSP_MAX_SAMPLES];
    float doppler_window[RADAR_DSP_MAX_CHIRPS];
    RadarComplex range_twiddle[RADAR_DSP_MAX_SAMPLES / 2u];
    RadarComplex doppler_twiddle[RADAR_DSP_MAX_CHIRPS / 2u];
    RadarComplex scratch[RADAR_DSP_MAX_SAMPLES > RADAR_DSP_MAX_CHIRPS ? RADAR_DSP_MAX_SAMPLES : RADAR_DSP_MAX_CHIRPS];
    RadarComplex cube[RADAR_DSP_RANGE_BINS][RADAR_DSP_MAX_RX][RADAR_DSP_MAX_CHIRPS];
    float power[RADAR_DSP_RANGE_BINS][RADAR_DSP_MAX_CHIRPS];
    float median_scratch[RADAR_DSP_RANGE_BINS * RADAR_DSP_MAX_CHIRPS];
    float total_power[RADAR_DSP_RANGE_BINS];
    float static_power[RADAR_DSP_RANGE_BINS];
} RadarDsp;

RadarDspStatus radar_dsp_init(RadarDsp *state, const RadarDspConfig *config);
/* Exact sample count is required. A partial/stale frame produces NO detections.
 * No samples from different frames are combined; a gap cannot fabricate speed. */
RadarDspStatus radar_dsp_process(RadarDsp *state, const RadarComplex16 *samples,
    size_t sample_count, const RadarFrameInfo *frame, RadarDspResult *result);
/* Configuration-independent original FFT. Forward, unnormalized, natural order.
 * n power-of-two; n>=2; twiddles[k]=exp(-2*pi*i*k/n), k<n/2. */
void radar_dsp_fft(RadarComplex *values, unsigned n, const RadarComplex *twiddles);

#endif
