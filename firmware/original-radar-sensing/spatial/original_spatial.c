/* Independently authored mathematical phase-pair estimator.
 * Hardware facts: TI SDK ti/board/antenna_geometry.c/h, IWR6843AOP RX offsets
 * [(1,1),(1,0),(0,1),(0,0)], pitch 2.5 mm. No TI algorithm source is imported.
 */
#include "original_spatial.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#define OS_PI 3.14159265358979323846f
#define OS_C 299792458.0f
static int finite_number(float x) { return isfinite(x) != 0; }
static os_complex conjugate_product(os_complex a, os_complex b) {
    os_complex r;
    r.real=a.real*b.real+a.imag*b.imag;
    r.imag=a.imag*b.real-a.real*b.imag;
    return r;
}
static float length(os_complex a) { return sqrtf(a.real*a.real+a.imag*a.imag); }
static os_complex add(os_complex a, os_complex b) {
    os_complex r; r.real=a.real+b.real; r.imag=a.imag+b.imag; return r;
}
void os_default_config(os_config *cfg, float carrier_hz) {
    unsigned i;
    if (cfg == NULL) return;
    memset(cfg,0,sizeof(*cfg));
    cfg->carrier_hz=carrier_hz;
    cfg->spacing_m=0.0025f;
    cfg->bracket_height_m=2.0f;
    cfg->antenna_vertical_offset_m=NAN;
    cfg->down_tilt_deg=15.0f;
    cfg->horizontal_phase_sign=1.0f;
    cfg->vertical_phase_sign=1.0f;
    cfg->maximum_direction_cosine=0.85f;
    cfg->minimum_pair_coherence=0.8f;
    for (i=0;i<4;i++) cfg->rx_correction[i].real=1.0f;
}
/* Fixed antenna-feed polarity, not measured per-unit phase calibration.
 * TI SWRU546E Rev G guide Figure 4-8 documents alternating RX groups 180 deg
 * apart. SDK 03.06.02.00-LTS AOP profile uses +1,-1,+1,-1 in physical RX order.
 * Inverting the complementary group differs only by a common phase factor.
 */
