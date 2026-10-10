#ifndef ORIGINAL_SERIAL_LINE_H
#define ORIGINAL_SERIAL_LINE_H
#include <stdint.h>
#include <stddef.h>
#include "sensing_config.h"
typedef struct {char text[OR_LINE_SIZE];size_t used;uint8_t discard;} OrSerialLine;
/* 1 complete printable line, -1 discarded line, 0 not complete. LF consumes/reset. */
int or_serial_feed(OrSerialLine *line,uint8_t byte);
#endif
