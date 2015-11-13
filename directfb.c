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

#include <directfb.h>
#include "event.h"
#include "keys.h"

typedef struct {
  IDirectFBEventBuffer *event_buffer;
  int expose;
} DFBPrivate;

int init(int *width, int *height, int *err)
{
  int ret = 0;
  IDirectFB *dfb = NULL;
  DFBPrivate *private = NULL;
  IDirectFBScreen *screen = NULL;

  ret = DirectFBInit(NULL, NULL);
  if (ret) {
    printf("DirectFBInit failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = DirectFBCreate(&dfb);
  if (ret) {
    printf("DirectFBCreate failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  private = calloc(1, sizeof(DFBPrivate));
  if (!private) {
    printf("DFBPrivate calloc failed\n");
    goto fail;
  }

  dfb->refs = (int)private;

  ret = dfb->CreateInputEventBuffer(dfb, DICAPS_KEYS, DFB_FALSE, &private->event_buffer);
  if (ret) {
    printf("CreateInputEventBuffer failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = dfb->GetScreen(dfb, DSCID_PRIMARY, &screen);
  if (ret) {
    printf("GetScreen failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = screen->GetSize(screen, width, height);
  if (ret) {
    printf("GetSize failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  screen->Release(screen);

  *err = 0;

  return (int)dfb;

fail:
  if (screen) {
    screen->Release(screen);
  }
  if (private) {
    if (private->event_buffer) {
      private->event_buffer->Release(private->event_buffer);
    }
    free(private);
  }
  if (dfb) {
    dfb->Release(dfb);
  }
  *err = -1;
  return 0;
}

int create_window(int dpy, int width, int height, int opt, int *err)
{
  int ret = 0;
  IDirectFB *dfb = (IDirectFB *)dpy;
  DFBSurfaceDescription desc;
  IDirectFBSurface *surface = NULL;

  memset(&desc, 0, sizeof(DFBSurfaceDescription));
  desc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;
  desc.caps = DSCAPS_PRIMARY | opt;
  desc.width = width;
  desc.height = height;
  ret = dfb->CreateSurface(dfb, &desc, &surface);
  if (ret) {
    printf("CreateSurface failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  *err = 0;

  return (int)surface;

fail:
  *err = -1;
  return 0;
}

void destroy_window(int dpy, int win)
{
  IDirectFBSurface *surface = (IDirectFBSurface *)win;

  surface->Release(surface);
}

void fini(int dpy)
{
  IDirectFB *dfb = (IDirectFB *)dpy;
  DFBPrivate *private = NULL;

  private = (DFBPrivate *)dfb->refs;
  private->event_buffer->Release(private->event_buffer);
  free(private);
  dfb->Release(dfb);
}

int get_event(int dpy, int *type, int *key)
{
  IDirectFB *dfb = (IDirectFB *)dpy;
  DFBPrivate *private = NULL;
  DFBInputEvent event;
  int win = 0;

  private = (DFBPrivate *)dfb->refs;

  *type = EVENT_NONE;
  *key = 0;

  if (!private->expose) {
    private->expose = 1;
    *type = EVENT_DISPLAY;
  }
  else {
    memset(&event, 0, sizeof(DFBInputEvent));
    if (!private->event_buffer->GetEvent(private->event_buffer, (DFBEvent *)&event) && event.type == DIET_KEYPRESS) {
      switch (event.key_symbol) {
        case DIKS_F1:           *key = F1;        break;
        case DIKS_F2:           *key = F2;        break;
        case DIKS_F3:           *key = F3;        break;
        case DIKS_F4:           *key = F4;        break;
        case DIKS_F5:           *key = F5;        break;
        case DIKS_F6:           *key = F6;        break;
        case DIKS_F7:           *key = F7;        break;
        case DIKS_F8:           *key = F8;        break;
        case DIKS_F9:           *key = F9;        break;
        case DIKS_F10:          *key = F10;       break;
        case DIKS_F11:          *key = F11;       break;
        case DIKS_F12:          *key = F12;       break;
        case DIKS_CURSOR_LEFT:  *key = LEFT;      break;
        case DIKS_CURSOR_UP:    *key = UP;        break;
        case DIKS_CURSOR_RIGHT: *key = RIGHT;     break;
        case DIKS_CURSOR_DOWN:  *key = DOWN;      break;
        case DIKS_PAGE_UP:      *key = PAGE_UP; break;
        case DIKS_PAGE_DOWN:    *key = PAGE_DOWN; break;
        default:                *key = 0;         break;
      }
      if (!*key) {
        *key = event.key_symbol;
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
