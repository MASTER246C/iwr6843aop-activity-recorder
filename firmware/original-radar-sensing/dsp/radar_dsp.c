#include "radar_dsp.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define RADAR_MAGIC 0x52445031u
#define RADAR_PI 3.14159265358979323846f
#define RADAR_C_MPS 299792458.0f

static int finite_positive(float x) { return isfinite(x) && x > 0.0f; }
static int power_of_two(unsigned n) { return n != 0u && (n & (n - 1u)) == 0u; }
static float magnitude_squared(RadarComplex z) { return z.real*z.real + z.imag*z.imag; }

void radar_dsp_fft(RadarComplex *values, unsigned n, const RadarComplex *twiddles)
{
    unsigned i, j, span;
    if (values == NULL || twiddles == NULL || n < 2u || !power_of_two(n)) return;
    /* Binary-reversal permutation followed by increasing radix-two butterflies. */
    j = 0u;
    for (i = 1u; i < n; ++i) {
        unsigned bit = n >> 1u;
        while ((j & bit) != 0u) { j ^= bit; bit >>= 1u; }
        j ^= bit;
        if (i < j) { RadarComplex temp = values[i]; values[i] = values[j]; values[j] = temp; }
    }
    for (span = 2u; span <= n; span <<= 1u) {
        unsigned base, half = span >> 1u, stride = n / span;
        for (base = 0u; base < n; base += span) {
            for (j = 0u; j < half; ++j) {
                RadarComplex a = values[base+j], b = values[base+j+half];
                RadarComplex w = twiddles[j*stride];
                float br = b.real*w.real - b.imag*w.imag;
                float bi = b.real*w.imag + b.imag*w.real;
                values[base+j].real = a.real + br;
                values[base+j].imag = a.imag + bi;
                values[base+j+half].real = a.real - br;
                values[base+j+half].imag = a.imag - bi;
            }
        }
        if (span == n) break; /* also prevents unsigned overflow for very large n */
    }
}

static int config_valid(const RadarDspConfig *c)
{
    unsigned i, j;
    float range_step, velocity_step;
    if (c == NULL || c->adc_samples < 16u || c->adc_samples > RADAR_DSP_MAX_SAMPLES ||
        !power_of_two(c->adc_samples) || c->chirps < 8u ||
        c->chirps > RADAR_DSP_MAX_CHIRPS || !power_of_two(c->chirps) ||
        c->receivers == 0u || c->receivers > RADAR_DSP_MAX_RX ||
        c->first_range_bin < 1u || c->last_range_bin < c->first_range_bin ||
        c->last_range_bin >= c->adc_samples/2u || !finite_positive(c->sample_rate_hz) ||
        !finite_positive(c->chirp_slope_hz_per_s) || !finite_positive(c->chirp_period_s) ||
        !finite_positive(c->carrier_hz) || !isfinite(c->minimum_power) || c->minimum_power < 0.0f ||
        !isfinite(c->noise_multiplier) || c->noise_multiplier < 1.0f) return 0;
    for (i = 0u; i < c->receivers; ++i) {
        if (c->physical_rx[i] >= RADAR_DSP_MAX_RX) return 0;
        for (j = 0u; j < i; ++j) if (c->physical_rx[i] == c->physical_rx[j]) return 0;
    }
    /* Reject finite inputs whose derived scales overflow or underflow. */
    range_step = (RADAR_C_MPS/c->chirp_slope_hz_per_s) *
                 (c->sample_rate_hz/(2.0f*c->adc_samples));
    velocity_step = (RADAR_C_MPS/c->carrier_hz) /
                    (2.0f*c->chirps*c->chirp_period_s);
    if (!finite_positive(range_step) || !finite_positive(velocity_step) ||
        !finite_positive(range_step*(float)c->last_range_bin) ||
        !finite_positive(velocity_step*(float)(c->chirps/2u))) return 0;
    return 1;
}

RadarDspStatus radar_dsp_init(RadarDsp *state, const RadarDspConfig *config)
{
    unsigned i;
    RadarDspConfig copied;
    if (state == NULL) return RADAR_DSP_BAD_CONFIG;
    if (!config_valid(config)) { state->magic = 0u; return RADAR_DSP_BAD_CONFIG; }
    copied = *config; /* permits reinitializing with &state->config */
    memset(state, 0, sizeof(*state));
    state->config = copied;
    for (i = 0u; i < copied.adc_samples; ++i) {
        float angle = 2.0f*RADAR_PI*(float)i/(float)copied.adc_samples;
        /* Normalize periodic Hann by its exact sum N/2. */
        state->range_window[i] = (1.0f-cosf(angle))/(float)copied.adc_samples;
        if (i < copied.adc_samples/2u) {
            state->range_twiddle[i].real = cosf(angle);
            state->range_twiddle[i].imag = -sinf(angle);
        }
    }
    for (i = 0u; i < copied.chirps; ++i) {
        float angle = 2.0f*RADAR_PI*(float)i/(float)copied.chirps;
        state->doppler_window[i] = (1.0f-cosf(angle))/(float)copied.chirps;
        if (i < copied.chirps/2u) {
            state->doppler_twiddle[i].real = cosf(angle);
            state->doppler_twiddle[i].imag = -sinf(angle);
        }
    }
    state->magic = RADAR_MAGIC;
    return RADAR_DSP_OK;
}

