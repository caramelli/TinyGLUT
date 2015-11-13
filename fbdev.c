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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include "event.h"
#include "keys.h"

#define MAX(a,b) a > b ? a : b

#define W 4
#define H 6

struct fb_list {
  struct fb_list *next;
  struct fb_list *prev;
};

struct fb_window {
  int width;
  int height;
  int posx;
  int posy;
  struct fb_list link;
};

enum {
  FB_EVENT_EXPOSE,
  FB_EVENT_KEYBOARD,
  FB_EVENT_MOUSE
};

struct fb_event {
  int type;
  int keycode;
  int x;
  int y;
  struct fb_window *window;
  struct fb_list link;
};

struct fb_user_data {
  int bpp;
  int w;
  int h;
  unsigned char *screen;
  int cx;
  int cy;
  int cw;
  int ch;
  unsigned char *cursor;
  int keyboard;
  int mouse;
  struct fb_list window_list;
  struct fb_list event_list;
  int pipe[2];
  pthread_t thread;
};

static void *input_thread(void *data)
{
  struct fb_user_data *user_data = data;
  fd_set set;
  struct input_event input;
  int i = 0;
  unsigned char *ptr = NULL;
  struct fb_window *window = NULL;
  struct fb_list *window_link = NULL;
  struct fb_event *event = NULL;
  struct fb_list *event_link = NULL;

  while (1) {
    FD_ZERO(&set);
    FD_SET(user_data->pipe[0], &set);
    FD_SET(user_data->keyboard, &set);
    FD_SET(user_data->mouse, &set);

    if (select(MAX(MAX(user_data->keyboard, user_data->mouse), user_data->pipe[0]) + 1, &set, NULL, NULL, NULL) >= 0) {
      memset(&input, 0, sizeof(struct input_event));

      if (FD_ISSET(user_data->keyboard, &set)) {
        read(user_data->keyboard, &input, sizeof(struct input_event));
      }
      else if (FD_ISSET(user_data->mouse, &set)) {
        read(user_data->mouse, &input, sizeof(struct input_event));
      }
      else if (FD_ISSET(user_data->pipe[0], &set)) {
        break;
      }

      if (input.type == EV_REL) {
        ptr = user_data->screen + (user_data->cy - H) * user_data->w * user_data->bpp + (user_data->cx - W) * user_data->bpp;
        for (i = 0; i < user_data->ch; i++) {
          memcpy(ptr, user_data->cursor + i * user_data->cw * user_data->bpp, user_data->cw * user_data->bpp);
          ptr += user_data->w * user_data->bpp;
        }

        if (input.code == REL_X) {
          user_data->cx += input.value;
          user_data->cx = user_data->cx < W ? W : user_data->cx;
          user_data->cx = user_data->cx > user_data->w - W - 1 ? user_data->w - W - 1 : user_data->cx;
        }

        if (input.code == REL_Y) {
          user_data->cy += input.value;
          user_data->cy = user_data->cy < H ? H : user_data->cy;
          user_data->cy = user_data->cy > user_data->h - H - 1 ? user_data->h - H - 1 : user_data->cy;
        }

        ptr = user_data->screen + (user_data->cy - H) * user_data->w * user_data->bpp + (user_data->cx - W) * user_data->bpp;
        for (i = 0; i < user_data->ch; i++) {
          memcpy(user_data->cursor + i * user_data->cw * user_data->bpp, ptr, user_data->cw * user_data->bpp);
          ptr += user_data->w * user_data->bpp;
        }

        ptr = user_data->screen + (user_data->cy - H) * user_data->w * user_data->bpp + (user_data->cx - W) * user_data->bpp;
        for (i = 0; i < user_data->ch; i++) {
          memset(ptr, 255, user_data->cw * user_data->bpp);
          ptr += user_data->w * user_data->bpp;
        }
      }

      if ((input.type == EV_KEY && input.value) || input.type == EV_REL) {
        for (window_link = user_data->window_list.next; window_link != &user_data->window_list; window_link = window_link->next) {
          window = (struct fb_window *)((char *)window_link - (char *)&((struct fb_window *)NULL)->link);

          if (user_data->cx >= window->posx && user_data->cx < window->posx + window->width - 1 && user_data->cy >= window->posy && user_data->cy < window->posy + window->height - 1) {
            event = calloc(1, sizeof(struct fb_event));
            if (!event) {
              printf("event calloc failed\n");
            }

            if (input.type == EV_KEY) {
              event->type = FB_EVENT_KEYBOARD;
              event->keycode = input.code;
            }

            if (input.type == EV_REL) {
              event->type = FB_EVENT_MOUSE;
              event->x = user_data->cx - window->posx;
              event->y = user_data->cy - window->posy;
            }

            event->window = window;
            event_link = &event->link;
            event_link->next = user_data->event_list.next;
            event_link->prev = &user_data->event_list;
            user_data->event_list.next->prev = event_link;
            user_data->event_list.next = event_link;
            break;
          }
        }
      }
    }
    else {
      printf("select failed: %s\n", strerror(errno));
    }
  }

  return NULL;
}

