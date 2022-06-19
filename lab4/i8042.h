#include <lcom/lcf.h>

/* KBC registers */

#define KBC_IN_BUF_DATA     0x60 
#define KBC_IN_BUF_CMD      0x64 
#define KBC_OUT_BUF_SCAN    0x60 
#define KBC_OUT_BUF_STATUS  0x64

/* KBC useful masks */

#define KBC_STATUS_OK_MASK      (BIT(7) | BIT(6))
#define KBC_STATUS_INBUF_FULL   (BIT(1))
#define KBC_STATUS_OUTBUF_FULL  (BIT(0))
#define KBC_SCANCODE_TYPE_MASK  (BIT(7))
#define KBC_READ_CMD_MASK 0x20
#define KBC_WRITE_CMD_MASK 0x60