/* Deterministic in-place heapsort for the bounded background median buffer.
 * Unlike recursive quickselect, its worst-case work and stack are explicit. */
static void heap_down(float *v, unsigned root, unsigned count)
{
    for (;;) {
        unsigned child = root*2u + 1u;
        float temp;
        if (child >= count) return;
        if (child+1u < count && v[child+1u] > v[child]) ++child;
        if (v[root] >= v[child]) return;
        temp = v[root]; v[root] = v[child]; v[child] = temp;
        root = child;
    }
}

static float median(float *values, unsigned count)
{
    unsigned i;
    for (i = count/2u; i > 0u; --i) heap_down(values, i-1u, count);
    for (i = count; i > 1u; --i) {
        float temp = values[0]; values[0] = values[i-1u]; values[i-1u] = temp;
        heap_down(values, 0u, i-1u);
    }
    if ((count & 1u) != 0u) return values[count/2u];
    return 0.5f*(values[count/2u-1u]+values[count/2u]);
}

static int local_peak(const RadarDsp *s, unsigned r, unsigned d)
{
    int dr, dd;
    unsigned chirps = s->config.chirps;
    float power = s->power[r][d];
    unsigned index = r*chirps+d;
    for (dr = -1; dr <= 1; ++dr) {
        int nr = (int)r+dr;
        if (nr < (int)s->config.first_range_bin || nr > (int)s->config.last_range_bin) continue;
        for (dd = -1; dd <= 1; ++dd) {
            unsigned nd = (unsigned)((int)d+dd+(int)chirps) % chirps;
            unsigned neighbor = (unsigned)nr*chirps+nd;
            float other = s->power[nr][nd];
            if (neighbor == index) continue;
            /* Deterministic plateau tie-breaker, independent of traversal order. */
            if (other > power || (other == power && neighbor < index)) return 0;
        }
    }
    return 1;
}

static void add_detection(const RadarDsp *s, RadarDspResult *out, unsigned r, unsigned d)
{
    unsigned position, rx;
    RadarDetection candidate;
    float power = s->power[r][d];
    if (out->detection_count == RADAR_DSP_MAX_DETECTIONS &&
        power <= out->detections[RADAR_DSP_MAX_DETECTIONS-1u].power) return;
    memset(&candidate, 0, sizeof(candidate));
    candidate.range_bin = (uint16_t)r;
    candidate.doppler_bin = (int16_t)(d < s->config.chirps/2u ? (int)d : (int)d-(int)s->config.chirps);
    candidate.range_m = (float)r*out->range_bin_m;
    candidate.radial_velocity_mps = (float)candidate.doppler_bin*out->velocity_bin_mps;
    candidate.power = power;
    candidate.background_power = out->background_power;
    candidate.range_total_power = s->total_power[r];
    candidate.range_static_power = s->static_power[r];
    candidate.range_dynamic_power = s->total_power[r]-s->static_power[r];
    if (candidate.range_dynamic_power < 0.0f) candidate.range_dynamic_power = 0.0f;
    for (rx = 0u; rx < s->config.receivers; ++rx) candidate.rx_phasor[rx] = s->cube[r][rx][d];
    position = out->detection_count < RADAR_DSP_MAX_DETECTIONS ?
        out->detection_count++ : RADAR_DSP_MAX_DETECTIONS-1u;
    while (position > 0u && candidate.power > out->detections[position-1u].power) {
        out->detections[position] = out->detections[position-1u]; --position;
    }
    out->detections[position] = candidate;
}

