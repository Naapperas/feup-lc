#include <lcom/lcf.h>
#include <lcom/lab4.h>
#include <lcom/utils.h>

#include <stdbool.h>

#include "i8042.h"
#include "mouse.h"
#include "util.h"

int32_t mouse_global_hook_id;

struct packet mouse_packet;

uint8_t counter = 0;

bool mouse_ready = false;

/* declared in LCF */

struct packet(get_mouse_packet)() {
  return mouse_packet;
}

int(mouse_subscribe_int)(uint8_t *bit_no) {
  int hook_id = MOUSE_HOOK_ID;

  (*bit_no) = (uint8_t) hook_id;

  int res = sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id);

  mouse_global_hook_id = hook_id;

  return res;
}

int(mouse_unsubscribe_int)() {
  return sys_irqrmpolicy((int *) &mouse_global_hook_id);
}

void(mouse_ih)() {
  uint8_t byte = 0;

  bool obf_full = kbc_outbuf_full();

  util_sys_inb(KBC_OUT_BUF_SCAN, &byte);
  if (!obf_full)
    return;

  switch (counter) {
    case 0:
      if (!(byte & MOUSE_FBYTE)) {
        mouse_ready = false;
        return;
      }

      mouse_packet.rb = byte & MOUSE_RB;
      mouse_packet.lb = byte & MOUSE_LB;
      mouse_packet.mb = byte & MOUSE_MB;
      mouse_packet.x_ov = byte & MOUSE_XOVFL;
      mouse_packet.y_ov = byte & MOUSE_YOVFL;

      mouse_packet.delta_x = 0;
      mouse_packet.delta_y = 0;

      break;
    case 1: {
      bool sign = mouse_packet.bytes[0] & MOUSE_XSIGN;

      mouse_packet.delta_x = uint8_to_int16(byte, sign);
      break;
    }
    case 2: {
      bool sign = mouse_packet.bytes[0] & MOUSE_YSIGN;

      mouse_packet.delta_y = uint8_to_int16(byte, sign);
      break;
    }
  }
  mouse_packet.bytes[counter] = byte;
  counter = (counter + 1) % (sizeof(mouse_packet.bytes));

  mouse_ready = counter == 0; // when we catch a 0 at this point, we know we have successfully parsed 3 bytes
}

void(kbc_enable_data_report)() {
  wait_for_inbuff_empty();
  write_command_to_mouse(ENABLE_DATA_REPORTING_CMD);
}

bool(kbc_outbuf_full)() {
  uint8_t st_reg = 0;
  util_sys_inb(KBC_OUT_BUF_STATUS, &st_reg);

  return (st_reg & (KBC_STATUS_OUTBUF_FULL | BIT(5)));
}

int16_t(uint8_to_int16)(uint8_t uint8_val, bool sign) {
  return ((sign ? 0xFF : 0x00) << 8) | uint8_val;
}

void(reset_kbc)() {
  write_command_to_mouse(DISABLE_DATA_REPORTING_CMD);
  return;
}

void(wait_for_inbuff_empty)() {
  uint8_t status_reg;

  do {
    tickdelay(micros_to_ticks(20000));
    status_reg = 0;
    util_sys_inb(KBC_OUT_BUF_STATUS, &status_reg);
  } while (status_reg & KBC_STATUS_INBUF_FULL);

  return;
}

uint8_t(mouse_command_feedback)() {
  uint8_t byte = 0;

  util_sys_inb(KBC_OUT_BUF_SCAN, &byte);

  return byte;
}

void(write_command_to_mouse)(uint8_t command) {
  uint8_t feedback = 0;

  uint8_t temp = 0;

  do {
    wait_for_inbuff_empty();

    sys_outb(KBC_IN_BUF_CMD, WRITE_BYTE_TO_MOUSE);

    wait_for_inbuff_empty();

    sys_outb(KBC_IN_BUF_DATA, command);

    tickdelay(micros_to_ticks(25000));

    feedback = mouse_command_feedback();

    temp++;
  } while (feedback != MOUSE_ACK && feedback != MOUSE_ERROR && temp < 2); // technically should never run more than twice

  if (feedback != MOUSE_ACK) {
    fprintf(stderr, "%s failed while trying to execute command %x\n", __func__, command);
  }
}

