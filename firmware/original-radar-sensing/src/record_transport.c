/* Independently authored record-boundary readiness guard for the TI UART API. */
#include "record_transport.h"
#include <limits.h>
static int wait_ready(const OrRecordTxOps *ops)
{
    uint32_t begin=ops->ticks(ops->context),poll;
    for(poll=0U;poll<OR_TX_READY_MAX_POLLS;++poll) {
        uint32_t elapsed=(uint32_t)(ops->ticks(ops->context)-begin);
        if(elapsed>OR_TX_READY_TIMEOUT_TICKS)return 0;
        if(ops->ready(ops->context))return 1;
        if(elapsed>=OR_TX_READY_TIMEOUT_TICKS)return 0;
        ops->yield_tick(ops->context);
    }
    return 0;
}
int or_record_tx_send(OrRecordTx *state,const OrRecordTxOps *ops,
                      const char *text,size_t length)
{
    int32_t written;
    if(state==NULL)return -1;
    if(state->failed)return -1;
    if(ops==NULL || ops->ready==NULL || ops->ticks==NULL ||
       ops->yield_tick==NULL || ops->write==NULL || text==NULL ||
       length==0U || length>(size_t)INT32_MAX) {
        state->failed=1U;return -1;
    }
    if(!wait_ready(ops)){state->failed=1U;return -1;}
    written=ops->write(ops->context,(const uint8_t *)text,(uint32_t)length);
    if(written!=(int32_t)length){state->failed=1U;return -1;}
    /* UART_write counts bytes queued into TD, not necessarily drained from TD.
     * Keep the caller's lock until the final byte has moved out of TD. */
    if(!wait_ready(ops)){state->failed=1U;return -1;}
    return 0;
}
