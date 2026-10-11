"""Host model of exact application status/ACK source, not hardware verification.
Usage: python3 test_exact_record_framing.py PATH_TO_FIRMWARE_SOURCE_ROOT
Compiles in a temporary directory; no firmware source is modified.
"""
from pathlib import Path
import json, os, subprocess, sys, tempfile
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
source=(root/'src/main.c').read_text()
functions=source[source.index('static int transport_ready('):source.index('static const char *command_name(')]
preamble=r'''
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "record_transport.h"
#include "build_identity.h"
#include "spatial_selftest.h"
#define PROTOCOL "original-radar-v1"
typedef int IArg;
typedef struct { volatile uint32_t SCIFLR; } SCIRegs;
static SCIRegs mock_sci={256U};
#define SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS (&mock_sci)
#define SCIFLR_TXRDY_BIT_START 8U
static void *uart=(void*)1;
static int tx_lock;
static OrRecordTx record_tx;
static struct {int committed;} config;
static uint32_t app_fault,previous_tx_us,deadline_misses,boot_selftest_mask,now;
static int pending=-1;
static int GateMutex_enter(int x){return x;}
static void GateMutex_leave(int x,IArg y){(void)x;(void)y;}
static uint32_t Clock_getTicks(void){return now;}
static void drain(void){if(pending>=0){putchar(pending);pending=-1;}mock_sci.SCIFLR=256U;}
static void Task_sleep(unsigned ticks){now+=ticks;drain();}
/* Suspected TD-slot overwrite semantics. This model is not physical proof. */
static int32_t UART_write(void *h,uint8_t *s,uint32_t n){
 (void)h;
 for(uint32_t i=0;i<n;i++){
   if(i)drain();
   pending=s[i];mock_sci.SCIFLR=0U;
 }
 return (int32_t)n;
}
static int selftest_passed(void){return 1;}
static const char *state_name(void){return "idle";}
static uint32_t or_acquisition_fault(void){return 0;}
static uint32_t or_radio_fault(void){return 0;}
static int or_radio_error(void){return 0;}
static uint32_t or_acquisition_completed(void){return 0;}
'''
post='\nint main(void){for(unsigned i=0;i<1000;i++){status_message();ack("status",0);}drain();return app_fault?1:0;}\n'
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp);c=d/'harness.c';c.write_text(preamble+functions+post)
 command=[os.environ.get('HOST_CC','cc'),'-std=c99','-O2','-g','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-I'+str(root/'src'),'-I'+str(root/'spatial'),str(c),str(root/'src/record_transport.c'),'-o',str(d/'test')]
 subprocess.run(command,check=True)
 wire=subprocess.check_output([str(d/'test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 rows=wire.splitlines();assert len(rows)==2000;assert wire.count(b'\n')==2000 and b'\r' not in wire
 records=[json.loads(row) for row in rows]
 for i in range(0,2000,2):
  status,ack=records[i:i+2]
  assert status['type']=='status' and ack['type']=='ack' and ack['command']=='status'
  assert status['state']=='idle' and ack['state']=='idle' and ack['code']==0
  assert status['selftest_ok'] is True and status['selftest_mask']==0
  assert status['firmware_version']=='0.2.2-rc1' and len(status['build_id'])==24
  assert all(status[k]==0 for k in ['fault','capture_fault','rf_fault','rf_error','frames','deadline_misses'])
 print(json.dumps({'result':'PASS','model':'pending-TD overwrite','pairs':1000,'records':2000,'lf':2000,'bytes':len(wire),'firmware_version':records[0]['firmware_version'],'build_id':records[0]['build_id'],'hardware_verified':False},sort_keys=True))
