#include "radar_dsp.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.1415926535897932384626433832795
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } ++checks; } while (0)
static unsigned checks;
static RadarDsp state;
static RadarDspResult result;
static RadarComplex16 frame[RADAR_DSP_MAX_SAMPLES*RADAR_DSP_MAX_CHIRPS*RADAR_DSP_MAX_RX];

static RadarDspConfig config(void)
{
    RadarDspConfig c;
    unsigned i;
    memset(&c,0,sizeof(c));
    c.adc_samples = 64u; c.chirps = 32u; c.receivers = 4u;
    c.first_range_bin = 1u; c.last_range_bin = 31u;
    for (i=0u;i<4u;++i) c.physical_rx[i]=(uint8_t)i;
    c.sample_rate_hz = 2500000.0f; c.chirp_slope_hz_per_s = 60000000000000.0f;
    c.chirp_period_s = 0.000250f; c.carrier_hz = 61378000000.0f;
    c.minimum_power = 25.0f; c.noise_multiplier = 12.0f;
    return c;
}

static RadarFrameInfo info(uint32_t sequence, unsigned chirps)
{
    RadarFrameInfo f;
    memset(&f,0,sizeof(f)); f.sequence=sequence; f.timestamp_us=(uint64_t)sequence*200000u;
    f.valid_chirps=(uint16_t)chirps; f.capture_complete=1u; return f;
}

static size_t sample_count(const RadarDspConfig *c) { return (size_t)c->adc_samples*c->chirps*c->receivers; }
static void clear_frame(void) { memset(frame,0,sizeof(frame)); }

static void add_tone(const RadarDspConfig *c, double range, double doppler, double amplitude, const double *phase)
{
    unsigned k,rx,n;
    for(k=0u;k<c->chirps;++k) for(rx=0u;rx<c->receivers;++rx) for(n=0u;n<c->adc_samples;++n) {
        size_t at=((size_t)k*c->receivers+rx)*c->adc_samples+n;
        double angle=2.0*PI*(range*(double)n/c->adc_samples+doppler*(double)k/c->chirps)+(phase?phase[rx]:0.0);
        long real=(long)frame[at].real+lround(amplitude*cos(angle));
        long imag=(long)frame[at].imag+lround(amplitude*sin(angle));
        if(real<INT16_MIN || real>INT16_MAX || imag<INT16_MIN || imag>INT16_MAX) abort();
        frame[at].real=(int16_t)real; frame[at].imag=(int16_t)imag;
    }
}

static const RadarDetection *find_peak(unsigned r,int d)
{
    unsigned i;
    for(i=0u;i<result.detection_count;++i)
        if(result.detections[i].range_bin==r && result.detections[i].doppler_bin==d) return &result.detections[i];
    return NULL;
}
static int near(double a,double b,double relative) { return fabs(a-b)<=relative*fmax(1.0,fabs(b)); }


/* Independent direct 2-D mathematical sum on the actual quantized samples;
 * shares neither FFT code nor window tables with the implementation. */
static void check_direct_2d(const RadarDspConfig *c, const RadarDetection *p)
{
    unsigned rx,k,n;
    for(rx=0u;rx<c->receivers;++rx) {
        long double re=0.0L,im=0.0L;
        for(k=0u;k<c->chirps;++k) for(n=0u;n<c->adc_samples;++n) {
            size_t at=((size_t)k*c->receivers+rx)*c->adc_samples+n;
            long double wn=(1.0L-cosl(2.0L*PI*n/c->adc_samples))/c->adc_samples;
            long double wk=(1.0L-cosl(2.0L*PI*k/c->chirps))/c->chirps;
            long double angle=-2.0L*PI*((long double)p->range_bin*n/c->adc_samples+(long double)p->doppler_bin*k/c->chirps);
            re+=wn*wk*(frame[at].real*cosl(angle)-frame[at].imag*sinl(angle));
            im+=wn*wk*(frame[at].real*sinl(angle)+frame[at].imag*cosl(angle));
        }
        CHECK(fabs((double)p->rx_phasor[rx].real-(double)re)<0.002);
        CHECK(fabs((double)p->rx_phasor[rx].imag-(double)im)<0.002);
    }
}

