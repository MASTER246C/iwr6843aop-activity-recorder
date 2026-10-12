#ifndef ORIGINAL_CAPTURE_MODEL_H
#define ORIGINAL_CAPTURE_MODEL_H
#include <stdint.h>
/* This state machine is called under interrupt exclusion on hardware. */
enum { OR_BUF_FREE=0, OR_BUF_FILLING=1, OR_BUF_READY=2, OR_BUF_PROCESSING=3 };
typedef struct {
    uint32_t generation[2];
    uint32_t sequence[2];
    uint64_t timestamp_ms[2];
    uint64_t elapsed_ms;
    uint32_t last_tick_ms;
    uint32_t completed;
    uint32_t frame_starts;
    uint32_t overruns;
    uint32_t fault;
    uint8_t state[2];
    uint8_t running;
} OrCaptureModel;
void or_capture_model_arm(volatile OrCaptureModel *m,uint32_t now_ms);
int or_capture_model_frame_start(volatile OrCaptureModel *m);
int or_capture_model_complete(volatile OrCaptureModel *m, unsigned slot, uint32_t timestamp);
int or_capture_model_take(volatile OrCaptureModel *m, unsigned *slot, uint32_t *generation);
int or_capture_model_release(volatile OrCaptureModel *m, unsigned slot, uint32_t generation);
void or_capture_model_stop(volatile OrCaptureModel *m);
#endif
