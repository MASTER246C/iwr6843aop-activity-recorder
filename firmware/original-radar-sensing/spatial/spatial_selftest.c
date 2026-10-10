/* Independently authored deterministic estimator-only self-test.
 * No radio, serial, board, SDK, driver, scheduler or flash calls.
 * Return a failure bitmask; debugger can inspect os_selftest_result.
 * Build as a standalone target image with a reviewed non-RF startup.
 */
#include "original_spatial.h"
#include <math.h>
#include <stdint.h>
#ifdef OS_SELFTEST_HOST_MAIN
#include <stdio.h>
#endif
#define GUARD_A 0x51A7C0DEu
#define GUARD_B 0xA63E19B2u
volatile uint32_t os_selftest_result=0xffffffffu;
#ifdef OS_SCRATCH_GUARDS
extern volatile uint32_t os_scratch_guard_failures;
#endif
static int close_float(float a,float b) { return isfinite(a) && fabsf(a-b)<2.0e-5f; }
uint32_t os_spatial_no_rf_selftest(void) {
    struct { volatile uint32_t before[4]; os_complex rx[4]; volatile uint32_t after[4]; } in;
    struct { volatile uint32_t before[4]; os_result result; volatile uint32_t after[4]; } out;
    os_config cfg;
    uint32_t failed=0;
    unsigned i;
    float expected;
    for(i=0;i<4;i++) { in.before[i]=GUARD_A;in.after[i]=GUARD_B;out.before[i]=GUARD_A;out.after[i]=GUARD_B; }
    os_default_config(&cfg,61.378e9f);
    for(i=0;i<4;i++) { in.rx[i].real=1.0f;in.rx[i].imag=0.0f; }
    if(!os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=3u ||
       !close_float(out.result.x_sensor_m,0.0f) || !close_float(out.result.y_sensor_m,2.0f) ||
       !close_float(out.result.z_sensor_m,0.0f) || !isnan(out.result.z_world_m)) failed|=1u;
    /* Analytic +pi/4 phase difference on both axes, unequal components. */
    in.rx[0].real=0.0f; in.rx[0].imag=1.0f;
    in.rx[1].real=0.7071067811865475f; in.rx[1].imag=0.7071067811865475f;
    in.rx[2].real=0.7071067811865475f; in.rx[2].imag=0.7071067811865475f;
    in.rx[3].real=1.0f; in.rx[3].imag=0.0f;
    expected=299792458.0f/(8.0f*cfg.spacing_m*cfg.carrier_hz);
    if(!os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=3u ||
       !close_float(out.result.horizontal_direction_cosine,expected) ||
       !close_float(out.result.vertical_direction_cosine,expected)) failed|=2u;
    /* Nontrivial per-RX rotations undo known phase, amplitudes 2/4/8/16. */
    in.rx[0].real=2.0f;in.rx[0].imag=0.0f;
    in.rx[1].real=0.0f;in.rx[1].imag=4.0f;
    in.rx[2].real=-8.0f;in.rx[2].imag=0.0f;
    in.rx[3].real=0.0f;in.rx[3].imag=-16.0f;
    cfg.rx_correction[1].real=0.0f;cfg.rx_correction[1].imag=-1.0f;
    cfg.rx_correction[2].real=-1.0f;cfg.rx_correction[2].imag=0.0f;
    cfg.rx_correction[3].real=0.0f;cfg.rx_correction[3].imag=1.0f;
    if(!os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=3u ||
       !close_float(out.result.x_sensor_m,0.0f) || !close_float(out.result.y_sensor_m,2.0f) ||
       !close_float(out.result.z_sensor_m,0.0f)) failed|=4u;
    os_default_config(&cfg,61.378e9f);
    for(i=0;i<4;i++) { in.rx[i].real=0.0f;in.rx[i].imag=0.0f; }
    if(os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=0u) failed|=8u;
    for(i=0;i<4;i++) in.rx[i].real=1.0f;
    in.rx[2].real=NAN;
    if(os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=0u) failed|=16u;
    in.rx[2].real=1.0f;in.rx[0].real=-1.0f;
    if(os_estimate(&cfg,in.rx,2.0f,&out.result) || out.result.flags!=0u) failed|=32u;
    for(i=0;i<4;i++) {
        if(in.before[i]!=GUARD_A || in.after[i]!=GUARD_B || out.before[i]!=GUARD_A || out.after[i]!=GUARD_B) failed|=64u;
    }
#ifdef OS_SCRATCH_GUARDS
    if(os_scratch_guard_failures) failed|=128u;
#endif
    os_selftest_result=failed;
    return failed;
}
#ifdef OS_SELFTEST_HOST_MAIN
int main(void) { uint32_t f=os_spatial_no_rf_selftest(); printf("no-RF self-test failure mask: 0x%08lx\n",(unsigned long)f);return f?1:0; }
#endif
