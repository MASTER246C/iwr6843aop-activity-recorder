#include "record_transport.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef struct {
    char wire[2048];unsigned used,writes,yields,ready_calls;
    uint32_t now,delay;
    int pending,stuck,frozen,short_write,post_stuck;
    uint32_t tick_step;
    uint8_t tail;
} Model;
static int ready(void *context)
{
    Model *m=context;m->ready_calls++;
    if(m->stuck)return 0;
    if(m->pending && m->delay==0U){m->wire[m->used++]=(char)m->tail;m->pending=0;}
    return !m->pending;
}
static uint32_t ticks(void *context){return ((Model*)context)->now;}
static void yield_tick(void *context)
{
    Model*m=context;m->yields++;if(!m->frozen)m->now+=m->tick_step?m->tick_step:1U;
    if(m->delay)m->delay--;
}
static int32_t write_bytes(void *context,const uint8_t *text,uint32_t length)
{
    Model*m=context;uint32_t i;m->writes++;
    /* Model SDK's blind first-byte load overwriting a still-occupied TD. */
    if(m->pending)m->pending=0;
    if(m->short_write)return m->short_write<0?-1:(int32_t)length-1;
    for(i=0;i+1<length;i++)m->wire[m->used++]=(char)text[i];
    m->tail=text[length-1U];m->pending=1;m->delay=1U;if(m->post_stuck)m->stuck=1;
    return (int32_t)length;
}
static OrRecordTxOps ops(Model*m)
{
    OrRecordTxOps o={m,ready,ticks,yield_tick,write_bytes};return o;
}
int main(void)
{
    const char status[]="{\"type\":\"status\",\"selftest_ok\":true}\n";
    const char ack[]="{\"type\":\"ack\",\"command\":\"status\",\"code\":0}\n";
    Model m={0};OrRecordTx tx={0};OrRecordTxOps o=ops(&m);unsigned i;
    /* Control demonstrates the missing-LF mechanism before protection. */
    write_bytes(&m,(const uint8_t*)status,sizeof(status)-1U);
    write_bytes(&m,(const uint8_t*)ack,sizeof(ack)-1U);m.delay=0;assert(ready(&m));
    assert(strstr(m.wire,"}{")!=NULL);
    memset(&m,0,sizeof(m));
    for(i=0;i<10;i++) {
        assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)==0);
        assert(or_record_tx_send(&tx,&o,ack,sizeof(ack)-1U)==0);
    }
    assert(m.writes==20U);assert(m.used==10U*(sizeof(status)+sizeof(ack)-2U));
    for(i=0;i<10;i++) {
        unsigned offset=i*(sizeof(status)+sizeof(ack)-2U);
        assert(memcmp(m.wire+offset,status,sizeof(status)-1U)==0);
        assert(memcmp(m.wire+offset+sizeof(status)-1U,ack,sizeof(ack)-1U)==0);
    }
    /* Pre-existing final byte must drain before the first record write. */
    memset(&m,0,sizeof(m));tx.failed=0;m.pending=1;m.tail='!';m.delay=2;
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)==0);assert(m.wire[0]=='!');
    /* Bounded pre-write failure; later readiness cannot silently retry. */
    memset(&m,0,sizeof(m));tx.failed=0;m.stuck=1;
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)<0);
    assert(tx.failed && m.writes==0 && m.yields<=OR_TX_READY_MAX_POLLS);
    m.stuck=0;assert(or_record_tx_send(&tx,&o,ack,sizeof(ack)-1U)<0);assert(m.writes==0);
    /* Post-write timeout leaves its pending tail untouched and blocks ACK/retry. */
    memset(&m,0,sizeof(m));tx.failed=0;m.post_stuck=1;
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)<0);
    assert(tx.failed && m.writes==1 && m.pending);
    m.stuck=0;m.post_stuck=0;
    assert(or_record_tx_send(&tx,&o,ack,sizeof(ack)-1U)<0);assert(m.writes==1);
    /* A scheduled wake after deadline also fails closed, even if TD is ready. */
    memset(&m,0,sizeof(m));tx.failed=0;m.tick_step=50;
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)<0);assert(tx.failed && m.writes==1);
    /* Single-byte records still wait for their pending byte. */
    memset(&m,0,sizeof(m));tx.failed=0;
    assert(or_record_tx_send(&tx,&o,"\n",1U)==0);assert(m.used==1 && m.wire[0]=='\n');
    /* Tick wrap is valid; frozen ticks still have a finite poll bound. */
    memset(&m,0,sizeof(m));tx.failed=0;m.now=UINT32_MAX;m.pending=1;m.delay=2;m.tail='!';
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)==0);
    memset(&m,0,sizeof(m));tx.failed=0;m.frozen=1;m.stuck=1;
    assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)<0);assert(m.yields==OR_TX_READY_MAX_POLLS);
    /* Negative/short writes latch failure; callbacks are not retried. */
    for(i=0;i<2;i++) {
        memset(&m,0,sizeof(m));tx.failed=0;m.short_write=i?1:-1;
        assert(or_record_tx_send(&tx,&o,status,sizeof(status)-1U)<0);
        assert(m.writes==1 && tx.failed);assert(or_record_tx_send(&tx,&o,ack,sizeof(ack)-1U)<0);assert(m.writes==1);
    }
    memset(&m,0,sizeof(m));tx.failed=0;
    assert(or_record_tx_send(&tx,&o,NULL,1)<0 && tx.failed);
    puts("PASS: record boundary readiness, pending-LF model, bounded timeout, wrap/frozen ticks, no retry after failure.");
    return 0;
}
