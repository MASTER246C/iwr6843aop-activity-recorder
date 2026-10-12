#include "capture_model.h"
#include <string.h>
void or_capture_model_arm(volatile OrCaptureModel *m,uint32_t now_ms)
{
    uint32_t g0=m->generation[0],g1=m->generation[1];
    m->generation[0]=g0+1U;m->generation[1]=g1+1U;
    m->sequence[0]=0U;m->sequence[1]=0U;m->timestamp_ms[0]=0U;m->timestamp_ms[1]=0U;
    m->elapsed_ms=0U;m->last_tick_ms=now_ms;
    m->completed=0U;m->frame_starts=0U;m->overruns=0U;m->fault=0U;
    m->state[0]=OR_BUF_FILLING;m->state[1]=OR_BUF_FREE;m->running=1U;
}
int or_capture_model_frame_start(volatile OrCaptureModel *m)
{
    if(!m->running)return 0;
    /* A previous partial capture must never roll into the next RF frame. */
    if(m->completed!=m->frame_starts){m->fault=3U;m->running=0U;return -1;}
    ++m->frame_starts;return 0;
}
int or_capture_model_complete(volatile OrCaptureModel *m,unsigned slot,uint32_t timestamp)
{
    unsigned next;
    if(!m->running || slot>1U || m->state[slot]!=OR_BUF_FILLING || m->frame_starts!=m->completed+1U) {m->fault=1U;return -1;}
    /* Completion callbacks arrive every 200ms in a healthy run. The unsigned
     * delta extends the 32-bit 1ms clock across rollover without truncating us.
     * Hardware callers read/write these 64-bit fields under interrupt exclusion. */
    m->elapsed_ms+=(uint32_t)(timestamp-m->last_tick_ms);m->last_tick_ms=timestamp;
    m->sequence[slot]=m->completed++;m->timestamp_ms[slot]=m->elapsed_ms;
    m->state[slot]=OR_BUF_READY; next=slot^1U;
    if(m->state[next]!=OR_BUF_FREE) {++m->overruns;m->fault=2U;m->running=0U;return -1;}
    m->state[next]=OR_BUF_FILLING; ++m->generation[next]; return 0;
}
int or_capture_model_take(volatile OrCaptureModel *m,unsigned *slot,uint32_t *generation)
{
    unsigned i;
    if(!m->running || m->fault) return 0;
    for(i=0U;i<2U;++i) if(m->state[i]==OR_BUF_READY) {
        m->state[i]=OR_BUF_PROCESSING; *slot=i; *generation=m->generation[i]; return 1;
    }
    return 0;
}
int or_capture_model_release(volatile OrCaptureModel *m,unsigned slot,uint32_t generation)
{
    if(slot>1U || !m->running || m->fault || m->state[slot]!=OR_BUF_PROCESSING ||
       generation!=m->generation[slot]) return -1;
    m->state[slot]=OR_BUF_FREE; return 0;
}
void or_capture_model_stop(volatile OrCaptureModel *m)
{
    m->running=0U; ++m->generation[0];++m->generation[1];
    m->state[0]=OR_BUF_FREE;m->state[1]=OR_BUF_FREE;
}
