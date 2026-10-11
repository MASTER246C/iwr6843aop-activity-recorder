#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sensing_config.h"
#include "serial_line.h"
#include "rf_profile.h"
#include "capture_model.h"
#include "json_output.h"
#include "energy_summary.h"
static OrConfig cfg;
static RadarDspResult result;
static RadarDsp profile;
static void configuration(void)
{
    OrConfig saved;char nonterminated[OR_LINE_SIZE];
    or_config_reset(&cfg);
    assert(or_config_line(&cfg,"start",0)==OR_CMD_INCOMPLETE);
    assert(or_config_line(&cfg,"orConfig 1",0)==OR_CMD_CHANGED);
    assert(or_config_line(&cfg,"profile room64_v1",0)==OR_CMD_CHANGED);
    assert(or_config_line(&cfg,"mount 2000 15000 unknown",0)==OR_CMD_CHANGED);
    assert(!cfg.antenna_offset_known);
    assert(or_config_line(&cfg,"detect 4 30 64 8",0)==OR_CMD_CHANGED);
    assert(or_config_line(&cfg,"session 5",0)==OR_CMD_CHANGED);
    assert(or_config_line(&cfg,"commit",0)==OR_CMD_COMMIT);
    assert(cfg.committed&&or_config_valid(&cfg));saved=cfg;
    assert(or_config_line(&cfg,"start",0)==OR_CMD_START);
    assert(or_config_line(&cfg,"session 0",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"session 6",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"session 60",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"session 9999999999999999999999999999999999",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"profile room64_v1 extra",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"start garbage",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"detect 30 4 64 8",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"detect -1 30 64 8",0)==OR_CMD_INVALID);
    assert(or_config_line(&cfg,"mount 2000 nan unknown",0)==OR_CMD_INVALID);
    memset(nonterminated,'x',sizeof(nonterminated));
    assert(or_config_line(&cfg,nonterminated,0)==OR_CMD_INVALID);
    assert(memcmp(&cfg,&saved,sizeof(cfg))==0);
    assert(or_config_line(&cfg,"orConfig 1",1)==OR_CMD_BUSY);
    assert(or_config_line(&cfg,"start",1)==OR_CMD_BUSY);
    assert(or_config_line(&cfg,"stop",1)==OR_CMD_STOP);
    assert(or_config_line(&cfg,"status",1)==OR_CMD_STATUS);
    assert(or_config_line(&cfg,"# ignored",1)==OR_CMD_NONE);
    assert(memcmp(&cfg,&saved,sizeof(cfg))==0);
    assert(or_config_line(&cfg,"mount 100 15000 -200",0)==OR_CMD_INVALID);
    assert(memcmp(&cfg,&saved,sizeof(cfg))==0);
    cfg=saved;
}
static void byte_assembly(void)
{
    OrSerialLine line={ {0},0,0 };unsigned i;int status=0;
    const uint8_t bad[]={ 's','t','a','r','t',0,'x','\n' };
    for(i=0;i<sizeof(bad);++i)status=or_serial_feed(&line,bad[i]);
    assert(status==-1 && line.used==0 && line.discard==0);
    for(i=0;i<OR_LINE_SIZE+10U;++i)assert(or_serial_feed(&line,'a')==0);
    assert(or_serial_feed(&line,'\n')==-1);
    assert(or_serial_feed(&line,255U)==0&&or_serial_feed(&line,'\n')==-1);
    assert(or_serial_feed(&line,1U)==0&&or_serial_feed(&line,'\n')==-1);
    assert(or_serial_feed(&line,'s')==0);assert(or_serial_feed(&line,'t')==0);
    assert(or_serial_feed(&line,'o')==0);assert(or_serial_feed(&line,'p')==0);
    assert(or_serial_feed(&line,'\r')==0);assert(or_serial_feed(&line,'\n')==1);
    assert(strcmp(line.text,"stop\r")==0);
}
static void profile_units(void)
{
    OrRfPhysical p;double bin;
    assert(or_rf_physical(3.6,&p)<0);assert(or_rf_physical(NAN,&p)<0);
    assert(or_rf_physical(2.7,NULL)<0);assert(or_rf_physical(2.7,&p)==0);
    assert(p.start_code==1497521872U && p.slope_code==828);
    assert((p.start_code&1U)==0U&&(p.slope_code&1)==0);
    assert(fabs(p.start_hz-60.25e9)<81.0);
    assert(p.slope_hz_per_s>29.98e12&&p.slope_hz_per_s<30.0e12);
    assert(p.carrier_hz>60.81e9&&p.carrier_hz<60.82e9);
    assert(p.start_hz+p.slope_hz_per_s*40e-6<61.45e9);
    bin=299792458.0*2.5e6/(2.0*p.slope_hz_per_s*64.0);
    assert(bin>0.195&&bin<0.196);
    printf("Verified60GHz encodings: start=%lu slope=%d; physicalslope=%.12g Hz/s; rangebin=%.9fm\n",(unsigned long)p.start_code,(int)p.slope_code,p.slope_hz_per_s,bin);
}
static void ownership(void)
{
    OrCaptureModel m={0};unsigned slot;uint32_t gen,old;
    or_capture_model_arm(&m);
    assert(or_capture_model_frame_start(&m)==0);
    assert(or_capture_model_complete(&m,0,8)==0);
    assert(or_capture_model_take(&m,&slot,&gen)&&slot==0);
    assert(or_capture_model_release(&m,slot,gen)==0);
    assert(or_capture_model_frame_start(&m)==0);
    assert(or_capture_model_complete(&m,1,208)==0);
    assert(or_capture_model_take(&m,&slot,&gen)&&slot==1);old=gen;
    or_capture_model_stop(&m);
    assert(or_capture_model_release(&m,slot,gen)<0);
    or_capture_model_arm(&m);
    assert(or_capture_model_frame_start(&m)==0);
    assert(or_capture_model_complete(&m,0,408)==0);
    assert(or_capture_model_take(&m,&slot,&gen));
    assert(or_capture_model_frame_start(&m)==0);
    /* Slow consumer means next DMA destination still owned: disable, don't overwrite. */
    assert(or_capture_model_complete(&m,1,608)<0&&m.overruns==1);
    assert(or_capture_model_release(&m,slot,gen)<0);
    assert(!or_capture_model_take(&m,&slot,&gen));
    or_capture_model_stop(&m);or_capture_model_arm(&m);
    assert(or_capture_model_release(&m,1,old)<0);
    assert(or_capture_model_frame_start(&m)==0);
    /* Missing chirp means no complete frame before next frame start: reject mixing. */
    assert(or_capture_model_frame_start(&m)<0&&m.fault==3);
    or_capture_model_stop(&m);or_capture_model_arm(&m);
    assert(or_capture_model_complete(&m,0,0)<0); /* No RF frame start. */
}
static void energy(void)
{
    float values[3];
    profile.config.first_range_bin=4;profile.config.last_range_bin=5;
    profile.total_power[4]=100;profile.static_power[4]=70;
    profile.total_power[5]=50;profile.static_power[5]=50.00001f;
    assert(or_profile_energy(&profile,values)==0);
    assert(values[0]==150.0f&&fabsf(values[1]-120.00001f)<0.0001f&&values[2]==30.0f);
    profile.total_power[4]=NAN;assert(or_profile_energy(&profile,values)<0);
    profile.total_power[4]=FLT_MAX;profile.total_power[5]=FLT_MAX;
    assert(or_profile_energy(&profile,values)<0);
    assert(or_profile_energy(NULL,values)<0);
}
static void json(void)
{
    OrLinkSession link={"0123456789abcdef0123456789abcdef",UINT32_MAX,UINT32_MAX,0,1,0};
    os_config spatial;OrTiming timing={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,{FLT_MAX,FLT_MAX,FLT_MAX},&link};
    char out[OR_JSON_MAX+1U];char guard[4]={'A','B','C','D'};size_t n;unsigned i,rx;
    os_default_config(&spatial,60.8e9f);memset(&result,0,sizeof(result));
    result.sequence=UINT32_MAX;result.timestamp_us=UINT64_MAX;result.missed_frames=UINT32_MAX;
    result.quality_flags=UINT32_MAX;result.range_bin_m=FLT_MAX;result.velocity_bin_mps=FLT_MAX;
    result.detection_count=8;
    for(i=0;i<8;++i) {
        result.detections[i].range_m=FLT_MAX;result.detections[i].radial_velocity_mps=-FLT_MAX;
        result.detections[i].power=FLT_MAX;
        for(rx=0;rx<4;++rx){result.detections[i].rx_phasor[rx].real=-FLT_MAX;result.detections[i].rx_phasor[rx].imag=FLT_MAX;}
    }
    n=or_frame_json(out,sizeof(out),&cfg,&spatial,&result,&timing);
    assert(n>0&&n<=OR_JSON_MAX&&n==strlen(out)&&out[n-1]=='\n');
    assert(strstr(out,"\"world_xyz_m\":null")&&strstr(out,"\"activity\":null")&&strstr(out,"\"count\":3"));
    assert(strstr(out,"\"profile_energy\":["));
    assert(strstr(out,"\"antenna_offset_m\":null")&&strstr(out,"\"truncated\":true"));
    assert(or_frame_json(guard,1,&cfg,&spatial,&result,&timing)==0);
    assert(guard[1]=='B'&&guard[2]=='C'&&guard[3]=='D');
    printf("Worst-case numeric frame length: %lu bytes (cap1800)\n",(unsigned long)n);
    memset(&result,0,sizeof(result));result.range_bin_m=0.1953125f;result.velocity_bin_mps=0.306f;
    result.detection_count=1;result.detections[0].range_m=2.34375f;result.detections[0].power=100.0f;
    for(rx=0;rx<4;++rx)result.detections[0].rx_phasor[rx].real=10.0f;
    memset(&timing,0,sizeof(timing));timing.link=&link;
    n=or_frame_json(out,sizeof(out),&cfg,&spatial,&result,&timing);assert(n>0);fputs(out,stdout);
    result.range_bin_m=NAN;result.detections[0].power=INFINITY;
    n=or_frame_json(out,sizeof(out),&cfg,&spatial,&result,&timing);assert(n==0);
}
int main(void){configuration();byte_assembly();profile_units();ownership();energy();json();puts("PASS: bounded config, state/epoch ownership, incomplete-frame rejection, strict JSON/nulls and UART budget");return 0;}
