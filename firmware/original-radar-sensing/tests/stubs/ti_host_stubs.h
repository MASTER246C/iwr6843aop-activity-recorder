/* Host-test stand-ins for the TI SDK/SYS-BIOS names used by src/radio.c and
 * src/acquisition.c. Original declarations written from those call sites only;
 * no TI header text is copied. Constants, layouts and semantics are NOT TI's:
 * these files exist so the real sources compile on a host against recording
 * fakes in tests/test_radio.c and tests/test_acquisition.c. Never use them for
 * a target build, and passing host tests does not validate TI driver behavior. */
#ifndef OR_TI_HOST_STUBS_H
#define OR_TI_HOST_STUBS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SYS/BIOS kernel and OSAL. */
typedef void *Semaphore_Handle;
uint32_t Clock_getTicks(void);
void Task_sleep(uint32_t ticks);
void Semaphore_post(Semaphore_Handle handle);
uintptr_t HwiP_disable(void);
void HwiP_restore(uintptr_t key);

/* SOC. */
typedef void *SOC_Handle;
typedef void *SOC_SysIntListenerHandle;
typedef struct { uint32_t systemInterrupt; void (*listenerFxn)(uintptr_t argument); } SOC_SysIntListenerCfg;
#define SOC_TranslateAddr_Dir_TO_EDMA 1
#define SOC_TRANSLATEADDR_INVALID 0xffffffffU
#define SOC_XWR68XX_MSS_ADCBUF_BASE_ADDRESS 0x52080000U
#define SOC_XWR68XX_MSS_FRAME_START_INT 7U
double SOC_getDeviceRFFreqScaleFactor(SOC_Handle handle,int32_t *error);
uint32_t SOC_translateAddress(uint32_t address,int direction,int32_t *error);
SOC_SysIntListenerHandle SOC_registerSysIntListener(SOC_Handle handle,SOC_SysIntListenerCfg *cfg,int32_t *error);

/* Mailbox and CRC. */
#define MAILBOX_TYPE_MSS 1
#define CRC_Channel_CH1 1
int32_t Mailbox_init(int type);

/* mmWave control and link names. */
typedef void *MMWave_Handle;
typedef void *MMWave_ProfileHandle;
typedef void *MMWave_ChirpHandle;
typedef int32_t (*MMWave_EventFxn)(uint16_t message,uint16_t subblock,uint16_t length,uint8_t *payload);
#define RL_GET_SBID_FROM_UNIQ_SBID(id) ((uint16_t)((id)%32U))
#define RL_RF_ASYNC_EVENT_MSG 0x10U
#define RL_RF_AE_CPUFAULT_SB 1U
#define RL_RF_AE_ESMFAULT_SB 2U
#define RL_RF_AE_ANALOG_FAULT_SB 3U
#define RL_RF_AE_FRAME_END_SB 4U
enum { MMWave_Domain_MSS=1, MMWave_ExecutionMode_ISOLATION=1, MMWave_ConfigurationMode_FULL=1,
       MMWave_DefaultAsyncEventHandler_MSS=1, MMWave_DFEDataOutputMode_FRAME=1 };
typedef enum { MMWave_ErrorLevel_SUCCESS=0, MMWave_ErrorLevel_WARNING, MMWave_ErrorLevel_ERROR } MMWave_ErrorLevel;
typedef struct { uint32_t useCRCDriver; int crcChannel; } MMWave_LinkCRCCfg;
typedef struct {
    int domain; SOC_Handle socHandle; MMWave_EventFxn eventFxn; int executionMode;
    MMWave_LinkCRCCfg linkCRCCfg; int cfgMode;
} MMWave_InitCfg;
typedef struct {
    uint32_t freqLimitLow,freqLimitHigh;
    struct { uint32_t rxChannelEn,txChannelEn; } chCfg;
    struct { uint32_t lpAdcMode; } lowPowerMode;
    struct { struct { uint32_t b2AdcBits,b2AdcOutFmt; } fmt; } adcOutCfg;
    int defaultAsyncEventHandler;
    bool disableFrameStartAsyncEvent,disableFrameStopAsyncEvent,useCustomCalibration;
    uint32_t calibMonTimeUnit;
} MMWave_OpenCfg;
typedef struct {
    uint32_t profileId,startFreqConst,idleTimeConst,adcStartTimeConst,rampEndTime;
    int32_t freqSlopeConst,txStartTime;
    uint32_t txOutPowerBackoffCode,numAdcSamples,digOutSampleRate,hpfCornerFreq1,hpfCornerFreq2,rxGain;
} rlProfileCfg_t;
typedef struct { uint32_t profileId,txEnable; } rlChirpCfg_t;
typedef struct {
    uint32_t chirpStartIdx,chirpEndIdx,numLoops,numFrames,numAdcSamples,framePeriodicity,triggerSelect;
} rlFrameCfg_t;
typedef struct {
    int dfeDataOutputMode;
    union { struct { MMWave_ProfileHandle profileHandle[4]; rlFrameCfg_t frameCfg; } frameCfg; } u;
} MMWave_CtrlCfg;
typedef struct {
    int dfeDataOutputMode;
    union { struct { bool enableCalibration,enablePeriodicity; uint32_t periodicTimeInFrames; } chirpCalibrationCfg; } u;
} MMWave_CalibrationCfg;
MMWave_Handle MMWave_init(MMWave_InitCfg *cfg,int32_t *error);
int32_t MMWave_sync(MMWave_Handle handle,int32_t *error);
int32_t MMWave_execute(MMWave_Handle handle,int32_t *error);
int32_t MMWave_open(MMWave_Handle handle,const MMWave_OpenCfg *cfg,const void *calibration,int32_t *error);
int32_t MMWave_flushCfg(MMWave_Handle handle,int32_t *error);
MMWave_ProfileHandle MMWave_addProfile(MMWave_Handle handle,const rlProfileCfg_t *cfg,int32_t *error);
MMWave_ChirpHandle MMWave_addChirp(MMWave_ProfileHandle profile,const rlChirpCfg_t *cfg,int32_t *error);
int32_t MMWave_config(MMWave_Handle handle,MMWave_CtrlCfg *cfg,int32_t *error);
int32_t MMWave_start(MMWave_Handle handle,const MMWave_CalibrationCfg *cfg,int32_t *error);
int32_t MMWave_stop(MMWave_Handle handle,int32_t *error);
int32_t MMWave_close(MMWave_Handle handle,int32_t *error);
void MMWave_decodeError(int32_t error,MMWave_ErrorLevel *level,int16_t *code,int16_t *subsystem);