mouse_event_t mouse_event_from_data_packet(const struct packet *data_packet, const mouse_event_t *old) {
  mouse_event_t new;
  new.moveX = data_packet->delta_x;
  new.moveY = data_packet->delta_y;
  new.lbdown = data_packet->lb;
  new.rbdown = data_packet->rb;
  new.mbdown = data_packet->mb;

  if (old == 0) {
    if (new.moveX != 0 || new.moveY != 0)
      new.type = MOVE;
    else if (new.lbdown)
      new.type = LBDOWN;
    else if (new.rbdown)
      new.type = RBDOWN;
    else if (new.mbdown)
      new.type = MBDOWN;
    else
      new.type = STQUO;

    printf("event type = %d\n", new.type);
    return new;
  }

  if (new.moveX != 0 || new.moveY != 0)
    new.type = MOVE;
  else if (!old->lbdown && new.lbdown)
    new.type = LBDOWN;
  else if (!old->rbdown && new.rbdown)
    new.type = RBDOWN;
  else if (!old->mbdown && new.mbdown)
    new.type = MBDOWN;
  else if (old->lbdown && !new.lbdown)
    new.type = LBUP;
  else if (old->rbdown && !new.rbdown)
    new.type = RBUP;
  else if (old->mbdown && !new.mbdown)
    new.type = MBUP;
  else
    new.type = STQUO;

  printf("event type = %d\n", new.type);
  return new;
}

bool only_lb_pressed(const mouse_event_t evt) {
  return evt.lbdown && !evt.rbdown && !evt.mbdown;
}

bool only_rb_pressed(const mouse_event_t evt) {
  return !evt.lbdown && evt.rbdown && !evt.mbdown;
}

bool no_buttons_pressed(const mouse_event_t evt) {
  return !evt.lbdown && !evt.rbdown && !evt.mbdown;
}

bool valid_slope(const mouse_event_t evt) {
  return float_abs(evt.moveY / evt.moveY) >= MIN_SLOPE;
}

/**
 * @brief checks if the event continues a positive slope left to right diagonal movement with the lb pressed
 */
bool ltr_up_and_lb(const mouse_event_t evt, const uint8_t tolerance) {
  return evt.moveX >= -tolerance && // x is positive
         evt.moveY > -tolerance &&  // y is positive
         only_lb_pressed(evt) &&
         valid_slope(evt);
}

/**
 * @brief checks if the event performs a valid finisher to the first line of the pattern,
 * in specific, moves diagonally left to right with a positive slope and no buttons pressed
 */
bool ltr_up_and_release_lb(const mouse_event_t evt, const uint8_t x_len, const uint8_t tolerance, const uint8_t lineXLen) {
  return evt.moveX >= -tolerance && // x is positive
         evt.moveY > -tolerance &&  // y is positive
         no_buttons_pressed(evt) &&
         valid_slope(evt) &&
         lineXLen + evt.moveX > x_len;
}

/**
 * @brief checks if the event performs a valid finisher to the first line of the pattern,
 * in specific, moves diagonally left to right with a negative slope and the right button pressed
 */
bool finisher_ltr_down_and_rb(const mouse_event_t evt, const uint8_t x_len, const uint8_t tolerance, const uint8_t lineXLen) {
  return evt.moveX >= -tolerance && // positive x
         evt.moveY < tolerance &&   // negative y
         only_rb_pressed(evt) &&
         valid_slope(evt) &&
         lineXLen >= x_len;
}

/**
 * @brief checks if the event continues a negative slope left to right diagonal movement with the rb pressed
 */
bool ltr_down_and_rb(const mouse_event_t evt, const uint8_t tolerance) {
  return evt.moveX >= -tolerance && // x is positive
         evt.moveY < tolerance &&   // y is negative
         only_rb_pressed(evt) &&
         valid_slope(evt);
}

/**
 * @brief checks if the event finishes the second line of the pattern with a negative slope left to right diagonal movement and no buttons pressed
 */
bool finisher_ltr_down_and_release_rb(const mouse_event_t evt, const uint8_t tolerance, const uint8_t lineXLen, uint8_t x_len) {
  return evt.moveX >= -tolerance && // positive x
         evt.moveY < tolerance &&   // negative y
         no_buttons_pressed(evt) &&
         valid_slope(evt) &&
         lineXLen + evt.moveX >= x_len;
}

