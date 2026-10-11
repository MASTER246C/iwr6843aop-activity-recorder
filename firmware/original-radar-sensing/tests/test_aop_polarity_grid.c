/* Synthetic analytical waves only. No private captures or hardware/RF calls. */
#include "original_spatial.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void wave(os_complex rx[4],float carrier,float ux,float uz,float common) {
 const float x[4]={1,1,0,0},z[4]={1,0,1,0},pol[4]={1,-1,1,-1},amp[4]={0.25f,2,0.5f,4};
 const float scale=2.0f*3.14159265358979323846f*0.0025f*carrier/299792458.0f;
 for(unsigned i=0;i<4;i++){float a=common+scale*(x[i]*ux+z[i]*uz);rx[i].real=pol[i]*amp[i]*cosf(a);rx[i].imag=pol[i]*amp[i]*sinf(a);}
}
int main(void){
 os_config c;os_complex rx[4];os_result r;unsigned count=0;float carriers[3]={60e9f,61.378e9f,64e9f};
 os_iwr6843aop_config(NULL,61e9f);
 for(unsigned f=0;f<3;f++)for(int x=-8;x<=8;x++)for(int z=-8;z<=8;z++)for(unsigned phase=0;phase<4;phase++) {
  float ux=x*0.1f,uz=z*0.1f;if(ux*ux+uz*uz>=0.99f)continue;
  os_iwr6843aop_config(&c,carriers[f]);wave(rx,carriers[f],ux,uz,phase*1.2f);
  assert(!c.rx_phase_calibrated&&!c.axis_orientation_verified&&!c.antenna_offset_known);
  assert(c.maximum_direction_cosine==0.85f&&c.minimum_pair_coherence==0.8f);
  assert(os_estimate(&c,rx,2.5f,&r));assert(r.flags==3U);
  assert(fabsf(r.horizontal_direction_cosine-ux)<2e-6f&&fabsf(r.vertical_direction_cosine-uz)<2e-6f);
  assert(fabsf(r.x_sensor_m-2.5f*ux)<1e-5f&&fabsf(r.z_sensor_m-2.5f*uz)<1e-5f);
  assert(isnan(r.x_world_m)&&isnan(r.y_world_m)&&isnan(r.z_world_m));
  for(unsigned i=0;i<4;i++){rx[i].real=-rx[i].real;rx[i].imag=-rx[i].imag;}
  assert(os_estimate(&c,rx,2.5f,&r)&&r.flags==3U);
  assert(fabsf(r.horizontal_direction_cosine-ux)<2e-6f&&fabsf(r.vertical_direction_cosine-uz)<2e-6f);count++;
 }
 os_iwr6843aop_config(&c,61.378e9f);wave(rx,c.carrier_hz,0.9f,0,0);assert(!os_estimate(&c,rx,2.5f,&r));
 wave(rx,c.carrier_hz,0,0.9f,0);assert(!os_estimate(&c,rx,2.5f,&r));
 wave(rx,c.carrier_hz,0.8f,0.8f,0);assert(!os_estimate(&c,rx,2.5f,&r));
 wave(rx,c.carrier_hz,0,0,0);rx[0].real=NAN;assert(!os_estimate(&c,rx,2.5f,&r));
 os_default_config(&c,61.378e9f);wave(rx,c.carrier_hz,0,0,0);assert(!os_estimate(&c,rx,2.5f,&r));
 printf("PASS: %u synthetic AOP direction/carrier/common-phase cases plus common-sign invariance; existing FOV/disk/nonfinite guards and uncalibrated world state preserved.\n",count);return 0;
}