static void test_fft_reference(void)
{
    unsigned n,i,k;
    uint32_t rng=0x916923u;
    for(n=2u;n<=RADAR_DSP_MAX_SAMPLES;n*=2u) {
        RadarComplex x[RADAR_DSP_MAX_SAMPLES], original[RADAR_DSP_MAX_SAMPLES], w[RADAR_DSP_MAX_SAMPLES/2u];
        double input_energy=0.0,output_energy=0.0;
        for(i=0u;i<n;++i) {
            rng=rng*1664525u+1013904223u; x[i].real=(float)((int)(rng>>16u)-32768)/32768.0f;
            rng=rng*1664525u+1013904223u; x[i].imag=(float)((int)(rng>>16u)-32768)/32768.0f;
            input_energy+=(double)x[i].real*x[i].real+(double)x[i].imag*x[i].imag;
            original[i]=x[i];
            if(i<n/2u) { w[i].real=(float)cos(-2.0*PI*i/n); w[i].imag=(float)sin(-2.0*PI*i/n); }
        }
        radar_dsp_fft(x,n,w);
        for(k=0u;k<n;++k) {
            long double re=0.0L,im=0.0L;
            for(i=0u;i<n;++i) {
                long double angle=-2.0L*(long double)PI*k*i/n;
                re+=(long double)original[i].real*cosl(angle)-(long double)original[i].imag*sinl(angle);
                im+=(long double)original[i].real*sinl(angle)+(long double)original[i].imag*cosl(angle);
            }
            CHECK(fabs((double)x[k].real-(double)re)<0.00004);
            CHECK(fabs((double)x[k].imag-(double)im)<0.00004);
            output_energy+=(double)x[k].real*x[k].real+(double)x[k].imag*x[k].imag;
        }
        CHECK(near(output_energy/n,input_energy,0.000001));
    }
}

static void test_static_and_moving(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f=info(100u,c.chirps);
    const RadarDetection *still,*moving,*negative;
    const double phase[4]={0.0,0.20,-0.30,0.50};
    unsigned rx;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,8.0,0.0,2200.0,phase); add_tone(&c,8.0,5.0,1600.0,phase); add_tone(&c,20.0,-7.0,1200.0,phase);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    still=find_peak(8u,0); moving=find_peak(8u,5); negative=find_peak(20u,-7);
    CHECK(still!=NULL); CHECK(moving!=NULL); CHECK(negative!=NULL);
    check_direct_2d(&c,still); check_direct_2d(&c,moving); check_direct_2d(&c,negative);
    CHECK(near(still->power,2200.0*2200.0,0.001));
    CHECK(near(moving->power,1600.0*1600.0,0.001));
    CHECK(near(negative->power,1200.0*1200.0,0.001));
    CHECK(near(still->range_static_power,2200.0*2200.0,0.001));
    CHECK(near(still->range_dynamic_power,1600.0*1600.0,0.001));
    CHECK(near(still->range_total_power,2200.0*2200.0+1600.0*1600.0,0.001));
    CHECK(near(result.range_bin_m,299792458.0*2500000.0/(2.0*60000000000000.0*64.0),0.000001));
    CHECK(near(result.velocity_bin_mps,299792458.0/(2.0*61378000000.0*32.0*0.000250),0.000001));
    CHECK(negative->radial_velocity_mps<0.0f);
    for(rx=0u;rx<4u;++rx) {
        CHECK(fabs(atan2(still->rx_phasor[rx].imag,still->rx_phasor[rx].real)-phase[rx])<0.001);
        CHECK(fabs(atan2(moving->rx_phasor[rx].imag,moving->rx_phasor[rx].real)-phase[rx])<0.001);
    }
    CHECK((result.quality_flags&RADAR_QUALITY_RX_PHASE_UNCALIBRATED)!=0u);
    CHECK((result.quality_flags&RADAR_QUALITY_POWER_UNCALIBRATED)!=0u);
    CHECK((result.quality_flags&RADAR_QUALITY_MOTION_SIGN_UNVERIFIED)!=0u);
    CHECK((result.quality_flags&RADAR_QUALITY_INVALID_FRAME)==0u);
    CHECK(result.sequence==f.sequence && result.timestamp_us==f.timestamp_us);
    /* Pure stationary energy stays detectable, without a high-pass erasing it. */
    clear_frame(); add_tone(&c,12.0,0.0,2000.0,phase); ++f.sequence;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    still=find_peak(12u,0); CHECK(still!=NULL);
    CHECK(still->range_dynamic_power < 3.0f);
    CHECK(near(still->range_static_power,4000000.0,0.001));
}

