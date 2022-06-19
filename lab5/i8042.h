#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#include <lcom/lcf.h>

#define KBC_IRQ 1 /**< @brief KBC IRQ line */

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
#define KBC_ENABLE_INT_MASK BIT(0)
#define KBC_BREAKCODE_BIT BIT(7)

/* Scancode size byte indicators */

#define KBC_SCAN_DOUBLE_BYTE (0xE0)
#define KBC_SCAN_TRIPLE_BYTE (0xE1)

/* Scancode types */

#define KBC_SCANCODE_BREAK 0
#define KBC_SCANCODE_MAKE  1

/* Important keys */

#define ESC_KEY_MAKE_CODE  0x01
#define ESC_KEY_BREAK_CODE 0x81

#endif
