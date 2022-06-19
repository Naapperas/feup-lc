#include <lcom/lcf.h>
#include <lcom/utils.h>
#include "i8042.h"
#include "../lab2/i8254.h"

#define LSB_MASK 0xFF

uint8_t (get_LSB)(uint32_t num) {
	return (uint8_t) num & LSB_MASK;
}

int(util_sys_inb)(int port, uint8_t *value) {
	uint32_t temp = 0;
	int ret = sys_inb(port, &temp);
	*value = get_LSB(temp);
	return ret; 
}

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

float float_abs(const float num) {
  if (num < 0.0) return -num;
  else return num;
}

bool max(const int a, const int b) {
  if (a >= b) return a;
  return b;
}
