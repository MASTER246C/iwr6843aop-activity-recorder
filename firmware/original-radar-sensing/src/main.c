/* Original sensing application, independent of TI demo application/processing code. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <xdc/std.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/common/sys_common.h>
#include <ti/drivers/soc/soc.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/pinmux/pinmux.h>
#include <ti/drivers/uart/UART.h>
#include "sensing_config.h"
#include "serial_line.h"
#include "acquisition.h"
#include "radio.h"
#include "json_output.h"
#include "energy_summary.h"
#define PROTOCOL "original-radar-v1"
enum {APP_IDLE=0,APP_STARTING,APP_RUNNING,APP_STOPPING,APP_FAULT};
static SOC_Handle soc;
static UART_Handle uart;
static GateMutex_Handle tx_lock;
static Semaphore_Handle frame_ready;
static OrConfig config;
static RadarDsp dsp;
static RadarDspResult result;
static os_config spatial;
static volatile uint32_t app_state=APP_IDLE;
static volatile uint32_t app_fault;
static volatile uint32_t dsp_busy;
static uint32_t start_ms;
static volatile uint32_t deadline_misses,previous_tx_us;
/* Preserved debugger indication for startup failures before the UART is usable. */
volatile int32_t g_original_sensing_boot_fault;

static const char *state_name(void)
{
    static const char *names[]={"idle","starting","running","stopping","fault"};
    return app_state<=APP_FAULT?names[app_state]:"fault";
}
static int send_text(const char *text,size_t length)
{
    IArg key;int32_t written;uint32_t begin;
    if(uart==NULL)return -1;
    key=GateMutex_enter(tx_lock);begin=Clock_getTicks();
    written=UART_write(uart,(uint8_t *)text,(uint32_t)length);
    previous_tx_us=(uint32_t)(Clock_getTicks()-begin)*1000U;
    GateMutex_leave(tx_lock,key);
    if(written!=(int32_t)length){app_fault=401U;return -1;}return 0;
}
static void status_message(void)
{
    char text[384];int n=snprintf(text,sizeof(text),
        "{\"type\":\"status\",\"protocol\":\"%s\",\"state\":\"%s\",\"configured\":%s,"
        "\"fault\":%lu,\"capture_fault\":%lu,\"rf_fault\":%lu,\"rf_error\":%ld,"
        "\"frames\":%lu,\"deadline_misses\":%lu}\n",PROTOCOL,state_name(),config.committed?"true":"false",
        (unsigned long)app_fault,(unsigned long)or_acquisition_fault(),(unsigned long)or_radio_fault(),
        (long)or_radio_error(),(unsigned long)or_acquisition_completed(),(unsigned long)deadline_misses);
    if(n>0&&(size_t)n<sizeof(text))(void)send_text(text,(size_t)n);
}
static void ack(const char *command,int code)
{
    char text[256];int n=snprintf(text,sizeof(text),
        "{\"type\":\"ack\",\"protocol\":\"%s\",\"command\":\"%s\",\"code\":%d,\"state\":\"%s\",\"configured\":%s}\n",
        PROTOCOL,command,code,state_name(),config.committed?"true":"false");
    if(n>0&&(size_t)n<sizeof(text))(void)send_text(text,(size_t)n);
}
static const char *command_name(const char *line)
{
    static const char *names[]={"orConfig","profile","mount","detect","session","commit","start","stop","status"};
    unsigned i;size_t n;
    while(*line==' '||*line=='\t')++line;
    for(i=0U;i<sizeof(names)/sizeof(names[0]);++i) {
        n=strlen(names[i]);
        if(strncmp(line,names[i],n)==0&&(line[n]=='\0'||line[n]==' '||line[n]=='\t'||line[n]=='\r'))return names[i];
    }
    return "invalid";
}
static int end_run(void)
{
    int radio_result;uint32_t begin;
    app_state=APP_STOPPING;
    radio_result=or_radio_stop();
    or_acquisition_stop();
    begin=Clock_getTicks();
    while(dsp_busy && (uint32_t)(Clock_getTicks()-begin)<500U)Task_sleep(1U);
    if(dsp_busy)app_fault=402U;
    if(radio_result<0)app_fault=403U;
    app_state=app_fault?APP_FAULT:APP_IDLE;
    return app_fault?-1:0;
}
static int begin_run(void)
{
    OrRfPhysical physical;RadarDspConfig settings;int status;unsigned i;
    if(app_state!=APP_IDLE || dsp_busy || !config.committed || !or_config_valid(&config))return -20;
    app_state=APP_STARTING;
    status=or_radio_prepare(&config,&physical);
    if(status<0){app_fault=(uint32_t)(420-status);(void)end_run();return -21;}
    memset(&settings,0,sizeof(settings));settings.adc_samples=OR_ADC_SAMPLES;
    settings.chirps=OR_CHIRPS;settings.receivers=OR_RX;
    settings.first_range_bin=config.first_range_bin;settings.last_range_bin=config.last_range_bin;
    for(i=0;i<OR_RX;++i)settings.physical_rx[i]=(uint8_t)i;
    settings.sample_rate_hz=2500000.0f;settings.chirp_slope_hz_per_s=(float)physical.slope_hz_per_s;
    settings.chirp_period_s=0.000250f;settings.carrier_hz=(float)physical.carrier_hz;
    settings.minimum_power=(float)config.minimum_power;settings.noise_multiplier=(float)config.noise_multiplier;
    if(radar_dsp_init(&dsp,&settings)!=RADAR_DSP_OK){app_fault=440U;(void)end_run();return -22;}
    os_default_config(&spatial,settings.carrier_hz);
    spatial.bracket_height_m=config.bracket_top_mm/1000.0f;
    spatial.down_tilt_deg=config.down_tilt_mdeg/1000.0f;
    spatial.antenna_offset_known=config.antenna_offset_known;
    if(config.antenna_offset_known)spatial.antenna_vertical_offset_m=config.antenna_vertical_offset_mm/1000.0f;
    /* RX phase and axes remain explicitly uncalibrated, even when offset is known. */
    while(Semaphore_pend(frame_ready,0U)){}
    if(or_acquisition_arm()<0){app_fault=441U;(void)end_run();return -23;}
    deadline_misses=0U;previous_tx_us=0U;start_ms=Clock_getTicks();
    app_state=APP_RUNNING;
    if(or_radio_start()<0){app_fault=442U;(void)end_run();return -24;}
    return 0;
}
static void radio_task(UArg unused0,UArg unused1)
{
    (void)unused0;(void)unused1;
    for(;;)or_radio_execute();
}
static void processing_task(UArg unused0,UArg unused1)
{
    const RadarComplex16 *samples;RadarFrameInfo frame;OrTiming timing;
    unsigned slot;uint32_t generation,begin;size_t size;char text[OR_JSON_MAX+1U];
    RadarDspStatus processed;int still_valid;
    (void)unused0;(void)unused1;
    for(;;) {
        (void)Semaphore_pend(frame_ready,BIOS_WAIT_FOREVER);
        dsp_busy=1U;
        if(app_state!=APP_RUNNING){dsp_busy=0U;continue;}
        if(!or_acquisition_take(&samples,&frame,&slot,&generation)){dsp_busy=0U;continue;}
        begin=Clock_getTicks();
        processed=radar_dsp_process(&dsp,samples,OR_FRAME_SAMPLES,&frame,&result);
        still_valid=or_acquisition_release(slot,generation);
        if(processed!=RADAR_DSP_OK||still_valid<0||app_state!=APP_RUNNING||or_acquisition_fault()) {
            if(app_state==APP_RUNNING)app_fault=450U;
            dsp_busy=0U;continue;
        }
        if(or_profile_energy(&dsp,timing.profile_energy)<0){app_fault=453U;dsp_busy=0U;continue;}
        timing.processing_us=(uint32_t)(Clock_getTicks()-begin)*1000U;
        timing.frame_events=or_acquisition_frame_events();timing.previous_tx_us=previous_tx_us;timing.deadline_misses=deadline_misses;timing.capture_fault=0U;
        size=or_frame_json(text,sizeof(text),&config,&spatial,&result,&timing);
        if(size==0U){app_fault=451U;dsp_busy=0U;continue;}
        if(app_state==APP_RUNNING&&!or_acquisition_fault())(void)send_text(text,size);
        if((uint32_t)(Clock_getTicks()-begin)>=180U){++deadline_misses;app_fault=452U;}
        dsp_busy=0U;
    }
}
static void watchdog(void)
{
    if(app_state!=APP_RUNNING)return;
    if(or_radio_fault())app_fault=460U;
    if(or_acquisition_fault())app_fault=461U;
    if((uint32_t)(Clock_getTicks()-or_acquisition_last_ms())>1000U)app_fault=462U;
    if(app_fault || or_radio_frame_end() ||
       (uint32_t)(Clock_getTicks()-start_ms)>(uint32_t)config.session_seconds*1000U+1000U) {
        (void)end_run();status_message();
    }
}
static void handle_line(char *line)
{
    OrCommand cmd=or_config_line(&config,line,app_state!=APP_IDLE);int code=0;
    const char *name=command_name(line);
    if(cmd==OR_CMD_NONE)return;
    if(cmd<0){ack(name,(int)cmd);return;}
    if(cmd==OR_CMD_START)code=begin_run();
    else if(cmd==OR_CMD_STOP) {
        if(app_state==APP_RUNNING||app_state==APP_STARTING||app_state==APP_FAULT)code=end_run();
    } else if(cmd==OR_CMD_STATUS)status_message();
    ack(name,code);
}
static void command_task(UArg unused0,UArg unused1)
{
    OrSerialLine line;uint8_t byte;int assembled;int32_t count;
    memset(&line,0,sizeof(line));
    (void)unused0;(void)unused1;
    for(;;) {
        watchdog();count=UART_read(uart,&byte,1U);
        if(count<0){app_fault=470U;watchdog();Task_sleep(10U);continue;}
        if(count==0)continue;
        assembled=or_serial_feed(&line,byte);
        if(assembled<0)ack("invalid",-9);
        else if(assembled>0)handle_line(line.text);
    }
}
static void fail_boot(int32_t code)
{
    g_original_sensing_boot_fault=code;
    for(;;){}
}
static void init_task(UArg unused0,UArg unused1)
{
    UART_Params uart_options;Task_Params task;Semaphore_Params sem;
    const char *identity="{\"type\":\"identity\",\"protocol\":\"original-radar-v1\",\"app\":\"original-radar-sensing\",\"version\":\"0.2.0\",\"baud\":115200,\"sensing\":true,\"classifier\":false}\n";
    (void)unused0;(void)unused1;
    /* SDK N5/N4 PADBE/PADBD aliases correspond to AOP U16/V16 pads. */
    if(Pinmux_Set_OverrideCtrl(SOC_XWR68XX_PINN5_PADBE,PINMUX_OUTEN_RETAIN_HW_CTRL,PINMUX_INPEN_RETAIN_HW_CTRL)<0 ||
       Pinmux_Set_FuncSel(SOC_XWR68XX_PINN5_PADBE,SOC_XWR68XX_PINN5_PADBE_MSS_UARTA_TX)<0 ||
       Pinmux_Set_OverrideCtrl(SOC_XWR68XX_PINN4_PADBD,PINMUX_OUTEN_RETAIN_HW_CTRL,PINMUX_INPEN_RETAIN_HW_CTRL)<0 ||
       Pinmux_Set_FuncSel(SOC_XWR68XX_PINN4_PADBD,SOC_XWR68XX_PINN4_PADBD_MSS_UARTA_RX)<0)fail_boot(-2);
    tx_lock=GateMutex_create(NULL,NULL);if(tx_lock==NULL)fail_boot(-3);
    UART_init();UART_Params_init(&uart_options);uart_options.clockFrequency=200000000U;
    uart_options.baudRate=115200U;uart_options.writeDataMode=UART_DATA_BINARY;
    uart_options.readDataMode=UART_DATA_BINARY;uart_options.readEcho=UART_ECHO_OFF;
    uart_options.readTimeout=10U;uart_options.writeTimeout=200U;uart_options.isPinMuxDone=1U;
    uart_options.dmaHandle=NULL;
    uart=UART_open(0U,&uart_options);if(uart==NULL)fail_boot(-4);
    or_config_reset(&config);(void)send_text(identity,strlen(identity));
    Semaphore_Params_init(&sem);sem.mode=Semaphore_Mode_COUNTING;
    frame_ready=Semaphore_create(0,&sem,NULL);if(frame_ready==NULL)fail_boot(-5);
    if(or_acquisition_init(soc,frame_ready)<0){app_fault=480U;app_state=APP_FAULT;status_message();fail_boot(-6);}
    if(or_radio_init(soc)<0){app_fault=481U;app_state=APP_FAULT;status_message();fail_boot(-7);}
    Task_Params_init(&task);task.priority=6;task.stackSize=6144U;
    if(Task_create(radio_task,&task,NULL)==NULL)fail_boot(-8);
    Task_Params_init(&task);task.priority=3;task.stackSize=8192U;
    if(Task_create(processing_task,&task,NULL)==NULL)fail_boot(-9);
    Task_Params_init(&task);task.priority=4;task.stackSize=6144U;
    if(Task_create(command_task,&task,NULL)==NULL)fail_boot(-10);
    status_message();
}
int main(void)
{
    SOC_Cfg cfg;int32_t error=0;Task_Params task;
    ESM_init(0U);memset(&cfg,0,sizeof(cfg));cfg.clockCfg=SOC_SysClock_INIT;
    cfg.mpuCfg=SOC_MPUCfg_CONFIG;cfg.dssCfg=SOC_DSSCfg_HALT;
    soc=SOC_init(&cfg,&error);if(soc==NULL||error)fail_boot(-1);
    Task_Params_init(&task);task.priority=5;task.stackSize=4096U;
    if(Task_create(init_task,&task,NULL)==NULL)fail_boot(-11);
    BIOS_start();return 0;
}
