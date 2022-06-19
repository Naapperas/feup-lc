#include <lcom/lcf.h>

#include <stdbool.h>

#include "PS2.h"
#include "i8042.h"

#define DISABLE_DATA_REPORTING_CMD 0xF5
#define ENABLE_DATA_REPORTING_CMD 0xF4
#define SET_STREAMMODE_CMD 0XEA

#define MIN_SLOPE 1.0

typedef enum { INIT, DRAW_UP, DRAW_DOWN, DETECTED } state_t;
typedef enum { 
  RBDOWN, //0
  RBUP,  //1
  LBDOWN, //2
  LBUP, //3
  MBDOWN, //4
  MBUP, //5
  MOVE, //6
  STQUO //7
  } event_type_t;

typedef struct {
  event_type_t type;
  int16_t moveX;
  int16_t moveY;
  bool lbdown, rbdown, mbdown;
} mouse_event_t;

int (mouse_subscribe_int)(uint8_t *bit_no);
int (mouse_unsubscribe_int)();

bool (kbc_outbuf_full)();
struct packet (get_mouse_packet)();
int16_t (uint8_to_int16) (uint8_t uint8_val, bool sign);
void (reset_kbc)();

void (wait_for_inbuff_empty)();
void (write_command_to_mouse) (uint8_t command);
void (kbc_enable_data_report)();
uint8_t (mouse_command_feedback)();

bool mouse_check_pattern(mouse_event_t evt, uint8_t x_len, uint8_t tolerance);
mouse_event_t mouse_event_from_data_packet(const struct packet* const data_packet, const mouse_event_t* const old);
