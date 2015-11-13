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
  IDirectFBDisplayLayer *layer;
} DFBPrivate;

typedef struct {
  IDirectFBSurface *surface;
  int expose;
} DFBWindowProperty;

int init(int *width, int *height, int *err)
{
  int ret = 0;
  IDirectFB *dfb = NULL;
  DFBPrivate *private = NULL;
  DFBDisplayLayerConfig layer_config;

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

  ret = dfb->CreateEventBuffer(dfb, &private->event_buffer);
  if (ret) {
    printf("CreateEventBuffer failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &private->layer);
  if (ret) {
    printf("GetDisplayLayer failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  memset(&layer_config, 0, sizeof(DFBDisplayLayerConfig));
  ret = private->layer->GetConfiguration(private->layer, &layer_config);
  if (ret) {
    printf("GetConfiguration failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  *width = layer_config.width;
  *height = layer_config.height;

  *err = 0;

  return (int)dfb;

fail:
  if (private) {
    if (private->layer) {
      private->layer->Release(private->layer);
    }
    free(private);
  }
  if (dfb) {
    dfb->Release(dfb);
  }
  *err = -1;
  return 0;
}

int create_window(int dpy, int posx, int posy, int width, int height, int opt, int *err)
{
  int ret = 0;
  IDirectFB *dfb = (IDirectFB *)dpy;
  DFBPrivate *private = NULL;
  DFBWindowDescription desc;
  IDirectFBWindow *window = NULL;
  IDirectFBSurface *surface = NULL;
  DFBWindowProperty *property = NULL;

  private = (DFBPrivate *)dfb->refs;

  memset(&desc, 0, sizeof(DFBWindowDescription));
  desc.flags = DWDESC_SURFACE_CAPS | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_POSX | DWDESC_POSY;
  desc.surface_caps = opt;
  desc.width = width;
  desc.height = height;
  desc.posx = posx;
  desc.posy = posy;
  ret = private->layer->CreateWindow(private->layer, &desc, &window);
  if (ret) {
    printf("CreateWindow failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = window->GetSurface(window, &surface);
  if (ret) {
    printf("GetSurface failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = window->DisableEvents(window, DWET_ALL ^ (DWET_GOTFOCUS | DWET_KEYDOWN | DWET_MOTION));
  if (ret) {
    printf("DisableEvents failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = window->AttachEventBuffer(window, private->event_buffer);
  if (ret) {
    printf("AttachEventBuffer failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = window->SetOpacity(window, 0xff);
  if (ret) {
    printf("SetOpacity failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  ret = window->RequestFocus(window);
  if (ret) {
    printf("RequestFocus failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  property = calloc(1, sizeof(DFBWindowProperty));
  if (!property) {
    printf("DFBWindowProperty calloc failed\n");
    goto fail;
  }

  property->surface = surface;

  ret = window->SetProperty(window, "property", property, NULL);
  if (ret) {
    printf("SetProperty failed: %s\n", DirectFBErrorString(ret));
    goto fail;
  }

  *err = 0;

  return (int)surface;

fail:
  if (property) {
    free(property);
  }
  if (surface) {
    surface->Release(surface);
  }
  if (window) {
    window->Release(window);
  }
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
  private->layer->Release(private->layer);
  private->event_buffer->Release(private->event_buffer);
  free(private);
  dfb->Release(dfb);
}

int get_event(int dpy, int *type, int *key, int *x, int *y)
{
  IDirectFB *dfb = (IDirectFB *)dpy;
  DFBPrivate *private = NULL;
  IDirectFBWindow *window = NULL;
  DFBWindowEvent event;
  DFBWindowProperty *property = NULL;
  int win = 0;

  private = (DFBPrivate *)dfb->refs;

  *type = EVENT_NONE;
  *key = *x = *y = 0;

  memset(&event, 0, sizeof(DFBWindowEvent));
  if (!private->event_buffer->GetEvent(private->event_buffer, (DFBEvent *)&event)) {
    private->layer->GetWindow(private->layer, event.window_id, &window);
    window->GetProperty(window, "property", (void *)&property);
    if (event.type == DWET_GOTFOCUS) {
      if (!property->expose) {
        property->expose = 1;
        window->SetProperty(window, "property", property, NULL);
        *type = EVENT_DISPLAY;
      }
    }
    else if (event.type == DWET_KEYDOWN) {
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
    else if (event.type == DWET_MOTION) {
      *x = event.x;
      *y = event.y;
      *type = EVENT_PASSIVEMOTION;
    }
  }

  if (*type) {
    win = (int)property->surface;
  }

  return win;
}