void os_iwr6843aop_config(os_config *cfg, float carrier_hz) {
    os_default_config(cfg,carrier_hz);
    if(cfg==NULL)return;
    cfg->rx_correction[1].real=-1.0f;
    cfg->rx_correction[3].real=-1.0f;
}
int os_estimate(const os_config *cfg, const os_complex rx[4], float range_m,
                os_result *out) {
    os_complex r[4], a,b,x,z;
    float amplitudes[4], largest=0.0f, dx,dz,cx,cz,phase_scale,ux,uz,disk;
    float radians,c,s;
    unsigned i;
    if (out == NULL) return 0;
    memset(out,0,sizeof(*out));
    out->x_world_m=NAN; out->y_world_m=NAN; out->z_world_m=NAN;
    if (cfg == NULL || rx == NULL || !finite_number(range_m) || range_m<=0.0f ||
        !finite_number(cfg->carrier_hz) || cfg->carrier_hz<60.0e9f || cfg->carrier_hz>64.0e9f ||
        !finite_number(cfg->spacing_m) || cfg->spacing_m<=0.0f || cfg->spacing_m>0.01f ||
        !finite_number(cfg->bracket_height_m) || cfg->bracket_height_m<0.0f ||
        !finite_number(cfg->down_tilt_deg) || fabsf(cfg->down_tilt_deg)>90.0f ||
        !(cfg->horizontal_phase_sign==1.0f || cfg->horizontal_phase_sign==-1.0f) ||
        !(cfg->vertical_phase_sign==1.0f || cfg->vertical_phase_sign==-1.0f) ||
        !finite_number(cfg->maximum_direction_cosine) || cfg->maximum_direction_cosine<=0.0f || cfg->maximum_direction_cosine>=1.0f ||
        !finite_number(cfg->minimum_pair_coherence) || cfg->minimum_pair_coherence<0.0f || cfg->minimum_pair_coherence>1.0f ||
        (cfg->antenna_offset_known && (!finite_number(cfg->antenna_vertical_offset_m) ||
         cfg->bracket_height_m+cfg->antenna_vertical_offset_m<=0.0f))) return 0;
    /* Scale inputs before pair products to avoid overflow with large FFT values. */
    for(i=0;i<4;i++) {
        if(!finite_number(rx[i].real)||!finite_number(rx[i].imag)||
           !finite_number(cfg->rx_correction[i].real)||!finite_number(cfg->rx_correction[i].imag)) return 0;
        if(fabsf(rx[i].real)>largest) largest=fabsf(rx[i].real);
        if(fabsf(rx[i].imag)>largest) largest=fabsf(rx[i].imag);
    }
    if(largest<1.0e-20f) return 0;
    for(i=0;i<4;i++) {
        /* Candidate workaround: avoid aggregate-return assignment to r[i].
         * Keep component stores explicit; inspect TI target assembly before use.
         * This does not establish a compiler defect or validate target code. */
        const float sr=rx[i].real/largest, si=rx[i].imag/largest;
        const float cr=cfg->rx_correction[i].real, ci=cfg->rx_correction[i].imag;
        r[i].real=sr*cr-si*ci;
        r[i].imag=sr*ci+si*cr;
        if(!finite_number(r[i].real)||!finite_number(r[i].imag)) return 0;
        amplitudes[i]=length(r[i]);
        if(!finite_number(amplitudes[i])||amplitudes[i]<1.0e-12f) return 0;
    }
    a=conjugate_product(r[0],r[2]); b=conjugate_product(r[1],r[3]); x=add(a,b);
    dx=amplitudes[0]*amplitudes[2]+amplitudes[1]*amplitudes[3];
    a=conjugate_product(r[0],r[1]); b=conjugate_product(r[2],r[3]); z=add(a,b);
    dz=amplitudes[0]*amplitudes[1]+amplitudes[2]*amplitudes[3];
    cx=length(x)/dx; cz=length(z)/dz;
    if(!finite_number(cx)||!finite_number(cz)) return 0;
    out->pair_coherence=fminf(1.0f,fminf(cx,cz));
    if(out->pair_coherence<cfg->minimum_pair_coherence) return 0;
    phase_scale=2.0f*OS_PI*cfg->spacing_m*cfg->carrier_hz/OS_C;
    /* Our conservative FOV must lie inside the unambiguous phase interval. */
    if(phase_scale*cfg->maximum_direction_cosine>=OS_PI) return 0;
    ux=cfg->horizontal_phase_sign*atan2f(x.imag,x.real)/phase_scale;
    uz=cfg->vertical_phase_sign*atan2f(z.imag,z.real)/phase_scale;
    disk=ux*ux+uz*uz;
    if(!finite_number(disk) || fabsf(ux)>cfg->maximum_direction_cosine ||
       fabsf(uz)>cfg->maximum_direction_cosine || disk>=1.0f) return 0;
    out->range_m=range_m;
    out->horizontal_direction_cosine=ux;
    out->vertical_direction_cosine=uz;
    out->x_sensor_m=range_m*ux;
    out->y_sensor_m=range_m*sqrtf(1.0f-disk);
    out->z_sensor_m=range_m*uz;
    out->flags=OS_NUMERIC_VALID|OS_ANGLE_IN_FOV;
    if(cfg->rx_phase_calibrated) out->flags|=OS_PHASE_CALIBRATED;
    if(cfg->axis_orientation_verified) out->flags|=OS_AXES_VERIFIED;
    if(cfg->rx_phase_calibrated && cfg->axis_orientation_verified) {
        radians=cfg->down_tilt_deg*OS_PI/180.0f; c=cosf(radians); s=sinf(radians);
        out->x_world_m=out->x_sensor_m;
        out->y_world_m=out->y_sensor_m*c+out->z_sensor_m*s;
        if(cfg->antenna_offset_known) {
            out->z_world_m=cfg->bracket_height_m+cfg->antenna_vertical_offset_m
                          +out->z_sensor_m*c-out->y_sensor_m*s;
            out->flags|=OS_WORLD_HEIGHT_VALID;
        }
    }
    return 1;
}
