/* Real src/acquisition.c ISR/state logic against recording host fakes (tests/stubs).
 * Callbacks are invoked directly as the TI drivers would; no ADCBUF/EDMA hardware,
 * interrupt timing or DMA data movement is modeled. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "acquisition.h"
static uint32_t now,channel_offset[4],translate_fail;
static int hwi_depth;
static unsigned posts,enables,disables,closes,opens,listeners;
static void (*frame_start_isr)(uintptr_t argument);
static EDMA_transferCompletionCallbackFxn_t completion_isr;
static void (*controller_isr)(EDMA_Handle handle,EDMA_errorInfo_t *info);
static void (*transfer_isr)(EDMA_Handle handle,EDMA_transferControllerErrorInfo_t *info);
static EDMA_paramSetConfig_t channel_set,shadow_set[2];
static uint16_t links[3][2];static unsigned link_count;
uint32_t Clock_getTicks(void){return now;}
void Semaphore_post(Semaphore_Handle handle){assert(handle==(Semaphore_Handle)7);++posts;}
uintptr_t HwiP_disable(void){return (uintptr_t)(++hwi_depth);}
void HwiP_restore(uintptr_t key){assert(key==(uintptr_t)hwi_depth);--hwi_depth;}
uint32_t SOC_translateAddress(uint32_t address,int direction,int32_t *error)
{
    assert(direction==SOC_TranslateAddr_Dir_TO_EDMA);*error=0;
    return translate_fail?SOC_TRANSLATEADDR_INVALID:address;
}
SOC_SysIntListenerHandle SOC_registerSysIntListener(SOC_Handle handle,SOC_SysIntListenerCfg *cfg,int32_t *error)
{
    (void)handle;assert(cfg->systemInterrupt==SOC_XWR68XX_MSS_FRAME_START_INT);
    frame_start_isr=cfg->listenerFxn;++listeners;*error=0;return (SOC_SysIntListenerHandle)1;
}
void ADCBuf_init(void){}
void ADCBuf_Params_init(ADCBuf_Params *params){memset(params,0,sizeof(*params));}
ADCBuf_Handle ADCBuf_open(uint32_t index,ADCBuf_Params *params)
{
    assert(index==0U && params->source==ADCBUF_SOURCE_DFE && params->chirpThresholdPing==1U);
    return (ADCBuf_Handle)3;
}
int32_t ADCBuf_control(ADCBuf_Handle handle,uint32_t command,void *argument)
{
    (void)handle;
    if(command==ADCBufMMWave_CMD_CHANNEL_ENABLE) {
        const ADCBuf_RxChanConf *conf=argument;assert(conf->channel<4U);
        channel_offset[conf->channel]=conf->offset;
    } else if(command==ADCBufMMWave_CMD_CONF_DATA_FORMAT) {
        const ADCBuf_dataFormat *format=argument;
        assert(format->sampleInterleave==0U && format->channelInterleave==1U && format->adcOutFormat==0U);
    }
    return 0;
}
uint32_t ADCBuf_getChanBufAddr(ADCBuf_Handle handle,uint8_t channel,int32_t *error)
{(void)handle;*error=0;return SOC_XWR68XX_MSS_ADCBUF_BASE_ADDRESS+channel_offset[channel];}
int32_t EDMA_init(uint8_t instance){assert(instance==0U);return EDMA_NO_ERROR;}
EDMA_Handle EDMA_open(uint8_t instance,int32_t *error,EDMA_instanceInfo_t *info)
{(void)instance;(void)info;*error=0;++opens;return (EDMA_Handle)5;}
int32_t EDMA_configErrorMonitoring(EDMA_Handle handle,EDMA_errorConfig_t *cfg)
{(void)handle;controller_isr=cfg->callbackFxn;transfer_isr=cfg->transferControllerCallbackFxn;return EDMA_NO_ERROR;}
int32_t EDMA_configChannel(EDMA_Handle handle,EDMA_channelConfig_t *cfg,bool enable)
{
    (void)handle;assert(!enable && cfg->channelId==EDMA_TPCC0_REQ_DFE_CHIRP_AVAIL);
    channel_set=cfg->paramSetConfig;completion_isr=cfg->transferCompletionCallbackFxn;return EDMA_NO_ERROR;
}
int32_t EDMA_configParamSet(EDMA_Handle handle,uint16_t param,EDMA_paramConfig_t *cfg)
{
    (void)handle;assert(param==64U || param==65U);
    shadow_set[param-64U]=cfg->paramSetConfig;return EDMA_NO_ERROR;
}
int32_t EDMA_linkParamSets(EDMA_Handle handle,uint16_t from,uint16_t to)
{(void)handle;assert(link_count<3U);links[link_count][0]=from;links[link_count++][1]=to;return EDMA_NO_ERROR;}
int32_t EDMA_enableChannel(EDMA_Handle handle,uint8_t channel,uint8_t type){(void)handle;(void)channel;(void)type;++enables;return EDMA_NO_ERROR;}
int32_t EDMA_disableChannel(EDMA_Handle handle,uint8_t channel,uint8_t type){(void)handle;(void)channel;(void)type;++disables;return EDMA_NO_ERROR;}
int32_t EDMA_close(EDMA_Handle handle){(void)handle;++closes;return EDMA_NO_ERROR;}

static void frame(uint8_t tcc,uint32_t at){frame_start_isr(0U);now=at;completion_isr(0U,tcc);}
static void arm(void){link_count=0U;assert(or_acquisition_arm()==0 && hwi_depth==0);}
int main(void)
{
    const RadarComplex16 *samples,*first;RadarFrameInfo info;unsigned slot;uint32_t generation;unsigned rx;
    assert(or_acquisition_init((SOC_Handle)1,(Semaphore_Handle)7)==0 && listeners==1U);
    for(rx=0U;rx<4U;++rx)assert(channel_offset[rx]==rx*OR_ADC_SAMPLES*4U);
    arm();assert(opens==1U && enables==1U && or_acquisition_arm()==-1);
    /* Ping/pong: channel and shadow 64 fill slot 0, shadow 65 fills slot 1, linked alternately. */
    assert(channel_set.aCount==OR_CHIRP_BYTES && channel_set.bCount==OR_CHIRPS && channel_set.destinationBindex==(int16_t)OR_CHIRP_BYTES);
    assert(channel_set.transferCompletionCode==0U && shadow_set[0].transferCompletionCode==0U && shadow_set[1].transferCompletionCode==1U);
    assert(channel_set.destinationAddress==shadow_set[0].destinationAddress);
    assert(shadow_set[1].destinationAddress-shadow_set[0].destinationAddress==OR_FRAME_BYTES);
    assert(link_count==3U && links[0][1]==65U && links[1][0]==65U && links[1][1]==64U && links[2][0]==64U && links[2][1]==65U);

    /* Healthy frames: completion posts, take/release own the slot, elapsed timestamps. */
    frame(0U,200U);
    assert(posts==1U && or_acquisition_completed()==1U && or_acquisition_frame_events()==1U && or_acquisition_last_ms()==200U);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==1 && hwi_depth==0 && slot==0U);
    assert(info.sequence==0U && info.timestamp_us==200000U && info.valid_chirps==OR_CHIRPS && info.capture_complete==1U);
    first=samples;assert(or_acquisition_release(slot,generation)==0);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==0);
    frame(1U,400U);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==1 && slot==1U && samples==first+OR_FRAME_SAMPLES);
    assert(info.sequence==1U && info.timestamp_us==400000U && or_acquisition_release(slot,generation)==0);
    assert(or_acquisition_fault()==0U);

    /* Slow consumer: the next DMA destination is still owned, so stop instead of overwriting. */
    frame(0U,600U);frame(1U,800U);
    assert(or_acquisition_fault()==102U && disables==1U && posts==4U);
    frame_start_isr(0U);completion_isr(0U,0U);
    assert(or_acquisition_completed()==4U && or_acquisition_last_ms()==800U && posts==4U);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==0);
    /* Stop closes EDMA; a later arm clears the latched fault. */
    or_acquisition_stop();assert(hwi_depth==0 && disables==2U && closes==1U);
    assert(or_acquisition_release(0U,generation)<0);
    now=UINT32_MAX-100U;arm();assert(or_acquisition_fault()==0U && opens==2U);
    /* Elapsed time crosses the 32-bit tick rollover without truncation. */
    frame(0U,99U);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==1 && info.timestamp_us==200000U);
    assert(or_acquisition_release(slot,generation)==0);
    /* Microseconds beyond 32 bits (over 71.6 minutes) stay in 64-bit arithmetic. */
    frame(1U,99U+4300000U);
    assert(or_acquisition_take(&samples,&info,&slot,&generation)==1 && info.timestamp_us==4300200000ULL);
    assert(or_acquisition_release(slot,generation)==0);
    /* A frame start before the previous frame completed means a partial frame. */
    frame_start_isr(0U);frame_start_isr(0U);assert(or_acquisition_fault()==103U && disables==3U);
    or_acquisition_stop();arm();
    /* Completion for the wrong slot or without a frame start is rejected. */
    frame_start_isr(0U);completion_isr(0U,1U);assert(or_acquisition_fault()==101U);
    or_acquisition_stop();arm();
    completion_isr(0U,0U);assert(or_acquisition_fault()==101U);
    or_acquisition_stop();arm();
    /* EDMA controller/transfer errors latch distinct faults and wake the consumer. */
    controller_isr((EDMA_Handle)5,NULL);assert(or_acquisition_fault()==110U);
    or_acquisition_stop();arm();
    {unsigned before=posts;transfer_isr((EDMA_Handle)5,NULL);assert(or_acquisition_fault()==111U && posts==before+1U);}
    or_acquisition_stop();
    /* A failed arm leaves EDMA open until stop, which then permits a clean re-arm. */
    translate_fail=1U;assert(or_acquisition_arm()==-5 && or_acquisition_arm()==-1);
    translate_fail=0U;{unsigned before=closes;or_acquisition_stop();assert(closes==before+1U);}
    arm();or_acquisition_stop();
    puts("PASS: acquisition ping/pong EDMA wiring, ISR ownership, overrun/partial/wrong-slot/EDMA-error faults, rollover timestamps and stop/re-arm.");
    return 0;
}