int init(int *width, int *height, int *err)
{
  int ret = 0;
  int fb = -1;
  struct fb_user_data *user_data = NULL;
  struct fb_var_screeninfo info;
  int i = 0;
  char name[32], path[32];
  unsigned char *ptr = NULL;

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

  user_data = calloc(1, sizeof(struct fb_user_data));
  if (!user_data) {
    printf("user_data calloc failed\n");
    goto fail;
  }
  else {
    user_data->keyboard = -1;
    user_data->mouse = -1;
  }

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ret = ioctl(fb, FBIOGET_VSCREENINFO, &info);
  if (ret == -1) {
    printf("ioctl FBIOGET_VSCREENINFO failed: %s\n", strerror(errno));
    goto fail;
  }

  info.reserved[0] = (int)user_data;
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

  user_data->bpp = info.bits_per_pixel >> 3;

  user_data->w = info.xres;
  user_data->h = info.yres;
  user_data->screen = mmap(NULL, user_data->w * user_data->h * user_data->bpp, PROT_WRITE, MAP_SHARED, fb, 0);

  user_data->cx = info.xres >> 1;
  user_data->cy = info.yres >> 1;

  user_data->cw = 2 * W + 1;
  user_data->ch = 2 * H + 1;
  user_data->cursor = calloc(user_data->cw * user_data->ch, user_data->bpp);
  if (!user_data->cursor) {
    printf("cursor calloc failed\n");
    goto fail;
  }

  ptr = user_data->screen + (user_data->cy - H) * user_data->w * user_data->bpp + (user_data->cx - W) * user_data->bpp;
  for (i = 0; i < user_data->ch; i++) {
    memcpy(user_data->cursor + i * user_data->cw * user_data->bpp, ptr, user_data->cw * user_data->bpp);
    memset(ptr, 255, user_data->cw * user_data->bpp);
    ptr += user_data->w * user_data->bpp;
  }

  if (getenv("KEYBOARD")) {
    user_data->keyboard = open(getenv("KEYBOARD"), O_RDONLY);
    if (user_data->keyboard == -1) {
      printf("open %s failed: %s\n", getenv("KEYBOARD"), strerror(errno));
      goto fail;
    }
  }
  else {
    i = 0;
    while (1) {
      sprintf(path, "/dev/input/event%d", i);
      user_data->keyboard = open(path, O_RDONLY);
      if (user_data->keyboard == -1) {
        sprintf(path, "/dev/input/event0");
        break;
      }
      ioctl(user_data->keyboard, EVIOCGNAME(sizeof(name)), name);
      close(user_data->keyboard);
      if (!strcmp(name, "uinput-keyboard")) {
        break;
      }
      i++;
    }
    user_data->keyboard = open(path, O_RDONLY);
    if (user_data->keyboard == -1) {
      printf("open %s failed: %s\n", path, strerror(errno));
      goto fail;
    }
  }

  if (getenv("MOUSE")) {
    user_data->mouse = open(getenv("MOUSE"), O_RDONLY);
    if (user_data->mouse == -1) {
      printf("open %s failed: %s\n", getenv("MOUSE"), strerror(errno));
      goto fail;
    }
  }
  else {
    i = 0;
    while (1) {
      sprintf(path, "/dev/input/event%d", i);
      user_data->mouse = open(path, O_RDONLY);
      if (user_data->mouse == -1) {
        sprintf(path, "/dev/input/event1");
        break;
      }
      ioctl(user_data->mouse, EVIOCGNAME(sizeof(name)), name);
      close(user_data->mouse);
      if (!strcmp(name, "uinput-mouse")) {
        break;
      }
      i++;
    }
    user_data->mouse = open(path, O_RDONLY);
    if (user_data->mouse == -1) {
      printf("open %s failed: %s\n", path, strerror(errno));
      goto fail;
    }
  }

  user_data->window_list.next = &user_data->window_list;
  user_data->window_list.prev = &user_data->window_list;

  user_data->event_list.next = &user_data->event_list;
  user_data->event_list.prev = &user_data->event_list;

  ret = pipe(user_data->pipe);
  if (ret == -1) {
    printf("pipe failed: %s\n", strerror(errno));
    goto fail;
  }

  pthread_create(&user_data->thread, NULL, input_thread, user_data);

  *width = info.xres;
  *height = info.yres;

  *err = 0;

  return fb;

fail:
  if (user_data) {
    if (user_data->mouse != -1) {
      close(user_data->mouse);
    }
    if (user_data->keyboard != -1) {
      close(user_data->keyboard);
    }
    if (user_data->cursor) {
      free(user_data->cursor);
    }
    free(user_data);
  }
  if (fb != -1) {
    close(fb);
  }
  *err = -1;
  return 0;
}

