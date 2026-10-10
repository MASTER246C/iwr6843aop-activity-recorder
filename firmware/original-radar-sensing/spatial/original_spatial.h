#ifndef ORIGINAL_SPATIAL_H
#define ORIGINAL_SPATIAL_H
#include <stdint.h>
/* Original, coarse 2x2-array phase interferometry. No tracking or classifier. */
typedef struct { float real, imag; } os_complex;
typedef struct {
    float carrier_hz;
    float spacing_m;
    float bracket_height_m;
    float antenna_vertical_offset_m; /* Signed vertical offset AFTER tilt. */
    float down_tilt_deg;
    float horizontal_phase_sign; /* Explicitly +1 or -1; board check required. */
    float vertical_phase_sign;
    float maximum_direction_cosine;
    float minimum_pair_coherence;
    os_complex rx_correction[4];
    uint8_t antenna_offset_known;
    uint8_t rx_phase_calibrated;
    uint8_t axis_orientation_verified;
} os_config;
enum {
    OS_NUMERIC_VALID = 1u,
    OS_ANGLE_IN_FOV = 2u,
    OS_PHASE_CALIBRATED = 4u,
    OS_AXES_VERIFIED = 8u,
    OS_WORLD_HEIGHT_VALID = 16u
};
typedef struct {
    float range_m;
    float horizontal_direction_cosine;
    float vertical_direction_cosine;
    float pair_coherence;
    float x_sensor_m, y_sensor_m, z_sensor_m;
    float x_world_m, y_world_m, z_world_m;
    uint32_t flags;
} os_result;
void os_default_config(os_config *cfg, float carrier_hz);
/* RX order physical array offsets [(1,1),(1,0),(0,1),(0,0)].
 * Coordinates are exploratory unless phase and axis flags are both valid.
 * Returns 1 for finite in-FOV sensor estimate; world height may remain unknown.
 * On failure result is cleared. Unknown world coordinates are NAN, never 0. */
int os_estimate(const os_config *cfg, const os_complex rx[4], float range_m,
                os_result *result);
#endif
