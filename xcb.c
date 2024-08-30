/*
  TinyGLUT                 Small implementation of GLUT (OpenGL Utility Toolkit)
  Copyright (c) 2015-2024, Nicolas Caramelli

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "event.h"
#include "keys.h"

int init(int *width, int *height, int *err)
{
  xcb_connection_t *connection = NULL;

  connection = xcb_connect(NULL, NULL);
  if (!connection) {
    printf("xcb_connect failed\n");
    goto fail;
  }

  *width = xcb_setup_roots_iterator(xcb_get_setup(connection)).data->width_in_pixels;
  *height = xcb_setup_roots_iterator(xcb_get_setup(connection)).data->height_in_pixels;

  *err = 0;

  return (long)connection;

fail:
  *err = -1;
  return 0;
}

int create_window(int dpy, int posx, int posy, int width, int height, int opt, int *err)
{
  xcb_connection_t *connection = (xcb_connection_t *)(long)dpy;
  uint32_t value_list[2];
  xcb_void_cookie_t cookie;
  xcb_window_t window = -1;

  window = xcb_generate_id(connection);
  value_list[0] = 0;
  value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_POINTER_MOTION;
  cookie = xcb_create_window_checked(connection, XCB_COPY_FROM_PARENT, window, xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root, posx, posy, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, value_list);
  window = xcb_request_check(connection, cookie) ? -1 : window;
  if (window == -1) {
    printf("xcb_create_window failed\n");
    goto fail;
  }

  xcb_map_window(connection, window);

  value_list[0] = posx;
  value_list[1] = posy;
  xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, value_list);

  xcb_flush(connection);

  *err = 0;

  return window;

fail:
  *err = -1;
  return 0;
}

void destroy_window(int dpy, int win)
{
  xcb_connection_t *connection = (xcb_connection_t *)(long)dpy;
  xcb_window_t window = win;

  xcb_destroy_window(connection, window);
}

void fini(int dpy)
{
  xcb_connection_t *connection = (xcb_connection_t *)(long)dpy;

  xcb_disconnect(connection);
}

int get_event(int dpy, int *type, int *key, int *x, int *y)
{
  xcb_connection_t *connection = (xcb_connection_t *)(long)dpy;
  xcb_generic_event_t *event = NULL;
  xcb_key_symbols_t *key_symbols = NULL;
  xcb_keysym_t keysym;
  int win = 0;

  *type = EVENT_NONE;
  *key = *x = *y = 0;

  event = xcb_poll_for_event(connection);
  if (event) {
    if ((event->response_type & 0x7f) == XCB_EXPOSE) {
      *type = EVENT_DISPLAY;
    }
    else if ((event->response_type & 0x7f) == XCB_KEY_PRESS) {
      key_symbols = xcb_key_symbols_alloc(connection);
      keysym = xcb_key_symbols_get_keysym(key_symbols, ((xcb_key_press_event_t *)event)->detail, 0);
      switch (keysym) {
        case 0xffbe:            *key = F1;        break;
        case 0xffbf:            *key = F2;        break;
        case 0xffc0:            *key = F3;        break;
        case 0xffc1:            *key = F4;        break;
        case 0xffc2:            *key = F5;        break;
        case 0xffc3:            *key = F6;        break;
        case 0xffc4:            *key = F7;        break;
        case 0xffc5:            *key = F8;        break;
        case 0xffc6:            *key = F9;        break;
        case 0xffc7:            *key = F10;       break;
        case 0xffc8:            *key = F11;       break;
        case 0xffc9:            *key = F12;       break;
        case 0xff51:            *key = LEFT;      break;
        case 0xff52:            *key = UP;        break;
        case 0xff53:            *key = RIGHT;     break;
        case 0xff54:            *key = DOWN;      break;
        case 0xff55:            *key = PAGE_UP;   break;
        case 0xff56:            *key = PAGE_DOWN; break;
        default:                *key = 0;         break;
      }
      if (!*key) {
        *key = keysym;
        *type = EVENT_KEYBOARD;
      }
      else {
        *type = EVENT_SPECIAL;
      }
    }
    else if ((event->response_type & 0x7f) == XCB_MOTION_NOTIFY) {
      *x = ((xcb_motion_notify_event_t *)event)->event_x;
      *y = ((xcb_motion_notify_event_t *)event)->event_y;
      *type = EVENT_PASSIVEMOTION;
    }
  }

  if (*type) {
    switch (event->response_type & 0x7f) {
      case XCB_EXPOSE:
        win = ((xcb_expose_event_t *)event)->window;
        break;
      case XCB_KEY_PRESS:
        win = ((xcb_motion_notify_event_t *)event)->event;
        break;
      case XCB_MOTION_NOTIFY:
        win = ((xcb_motion_notify_event_t *)event)->event;
        break;
      default:
        break;
    }
  }

  if (key_symbols) {
    xcb_key_symbols_free(key_symbols);
  }

  if (event) {
    free(event);
  }

  return win;
}
