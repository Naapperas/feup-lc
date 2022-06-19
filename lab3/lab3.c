#include <lcom/lcf.h>

#include <lcom/lab3.h>

#include <stdbool.h>
#include <stdint.h>

#include "i8042.h"
#include "kbc.h"
#include "lcom/timer.h"

extern int scancode_size;
extern int scancode_type;

extern uint32_t timer_global_hook_id;
extern uint32_t kbc_global_hook_id;

extern uint8_t scancode;
extern uint8_t st_reg;
extern uint8_t *scancodes;

extern int timer_counter;

int32_t used_ids = 0;

extern bool ready, scancode_processed;

// #define LAB3

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab3/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(kbd_test_scan)() {
  uint8_t bit;
  kbc_subscribe_int(&bit);

  int ipc_status, r;
  message msg;

  bool run = true;

  while (run) {
    /* Get a request message. */
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status) && // found interrupt
        _ENDPOINT_P(msg.m_source) == HARDWARE &&
        msg.m_notify.interrupts & BIT(bit)) {

      kbc_ih();
      
      if (scancode_size == 0) { //check with professor
        scancode_processed = true;
        continue;
      }

      if (ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement
        kbd_print_scancode(scancode_type, scancode_size, scancodes);
        scancode_processed = true;
        printf("In decimal: %d\n", scancodes[0]);

        // this should be inside this if because we can only safely verify a scan code when it is fully read
        if (scancodes[0] == ESC_KEY_BREAK_CODE) { // esc key was released
          run = false;
          free(scancodes); // next step of ih will not run so we must explicitely free the scancodes here
        }   
      }      
    }
  }

  kbc_unsubscribe_int();

  return 1;
}

int(kbd_test_poll)() {
  bool run = true;

  while (run) {
    if (kbc_outbuf_full()) {
      kbc_ih();

      if (scancode_size == 0) {
        scancode_processed = true;
        continue;
      }

      if (ready) {
        kbd_print_scancode(scancode_type, scancode_size, scancodes);
        scancode_processed = true;

        if (scancodes[0] == ESC_KEY_BREAK_CODE) { // esc key was released
          run = false;
          free(scancodes);
        }   
      }
    }

    tickdelay(micros_to_ticks(DELAY_US));
  }

  kbc_enable_int();
  return 1;
}

int(kbd_test_timed_scan)(uint8_t n) {
  uint8_t kbc_bit, timer_bit;
  
  kbc_subscribe_int(&kbc_bit);
  timer_subscribe_int(&timer_bit);

  int ipc_status, r;
  message msg;

  bool run = true;
  timer_counter = 0;

  while (run) {
    /* Get a request message. */
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status) && // found interrupt
        _ENDPOINT_P(msg.m_source) == HARDWARE) {

      if (msg.m_notify.interrupts & BIT(kbc_bit)) {
        kbc_ih();

        if (scancode_size == 0) {
          scancode_processed = true;
          continue;
        }

        if (ready) {
          kbd_print_scancode(scancode_type, scancode_size, scancodes);
          scancode_processed = true;

          if (scancodes[0] == ESC_KEY_BREAK_CODE) { // esc key was released
            run = false;
            free(scancodes);
          }   
        }

        timer_counter = 0;
      }

      if (msg.m_notify.interrupts & BIT(timer_bit)) {
        timer_int_handler(); // TODO: change to be consistent with keyboard
        
        if (timer_counter >= (n * 60)) 
          run = false;
      }
    }
  }

  timer_unsubscribe_int();
  kbc_unsubscribe_int();

  return 1;
}
