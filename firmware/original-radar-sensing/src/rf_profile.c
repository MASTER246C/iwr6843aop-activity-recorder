#include "rf_profile.h"
#include <stddef.h>
int or_rf_physical(double scale,OrRfPhysical *p)
{
    double slope_lsb_khz_per_us;
    if(p==NULL || !(scale>=2.699 && scale<=2.701))return -1;
    p->start_code=((uint32_t)(60.25*67108864.0/scale))&~1U;
    p->slope_code=(int16_t)(((uint32_t)(30.0*67108864.0/(scale*1000.0*900.0)))&~1U);
    p->start_hz=p->start_code*scale*1e9/67108864.0;
    /* TI60GHz slope LSB=(2.7e6*900)/2^26 kHz/us. kHz/us→Hz/s is×1e9. */
    slope_lsb_khz_per_us=scale*1e6*900.0/67108864.0;
    p->slope_hz_per_s=p->slope_code*slope_lsb_khz_per_us*1e9;
    p->carrier_hz=p->start_hz+p->slope_hz_per_s*(6e-6+64.0/2.5e6/2.0);
    if(p->start_hz<60e9 || p->start_hz+p->slope_hz_per_s*40e-6>64e9 ||
       p->slope_hz_per_s<29e12 || p->slope_hz_per_s>30e12)return -2;
    return 0;
}
