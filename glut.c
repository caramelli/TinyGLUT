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

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "attributes.h"
#include "event.h"
#include "glut.h"

static void *backend_handle = NULL;

static int t0 = 0;

static int glut_dpy = 0, glut_win = 0;

#define DISPLAY_CHECK() \
  if (!glut_dpy) { \
    printf("%s exit\n", __FUNCTION__); \
    exit(1); \
  }

#define WINDOW_CHECK() \
  if (!glut_win) { \
    printf("%s exit\n", __FUNCTION__); \
    exit(1); \
  }

static int (*InitProc)() = NULL;
static void (*InitWindowSizeProc)(int, int, int) = NULL;
static void (*InitDisplayModeProc)(int, int, int) = NULL;
static int (*CreateWindowProc)(int) = NULL;
static void (*SwapBuffersProc)(int, int) = NULL;
static struct attributes *(*GetDisplayAttribsProc)(int) = NULL;
static struct attributes *(*GetWindowAttribsProc)(int) = NULL;
static void (*DestroyWindowProc)(int, int) = NULL;
static void (*FiniProc)() = NULL;
static int (*GetEventProc)(int, int *, int *) = NULL;

static void (*ReshapeCb)() = NULL;
static void (*DisplayCb)() = NULL;
static void (*IdleCb)() = NULL;
static void (*KeyboardCb)(unsigned char, int, int) = NULL;
static void (*SpecialCb)(int, int, int) = NULL;

