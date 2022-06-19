#ifndef __KBC_H
#define __KBC_H

#include <stdint.h>

#define  DELAY_US 20000

int(kbc_subscribe_int)(uint8_t *bit_no);

int(kbc_unsubscribe_int)();

int (kbc_outbuf_full)();

void(kbc_enable_int)();



#endif
