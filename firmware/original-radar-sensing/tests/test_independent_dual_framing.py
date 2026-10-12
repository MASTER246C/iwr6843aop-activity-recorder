"""Synthetic dual-TD host model using exact application adapters/serializers.
Usage: python3 test_independent_dual_framing.py FIRMWARE_ROOT
No board, RF, serial device or user capture is accessed.
"""
from pathlib import Path
import json,os,subprocess,sys,tempfile
root=Path(sys.argv[1]).resolve();source=(root/'src/main.c').read_text()
adapters=source[source.index('static int transport_ready('):source.index('static const char *command_name(')]
pairing=source[source.index('static int pairing_command('):source.index('static void handle_line(')]
preamble=r'''
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "record_transport.h"
#include "build_identity.h"
#include "spatial_selftest.h"
#include "link_session.h"
#include "json_output.h"
#define PROTOCOL OR_PROTOCOL
enum {APP_IDLE=0,APP_STARTING,APP_RUNNING,APP_STOPPING,APP_FAULT};
typedef int IArg;typedef void* GateMutex_Handle;
typedef struct { volatile uint32_t SCIFLR; } SCIRegs;
static SCIRegs sci[2]={{256U},{256U}};
#define SOC_XWR68XX_MSS_SCI_A_BASE_ADDRESS (&sci[0])
#define SOC_XWR68XX_MSS_SCI_B_BASE_ADDRESS (&sci[1])
#define SCIFLR_TXRDY_BIT_START 8U
static void *uart=(void*)1,*data_uart=(void*)2;
static GateMutex_Handle tx_lock=(void*)1,data_tx_lock=(void*)2;
static OrRecordTx record_tx,data_record_tx;
static OrLinkSession link_session;
static OrConfig config;
static uint32_t app_fault,previous_tx_us,deadline_misses,boot_selftest_mask,now;
static uint32_t hardware_initialized=1,dsp_busy,app_state=APP_IDLE;
static int pending[2]={-1,-1},held[2];static FILE *wire[2];
static IArg GateMutex_enter(GateMutex_Handle x){int lane=x==(void*)2;assert(!held[lane]);held[lane]=1;return lane;}
static void GateMutex_leave(GateMutex_Handle x,IArg y){(void)x;assert(held[y]);held[y]=0;}
static uint32_t Clock_getTicks(void){return now;}
static void drain(unsigned lane){if(pending[lane]>=0){fputc(pending[lane],wire[lane]);pending[lane]=-1;}sci[lane].SCIFLR=256U;}
static void Task_sleep(unsigned ticks){now+=ticks;drain(0);drain(1);}
static int32_t UART_write(void *h,uint8_t *s,uint32_t n){
 unsigned lane=h==(void*)2;assert(held[lane]);
 for(uint32_t i=0;i<n;i++){if(i)drain(lane);pending[lane]=s[i];sci[lane].SCIFLR=0U;}
 return (int32_t)n;
}
static int selftest_passed(void){return 1;}
static const char *state_name(void){return "idle";}
static uint32_t or_acquisition_fault(void){return 0;}
static uint32_t or_radio_fault(void){return 0;}
static int or_radio_error(void){return 0;}
static uint32_t or_acquisition_completed(void){return 0;}
'''
post=r'''
int main(int argc,char**argv){
 assert(argc==3);wire[0]=fopen(argv[1],"wb");wire[1]=fopen(argv[2],"wb");assert(wire[0]&&wire[1]);
 or_config_reset(&config);config.fields_seen=15U;config.committed=1U;assert(or_config_valid(&config));
 assert(or_link_commit(&link_session)==0);
 for(unsigned i=0;i<1000;i++){
  char nonce[33],command[64],frame[OR_JSON_MAX+1U];
  RadarDspResult result={0};OrTiming timing={0};os_config spatial;os_default_config(&spatial,60.25e9f);
  snprintf(nonce,sizeof(nonce),"%032x",i+1U);snprintf(command,sizeof(command),"pair %s",nonce);
  assert(pairing_command("pair",command)==0);ack("pair",0);
  snprintf(command,sizeof(command),"pairConfirm %s",nonce);assert(pairing_command("pairConfirm",command)==0);ack("pairConfirm",0);
  status_message();ack("status",0);keepalive_ack(-34);
  result.sequence=i;result.range_bin_m=0.2f;result.velocity_bin_mps=0.3f;timing.link=&link_session;
  size_t size=or_frame_json(frame,sizeof(frame),&config,&spatial,&result,&timing);assert(size>0);
  assert(send_data(frame,size)==0);assert(link_session.session_id==i+1U);
 }
 assert(!record_tx.failed&&!data_record_tx.failed&&!app_fault);drain(0);drain(1);fclose(wire[0]);fclose(wire[1]);return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp);c=d/'harness.c';c.write_text(preamble+adapters+pairing+post)
 compiler=[os.environ.get('HOST_CC','cc')]
 if os.environ.get('HOST_CC_SUBCOMMAND'):compiler.append(os.environ['HOST_CC_SUBCOMMAND'])
 sanitizers=os.environ.get('HOST_SANITIZERS','address,undefined')
 flags=['-fsanitize='+sanitizers] if sanitizers else []
 executable=d/('test.exe' if os.name=='nt' else 'test')
 cmd=compiler+['-std=c99','-O2','-g','-Wall','-Wextra','-Werror','-pedantic']+flags+['-I'+str(root/n) for n in ['src','spatial','dsp']]+[str(c)]+[str(root/n) for n in ['src/record_transport.c','src/link_session.c','src/sensing_config.c','src/json_output.c','spatial/original_spatial.c']]+['-lm','-o',str(executable)]
 subprocess.run(cmd,check=True);subprocess.run([str(executable),str(d/'control.jsonl'),str(d/'data.jsonl')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 control=[json.loads(r) for r in (d/'control.jsonl').read_bytes().splitlines()];data=[json.loads(r) for r in (d/'data.jsonl').read_bytes().splitlines()]
 assert len(control)==5000 and len(data)==2000
 for i in range(1000):
  cs=control[i*5:(i+1)*5];ds=data[i*2:(i+1)*2]
  assert [r['type'] for r in cs]==['ack','ack','status','ack','ack'];assert [r['command'] for r in [cs[0],cs[1],cs[3],cs[4]]]==['pair','pairConfirm','status','keepalive']
  assert cs[4]['code']==-34 and cs[4]['pair_nonce']==f'{i+1:032x}' and cs[4]['session_id']==i+1 and cs[4]['config_generation']==1
  assert [r['type'] for r in ds]==['data_probe','frame']
  assert all(r['channel']=='control' and r['protocol']=='original-radar-v2' for r in cs)
  assert all(r['channel']=='data' and r['protocol']=='original-radar-v2' for r in ds)
  assert cs[2]['paired'] is True and cs[2]['selftest_ok'] is True and cs[2]['fault']==0
  for r in [cs[2]]+ds:
   assert r['pair_nonce']==f'{i+1:032x}' and r['session_id']==i+1 and r['config_generation']==1
  assert ds[1]['sequence']==i
 print(json.dumps({'result':'PASS','model':'two independent pending TD slots','sessions':1000,'control_records':5000,'data_records':2000,'version':control[2]['firmware_version'],'build_id':control[2]['build_id'],'physical_or_rf_tested':False},sort_keys=True))