void glutInit(int *argc, char **argv)
{
  char backend_path[PATH_MAX];

  if (glut_dpy) {
    printf("display already initialized\n");
    return;
  }

  if (!getenv("GLUT_BACKEND")) {
    printf("GLUT_BACKEND is not set\n");
    goto out;
  }
  else {
    sprintf(backend_path, "%s/%s.so", BACKENDSDIR, getenv("GLUT_BACKEND"));
  }

  backend_handle = dlopen(backend_path, RTLD_LAZY);
  if (!backend_handle) {
    printf("%s backend not found\n", getenv("GLUT_BACKEND"));
    goto out;
  }

  #define DLSYM(sym) sym##Proc = dlsym(backend_handle, #sym); \
  if (!sym##Proc) { \
    printf("%s not found\n", #sym); \
    goto out; \
  }

  DLSYM(Init);
  DLSYM(InitWindowSize);
  DLSYM(InitDisplayMode);
  DLSYM(CreateWindow);
  DLSYM(SwapBuffers);
  DLSYM(GetDisplayAttribs);
  DLSYM(GetWindowAttribs);
  DLSYM(DestroyWindow);
  DLSYM(Fini);
  DLSYM(GetEvent);

  glut_dpy = InitProc();
  if (!glut_dpy) {
    goto out;
  }

  return;

out:
  if (backend_handle) {
    dlclose(backend_handle);
    backend_handle = NULL;
  }
  exit(1);
}

void glutInitWindowSize(int width, int height)
{
  DISPLAY_CHECK();

  InitWindowSizeProc(glut_dpy, width, height);
}

void glutInitDisplayMode(unsigned int mode)
{
  int double_buffer = 0, depth_size = 0;

  DISPLAY_CHECK();

  if (mode & GLUT_DOUBLE) {
    double_buffer = 1;
  }

  if (mode & GLUT_DEPTH) {
    if (getenv("DEPTH_SIZE")) {
      depth_size = atoi(getenv("DEPTH_SIZE"));
    }
    else {
      depth_size = 1;
    }
  }

  InitDisplayModeProc(glut_dpy, double_buffer, depth_size);
}

int glutCreateWindow(const char *title)
{
  DISPLAY_CHECK();

  if (glut_win) {
    printf("window already created\n");
    return 0;
  }

  glut_win = CreateWindowProc(glut_dpy);
  if (!glut_win) {
    exit(1);
  }

  return glut_win;
}

void glutReshapeFunc(void (*func)(int, int))
{
  WINDOW_CHECK();

  ReshapeCb = func;
}

void glutDisplayFunc(void (*func)())
{
  WINDOW_CHECK();

  DisplayCb = func;
}

void glutIdleFunc(void (*func)())
{
  WINDOW_CHECK();

  IdleCb = func;
}

void glutKeyboardFunc(void (*func)(unsigned char, int, int))
{
  WINDOW_CHECK();

  KeyboardCb = func;
}

void glutSpecialFunc(void (*func)(int, int, int))
{
  WINDOW_CHECK();

  SpecialCb = func;
}

void glutSwapBuffers()
{
  WINDOW_CHECK();

  SwapBuffersProc(glut_dpy, glut_win);
}

void glutPostRedisplay()
{
  WINDOW_CHECK();

  if (DisplayCb) {
    DisplayCb();
  }
}

int glutGet(int query)
{
  if (query == GLUT_ELAPSED_TIME) {
    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    gettimeofday(&tv, NULL);
    int t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (!t0) {
      t0 = t;
      return 0;
    }
    else {
      return t - t0;
    }
  }
  else if (query == GLUT_SCREEN_WIDTH || query == GLUT_SCREEN_HEIGHT || query == GLUT_INIT_WINDOW_WIDTH || query == GLUT_INIT_WINDOW_HEIGHT || query == GLUT_INIT_DISPLAY_MODE) {
    DISPLAY_CHECK();

    struct attributes *attribs = GetDisplayAttribsProc(glut_dpy);
    int mode = 0;

    switch (query) {
      case GLUT_SCREEN_WIDTH: return attribs->dpy_width;
      case GLUT_SCREEN_HEIGHT: return attribs->dpy_height;
      case GLUT_INIT_WINDOW_WIDTH: return attribs->win_width;
      case GLUT_INIT_WINDOW_HEIGHT: return attribs->win_height;
      case GLUT_INIT_DISPLAY_MODE:
        if (attribs->double_buffer) mode |= GLUT_DOUBLE;
        if (attribs->depth_size) mode |= GLUT_DEPTH;
        return mode;
    }
  }
  else if (query == GLUT_WINDOW_WIDTH || query == GLUT_WINDOW_HEIGHT || query == GLUT_WINDOW_DOUBLEBUFFER || query == GLUT_WINDOW_DEPTH_SIZE) {
    WINDOW_CHECK();

    struct attributes *attribs = GetWindowAttribsProc(glut_win);

    switch (query) {
      case GLUT_WINDOW_WIDTH: return attribs->win_width;
      case GLUT_WINDOW_HEIGHT: return attribs->win_height;
      case GLUT_WINDOW_DOUBLEBUFFER: return attribs->double_buffer;
      case GLUT_WINDOW_DEPTH_SIZE: return attribs->depth_size;
    }
  }

  return 0;
}

void glutDestroyWindow(int window)
{
  WINDOW_CHECK();

  if (window != glut_win) {
    printf("Bad window\n");
    return;
  }

  DestroyWindowProc(glut_dpy, glut_win);

  glut_win = 0;
}

void glutMainLoop()
{
  int type = EVENT_NONE, key = 0;

  WINDOW_CHECK();

  if (ReshapeCb) {
    struct attributes *attribs = GetWindowAttribsProc(glut_win);
    ReshapeCb(attribs->win_width, attribs->win_height);
  }

  while (glut_win) {
    if (GetEventProc(glut_dpy, &type, &key)) {
      switch (type) {
        case EVENT_DISPLAY:
          if (DisplayCb) {
            DisplayCb();
          }
          break;
        case EVENT_KEYBOARD:
          if (KeyboardCb) {
            KeyboardCb(key, 0, 0);
          }
          break;
        case EVENT_SPECIAL:
          if (SpecialCb) {
            SpecialCb(key, 0, 0);
          }
          break;
        default:
          break;
      }
    }
    else {
      if (IdleCb) {
        IdleCb();
      }
    }
  }

  FiniProc(glut_dpy);
  glut_dpy = 0;
  t0 = 0;
  dlclose(backend_handle);
  backend_handle = NULL;
}