bool mouse_check_pattern(mouse_event_t evt, uint8_t x_len, uint8_t tolerance) {
  static state_t pattern_state = INIT; // initial state; keep state
  static int lineXLen = 0;             // line movement in X
  switch (pattern_state) {

    case INIT:
      if ((evt.type == LBDOWN && only_lb_pressed(evt))) {
        lineXLen = 0;
        pattern_state = DRAW_UP;
        printf("Got into DRAW_UP from INIT\n");
      } // else we stay on INIT
      else if (evt.type == MOVE && ltr_up_and_lb(evt, tolerance)) {
        lineXLen = max(evt.moveX, 0); // in case x was a negative movement within tolerance
        pattern_state = DRAW_UP;
        printf("Got into DRAW_UP from INIT\n");
      }
      break;

    case DRAW_UP:
      if (evt.type == MOVE) {
        if (ltr_up_and_lb(evt, tolerance)) {
          lineXLen += evt.moveX;
        }
        else if (ltr_up_and_release_lb(evt, x_len, tolerance, lineXLen)) {
          pattern_state = DRAW_DOWN;
          printf("Got into DRAW_DOWN from DRAW_UP\n");
          lineXLen = 0;
        }
        else if (finisher_ltr_down_and_rb(evt, x_len, tolerance, lineXLen)) {
          pattern_state = DRAW_DOWN;
          printf("Got into DRAW_DOWN from DRAW_UP\n");
          lineXLen = max(evt.moveX, 0); // in case x was a negative value within tolerance
        }
        else {
          pattern_state = INIT;
          printf("Got into INIT from DRAW_UP because it was a move but not recognized\n");
        }
      }
      else if (evt.type == RBDOWN) { // change the line draw
        if (only_rb_pressed(evt) && lineXLen >= x_len) {
          lineXLen = max(evt.moveX, 0);
          pattern_state = DRAW_DOWN;
          printf("Got into DRAW_DOWN from DRAW_UP\n");
        }
        else { // does not comply, right click with other buttons or length of UP too short
          pattern_state = INIT;
          printf("Got into INIT from DRAW_UP because lineXLen was %d when x_len is %d\n", lineXLen, x_len);
        }
      }

      else if (evt.type == STQUO || (evt.type == LBUP && no_buttons_pressed(evt))) {
        if (lineXLen >= x_len) {
          pattern_state = DRAW_DOWN;
          printf("Got into DRAW_DOWN from DRAW_UP\n");
          lineXLen = 0;
        }
        else {
          pattern_state = INIT;
          printf("Got into INIT from DRAW_UP because lineXLen was %d when x_len is %d\n", lineXLen, x_len);
        }
      }

      else { // other event type goes to restart
        pattern_state = INIT;
        printf("Got into INIT from DRAW_UP because of unrecognized event type\n");
      }
      break; // from case DRAW_UP

    case DRAW_DOWN:

      if (evt.type == MOVE) {
        if (ltr_down_and_rb(evt, tolerance)) {
          lineXLen += evt.moveX;
        }
        else if (finisher_ltr_down_and_release_rb(evt, tolerance, lineXLen, x_len)) {
          pattern_state = DETECTED;
          printf("Got into DETECTED\n");
          return true;
        }
        else if (ltr_up_and_lb(evt, tolerance)) {
          pattern_state = DRAW_UP;
          printf("Got into DRAW_UP from DRAW_DOWN\n");
          lineXLen = max(evt.moveX, 0); // in case x was a negative value within tolerance
        }
        else if (no_buttons_pressed(evt) && abs(evt.moveX) <= tolerance && abs(evt.moveY) <= tolerance) {} //residual, do nothing
        else {
          pattern_state = INIT;
          printf("Got into INIT from DRAW_DOWN\n");
        }
      }
      else if (evt.type == RBDOWN && only_rb_pressed(evt)) {
        //do nothing
      }
      else if ((evt.type == RBUP && no_buttons_pressed(evt)) || evt.type == STQUO) {
        if (lineXLen >= x_len) {
          pattern_state = DETECTED;
          return true;
        }
        else {
          pattern_state = INIT;
          printf("Got into INIT from DRAW_DOWN\n");
        }
      }
      else {
        pattern_state = INIT;
        printf("Got into INIT from DRAW_DOWN\n");
      }
      break;
    case DETECTED:
      printf("is this ever called\n");
      return true;
  }
  return false;
}
