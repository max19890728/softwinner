/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 *
 * file: us363_touch_lib.c
 ******************************************************************************/

#include "us363_touch_lib.h"

#ifdef _MGIAL_US363_TOUCH_LIB

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tslib.h>

static struct tsdev* touch_screen = NULL;
static int point_x = 0;
static int point_y = 0;
static int pressure = 0;

static int pressued_key = -2;

static int fd_touch_screen = -1;
static int fd_power_key = -1;

static unsigned char state[NR_KEYS];

/************************ Low Level Input Operations **************************/

/**
 * Touch Screen operations -- Event
 **/
static int update_touch_point(void) {
  struct ts_sample sample;
  if (!touch_screen) {
    goto error;
  }
  if (ts_read(touch_screen, &sample, 1) > 0) {
    if (sample.pressure > 0) {
      point_x = sample.x;
      point_y = sample.y;
      pressure = IAL_MOUSE_LEFTBUTTON;
    } else {
      pressure = 0;
    }
  } else {
    goto error;
  }
  return TRUE;

error:
  return FALSE;
}

static void get_touch_xy(int* x, int* y) {
  *x = point_x;
  *y = point_y;
}

static int get_touch_status(void) { return pressure; }

static int key_update(void) {
  if (pressued_key < 0) {
    memset(state, 0, sizeof(state));
    pressued_key = -2;
  } else {
    state[pressued_key] = 1;
  }
  return NR_KEYS;
}

static const char* get_key_state(void) { return &state[0]; }

static int wait_event(int which, int maxfd, fd_set* in, fd_set* out,
                      fd_set* except, struct timeval* timeout) {
  fd_set read_fd_set;
  int result = 0;
  int select_result = 0;
  if (!in) {
    in = &read_fd_set;
    FD_ZERO(in);
  }

  // - Check event
  if ((which & IAL_KEYEVENT) && (fd_power_key > 0)) {
    FD_SET(fd_power_key, in);
    if (fd_power_key > maxfd) maxfd = fd_power_key;
  }
  if ((which & IAL_MOUSEEVENT) && (fd_touch_screen >= 0)) {
    FD_SET(fd_touch_screen, in);
    if (fd_touch_screen > maxfd) maxfd = fd_touch_screen;
  }

  // - Read event
  select_result = select(maxfd + 1, in, out, except, timeout);
  if (select_result < 0) {
    fprintf(stderr, "[%s:%d] Select error!\n", __FILE__, __LINE__);
    return -1;
  } else if (select_result > 0) {
    struct input_event event;

    // - Read power key event
    if (fd_power_key > 0 && FD_ISSET(fd_power_key, in)) {
      FD_CLR(fd_power_key, in);
      read(fd_power_key, &event, sizeof(event));
      if (!!event.code) {
        pressued_key = event.value > 0 ? event.code : -1;
        result |= IAL_KEYEVENT;
      }
    }

    // - Read touch event
    if (fd_touch_screen > 0 && FD_ISSET(fd_touch_screen, in)) {
      FD_CLR(fd_touch_screen, in);
      read(fd_touch_screen, &event, sizeof(event));
      if (!!event.code) {
        switch (event.type) {
          case EV_KEY:
            pressued_key = event.value > 0 ? event.code : -1;
            result |= IAL_KEYEVENT;
            break;
          case EV_ABS:
            result |= IAL_MOUSEEVENT;
            break;
        }
      }
    }
  } else {
    // fprintf(stderr, "[%s:%d] Select timeout!\n", __FILE__, __LINE__);
  }
  return result;
}

/******************************************************************************/

BOOL InitUS363TouchLibInput(INPUT* input, const char* mdev, const char* mtype) {
  // - Open the event of power key
  fd_power_key = open(input->mdev0, O_RDONLY);
  if (fd_power_key < 0) {
    fprintf(stderr, "Power Key input node open failed.\n");
  }

  // - Open the event of touch
  touch_screen = ts_open(input->mdev2, TRUE);
  if (touch_screen) {
    fd_touch_screen = ts_fd(touch_screen);
    if (fd_touch_screen < 0) {
      fprintf(stderr, "Get touch screen file descriptor failed\n");
    }
    if (!ts_config(touch_screen)) {
      fprintf(stderr, "Config touch screen failed!\n");
    }
  }
  if (!touch_screen || (fd_touch_screen < 0)) {
    fprintf(stderr, "Touch Screen input node open failed\n");
  }
  // - Check status
  if (!touch_screen && (fd_power_key < 0)) {
    fprintf(stderr, "All input nodes init Failed!\n");
    return FALSE;
  }

  // - Set function for touch screen
  input->update_mouse = update_touch_point;
  input->get_mouse_xy = get_touch_xy;
  input->set_mouse_xy = NULL;
  input->get_mouse_button = get_touch_status;
  input->set_mouse_range = NULL;

  // - Set function for key
  input->update_keyboard = key_update;
  input->get_keyboard_state = get_key_state;

  // - Set Other
  input->set_leds = NULL;
  input->wait_event = wait_event;

  return TRUE;
}

void TermUS363TouchLibInput(void) {
  if (fd_power_key > 0) {
    close(fd_power_key);
    fd_power_key = -1;
  }
  if (touch_screen) {
    ts_close(touch_screen);
    fd_touch_screen = -1;
    touch_screen = NULL;
  }
}

#endif