static void test_frame_failures_and_gaps(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f=info(0xfffffffdu,c.chirps);
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,6.0,2.0,1000.0,NULL);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    ++f.sequence; f.capture_complete=0u;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_BAD_FRAME);
    CHECK(result.detection_count==0u && (result.quality_flags&RADAR_QUALITY_INVALID_FRAME)!=0u);
    f.capture_complete=1u; f.valid_chirps--;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_BAD_FRAME);
    f.valid_chirps=c.chirps;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c)-1u,&f,&result)==RADAR_DSP_BAD_FRAME);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c)+1u,&f,&result)==RADAR_DSP_BAD_FRAME);
    ++f.sequence;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(result.missed_frames==1u && (result.quality_flags&RADAR_QUALITY_SEQUENCE_GAP)!=0u);
    CHECK(find_peak(6u,2)!=NULL);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_STALE_FRAME);
    CHECK(result.detection_count==0u);
    f.sequence=0u;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(result.missed_frames==0u);
    f.sequence=0xffffffffu;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_STALE_FRAME);
    CHECK(radar_dsp_process(&state,NULL,sample_count(&c),&f,&result)==RADAR_DSP_BAD_FRAME);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),NULL,&result)==RADAR_DSP_BAD_FRAME);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,NULL)==RADAR_DSP_BAD_FRAME);
}

static void test_boundaries_and_zero(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f;
    const RadarDetection *peak;
    size_t i;
    c.adc_samples=16u; c.chirps=8u; c.receivers=1u; c.last_range_bin=7u;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK); f=info(1u,c.chirps);
    clear_frame();
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(result.detection_count==0u && result.background_power==0.0f);
    add_tone(&c,1.0,-4.0,1000.0,NULL); ++f.sequence;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    peak=find_peak(1u,-4); CHECK(peak!=NULL); CHECK(near(peak->power,1000000.0,0.001));
    clear_frame(); add_tone(&c,7.0,3.0,1000.0,NULL); ++f.sequence;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(find_peak(7u,3)!=NULL);
    clear_frame(); for(i=0u;i<sample_count(&c);++i) { frame[i].real=INT16_MAX; frame[i].imag=INT16_MIN; }
    ++f.sequence;
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK((result.quality_flags&RADAR_QUALITY_ADC_CLIPPED)!=0u);
    CHECK(result.clipped_components==2u*sample_count(&c));
    /* Restricting evaluated bins never indexes outside the configured range. */
    c.first_range_bin=3u; c.last_range_bin=3u;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,3.0,0.0,1000.0,NULL);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(find_peak(3u,0)!=NULL);
}

static void test_bounds_and_invalid_config(void)
{
    RadarDspConfig c=config(),bad; RadarFrameInfo f=info(1u,c.chirps);
    unsigned i;
    CHECK(radar_dsp_init(NULL,&c)==RADAR_DSP_BAD_CONFIG);
    CHECK(radar_dsp_init(&state,NULL)==RADAR_DSP_BAD_CONFIG);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_BAD_CONFIG);
