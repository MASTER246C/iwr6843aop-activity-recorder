/* Original lifecycle and fixed profile; TI mmWave handles low-level radio transport. */
#include <stdint.h>
#include <string.h>
#include <ti/control/mmwave/mmwave.h>
#include <ti/drivers/mailbox/mailbox.h>
#include <ti/drivers/crc/crc.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include "radio.h"
static MMWave_Handle handle;
static SOC_Handle soc_handle;
static int opened,started;
static volatile int32_t last_error;
static volatile uint32_t fault,frame_end;
static int32_t radio_event(uint16_t message,uint16_t subblock,uint16_t length,uint8_t *payload)
{
    uint16_t id=RL_GET_SBID_FROM_UNIQ_SBID(subblock);
    (void)length;(void)payload;
    if(message==RL_RF_ASYNC_EVENT_MSG) {
        if(id==RL_RF_AE_CPUFAULT_SB)fault=201U;
        else if(id==RL_RF_AE_ESMFAULT_SB)fault=202U;
        else if(id==RL_RF_AE_ANALOG_FAULT_SB)fault=203U;
        else if(id==RL_RF_AE_FRAME_END_SB)frame_end=1U;
    }
    return 0;
}
int or_radio_init(SOC_Handle soc)
{
    MMWave_InitCfg init;int32_t error=0;int32_t sync;uint32_t begin;
    soc_handle=soc;
    if(Mailbox_init(MAILBOX_TYPE_MSS)<0)return -1;
    memset(&init,0,sizeof(init));init.domain=MMWave_Domain_MSS;init.socHandle=soc;
    init.eventFxn=radio_event;init.executionMode=MMWave_ExecutionMode_ISOLATION;
    init.linkCRCCfg.useCRCDriver=1U;init.linkCRCCfg.crcChannel=CRC_Channel_CH1;
    init.cfgMode=MMWave_ConfigurationMode_FULL;
    handle=MMWave_init(&init,&error);last_error=error;if(handle==NULL)return -2;
    begin=Clock_getTicks();
    do {
        sync=MMWave_sync(handle,&error);
        if(sync<0){last_error=error;return -3;}
        if(sync==1)return 0;
        Task_sleep(1U);
    }while((uint32_t)(Clock_getTicks()-begin)<5000U);
    last_error=-1000;return -4;
}
void or_radio_execute(void)
{
    int32_t error=0;
    if(MMWave_execute(handle,&error)<0){last_error=error;fault=204U;Task_sleep(1U);}
}
int or_radio_prepare(const OrConfig *cfg,OrRfPhysical *physical)
{
    MMWave_OpenCfg open;MMWave_CtrlCfg control;rlProfileCfg_t profile;rlChirpCfg_t chirp;
    MMWave_ProfileHandle p;int32_t error=0;double scale;
    if(!or_config_valid(cfg)||!cfg->committed||opened||started||fault)return -1;
    scale=SOC_getDeviceRFFreqScaleFactor(soc_handle,&error);
    /* This image targets 60GHz IWR6843 only; never silently use 77GHz units. */
    if(error || or_rf_physical(scale,physical)<0){last_error=error;return -2;}
    memset(&open,0,sizeof(open));open.freqLimitLow=600U;open.freqLimitHigh=640U;
    open.chCfg.rxChannelEn=15U;open.chCfg.txChannelEn=1U;
    open.lowPowerMode.lpAdcMode=1U;open.adcOutCfg.fmt.b2AdcBits=2U;
    open.adcOutCfg.fmt.b2AdcOutFmt=1U; /* Complex1x, not image-band mode. */
    open.defaultAsyncEventHandler=MMWave_DefaultAsyncEventHandler_MSS;
    open.disableFrameStartAsyncEvent=false;open.disableFrameStopAsyncEvent=false;
    open.useCustomCalibration=false;open.calibMonTimeUnit=1U;
    /* RF open includes calibration and occurs only inside explicit start handling. */
    if(MMWave_open(handle,&open,NULL,&error)<0){last_error=error;return -3;}
    opened=1;
    if(MMWave_flushCfg(handle,&error)<0){last_error=error;return -4;}
    memset(&profile,0,sizeof(profile));
    profile.profileId=0U;
    profile.startFreqConst=physical->start_code;
    profile.freqSlopeConst=physical->slope_code;
    profile.idleTimeConst=21000U;profile.adcStartTimeConst=600U;profile.rampEndTime=4000U;
    profile.txStartTime=100;profile.txOutPowerBackoffCode=0x060606U;
    profile.numAdcSamples=OR_ADC_SAMPLES;profile.digOutSampleRate=2500U;
    profile.hpfCornerFreq1=0U;profile.hpfCornerFreq2=0U;profile.rxGain=30U;
    p=MMWave_addProfile(handle,&profile,&error);if(p==NULL){last_error=error;return -6;}
    memset(&chirp,0,sizeof(chirp));chirp.profileId=0U;chirp.txEnable=1U;
    if(MMWave_addChirp(p,&chirp,&error)==NULL){last_error=error;return -7;}
    memset(&control,0,sizeof(control));control.dfeDataOutputMode=MMWave_DFEDataOutputMode_FRAME;
    control.u.frameCfg.profileHandle[0]=p;
    control.u.frameCfg.frameCfg.chirpStartIdx=0U;control.u.frameCfg.frameCfg.chirpEndIdx=0U;
    control.u.frameCfg.frameCfg.numLoops=OR_CHIRPS;
    /* TI rlFrameCfg_t defines numFrames=0 as recurring frames until Stop. */
    control.u.frameCfg.frameCfg.numFrames=(uint16_t)(cfg->session_seconds*5U);
    control.u.frameCfg.frameCfg.numAdcSamples=OR_ADC_SAMPLES;
    control.u.frameCfg.frameCfg.framePeriodicity=40000000U; /*200ms /5ns.*/
    control.u.frameCfg.frameCfg.triggerSelect=1U;
    if(MMWave_config(handle,&control,&error)<0){last_error=error;return -8;}
    frame_end=0U;return 0;
}
int or_radio_start(void)
{
    MMWave_CalibrationCfg cal;int32_t error=0;
    if(!opened||started||fault)return -1;
    memset(&cal,0,sizeof(cal));cal.dfeDataOutputMode=MMWave_DFEDataOutputMode_FRAME;
    cal.u.chirpCalibrationCfg.enableCalibration=true;
    cal.u.chirpCalibrationCfg.enablePeriodicity=true;
    cal.u.chirpCalibrationCfg.periodicTimeInFrames=10U;
    if(MMWave_start(handle,&cal,&error)<0){last_error=error;return -2;}
    started=1;return 0;
}
int or_radio_stop(void)
{
    int32_t error=0;int result=0;MMWave_ErrorLevel level;int16_t mmcode,subcode;
    uint32_t begin;
    if(started) {
        if(MMWave_stop(handle,&error)<0) {
            MMWave_decodeError(error,&level,&mmcode,&subcode);
            if(level==MMWave_ErrorLevel_ERROR){last_error=error;result=-1;}
        }
        begin=Clock_getTicks();
        while(!frame_end && (uint32_t)(Clock_getTicks()-begin)<1000U)Task_sleep(1U);
        if(!frame_end){fault=205U;result=-2;}
        started=0;
    }
    if(opened) {
        if(MMWave_close(handle,&error)<0){last_error=error;result=-3;}
        opened=0;
    }
    return result;
}
int32_t or_radio_error(void){return last_error;}
uint32_t or_radio_fault(void){return fault;}
uint32_t or_radio_frame_end(void){return frame_end;}
