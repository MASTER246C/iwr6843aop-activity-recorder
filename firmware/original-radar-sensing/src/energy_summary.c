#include "energy_summary.h"
#include <float.h>
int or_profile_energy(const RadarDsp *state,float values[3])
{
    unsigned r;float total=0.0f,stationary=0.0f,varying=0.0f,a,b;
    if(state==0||values==0||state->config.first_range_bin<1U||
       state->config.first_range_bin>state->config.last_range_bin||
       state->config.last_range_bin>=RADAR_DSP_RANGE_BINS)return -1;
    for(r=state->config.first_range_bin;r<=state->config.last_range_bin;++r) {
        a=state->total_power[r];b=state->static_power[r];
        if(!(a>=0.0f&&a<=FLT_MAX&&b>=0.0f&&b<=FLT_MAX))return -1;
        total+=a;stationary+=b;if(a>b)varying+=a-b;
    }
    if(!(total<=FLT_MAX&&stationary<=FLT_MAX&&varying<=FLT_MAX))return -1;
    values[0]=total;values[1]=stationary;values[2]=varying;return 0;
}
