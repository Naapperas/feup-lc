// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab4.h>

#include <stdint.h>
#include <stdio.h>

// Any header files included below this line should have been created by you

#include "mouse.h"

extern bool mouse_ready;

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need/ it]
  lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab4/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(mouse_test_packet)(uint32_t cnt) {
  uint8_t bit;
  kbc_enable_data_report();
  mouse_subscribe_int(&bit);

  int ipc_status, r;
  message msg;

  while (cnt) {
    /* Get a request message. */
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    
    
    if (is_ipc_notify(ipc_status) && // found interrupt
        _ENDPOINT_P(msg.m_source) == HARDWARE &&
        msg.m_notify.interrupts & BIT(bit)) {
      mouse_ih();

      if (mouse_ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement

        struct packet data_packet = get_mouse_packet();

        mouse_print_packet(&data_packet);
        --cnt;
      }
    }
  }

  mouse_unsubscribe_int();
  reset_kbc();

  return 1;
}

int(mouse_test_async)(uint8_t idle_time) {

  uint8_t timer_bit, mouse_bit;
  timer_subscribe_int(&timer_bit);
  kbc_enable_data_report();
  mouse_subscribe_int(&mouse_bit);

  int ipc_status, r;
  message msg;

  bool cnt = true;
  extern int timer_counter;

  while(cnt) { 
      /* Get a request message. */
      if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) { 
          printf("driver_receive failed with: %d", r);
          continue;
      }
      if (is_ipc_notify(ipc_status)) { 
          switch (_ENDPOINT_P(msg.m_source)) {
              case HARDWARE: 			
                  if (msg.m_notify.interrupts & BIT(mouse_bit)) {
                    mouse_ih();
                    timer_counter = 0;
                    if (mouse_ready) {
                      struct packet data_packet = get_mouse_packet();
                      mouse_print_packet(&data_packet);
                    }
                  }
                  else if (msg.m_notify.interrupts & BIT(timer_bit)) {
                    timer_int_handler();
                    if (timer_counter / sys_hz() >= idle_time) {
                      cnt = false;
                    }
                  }
                  break;
              default:
                  break; 	
          }
      } else { /* received a standard message, not a notification */
          /* no standard messages expected: do nothing */
      }
  }

  mouse_unsubscribe_int();
  reset_kbc();
  timer_unsubscribe_int();

  return 1;
}

int(mouse_test_gesture)(uint8_t x_len, uint8_t tolerance) {
  uint8_t bit;
  kbc_enable_data_report();
  mouse_subscribe_int(&bit);
  
  bool event = false;
  mouse_event_t mouse_event = {7, 0, 0, false, false, false};

  int ipc_status, r;
  message msg;

  while (1) {
    /* Get a request message. */
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    
    
    if (is_ipc_notify(ipc_status) && // found interrupt
        _ENDPOINT_P(msg.m_source) == HARDWARE &&
        msg.m_notify.interrupts & BIT(bit)) {
      mouse_ih();

      if (mouse_ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement
        struct packet data_packet = get_mouse_packet();
        mouse_print_packet(&data_packet);

        mouse_event = mouse_event_from_data_packet(&data_packet, &mouse_event);
        event = true;
      }
    }
    if (event) {
      if (mouse_check_pattern(mouse_event, x_len, tolerance))
        break;
      event = false;
    }
  }

  mouse_unsubscribe_int();
  reset_kbc();

  return 1;
}

int(mouse_test_remote)(uint16_t period, uint8_t cnt) {
  /* This year you need not implement this. */
  printf("%s(%u, %u): under construction\n", __func__, period, cnt);
  return 1;
}