RadarDspStatus radar_dsp_process(RadarDsp *state, const RadarComplex16 *samples,
    size_t sample_count, const RadarFrameInfo *frame, RadarDspResult *result)
{
    const RadarDspConfig *c;
    unsigned chirp, rx, sample, r, d, background_count = 0u;
    float receiver_scale;
    uint32_t advance = 1u;
    if (result == NULL) return RADAR_DSP_BAD_FRAME;
    memset(result, 0, sizeof(*result));
    result->quality_flags = RADAR_QUALITY_INVALID_FRAME |
        RADAR_QUALITY_RX_PHASE_UNCALIBRATED | RADAR_QUALITY_POWER_UNCALIBRATED;
    if (frame != NULL) { result->sequence = frame->sequence; result->timestamp_us = frame->timestamp_us; }
    if (state == NULL || state->magic != RADAR_MAGIC) return RADAR_DSP_BAD_CONFIG;
    c = &state->config;
    if (!c->motion_sign_verified) result->quality_flags |= RADAR_QUALITY_MOTION_SIGN_UNVERIFIED;
    if (samples == NULL || frame == NULL || !frame->capture_complete || frame->valid_chirps != c->chirps ||
        sample_count != (size_t)c->adc_samples*c->chirps*c->receivers) return RADAR_DSP_BAD_FRAME;
    if (state->have_sequence) {
        advance = frame->sequence-state->last_sequence;
        /* Unsigned modulo arithmetic permits normal UINT32 sequence wrap. */
        if (advance == 0u || advance >= 0x80000000u) return RADAR_DSP_STALE_FRAME;
    }
    result->quality_flags &= ~RADAR_QUALITY_INVALID_FRAME;
    if (advance > 1u) {
        result->quality_flags |= RADAR_QUALITY_SEQUENCE_GAP;
        result->missed_frames = advance-1u;
    }
    result->range_bin_m = (RADAR_C_MPS/c->chirp_slope_hz_per_s) *
        (c->sample_rate_hz/(2.0f*c->adc_samples));
    result->velocity_bin_mps = (RADAR_C_MPS/c->carrier_hz)/(2.0f*c->chirps*c->chirp_period_s);
    receiver_scale = 1.0f/(float)c->receivers;
    for (chirp = 0u; chirp < c->chirps; ++chirp) {
        for (rx = 0u; rx < c->receivers; ++rx) {
            const RadarComplex16 *input = samples+((size_t)chirp*c->receivers+rx)*c->adc_samples;
            for (sample = 0u; sample < c->adc_samples; ++sample) {
                float window = state->range_window[sample];
                if (input[sample].real == INT16_MIN || input[sample].real == INT16_MAX) ++result->clipped_components;
                if (input[sample].imag == INT16_MIN || input[sample].imag == INT16_MAX) ++result->clipped_components;
                state->scratch[sample].real = (float)input[sample].real*window;
                state->scratch[sample].imag = (float)input[sample].imag*window;
            }
            radar_dsp_fft(state->scratch, c->adc_samples, state->range_twiddle);
            for (r = c->first_range_bin; r <= c->last_range_bin; ++r)
                state->cube[r][rx][chirp] = state->scratch[r];
        }
    }
    if (result->clipped_components != 0u) result->quality_flags |= RADAR_QUALITY_ADC_CLIPPED;
    for (r = c->first_range_bin; r <= c->last_range_bin; ++r) {
        state->total_power[r] = 0.0f;
        state->static_power[r] = 0.0f;
        for (d = 0u; d < c->chirps; ++d) state->power[r][d] = 0.0f;
        for (rx = 0u; rx < c->receivers; ++rx) {
            RadarComplex mean = {0.0f, 0.0f};
            float energy = 0.0f;
            for (chirp = 0u; chirp < c->chirps; ++chirp) {
                RadarComplex v = state->cube[r][rx][chirp];
                mean.real += v.real; mean.imag += v.imag;
                energy += magnitude_squared(v);
                state->scratch[chirp].real = v.real*state->doppler_window[chirp];
                state->scratch[chirp].imag = v.imag*state->doppler_window[chirp];
            }
            mean.real /= (float)c->chirps; mean.imag /= (float)c->chirps;
            state->total_power[r] += energy*receiver_scale/(float)c->chirps;
            state->static_power[r] += magnitude_squared(mean)*receiver_scale;
            radar_dsp_fft(state->scratch, c->chirps, state->doppler_twiddle);
            for (d = 0u; d < c->chirps; ++d) {
                state->cube[r][rx][d] = state->scratch[d];
                state->power[r][d] += magnitude_squared(state->scratch[d])*receiver_scale;
            }
        }
        for (d = 0u; d < c->chirps; ++d) state->median_scratch[background_count++] = state->power[r][d];
    }
    result->background_power = median(state->median_scratch, background_count);
    /* Keep diagnostics finite even with an extremely large configured gate. */
    result->detection_threshold = result->background_power > FLT_MAX/c->noise_multiplier ?
        FLT_MAX : result->background_power*c->noise_multiplier;
    if (result->detection_threshold < c->minimum_power) result->detection_threshold = c->minimum_power;
    for (r = c->first_range_bin; r <= c->last_range_bin; ++r) {
        for (d = 0u; d < c->chirps; ++d) {
            if (state->power[r][d] > result->detection_threshold && local_peak(state, r, d))
                add_detection(state, result, r, d);
        }
    }
    state->last_sequence = frame->sequence;
    state->have_sequence = 1u;
    return RADAR_DSP_OK;
}
