/* Original bounded JSON serializer. A partial/overflowed record is never transmitted. */
#include "json_output.h"
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>
#include <string.h>
typedef struct {char *data;size_t capacity,used;int failed;} Writer;
static void add(Writer *w,const char *format,...)
{
    va_list args;int n;
    if(w->failed)return;
    va_start(args,format);n=vsnprintf(w->data+w->used,w->capacity-w->used,format,args);va_end(args);
    if(n<0 || (size_t)n>=w->capacity-w->used){w->failed=1;return;}w->used+=(size_t)n;
}
static void number(Writer *w,float x)
{
    if(x!=x || x>FLT_MAX || x<-FLT_MAX)add(w,"null");else add(w,"%.5g",(double)x);
}
static void vector(Writer *w,float x,float y,float z)
{
    add(w,"[");number(w,x);add(w,",");number(w,y);add(w,",");number(w,z);add(w,"]");
}
static int finite_value(float x){return x==x&&x<=FLT_MAX&&x>=-FLT_MAX;}
size_t or_frame_json(char *out,size_t capacity,const OrConfig *cfg,const os_config *spatial,
                     const RadarDspResult *result,const OrTiming *timing)
{
    Writer w;unsigned count,i,rx;os_complex phasors[4];os_result point;int valid;
    if(out==NULL || capacity==0U || cfg==NULL || spatial==NULL || result==NULL || timing==NULL)return 0U;
    if(result->detection_count>RADAR_DSP_MAX_DETECTIONS ||
       !finite_value(result->range_bin_m)||result->range_bin_m<=0.0f ||
       !finite_value(result->velocity_bin_mps)||result->velocity_bin_mps<=0.0f)return 0U;
    for(i=0;i<3U;++i)if(!finite_value(timing->profile_energy[i])||timing->profile_energy[i]<0.0f)return 0U;
    for(i=0;i<result->detection_count;++i) {
        const RadarDetection *d=&result->detections[i];
        if(!finite_value(d->range_m)||d->range_m<=0.0f ||
           !finite_value(d->radial_velocity_mps)||!finite_value(d->power)||d->power<0.0f)return 0U;
        for(rx=0;rx<4U;++rx)if(!finite_value(d->rx_phasor[rx].real)||!finite_value(d->rx_phasor[rx].imag))return 0U;
    }
    w.data=out;w.capacity=capacity;w.used=0U;w.failed=0;
    count=result->detection_count;if(count>OR_JSON_MAX_DETECTIONS)count=OR_JSON_MAX_DETECTIONS;
    add(&w,"{\"type\":\"frame\",\"protocol\":\"original-radar-v1\",\"sequence\":%lu,\"timestamp_us\":%llu,"
        "\"complete\":true,\"missed_frames\":%lu,\"capture_fault\":%lu,\"quality\":%lu,"
        "\"frame_events\":%lu,\"processing_us\":%lu,\"previous_tx_us\":%lu,\"deadline_misses\":%lu,",
        (unsigned long)result->sequence,(unsigned long long)result->timestamp_us,
        (unsigned long)result->missed_frames,(unsigned long)timing->capture_fault,
        (unsigned long)result->quality_flags,(unsigned long)timing->frame_events,(unsigned long)timing->processing_us,
        (unsigned long)timing->previous_tx_us,(unsigned long)timing->deadline_misses);
    add(&w,"\"mount\":{\"bracket_top_m\":");number(&w,cfg->bracket_top_mm/1000.0f);
    add(&w,",\"down_tilt_deg\":");number(&w,cfg->down_tilt_mdeg/1000.0f);
    add(&w,",\"antenna_offset_m\":");if(cfg->antenna_offset_known)number(&w,cfg->antenna_vertical_offset_mm/1000.0f);else add(&w,"null");
    add(&w,",\"antenna_height_m\":");if(cfg->antenna_offset_known)number(&w,(cfg->bracket_top_mm+cfg->antenna_vertical_offset_mm)/1000.0f);else add(&w,"null");
    add(&w,",\"world_height_valid\":%s},",spatial->antenna_offset_known&&spatial->rx_phase_calibrated&&spatial->axis_orientation_verified?"true":"false");
    add(&w,"\"range_bin_m\":");number(&w,result->range_bin_m);add(&w,",\"velocity_bin_mps\":");number(&w,result->velocity_bin_mps);
    add(&w,",\"profile_energy\":");vector(&w,timing->profile_energy[0],timing->profile_energy[1],timing->profile_energy[2]);
    add(&w,",\"detected_count\":%u,\"count\":%u,\"truncated\":%s,\"detections\":[",
        (unsigned)result->detection_count,count,result->detection_count>count?"true":"false");
    for(i=0;i<count;++i) {
        const RadarDetection *d=&result->detections[i];
        for(rx=0;rx<4U;++rx){phasors[rx].real=d->rx_phasor[rx].real;phasors[rx].imag=d->rx_phasor[rx].imag;}
        valid=os_estimate(spatial,phasors,d->range_m,&point);
        if(i){add(&w,",");}
        add(&w,"{\"range_m\":");number(&w,d->range_m);
        add(&w,",\"velocity_mps\":");number(&w,d->radial_velocity_mps);
        add(&w,",\"power\":");number(&w,d->power);add(&w,",\"phasors\":[");
        for(rx=0;rx<4U;++rx){if(rx)add(&w,",");add(&w,"[");number(&w,phasors[rx].real);add(&w,",");number(&w,phasors[rx].imag);add(&w,"]");}
        add(&w,"],\"sensor_xyz_m\":");if(valid)vector(&w,point.x_sensor_m,point.y_sensor_m,point.z_sensor_m);else add(&w,"null");
        add(&w,",\"world_xyz_m\":");if(valid&&(point.flags&OS_WORLD_HEIGHT_VALID))vector(&w,point.x_world_m,point.y_world_m,point.z_world_m);else add(&w,"null");
        add(&w,",\"spatial_flags\":%lu,\"activity\":null}",(unsigned long)point.flags);
    }
    add(&w,"]}\n");
    return w.failed || w.used>OR_JSON_MAX?0U:w.used;
}
