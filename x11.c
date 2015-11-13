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

#include <stdio.h>
#include <string.h>
#include <X11/Xutil.h>
#include "event.h"
#include "keys.h"

int init(int *width, int *height, int *err)
{
  Display *display = NULL;

  display = XOpenDisplay(NULL);
  if (!display) {
    printf("XOpenDisplay failed\n");
    *err = -1;
    return 0;
  }

  *width = DisplayWidth(display, 0);
  *height = DisplayHeight(display, 0);

  *err = 0;

  return (int)display;
}

int create_window(int dpy, int width, int height, int opt, int *err)
{
  Display *display = (Display *)dpy;
  Window window = 0;

  window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, width, height, 0, 0, 0);
  if (!window) {
    printf("XCreateSimpleWindow failed\n");
    *err = -1;
    return 0;
  }

  XMapWindow(display, window);

  XSelectInput(display, window, ExposureMask | KeyPressMask);

  *err = 0;

  return window;
}

void destroy_window(int dpy, int win)
{
  Display *display = (Display *)dpy;
  Window window = win;

  XDestroyWindow(display, window);
}

void fini(int dpy)
{
  Display *display = (Display *)dpy;

  XCloseDisplay(display);
}

int get_event(int dpy, int *type, int *key)
{
  Display *display = (Display *)dpy;
  XEvent event;
  char keycode = 0;
  KeySym keysym = 0;
  int win = 0;

  *type = EVENT_NONE;
  *key = 0;

  if (XPending((Display *)display)) {
    memset(&event, 0, sizeof(XEvent));
    XNextEvent((Display *)display, &event);
    if (event.type == Expose) {
      *type = EVENT_DISPLAY;
    }
    else if (event.type == KeyPress) {
      XLookupString(&event.xkey, &keycode, 1, &keysym, NULL);
      switch (keysym) {
        case XK_F1:             *key = F1;        break;
        case XK_F2:             *key = F2;        break;
        case XK_F3:             *key = F3;        break;
        case XK_F4:             *key = F4;        break;
        case XK_F5:             *key = F5;        break;
        case XK_F6:             *key = F6;        break;
        case XK_F7:             *key = F7;        break;
        case XK_F8:             *key = F8;        break;
        case XK_F9:             *key = F9;        break;
        case XK_F10:            *key = F10;       break;
        case XK_F11:            *key = F11;       break;
        case XK_F12:            *key = F12;       break;
        case XK_Left:           *key = LEFT;      break;
        case XK_Up:             *key = UP;        break;
        case XK_Right:          *key = RIGHT;     break;
        case XK_Down:           *key = DOWN;      break;
        case XK_Page_Up:        *key = PAGE_UP;   break;
        case XK_Page_Down:      *key = PAGE_DOWN; break;
        default:                *key = 0;         break;
      }
      if (!*key) {
        *key = keycode;
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
