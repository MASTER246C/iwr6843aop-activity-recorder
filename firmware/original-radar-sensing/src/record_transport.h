#ifndef ORIGINAL_RECORD_TRANSPORT_H
#define ORIGINAL_RECORD_TRANSPORT_H
#include <stddef.h>
#include <stdint.h>
/* Caller must serialize the entire call. Tick interval is 1 ms in sensing.cfg.
 * Driver write retains its own 200 ms timeout. No RF or hardware types here. */
#define OR_TX_READY_TIMEOUT_TICKS 5U
#define OR_TX_READY_MAX_POLLS 6U
typedef struct {
    void *context;
    int (*ready)(void *context);
    uint32_t (*ticks)(void *context);
    void (*yield_tick)(void *context);
    int32_t (*write)(void *context,const uint8_t *text,uint32_t length);
} OrRecordTxOps;
typedef struct { volatile uint8_t failed; } OrRecordTx;
/* Failure is latched until reboot/reinitialization. Never retry a partial record. */
int or_record_tx_send(OrRecordTx *state,const OrRecordTxOps *ops,
                      const char *text,size_t length);
#endif
