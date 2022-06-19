// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#define VBE_INTERFACE 0x10

#define VBE_SET_MODE 0x4F02

#define LINEAR_MODE BIT(14)

#define VBE_MODE_1024x768_INDEXED 0x105

void getRGB(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b);
uint32_t makeRGB(uint8_t r, uint8_t g, uint8_t b);

int vg_draw_pattern(uint8_t no_rectangles, uint32_t first, uint8_t step);

uint32_t getIndexedColor(uint32_t index, int no_rectangles, int col, int row, int step);
uint32_t getDirectColor (uint32_t color, int no_rectangles, int col, int row, int step);