int create_window(int dpy, int posx, int posy, int width, int height, int opt, int *err)
{
  int fb = dpy;
  struct fb_var_screeninfo info;
  struct fb_user_data *user_data = NULL;
  struct fb_window *window = NULL;
  struct fb_list *window_link = NULL;
  struct fb_event *event = NULL;
  struct fb_list *event_link = NULL;

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ioctl(fb, FBIOGET_VSCREENINFO, &info);
  user_data = (struct fb_user_data *)info.reserved[0];

  window = calloc(1, sizeof(struct fb_window));
  if (!window) {
    printf("fb_window calloc failed\n");
    goto fail;
  }
  else {
    window->width = width;
    window->height = height;
    window->posx = posx;
    window->posy = posy;
  }

  window_link = &window->link;
  window_link->next = user_data->window_list.next;
  window_link->prev = &user_data->window_list;
  user_data->window_list.next->prev = window_link;
  user_data->window_list.next = window_link;

  event = calloc(1, sizeof(struct fb_event));
  if (!event) {
    printf("event calloc failed\n");
    goto fail;
  }

  event->type = FB_EVENT_EXPOSE;
  event->window = window;
  event_link = &event->link;
  event_link->next = user_data->event_list.next;
  event_link->prev = &user_data->event_list;
  user_data->event_list.next->prev = event_link;
  user_data->event_list.next = event_link;

  *err = 0;

  return (int)window;

fail:
  if (window) {
    if (window_link) {
      window_link->next->prev = window_link->prev;
      window_link->prev->next = window_link->next;
    }
    free(window);
  }
  *err = -1;
  return 0;
}

void destroy_window(int dpy, int win)
{
  struct fb_window *window = (struct fb_window *)win;
  struct fb_list *window_link = NULL;

  window_link = &window->link;
  window_link->next->prev = window_link->prev;
  window_link->prev->next = window_link->next;
  free(window);
}

void fini(int dpy)
{
  int fb = dpy;
  struct fb_var_screeninfo info;
  struct fb_user_data *user_data = NULL;
  struct fb_event *event = NULL;
  struct fb_list *event_link = NULL;

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ioctl(fb, FBIOGET_VSCREENINFO, &info);
  user_data = (struct fb_user_data *)info.reserved[0];
  for (event_link = user_data->event_list.next; event_link != &user_data->event_list; event_link = event_link->next) {
    event_link->next->prev = event_link->prev;
    event_link->prev->next = event_link->next;
    free(event);
  }
  write(user_data->pipe[1], "", 1);
  pthread_join(user_data->thread, NULL);
  close(user_data->pipe[0]);
  close(user_data->pipe[1]);
  close(user_data->mouse);
  close(user_data->keyboard);
  free(user_data->cursor);
  free(user_data);
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

int get_event(int dpy, int *type, int *key, int *x, int *y)
{
  int fb = dpy;
  struct fb_var_screeninfo info;
  struct fb_user_data *user_data = NULL;
  struct fb_event *event = NULL;
  struct fb_list *event_link = NULL;
  int win = 0;

  *type = EVENT_NONE;
  *key = *x = *y = 0;

  memset(&info, 0, sizeof(struct fb_var_screeninfo));
  ioctl(fb, FBIOGET_VSCREENINFO, &info);
  user_data = (struct fb_user_data *)info.reserved[0];

  event_link = &user_data->event_list;
  while (event_link->next != &user_data->event_list) {
    event_link = event_link->next;
  }

  if (event_link != &user_data->event_list) {
    event = (struct fb_event *)((char *)event_link - (char *)&((struct fb_event *)NULL)->link);
    if (event->type == FB_EVENT_EXPOSE) {
      *type = EVENT_DISPLAY;
    }
    else if (event->type == FB_EVENT_KEYBOARD) {
      switch (event->keycode) {
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
        *key = keymap[event->keycode];
        *type = EVENT_KEYBOARD;
      }
      else {
        *type = EVENT_SPECIAL;
      }
    }
    else if (event->type == FB_EVENT_MOUSE) {
      *x = event->x;
      *y = event->y;
      *type = EVENT_PASSIVEMOTION;
    }
  }

  if (*type) {
    win = (int)event->window;
    event_link->next->prev = event_link->prev;
    event_link->prev->next = event_link->next;
    free(event);
  }

  return win;
}
