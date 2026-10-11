"""Compile actual main.c functions with host peripheral mocks; no hardware.
TI and hardware includes are substituted in a generated translation unit only.
Production source is not changed or preprocessor-instrumented for this test.
"""
from pathlib import Path
import subprocess,os
root=Path(__file__).resolve().parents[1];rc=root/'rc' if (root/'rc').is_dir() else root;dest=root/'checks';dest.mkdir(exist_ok=True)
source=(rc/'src/main.c').read_text()
lines=[]
for line in source.splitlines():
    if line.startswith('#include <xdc/') or line.startswith('#include <ti/') or line in ['#include "acquisition.h"','#include "radio.h"']: continue
    if line=='int main(void)': line='int unused_board_main(void)'
    lines.append(line)
stubs=r'''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "radar_dsp.h"
#include "rf_profile.h"
#include "sensing_config.h"
typedef struct { volatile uint32_t SCIFLR; } SCIRegs;
static SCIRegs mock_sci={256U},mock_sci_b={256U};
#define SOC_XWR68XX_MSS_SCI_B_BASE_ADDRESS (&mock_sci_b)
#define SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS (&mock_sci)
#define SCIFLR_TXRDY_BIT_START 8U
typedef intptr_t IArg; typedef uintptr_t UArg;
typedef void *SOC_Handle;typedef void *UART_Handle;typedef void *GateMutex_Handle;typedef void *Semaphore_Handle;
typedef struct { int priority; unsigned stackSize; } Task_Params;
typedef struct { int mode; } Semaphore_Params;
typedef struct { unsigned clockFrequency,baudRate,writeDataMode,readDataMode,readEcho,readTimeout,writeTimeout,isPinMuxDone;void *dmaHandle;} UART_Params;
typedef struct { int clockCfg,mpuCfg,dssCfg;} SOC_Cfg;
#define SOC_XWR68XX_PINF14_PADAJ 3
#define SOC_XWR68XX_PINF14_PADAJ_MSS_UARTB_TX 0
#define SOC_XWR68XX_PINN5_PADBE 1
#define SOC_XWR68XX_PINN4_PADBD 2
#define PINMUX_OUTEN_RETAIN_HW_CTRL 0
#define PINMUX_INPEN_RETAIN_HW_CTRL 0
#define SOC_XWR68XX_PINN5_PADBE_MSS_UARTA_TX 0
#define SOC_XWR68XX_PINN4_PADBD_MSS_UARTA_RX 0
#define UART_DATA_BINARY 0
#define UART_ECHO_OFF 0
#define Semaphore_Mode_COUNTING 0
#define BIOS_WAIT_FOREVER 0
#define SOC_SysClock_INIT 0
#define SOC_MPUCfg_CONFIG 0
#define SOC_DSSCfg_HALT 0
static unsigned hardware_calls,starts,task_creates,chosen_mask,fail_data_open;
static unsigned control_writes,data_writes;
static char last_message[1024];
static int Pinmux_Set_OverrideCtrl(int a,int b,int c){(void)a;(void)b;(void)c;return 0;}
static int Pinmux_Set_FuncSel(int a,int b){(void)a;(void)b;return 0;}
static GateMutex_Handle GateMutex_create(void*a,void*b){(void)a;(void)b;return (void*)1;}
static IArg GateMutex_enter(GateMutex_Handle a){(void)a;return 0;}
static void GateMutex_leave(GateMutex_Handle a,IArg b){(void)a;(void)b;}
static uint32_t Clock_getTicks(void){return 0;}
static void UART_init(void){}
static void UART_Params_init(UART_Params*p){memset(p,0,sizeof(*p));}
static UART_Handle UART_open(unsigned a,UART_Params*b){assert(b->baudRate==(a?921600U:115200U));if(a && fail_data_open)return NULL;return a?(void*)2:(void*)1;}
static int32_t UART_write(UART_Handle a,uint8_t*b,uint32_t n){if(a==(void*)2)++data_writes;else ++control_writes;assert(n<sizeof(last_message));memcpy(last_message,b,n);last_message[n]=0;return (int32_t)n;}
static int32_t UART_read(UART_Handle a,uint8_t*b,uint32_t c){(void)a;(void)b;(void)c;return 0;}
static void Task_sleep(unsigned n){(void)n;}
static void Task_Params_init(Task_Params*p){memset(p,0,sizeof(*p));}
static void *Task_create(void(*f)(UArg,UArg),Task_Params*p,void*x){(void)f;(void)p;(void)x;task_creates++;return (void*)1;}
static void Semaphore_Params_init(Semaphore_Params*p){memset(p,0,sizeof(*p));}
static Semaphore_Handle Semaphore_create(int n,Semaphore_Params*p,void*x){(void)n;(void)p;(void)x;return (void*)1;}
static int Semaphore_pend(Semaphore_Handle s,unsigned t){(void)s;(void)t;return 0;}
static void ESM_init(unsigned n){(void)n;}
static SOC_Handle SOC_init(SOC_Cfg*p,int32_t*e){(void)p;*e=0;return (void*)1;}
static void BIOS_start(void){}
uint32_t os_spatial_no_rf_selftest(void){return chosen_mask;}
static uint32_t or_acquisition_fault(void){hardware_calls++;return 0;}
static uint32_t or_acquisition_completed(void){hardware_calls++;return 0;}
static uint32_t or_acquisition_frame_events(void){hardware_calls++;return 0;}
static uint32_t or_acquisition_last_ms(void){hardware_calls++;return 0;}
static int or_acquisition_init(SOC_Handle a,Semaphore_Handle b){(void)a;(void)b;hardware_calls++;return 0;}
static void or_acquisition_stop(void){hardware_calls++;}
static int or_acquisition_arm(void){hardware_calls++;return 0;}
static int or_acquisition_take(const RadarComplex16**a,RadarFrameInfo*b,unsigned*c,uint32_t*d){(void)a;(void)b;(void)c;(void)d;hardware_calls++;return 0;}
static int or_acquisition_release(unsigned a,uint32_t b){(void)a;(void)b;hardware_calls++;return 0;}
static uint32_t or_radio_fault(void){hardware_calls++;return 0;}
static int32_t or_radio_error(void){hardware_calls++;return 0;}
static uint32_t or_radio_frame_end(void){hardware_calls++;return 0;}
static int or_radio_init(SOC_Handle a){(void)a;hardware_calls++;return 0;}
static void or_radio_execute(void){hardware_calls++;}
static int or_radio_stop(void){hardware_calls++;return 0;}
static int or_radio_prepare(const OrConfig*a,OrRfPhysical*b){(void)a;hardware_calls++;return or_rf_physical(2.7,b);}
static int or_radio_start(void){hardware_calls++;starts++;return 0;}
'''
test=r'''
int main(void){
 unsigned bit;
 for(bit=0;bit<32;bit++) {
  chosen_mask=1U<<bit;hardware_calls=0;starts=0;task_creates=0;
  boot_selftest_mask=0xffffffffU;boot_selftest_completed=0;app_state=APP_IDLE;app_fault=0;hardware_initialized=0;
  assert(begin_run()==-25);assert(hardware_calls==0);
  init_task(0,0);
  assert(!selftest_passed());assert(app_state==APP_FAULT);assert(app_fault==482);
  assert(hardware_calls==0);assert(starts==0);assert(task_creates==1);
  assert(strstr(last_message,"\"selftest_ok\":false"));assert(strstr(last_message,"\"firmware_version\":\"0.3.1-rc1\""));
  {char status[]="status";char stop[]="stop";char start[]="start";handle_line(status);handle_line(stop);handle_line(start);}
  assert(hardware_calls==0);assert(starts==0);assert(app_state==APP_FAULT);
 }
 chosen_mask=0;hardware_calls=0;starts=0;task_creates=0;app_fault=0;app_state=APP_IDLE;
 boot_selftest_completed=0;boot_selftest_mask=0xffffffffU;
 init_task(0,0);assert(selftest_passed());assert(task_creates==3);assert(starts==0);
 assert(strstr(last_message,"\"selftest_ok\":true"));assert(strstr(last_message,"\"selftest_mask\":0"));assert(strstr(last_message,"\"configured\":false"));
 assert(begin_run()==-20);assert(starts==0);
 /* A completed config cannot override a latched transmit/application fault. */
 config.fields_seen=15U;config.committed=1U;
 record_tx.failed=1U;hardware_calls=0;
 assert(begin_run()==-26);assert(hardware_calls==0 && starts==0);
 record_tx.failed=0U;app_fault=401U;
 assert(begin_run()==-26);assert(hardware_calls==0 && starts==0);
 app_fault=0;hardware_calls=0;data_record_tx.failed=1U;
 assert(begin_run()==-26);assert(hardware_calls==0 && starts==0);
 data_record_tx.failed=0U;
 assert(begin_run()==-27);assert(hardware_calls==0 && starts==0);
 assert(or_link_commit(&link_session)==0);
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 assert(link_session.probe_sent && !link_session.paired);
 assert(strstr(last_message,"\"type\":\"data_probe\""));
 assert(pairing_command("pairConfirm","pairConfirm 0123456789abcdef0123456789abcdee")<0);
 assert(!link_session.paired && !link_session.probe_sent);
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 assert(pairing_command("pairConfirm","pairConfirm 0123456789abcdef0123456789abcdef")==0);
 assert(link_session.paired && link_session.start_permit);
 assert(hardware_calls==0 && starts==0);
 {char change[]="session 5";handle_line(change);}
 assert(!link_session.paired && !link_session.start_permit);
 config.committed=1U;
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 assert(pairing_command("pairConfirm","pairConfirm 0123456789abcdef0123456789abcdef")==0);
 {char stop[]="stop";handle_line(stop);}
 assert(!link_session.paired && !link_session.start_permit);
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 mock_sci_b.SCIFLR=0U;
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")<0);
 assert(data_record_tx.failed && app_fault==484U && !link_session.paired && !link_session.probe_sent);
 hardware_calls=0;assert(begin_run()==-26);assert(hardware_calls==0 && starts==0);
 status_message();assert(strstr(last_message,"\"data_tx_fault\":true"));
 /* Prove separate lanes and one-shot start before the mocked RF path. */
 app_fault=0;data_record_tx.failed=0;mock_sci_b.SCIFLR=256U;
 control_writes=data_writes=0;
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 assert(data_writes==1 && control_writes==0);
 ack("pair",0);status_message();assert(control_writes==2 && data_writes==1);
 assert(pairing_command("pairConfirm","pairConfirm 0123456789abcdef0123456789abcdef")==0);
 assert(begin_run()==0 && starts==1 && !link_session.start_permit && link_session.paired);
 assert(spatial.rx_correction[1].real==-1.0f && spatial.rx_correction[3].real==-1.0f);
 assert(!spatial.rx_phase_calibrated && !spatial.axis_orientation_verified);
 assert(begin_run()<0 && starts==1);
 assert(end_run()==0 && !link_session.paired);
 assert(begin_run()==-27 && starts==1);
 /* Each adapter selects its own volatile readiness register. */
 mock_sci.SCIFLR=0;mock_sci_b.SCIFLR=256U;
 assert(!transport_ready(NULL) && transport_ready((void*)1));
 mock_sci.SCIFLR=256U;mock_sci_b.SCIFLR=0;
 assert(transport_ready(NULL) && !transport_ready((void*)1));
 mock_sci_b.SCIFLR=256U;
 assert(pairing_command("pair","pair 0123456789abcdef0123456789abcdef")==0);
 assert(pairing_command("pairConfirm","pairConfirm 0123456789abcdef0123456789abcdef")==0);
 assert(begin_run()==0 && starts==2);
 mock_sci_b.SCIFLR=0;assert(send_data("{}\n",3)<0);
 assert(app_fault==484U && !link_session.paired && app_state==APP_RUNNING);
 hardware_calls=0;watchdog();
 assert(app_state==APP_FAULT && hardware_calls>0 && !link_session.start_permit);
 assert(!record_tx.failed);status_message();assert(strstr(last_message,"\"data_tx_fault\":true"));
 mock_sci_b.SCIFLR=256U;
 /* Data open failure must preserve control/status and create no RF resources. */
 fail_data_open=1;data_uart=NULL;data_record_tx.failed=0;app_fault=0;app_state=APP_IDLE;
 hardware_initialized=0;hardware_calls=0;starts=0;task_creates=0;
 init_task(0,0);
 assert(app_fault==483U && app_state==APP_FAULT && task_creates==1);
 assert(hardware_calls==0 && starts==0 && selftest_passed());
 assert(strstr(last_message,"\"data_tx_fault\":true"));
 {char stop[]="stop";handle_line(stop);}
 assert(hardware_calls==0 && starts==0);

 puts("PASS: boot, TX-latch and app faults block radio/acquisition; no auto RF/config.");
 return 0;
}
'''
p=dest/'boot_gate_harness.c';p.write_text(stubs+'\n'+'\n'.join(lines)+'\n'+test)
cmd=['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-I'+str(rc/'src'),'-I'+str(rc/'dsp'),'-I'+str(rc/'spatial'),str(p)]+[str(rc/s) for s in ['src/record_transport.c','src/link_session.c','src/sensing_config.c','src/rf_profile.c','src/serial_line.c','src/json_output.c','src/energy_summary.c','dsp/radar_dsp.c','spatial/original_spatial.c']]+['-lm','-o',str(dest/'boot_gate_harness')]
subprocess.run(cmd,check=True);subprocess.run([str(dest/'boot_gate_harness')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
