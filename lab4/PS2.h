#include <lcom/lcf.h>

#define MOUSE_IRQ 12
#define MOUSE_HOOK_ID 16

/* First packet byte structure*/

#define MOUSE_LB BIT(0)
#define MOUSE_RB BIT(1)
#define MOUSE_MB BIT(2)
#define MOUSE_FBYTE BIT(3)
#define MOUSE_XSIGN BIT(4)
#define MOUSE_YSIGN BIT(5)
#define MOUSE_XOVFL BIT(6)
#define MOUSE_YOVFL BIT(7)


/* Check if number is positive or negative */

#define NEG BIT(7)


/* Shift amounts */

#define MOUSE_XSIGN_SHIFT 4


/* mouse controller acknowledgment bytes */

#define MOUSE_ACK   0xFA
#define MOUSE_NACK  0xFE
#define MOUSE_ERROR 0xFC

#define WRITE_BYTE_TO_MOUSE 0xD4
