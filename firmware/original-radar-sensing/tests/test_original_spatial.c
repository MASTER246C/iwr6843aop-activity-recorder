#include "original_spatial.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void wave(os_complex r[4],float carrier,float ux,float uz) {
    unsigned i;
    const float ax[4]={1,1,0,0}, az[4]={1,0,1,0};
    float scale=2.0f*3.14159265358979323846f*0.0025f*carrier/299792458.0f;
    for(i=0;i<4;i++) { float p=scale*(ax[i]*ux+az[i]*uz); r[i].real=cosf(p); r[i].imag=sinf(p); }
}
int main(void) {
    os_config c; os_result o; os_complex r[4]; unsigned i,j; unsigned count=0;
    os_default_config(&c,61.378e9f);
    assert(!c.antenna_offset_known && isnan(c.antenna_vertical_offset_m));
    wave(r,c.carrier_hz,0,0);
    assert(os_estimate(&c,r,2.0f,&o)); assert(isnan(o.z_world_m));
    assert(!(o.flags&OS_WORLD_HEIGHT_VALID)); assert(fabsf(o.y_sensor_m-2)<1e-5f);
    c.rx_phase_calibrated=1; c.axis_orientation_verified=1;
    assert(os_estimate(&c,r,2,&o)); assert(isnan(o.z_world_m));
    c.antenna_offset_known=1; c.antenna_vertical_offset_m=-0.1f;
    assert(os_estimate(&c,r,2,&o)); assert(o.flags&OS_WORLD_HEIGHT_VALID);
    assert(fabsf(o.z_world_m-(1.9f-2.0f*sinf(15.0f*3.14159265358979323846f/180.0f)))<1e-5f);
    for(i=0;i<=30;i++) for(j=0;j<=30;j++) {
        float x=((float)i-15.0f)/20.0f,z=((float)j-15.0f)/20.0f;
        if(x*x+z*z>=0.95f) continue;
        wave(r,c.carrier_hz,x,z); assert(os_estimate(&c,r,1.5f,&o));
        assert(fabsf(o.horizontal_direction_cosine-x)<2e-5f);
        assert(fabsf(o.vertical_direction_cosine-z)<2e-5f); count++;
    }
    wave(r,c.carrier_hz,0.3f,-0.2f);
    for(i=0;i<4;i++) { r[i].real*=1e30f; r[i].imag*=1e30f; }
    assert(os_estimate(&c,r,2,&o));
    for(i=0;i<4;i++) { r[i].real=0;r[i].imag=0; }
    assert(!os_estimate(&c,r,2,&o));
    wave(r,c.carrier_hz,0.9f,0); assert(!os_estimate(&c,r,2,&o));
    wave(r,c.carrier_hz,0,0); r[0].real=-1; assert(!os_estimate(&c,r,2,&o));
    wave(r,c.carrier_hz,0,0); r[2].real=NAN; assert(!os_estimate(&c,r,2,&o));
    wave(r,c.carrier_hz,0,0); c.antenna_vertical_offset_m=NAN; assert(!os_estimate(&c,r,2,&o));
    os_iwr6843aop_config(&c,61.378e9f);
    assert(!c.rx_phase_calibrated && !c.axis_orientation_verified && !c.antenna_offset_known);
    assert(c.rx_correction[0].real==1 && c.rx_correction[1].real==-1 &&
           c.rx_correction[2].real==1 && c.rx_correction[3].real==-1);
    for(i=0;i<=30;i++) for(j=0;j<=30;j++) {
        unsigned rx;float x=((float)i-15.0f)/20.0f,z=((float)j-15.0f)/20.0f;
        if(x*x+z*z>=0.95f)continue;
        wave(r,c.carrier_hz,x,z);
        for(rx=1;rx<4;rx+=2){r[rx].real=-r[rx].real;r[rx].imag=-r[rx].imag;}
        assert(os_estimate(&c,r,1.5f,&o));assert(o.flags==3U && isnan(o.z_world_m));
        assert(fabsf(o.horizontal_direction_cosine-x)<2e-5f);
        assert(fabsf(o.vertical_direction_cosine-z)<2e-5f);count++;
    }
    wave(r,c.carrier_hz,0,0);r[1].real=-1;r[3].real=-1;
    assert(os_estimate(&c,r,2,&o) && fabsf(o.y_sensor_m-2)<1e-5f);
    os_default_config(&c,c.carrier_hz);assert(!os_estimate(&c,r,2,&o));
    os_iwr6843aop_config(&c,61.378e9f);
    wave(r,c.carrier_hz,0.9f,0);r[1].real=-r[1].real;r[1].imag=-r[1].imag;r[3].real=-r[3].real;r[3].imag=-r[3].imag;
    assert(!os_estimate(&c,r,2,&o)); /* Same conservative FOV remains enforced. */
    wave(r,c.carrier_hz,0.8f,0.8f);r[1].real=-r[1].real;r[1].imag=-r[1].imag;r[3].real=-r[3].real;r[3].imag=-r[3].imag;
    assert(!os_estimate(&c,r,2,&o)); /* Physical direction disk still enforced. */
    puts("Original spatial tests passed: unknown height, signed offset, tilt, plane waves, boundaries, invalid inputs.");
    printf("%u direction cases\n",count);
    return 0;
}
