// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>
#include <lcom/video_gr.h>

#include <stdint.h>
#include <stdio.h>

extern vbe_mode_info_t vg_mode_info;
extern uint8_t *scancodes;

// Any header files included below this line should have been created by you

#include "kbc.h"
#include "i8042.h"
#include "vbe.h"

int(vg_draw_xpm)(xpm_image_t xpm_info, uint16_t x, uint16_t y);
xpm_image_t(vg_load_xpm)(xpm_map_t map);
extern int scancode_size;
extern int scancode_processed;
extern bool ready;

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab5/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(video_test_init)(uint16_t mode, uint8_t delay) {
  vg_init(mode);

  sleep(delay);
  vg_exit();

  return 1;
}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint32_t color) {
  uint8_t kbc_bit = 0;
  kbc_subscribe_int(&kbc_bit);
  vg_init(mode);
  vg_draw_rectangle(x, y, width, height, color);

  bool run = true;
  int ipc_status, r;
  message msg;

  while (run) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          /* handle kbc interrupt */
          if (msg.m_notify.interrupts & BIT(kbc_bit)) {
            kbc_ih();

            if (scancode_size == 0) { // check with professor
              scancode_processed = true;
              continue;
            }

            if (ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement
              scancode_processed = true;

              // this should be inside this if because we can only safely verify a scan code when it is fully read
              if (scancodes[0] == 0x81) { // esc key was released
                run = false;
                free(scancodes); // next step of ih will not run so we must explicitely free the scancodes here
              }
            }
          }
      }
    }
  }

  kbc_unsubscribe_int();

  vg_exit();

  return 1;
}

int(video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {
  uint8_t kbc_bit = 0;
  kbc_subscribe_int(&kbc_bit);
  vg_init(mode);

  if (vg_draw_pattern(no_rectangles, first, step)) return 1;

  bool run = true;
  int ipc_status, r;
  message msg;

  while (run) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          /* handle kbc interrupt */
          if (msg.m_notify.interrupts & BIT(kbc_bit)) {
            kbc_ih();

            if (scancode_size == 0) { // check with professor
              scancode_processed = true;
              continue;
            }

            if (ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement
              scancode_processed = true;

              // this should be inside this if because we can only safely verify a scan code when it is fully read
              if (scancodes[0] == 0x81) { // esc key was released
                run = false;
                free(scancodes); // next step of ih will not run so we must explicitely free the scancodes here
              }
            }
          }
      }
    }
  }

  kbc_unsubscribe_int();
  vg_exit();

  return 1;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  uint8_t kbc_bit = 0;
// ZAMN
  kbc_subscribe_int(&kbc_bit);

  vg_init(0x105);
  xpm_image_t info = vg_load_xpm(xpm);
  vg_draw_xpm(info, x, y);

  bool run = true;
  int ipc_status, r;
  message msg;

  while (run) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          /* handle kbc interrupt */
          if (msg.m_notify.interrupts & BIT(kbc_bit)) {
            kbc_ih();

            if (scancode_size == 0) { // check with professor
              scancode_processed = true;
              continue;
            }

            if (ready) { // need only to check if we are ready to print the scancode since errors should have been caught by the previous if statement
              scancode_processed = true;

              // this should be inside this if because we can only safely verify a scan code when it is fully read
              if (scancodes[0] == 0x81) { // esc key was released
                run = false;
                free(scancodes); // next step of ih will not run so we must explicitely free the scancodes here
              }
            }
          }
      }
    }
  }

  kbc_unsubscribe_int();
  vg_exit();

  return 1;
}

int(video_test_move)(xpm_map_t xpm, uint16_t xi, uint16_t yi, uint16_t xf, uint16_t yf,
                     int16_t speed, uint8_t fr_rate) {
  if (xi != xf && yi != yf)
    panic("Not a straight line!\n");

  vg_init(0x105);
  bool horiz = yi == yf;
  bool scount = speed < 0;
  speed = abs(speed);
  int count = 0;
  double sleep_time = 1000000/fr_rate;

  while (xi != xf || yi != yf) {
    // vg_draw_xpm(xpm, xi, yi);
    vg_draw_rectangle(xi, yi, 100, 100, 10);
    usleep(sleep_time);
    vg_draw_rectangle(xi, yi, 100, 100, 0);

    if (scount) {
      if (++count % speed == 0)
        horiz ? ++xi : ++yi;
    } else {
      horiz ? (xi += speed) : (yi += speed);
    }
  }

  xi = xf;
  yi = yf;
  // vg_draw_xpm(xpm, xi, yi);
  vg_draw_rectangle(xi, yi, 100, 100, 10);

  sleep(3);
  vg_exit();

  return 1;
}

int(video_test_controller)() {
  /* To be completed */
  printf("%s(): under construction\n", __func__);

  return 1;
}