/* ADCBUF. */
typedef void *ADCBuf_Handle;
#define ADCBUF_SOURCE_DFE 1
enum { ADCBufMMWave_CMD_CHANNEL_DISABLE=1, ADCBufMMWave_CMD_CONF_DATA_FORMAT, ADCBufMMWave_CMD_CHANNEL_ENABLE };
typedef struct { SOC_Handle socHandle; int source; uint32_t continousMode,chirpThresholdPing,chirpThresholdPong; } ADCBuf_Params;
typedef struct { uint32_t adcOutFormat,sampleInterleave,channelInterleave; } ADCBuf_dataFormat;
typedef struct { uint8_t channel; uint16_t offset; } ADCBuf_RxChanConf;
void ADCBuf_init(void);
void ADCBuf_Params_init(ADCBuf_Params *params);
ADCBuf_Handle ADCBuf_open(uint32_t index,ADCBuf_Params *params);
int32_t ADCBuf_control(ADCBuf_Handle handle,uint32_t command,void *argument);
uint32_t ADCBuf_getChanBufAddr(ADCBuf_Handle handle,uint8_t channel,int32_t *error);

/* EDMA. */
typedef void *EDMA_Handle;
typedef struct { uint32_t unused; } EDMA_instanceInfo_t;
typedef struct { uint32_t unused; } EDMA_errorInfo_t;
typedef struct { uint32_t unused; } EDMA_transferControllerErrorInfo_t;
typedef void (*EDMA_transferCompletionCallbackFxn_t)(uintptr_t argument,uint8_t tcc);
#define EDMA_NO_ERROR 0
#define EDMA_TPCC0_REQ_DFE_CHIRP_AVAIL 9U
#define EDMA3_CHANNEL_TYPE_DMA 1U
#define EDMA_NULL_LINK_ADDRESS 0xffffU
#define EDMA3_SYNC_A 0U
#define EDMA3_ADDRESSING_MODE_LINEAR 0U
#define EDMA3_FIFO_WIDTH_8BIT 0U
#define EDMA_EVENT_QUEUE_THRESHOLD_MAX 16U
typedef struct {
    uint32_t sourceAddress,destinationAddress;
    uint16_t aCount,bCount,cCount,bCountReload;
    int16_t sourceBindex,destinationBindex,sourceCindex,destinationCindex;
    uint16_t linkAddress;
    uint8_t transferType,transferCompletionCode,sourceAddressingMode,destinationAddressingMode,fifoWidth;
    bool isStaticSet,isEarlyCompletion,isFinalTransferInterruptEnabled;
} EDMA_paramSetConfig_t;
typedef struct {
    bool isConfigAllEventQueues,isEventQueueThresholdingEnabled,isConfigAllTransferControllers,isEnableAllTransferControllerErrors;
    uint8_t eventQueueThreshold;
    void (*callbackFxn)(EDMA_Handle handle,EDMA_errorInfo_t *info);
    void (*transferControllerCallbackFxn)(EDMA_Handle handle,EDMA_transferControllerErrorInfo_t *info);
} EDMA_errorConfig_t;
typedef struct {
    uint8_t channelType,eventQueueId; uint16_t channelId,paramId;
    EDMA_paramSetConfig_t paramSetConfig;
    EDMA_transferCompletionCallbackFxn_t transferCompletionCallbackFxn;
} EDMA_channelConfig_t;
typedef struct {
    EDMA_paramSetConfig_t paramSetConfig;
    EDMA_transferCompletionCallbackFxn_t transferCompletionCallbackFxn;
} EDMA_paramConfig_t;
int32_t EDMA_init(uint8_t instance);
EDMA_Handle EDMA_open(uint8_t instance,int32_t *error,EDMA_instanceInfo_t *info);
int32_t EDMA_configErrorMonitoring(EDMA_Handle handle,EDMA_errorConfig_t *cfg);
int32_t EDMA_configChannel(EDMA_Handle handle,EDMA_channelConfig_t *cfg,bool enable);
int32_t EDMA_configParamSet(EDMA_Handle handle,uint16_t param,EDMA_paramConfig_t *cfg);
int32_t EDMA_linkParamSets(EDMA_Handle handle,uint16_t from,uint16_t to);
int32_t EDMA_enableChannel(EDMA_Handle handle,uint8_t channel,uint8_t type);
int32_t EDMA_disableChannel(EDMA_Handle handle,uint8_t channel,uint8_t type);
int32_t EDMA_close(EDMA_Handle handle);
#endif
