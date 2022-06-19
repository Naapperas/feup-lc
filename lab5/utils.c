#include "i8042.h"
#include "../lab2/i8254.h"
#include <lcom/lcf.h>

#include <stdint.h>

#define LSB_MASK 0x000000FF

int counter = 0;

uint8_t (get_LSB)(uint32_t num) {
	return (uint8_t) num & LSB_MASK;
}

int stall() {
  int i = 0;

  while (i < 1000000) {
    i++;
    if (i % 100000 == 0)
      printf("%d", i);
  }

  printf("\n");
  return i;
}

int(util_sys_inb)(int port, uint8_t *value) {
  #ifdef LAB3
			counter++;
	#endif
    
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

int(util_get_lsb)(uint16_t val, uint8_t *lsb) {
  *lsb = (uint8_t) (val & TIMER_LSB_MASK);
  return 0;
}

int(util_get_msb)(uint16_t val, uint8_t *msb) {
  *msb = (uint8_t) ((val & TIMER_MSB_MASK) >> 8);
  return 0;
}
