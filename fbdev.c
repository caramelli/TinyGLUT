/*
  TinyGLUT            Small implementation of GLUT (OpenGL Utility Toolkit)

  Copyright (C) 2015  Nicolas Caramelli
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include "event.h"
#include "keys.h"

struct fb_window {
  int width;
  int height;
  int posx;
  int posy;
};

int init(int *width, int *height, int *err)
{
  int ret = 0;
  int fb = -1;
  struct fb_var_screeninfo info;
  int i = 0;
  char name[32], path[32];

  if (getenv("FRAMEBUFFER")) {
    fb = open(getenv("FRAMEBUFFER"), O_RDWR);
    if (fb == -1) {
      printf("open %s failed: %s\n", getenv("FRAMEBUFFER"), strerror(errno));
      goto fail;
    }
  }
  else {
    fb = open("/dev/fb0", O_RDWR);
    if (fb == -1) {
      printf("open %s failed: %s\n", "/dev/fb0", strerror(errno));
      goto fail;
    }
  }

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ret = ioctl(fb, FBIOGET_VSCREENINFO, &info);
  if (ret == -1) {
    printf("ioctl FBIOGET_VSCREENINFO failed: %s\n", strerror(errno));
    goto fail;
  }

  info.reserved[0] = -1;
  ret = ioctl(fb, FBIOPUT_VSCREENINFO, &info);
  if (ret == -1) {
    printf("ioctl FBIOPUT_VSCREENINFO failed: %s\n", strerror(errno));
    goto fail;
  }
  ioctl(fb, FBIOGET_VSCREENINFO, &info);
  if (!info.reserved[0]) {
    printf("ioctl FBIOGET_VSCREENINFO reserved failed\n");
    goto fail;
  }

  if (getenv("KEYBOARD")) {
    info.reserved[0] = open(getenv("KEYBOARD"), O_RDONLY | O_NONBLOCK);
    if (info.reserved[0] == -1) {
      printf("open %s failed: %s\n", getenv("KEYBOARD"), strerror(errno));
      goto fail;
    }
  }
  else {
    while (1) {
      sprintf(path, "/dev/input/event%d", i);
      info.reserved[0] = open(path, O_RDONLY);
      if (info.reserved[0] == -1) {
        sprintf(path, "/dev/input/event0");
        break;
      }
      ioctl(info.reserved[0], EVIOCGNAME(sizeof(name)), name);
      close(info.reserved[0]);
      if (!strcmp(name, "uinput")) {
        break;
      }
      i++;
    }
    info.reserved[0] = open(path, O_RDONLY | O_NONBLOCK);
    if (info.reserved[0] == -1) {
      printf("open %s failed: %s\n", path, strerror(errno));
      goto fail;
    }
  }

  info.reserved[1] = 0;

  ioctl(fb, FBIOPUT_VSCREENINFO, &info);

  *width = info.xres;
  *height = info.yres;

  *err = 0;

  return fb;

fail:
  if (fb != -1) {
    close(fb);
  }
  *err = -1;
  return 0;
}

int create_window(int dpy, int width, int height, int opt, int *err)
{
  struct fb_window *window = NULL;

  window = calloc(1, sizeof(struct fb_window));
  if (!window) {
    printf("fb_window calloc failed\n");
    goto fail;
  }
  else {
    window->width = width;
    window->height = height;
  }

  *err = 0;

  return (int)window;

fail:
  *err = -1;
  return 0;
}

void destroy_window(int dpy, int win)
{
  struct fb_window *window = (struct fb_window *)win;

  free(window);
}

void fini(int dpy)
{
  int fb = dpy;
  struct fb_var_screeninfo info;

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ioctl(fb, FBIOGET_VSCREENINFO, &info);
  close(info.reserved[0]);
  close(fb);
}

static int keymap[] = {
  [ KEY_ESC ] = 0x1B,
  [ KEY_SPACE ] = 0x20,
  [ KEY_0 ] = 0x30,
  [ KEY_1 ] = 0x31,
  [ KEY_2 ] = 0x32,
  [ KEY_3 ] = 0x33,
  [ KEY_4 ] = 0x34,
  [ KEY_5 ] = 0x35,
  [ KEY_6 ] = 0x36,
  [ KEY_7 ] = 0x37,
  [ KEY_8 ] = 0x38,
  [ KEY_9 ] = 0x39,
  [ KEY_A ] = 0x61,
  [ KEY_B ] = 0x62,
  [ KEY_C ] = 0x63,
  [ KEY_D ] = 0x64,
  [ KEY_E ] = 0x65,
  [ KEY_F ] = 0x66,
  [ KEY_G ] = 0x67,
  [ KEY_H ] = 0x68,
  [ KEY_I ] = 0x69,
  [ KEY_J ] = 0x6A,
  [ KEY_K ] = 0x6B,
  [ KEY_L ] = 0x6C,
  [ KEY_M ] = 0x6D,
  [ KEY_N ] = 0x6E,
  [ KEY_O ] = 0x6F,
  [ KEY_P ] = 0x70,
  [ KEY_Q ] = 0x71,
  [ KEY_R ] = 0x72,
  [ KEY_S ] = 0x73,
  [ KEY_T ] = 0x74,
  [ KEY_U ] = 0x75,
  [ KEY_V ] = 0x76,
  [ KEY_W ] = 0x77,
  [ KEY_X ] = 0x78,
  [ KEY_Y ] = 0x79,
  [ KEY_Z ] = 0x7A,
};

int get_event(int dpy, int *type, int *key)
{
  int fb = dpy;
  struct fb_var_screeninfo info;
  struct input_event event;
  int win = 0;

  *type = EVENT_NONE;
  *key = 0;

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ioctl(fb, FBIOGET_VSCREENINFO, &info);

  if (!info.reserved[1]) {
    info.reserved[1] = 1;
    ioctl(fb, FBIOPUT_VSCREENINFO, &info);
    *type = EVENT_DISPLAY;
  }
  else {
    memset(&event, 0, sizeof(struct input_event));
    read(info.reserved[0], &event, sizeof(struct input_event));
    if (event.type == EV_KEY && event.value) {
      switch (event.code) {
        case KEY_F1:            *key = F1;        break;
        case KEY_F2:            *key = F2;        break;
        case KEY_F3:            *key = F3;        break;
        case KEY_F4:            *key = F4;        break;
        case KEY_F5:            *key = F5;        break;
        case KEY_F6:            *key = F6;        break;
        case KEY_F7:            *key = F7;        break;
        case KEY_F8:            *key = F8;        break;
        case KEY_F9:            *key = F9;        break;
        case KEY_F10:           *key = F10;       break;
        case KEY_F11:           *key = F11;       break;
        case KEY_F12:           *key = F12;       break;
        case KEY_LEFT:          *key = LEFT;      break;
        case KEY_UP:            *key = UP;        break;
        case KEY_RIGHT:         *key = RIGHT;     break;
        case KEY_DOWN:          *key = DOWN;      break;
        case KEY_PAGEUP:        *key = PAGE_UP;   break;
        case KEY_PAGEDOWN:      *key = PAGE_DOWN; break;
        default:                *key = 0;         break;
      }
      if (!*key) {
        *key = keymap[event.code];
        *type = EVENT_KEYBOARD;
      }
      else {
        *type = EVENT_SPECIAL;
      }
    }
  }

  if (*type) {
    win = 1;
  }

  return win;
}
