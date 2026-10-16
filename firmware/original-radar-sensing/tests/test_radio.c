/* Real src/radio.c lifecycle against recording host fakes (tests/stubs). No RF,
 * board or TI driver is exercised; this checks only the application's stop logic. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "radio.h"
#define STOP_OK 0
#define STOP_WARNING (-1)
#define STOP_ERROR (-2)
static MMWave_EventFxn event_fxn;
static uint32_t now,frame_end_at;
static unsigned stops,closes,starts;
static int32_t stop_script[8];
static uint32_t frames_configured;
static void frame_end_event(void)
{
    assert(event_fxn!=NULL);
    (void)event_fxn(RL_RF_ASYNC_EVENT_MSG,RL_RF_AE_FRAME_END_SB,0U,NULL);
}
uint32_t Clock_getTicks(void){return now;}
void Task_sleep(uint32_t ticks)
{
    now+=ticks;
    if(frame_end_at!=0U && now>=frame_end_at){frame_end_at=0U;frame_end_event();}
}
int32_t Mailbox_init(int type){(void)type;return 0;}
MMWave_Handle MMWave_init(MMWave_InitCfg *cfg,int32_t *error){event_fxn=cfg->eventFxn;*error=0;return (MMWave_Handle)1;}
int32_t MMWave_sync(MMWave_Handle handle,int32_t *error){(void)handle;*error=0;return 1;}
int32_t MMWave_execute(MMWave_Handle handle,int32_t *error){(void)handle;*error=0;return 0;}
double SOC_getDeviceRFFreqScaleFactor(SOC_Handle handle,int32_t *error){(void)handle;*error=0;return 2.7;}
int32_t MMWave_open(MMWave_Handle handle,const MMWave_OpenCfg *cfg,const void *calibration,int32_t *error)
{
    (void)handle;(void)calibration;*error=0;
    assert(cfg->freqLimitLow==600U && cfg->freqLimitHigh==640U && cfg->chCfg.txChannelEn==1U);
    return 0;
}
int32_t MMWave_flushCfg(MMWave_Handle handle,int32_t *error){(void)handle;*error=0;return 0;}
MMWave_ProfileHandle MMWave_addProfile(MMWave_Handle handle,const rlProfileCfg_t *cfg,int32_t *error)
{(void)handle;(void)cfg;*error=0;return (MMWave_ProfileHandle)2;}
MMWave_ChirpHandle MMWave_addChirp(MMWave_ProfileHandle profile,const rlChirpCfg_t *cfg,int32_t *error)
{(void)profile;(void)cfg;*error=0;return (MMWave_ChirpHandle)3;}
int32_t MMWave_config(MMWave_Handle handle,MMWave_CtrlCfg *cfg,int32_t *error)
{(void)handle;*error=0;frames_configured=cfg->u.frameCfg.frameCfg.numFrames;return 0;}
int32_t MMWave_start(MMWave_Handle handle,const MMWave_CalibrationCfg *cfg,int32_t *error)
{(void)handle;(void)cfg;*error=0;++starts;return 0;}
int32_t MMWave_stop(MMWave_Handle handle,int32_t *error)
{
    (void)handle;assert(stops<sizeof(stop_script)/sizeof(stop_script[0]));
    *error=stop_script[stops++];return *error==STOP_OK?0:-1;
}
void MMWave_decodeError(int32_t error,MMWave_ErrorLevel *level,int16_t *code,int16_t *subsystem)
{
    *level=error==STOP_ERROR?MMWave_ErrorLevel_ERROR:error==STOP_WARNING?MMWave_ErrorLevel_WARNING:MMWave_ErrorLevel_SUCCESS;
    *code=(int16_t)error;*subsystem=0;
}
int32_t MMWave_close(MMWave_Handle handle,int32_t *error){(void)handle;*error=0;++closes;return 0;}

static OrConfig config;
static void run(void)
{
    OrRfPhysical physical;
    assert(or_radio_prepare(&config,&physical)==0 && or_radio_start()==0);
}
int main(void)
{
    or_config_reset(&config);
    assert(or_config_line(&config,"orConfig 1",0)==OR_CMD_CHANGED);
    assert(or_config_line(&config,"profile room64_v1",0)==OR_CMD_CHANGED);
    assert(or_config_line(&config,"mount 1219 15000 unknown",0)==OR_CMD_CHANGED);
    assert(or_config_line(&config,"detect 4 30 64 8",0)==OR_CMD_CHANGED);
    assert(or_config_line(&config,"session continuous",0)==OR_CMD_CHANGED);
    assert(or_config_line(&config,"commit",0)==OR_CMD_COMMIT);
    assert(or_radio_init((SOC_Handle)1)==0);

    /* Unchanged success path: accepted stop, frame end, one close; repeat is a no-op. */
    run();assert(frames_configured==0U && starts==1U);
    stop_script[0]=STOP_OK;frame_end_at=now+3U;
    assert(or_radio_stop()==0 && stops==1U && closes==1U && or_radio_fault()==0U);
    assert(or_radio_stop()==0 && stops==1U && closes==1U);
    /* A warning (frame already ended) remains a successful stop. */
    run();frame_end_event();stop_script[1]=STOP_WARNING;
    assert(or_radio_stop()==0 && stops==2U && closes==2U);

    /* Rejected stop: RF stays marked started and open, no close, no new prepare. */
    run();frame_end_event();stop_script[2]=STOP_ERROR;
    assert(or_radio_stop()==-1 && stops==3U && closes==2U && or_radio_error()==STOP_ERROR);
    {OrRfPhysical physical;assert(or_radio_prepare(&config,&physical)<0 && or_radio_start()<0);}
    /* The next stop reissues MMWave_stop and only then closes. */
    stop_script[3]=STOP_OK;
    assert(or_radio_stop()==0 && stops==4U && closes==3U);
    assert(or_radio_stop()==0 && stops==4U && closes==3U);

    /* Continuous run, rejected stop and no frame end: fault 205, no close. */
    run();stop_script[4]=STOP_ERROR;
    assert(or_radio_stop()==-2 && or_radio_fault()==205U && stops==5U && closes==3U);
    /* Accepted retry still without frame end: keep RF open. */
    stop_script[5]=STOP_OK;
    assert(or_radio_stop()==-2 && stops==6U && closes==3U);
    /* An accepted stop is not reissued; the late frame end allows the close. */
    frame_end_at=now+5U;
    assert(or_radio_stop()==0 && stops==6U && closes==4U);
    assert(or_radio_stop()==0 && stops==6U && closes==4U);
    /* The latched RF fault still blocks any further RF preparation. */
    {OrRfPhysical physical;assert(or_radio_prepare(&config,&physical)<0 && starts==4U);}
    puts("PASS: radio stop keeps RF started/open after a rejected or unconfirmed stop, retries on the next stop, closes only after frame end; success path unchanged.");
    return 0;
}
