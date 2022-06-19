#include <lcom/lcf.h>
#include "i8254.h"

#include <stdint.h>

uint16_t(to_bcd)(uint16_t val) {
  uint16_t res = 0;
  short i = 0;

  while (val != 0) {
    res = res | ((val % 10) << i);
    val /= 10;
    i += 4;
  }

  return res;
}

int(util_get_lsb)(uint16_t val, uint8_t *lsb) {
  *lsb = (uint8_t) (val & TIMER_LSB_MASK);
  return 0;
}

int(util_get_msb)(uint16_t val, uint8_t *msb) {
  *msb = (uint8_t) ((val & TIMER_MSB_MASK) >> 8);
  return 0;
}

int (util_sys_inb)(int port, uint8_t *value) {
  return sys_inb(port, (uint32_t*) value);
}
