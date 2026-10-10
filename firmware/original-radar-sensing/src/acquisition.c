/* Original ADC acquisition. TI drivers provide register access and interrupt dispatch. */
#include <string.h>
#include <ti/common/sys_common.h>
#include <ti/drivers/adcbuf/ADCBuf.h>
#include <ti/drivers/edma/edma.h>
#include <ti/drivers/osal/HwiP.h>
#include <ti/sysbios/knl/Clock.h>
#include "acquisition.h"
#define OR_EDMA_CHANNEL EDMA_TPCC0_REQ_DFE_CHIRP_AVAIL
#define OR_SHADOW0 64U
#define OR_SHADOW1 65U
#pragma DATA_SECTION(capture_data, ".capture_frames")
#pragma DATA_ALIGN(capture_data, 128)
static RadarComplex16 capture_data[2][OR_FRAME_SAMPLES];
static ADCBuf_Handle adc;
static EDMA_Handle dma;
static Semaphore_Handle ready_sem;
static volatile OrCaptureModel model;
static SOC_SysIntListenerHandle frame_listener;
static uint32_t adc_edma_address;
static volatile uint32_t hardware_fault;
static volatile uint32_t last_completion_ms;

static void stop_events(uint32_t reason)
{
    hardware_fault=reason;
    if(dma!=NULL) (void)EDMA_disableChannel(dma,OR_EDMA_CHANNEL,EDMA3_CHANNEL_TYPE_DMA);
    model.running=0U;
    Semaphore_post(ready_sem);
}
static void frame_start(uintptr_t argument)
{
    (void)argument;
    if(model.running && or_capture_model_frame_start(&model)<0)stop_events(103U);
}
static void completed(uintptr_t argument,uint8_t tcc)
{
    (void)argument;
    if(hardware_fault || !model.running) return;
    last_completion_ms=Clock_getTicks();
    if(or_capture_model_complete(&model,tcc,last_completion_ms)<0) {
        stop_events(100U+model.fault);return;
    }
    Semaphore_post(ready_sem);
}
static void controller_error(EDMA_Handle h,EDMA_errorInfo_t *info)
{
    (void)h;(void)info;stop_events(110U);
}
static void transfer_error(EDMA_Handle h,EDMA_transferControllerErrorInfo_t *info)
{
    (void)h;(void)info;stop_events(111U);
}
static int parameters(EDMA_paramSetConfig_t *p,unsigned slot)
{
    int32_t error=0;
    memset(p,0,sizeof(*p));
    p->sourceAddress=adc_edma_address;
    p->destinationAddress=SOC_translateAddress((uint32_t)capture_data[slot],
                                  SOC_TranslateAddr_Dir_TO_EDMA,&error);
    if(error!=0 || p->destinationAddress==SOC_TRANSLATEADDR_INVALID) return -1;
    p->aCount=OR_CHIRP_BYTES; p->bCount=OR_CHIRPS; p->cCount=1U;
    p->bCountReload=OR_CHIRPS;
    /* ADCBUF presents the completed ping/pong bank at the same CPU/DMA window.
     * One threshold event copies RX0..RX3 (256 bytes each), then advances output. */
    p->sourceBindex=0; p->destinationBindex=OR_CHIRP_BYTES;
    p->sourceCindex=0; p->destinationCindex=0;
    p->linkAddress=EDMA_NULL_LINK_ADDRESS;
    p->transferType=EDMA3_SYNC_A; p->transferCompletionCode=(uint8_t)slot;
    p->sourceAddressingMode=EDMA3_ADDRESSING_MODE_LINEAR;
    p->destinationAddressingMode=EDMA3_ADDRESSING_MODE_LINEAR;
    p->fifoWidth=EDMA3_FIFO_WIDTH_8BIT;
    p->isStaticSet=false; p->isEarlyCompletion=false;
    p->isFinalTransferInterruptEnabled=true;
    return 0;
}
int or_acquisition_init(SOC_Handle soc,Semaphore_Handle ready)
{
    ADCBuf_Params params; ADCBuf_dataFormat format; ADCBuf_RxChanConf channel;
    uint32_t mask=15U; uint32_t address; unsigned rx; int32_t error=0;SOC_SysIntListenerCfg listener;
    ready_sem=ready;
    ADCBuf_init();ADCBuf_Params_init(&params);
    params.socHandle=soc;params.source=ADCBUF_SOURCE_DFE;params.continousMode=0U;
    /* Driver accepts actual threshold and subtracts one for the register. */
    params.chirpThresholdPing=1U;params.chirpThresholdPong=1U;
    adc=ADCBuf_open(0U,&params);if(adc==NULL)return -1;
    if(ADCBuf_control(adc,ADCBufMMWave_CMD_CHANNEL_DISABLE,&mask)<0)return -2;
    memset(&format,0,sizeof(format));format.adcOutFormat=0U;
    format.sampleInterleave=0U;format.channelInterleave=1U;
    if(ADCBuf_control(adc,ADCBufMMWave_CMD_CONF_DATA_FORMAT,&format)<0)return -3;
    for(rx=0;rx<OR_RX;++rx) {
        memset(&channel,0,sizeof(channel));channel.channel=(uint8_t)rx;
        channel.offset=(uint16_t)(rx*OR_ADC_SAMPLES*4U);
        if(ADCBuf_control(adc,ADCBufMMWave_CMD_CHANNEL_ENABLE,&channel)<0)return -4;
        address=ADCBuf_getChanBufAddr(adc,(uint8_t)rx,&error);
        if(error || address!=SOC_XWR68XX_MSS_ADCBUF_BASE_ADDRESS+channel.offset)return -5;
    }
    adc_edma_address=SOC_translateAddress(SOC_XWR68XX_MSS_ADCBUF_BASE_ADDRESS,
                                         SOC_TranslateAddr_Dir_TO_EDMA,&error);
    if(error || adc_edma_address==SOC_TRANSLATEADDR_INVALID)return -6;
    memset(&listener,0,sizeof(listener));listener.systemInterrupt=SOC_XWR68XX_MSS_FRAME_START_INT;
    listener.listenerFxn=frame_start;
    frame_listener=SOC_registerSysIntListener(soc,&listener,&error);
    return frame_listener==NULL||error?-7:0;
}
int or_acquisition_arm(void)
{
    EDMA_instanceInfo_t instance; EDMA_errorConfig_t errors;
    EDMA_channelConfig_t channel; EDMA_paramConfig_t shadow;
    int32_t error=0; uintptr_t key;
    if(dma!=NULL)return -1;
    /* This application owns EDMA0 exclusively. Reset clears stale hardware events. */
    if(EDMA_init(0U)!=EDMA_NO_ERROR)return -2;
    dma=EDMA_open(0U,&error,&instance);if(dma==NULL)return -3;
    memset(&errors,0,sizeof(errors));errors.isConfigAllEventQueues=true;
    errors.isEventQueueThresholdingEnabled=true;
    errors.eventQueueThreshold=EDMA_EVENT_QUEUE_THRESHOLD_MAX;
    errors.isConfigAllTransferControllers=true;errors.isEnableAllTransferControllerErrors=true;
    errors.callbackFxn=controller_error;errors.transferControllerCallbackFxn=transfer_error;
    if(EDMA_configErrorMonitoring(dma,&errors)!=EDMA_NO_ERROR)return -4;
    memset(&channel,0,sizeof(channel));channel.channelType=EDMA3_CHANNEL_TYPE_DMA;
    channel.channelId=OR_EDMA_CHANNEL;channel.paramId=OR_EDMA_CHANNEL;
    channel.eventQueueId=0U;channel.transferCompletionCallbackFxn=completed;
    if(parameters(&channel.paramSetConfig,0U)<0)return -5;
    if(EDMA_configChannel(dma,&channel,false)!=EDMA_NO_ERROR)return -6;
    memset(&shadow,0,sizeof(shadow));shadow.transferCompletionCallbackFxn=completed;
    if(parameters(&shadow.paramSetConfig,0U)<0 ||
       EDMA_configParamSet(dma,OR_SHADOW0,&shadow)!=EDMA_NO_ERROR)return -7;
    if(parameters(&shadow.paramSetConfig,1U)<0 ||
       EDMA_configParamSet(dma,OR_SHADOW1,&shadow)!=EDMA_NO_ERROR)return -8;
    if(EDMA_linkParamSets(dma,OR_EDMA_CHANNEL,OR_SHADOW1)!=EDMA_NO_ERROR ||
       EDMA_linkParamSets(dma,OR_SHADOW1,OR_SHADOW0)!=EDMA_NO_ERROR ||
       EDMA_linkParamSets(dma,OR_SHADOW0,OR_SHADOW1)!=EDMA_NO_ERROR)return -9;
    key=HwiP_disable();or_capture_model_arm(&model);hardware_fault=0U;
    last_completion_ms=Clock_getTicks();HwiP_restore(key);
    if(EDMA_enableChannel(dma,OR_EDMA_CHANNEL,EDMA3_CHANNEL_TYPE_DMA)!=EDMA_NO_ERROR)return -10;
    return 0;
}
void or_acquisition_stop(void)
{
    uintptr_t key=HwiP_disable();or_capture_model_stop(&model);
    if(dma!=NULL)(void)EDMA_disableChannel(dma,OR_EDMA_CHANNEL,EDMA3_CHANNEL_TYPE_DMA);
    HwiP_restore(key);
    if(dma!=NULL){(void)EDMA_close(dma);dma=NULL;}
}
int or_acquisition_take(const RadarComplex16 **samples,RadarFrameInfo *frame,
                        unsigned *slot,uint32_t *generation)
{
    uintptr_t key=HwiP_disable();int found=or_capture_model_take(&model,slot,generation);
    if(found) {
        memset(frame,0,sizeof(*frame));frame->sequence=model.sequence[*slot];
        frame->timestamp_us=(uint64_t)model.timestamp_ms[*slot]*1000U;
        frame->valid_chirps=OR_CHIRPS;frame->capture_complete=1U;
        *samples=capture_data[*slot];
    }
    HwiP_restore(key);return found;
}
int or_acquisition_release(unsigned slot,uint32_t generation)
{
    uintptr_t key=HwiP_disable();int status=or_capture_model_release(&model,slot,generation);
    HwiP_restore(key);return status;
}
uint32_t or_acquisition_fault(void){return hardware_fault?hardware_fault:model.fault;}
uint32_t or_acquisition_completed(void){return model.completed;}
uint32_t or_acquisition_last_ms(void){return last_completion_ms;}

uint32_t or_acquisition_frame_events(void){return model.frame_starts;}