#define BAD(field,value) do { bad=c; bad.field=(value); CHECK(radar_dsp_init(&state,&bad)==RADAR_DSP_BAD_CONFIG); } while(0)
    BAD(adc_samples,0u); BAD(adc_samples,48u); BAD(adc_samples,RADAR_DSP_MAX_SAMPLES+1u);
    BAD(chirps,0u); BAD(chirps,7u); BAD(chirps,RADAR_DSP_MAX_CHIRPS+1u);
    BAD(receivers,0u); BAD(receivers,5u); BAD(first_range_bin,0u); BAD(last_range_bin,32u);
    BAD(sample_rate_hz,0.0f); BAD(sample_rate_hz,NAN); BAD(chirp_slope_hz_per_s,-1.0f);
    BAD(chirp_period_s,INFINITY); BAD(chirp_slope_hz_per_s,1.0e-25f); BAD(carrier_hz,0.0f); BAD(noise_multiplier,0.5f); BAD(minimum_power,-1.0f);
    bad=c; bad.physical_rx[1]=0u; CHECK(radar_dsp_init(&state,&bad)==RADAR_DSP_BAD_CONFIG);
    bad=c; bad.physical_rx[2]=4u; CHECK(radar_dsp_init(&state,&bad)==RADAR_DSP_BAD_CONFIG);
#undef BAD
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    CHECK(radar_dsp_init(&state,&state.config)==RADAR_DSP_OK);
    clear_frame();
    for(i=0u;i<12u;++i) add_tone(&c,3.0+4.0*(i%7u),-12.0+5.0*(i/7u),300.0+20.0*i,NULL);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(result.detection_count==RADAR_DSP_MAX_DETECTIONS);
    for(i=1u;i<result.detection_count;++i) CHECK(result.detections[i-1u].power>=result.detections[i].power);
}

static void test_noninteger_tones_and_noise(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f=info(100u,c.chirps);
    size_t i; uint32_t rng=0x41327u;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,14.3,-3.2,700.0,NULL);
    for(i=0u;i<sample_count(&c);++i) {
        rng=rng*1664525u+1013904223u; frame[i].real+=(int16_t)((int)(rng>>24u)-128);
        rng=rng*1664525u+1013904223u; frame[i].imag+=(int16_t)((int)(rng>>24u)-128);
    }
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    CHECK(find_peak(14u,-3)!=NULL);
    check_direct_2d(&c,find_peak(14u,-3));
    CHECK(result.background_power>0.0f && isfinite(result.background_power));
    CHECK(result.detection_threshold>=c.minimum_power);
}


static void test_room_profile_scaling(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f=info(1u,c.chirps);
    const RadarDetection *p;
    c.chirp_slope_hz_per_s=30000000000000.0f;
    c.carrier_hz=60814000000.0f;
    c.first_range_bin=4u; c.last_range_bin=30u;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,20.0,2.0,1000.0,NULL);
    CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK);
    p=find_peak(20u,2); CHECK(p!=NULL);
    CHECK(near(p->range_m,20.0*299792458.0*2500000.0/(2.0*30000000000000.0*64.0),0.000001));
    CHECK(near(p->radial_velocity_mps,2.0*299792458.0/(2.0*60814000000.0*32.0*0.00025),0.000001));
    check_direct_2d(&c,p);
}

static void benchmark(void)
{
    RadarDspConfig c=config(); RadarFrameInfo f=info(1u,c.chirps);
    const unsigned count=300u; unsigned i; clock_t begin,end;
    CHECK(radar_dsp_init(&state,&c)==RADAR_DSP_OK);
    clear_frame(); add_tone(&c,8.0,0.0,2000.0,NULL); add_tone(&c,20.0,-5.0,1000.0,NULL);
    begin=clock();
    for(i=0u;i<count;++i) { f.sequence=i+1u; CHECK(radar_dsp_process(&state,frame,sample_count(&c),&f,&result)==RADAR_DSP_OK); }
    end=clock();
    printf("Host CPU benchmark: %.3f ms/frame (%u frames; not an R4F measurement)\n",1000.0*(double)(end-begin)/(double)CLOCKS_PER_SEC/count,count);
    printf("sizeof(RadarDsp)=%zu bytes; result=%zu; capture=%zu\n",sizeof(state),sizeof(result),sizeof(frame));
}

int main(void)
{
    test_fft_reference(); test_static_and_moving(); test_frame_failures_and_gaps();
    test_boundaries_and_zero(); test_bounds_and_invalid_config(); test_noninteger_tones_and_noise();
    test_room_profile_scaling();
    benchmark(); printf("PASS: %u checks\n",checks); return 0;
}
