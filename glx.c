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
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>
#include "attributes.h"

#ifdef FIU_ENABLE
#include <fiu-local.h>
#define FIU_CHECK(ptr) \
  fiu_init(0); \
  if (fiu_fail("BACKEND_ENOMEM")) { \
    free(ptr); \
    ptr = NULL; \
  }
#else
#define FIU_CHECK(ptr)
#endif

int init(int *width, int *height, int *err);
int create_window(int dpy, int posx, int posy, int width, int height, int opt, int *err);
void destroy_window(int dpy, int win);
void fini(int dpy);
int get_event(int dpy, int *type, int *key, int *x, int *y);

typedef struct {
  Display *x11_dpy;
  struct attributes attribs;
} glutDisplay;

typedef struct {
  int x11_win;
  GLXContext glx_ctx;
  struct attributes attribs;
} glutWindow;

int Init()
{
  int err = 0;
  glutDisplay *glut_dpy = NULL;

  glut_dpy = calloc(1, sizeof(glutDisplay));
  FIU_CHECK(glut_dpy);
  if (!glut_dpy) {
    printf("glut_dpy calloc error\n");
    return 0;
  }

  glut_dpy->x11_dpy = (Display *)init(&glut_dpy->attribs.dpy_width, &glut_dpy->attribs.dpy_height, &err);
  if (err == -1) {
    goto error;
  }

  glut_dpy->attribs.win_width = glut_dpy->attribs.dpy_width;
  glut_dpy->attribs.win_height = glut_dpy->attribs.dpy_height;

  return (int)glut_dpy;

error:
  free(glut_dpy);
  return 0;
}

void InitWindowPosition(int display, int posx, int posy)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  glut_dpy->attribs.win_posx = posx;
  glut_dpy->attribs.win_posy = posy;
}

void InitWindowSize(int display, int width, int height)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  glut_dpy->attribs.win_width = width;
  glut_dpy->attribs.win_height = height;
}

void InitDisplayMode(int display, int double_buffer, int depth_size)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  glut_dpy->attribs.double_buffer = double_buffer;
  glut_dpy->attribs.depth_size = depth_size;
}

int CreateWindow(int display)
{
  int err = 0;
  glutDisplay *glut_dpy = (glutDisplay *)display;
  glutWindow *glut_win = NULL;
  XVisualInfo *glx_visual = NULL;
  int glx_visual_attr[5];
  int i = 0;

  glut_win = calloc(1, sizeof(glutWindow));
  FIU_CHECK(glut_win);
  if (!glut_win) {
    printf("glut_win calloc error\n");
    return 0;
  }

  memcpy(&glut_win->attribs, &glut_dpy->attribs, sizeof(struct attributes));

  memset(glx_visual_attr, 0, sizeof(glx_visual_attr));
  glx_visual_attr[i++] = GLX_RGBA;
  if (glut_win->attribs.double_buffer) {
    glx_visual_attr[i++] = GLX_DOUBLEBUFFER;
  }
  if (glut_win->attribs.depth_size) {
    glx_visual_attr[i++] = GLX_DEPTH_SIZE;
    glx_visual_attr[i++] = glut_win->attribs.depth_size;
  }
  glx_visual_attr[i] = None;
  glx_visual = glXChooseVisual(glut_dpy->x11_dpy, 0, glx_visual_attr);
  if (!glx_visual) {
    printf("glXChooseVisual error\n");
    goto error;
  }

  glut_win->x11_win = create_window((int)glut_dpy->x11_dpy, glut_win->attribs.win_posx, glut_win->attribs.win_posy, glut_win->attribs.win_width, glut_win->attribs.win_height, 0, &err);
  if (err == -1) {
    goto error;
  }

  glut_win->glx_ctx = glXCreateContext(glut_dpy->x11_dpy, glx_visual, NULL, True);
  if (!glut_win->glx_ctx) {
    printf("glXCreateContext error\n");
    goto error;
  }

  if (glut_win->attribs.depth_size) {
    err = glXGetConfig(glut_dpy->x11_dpy, glx_visual, GLX_DEPTH_SIZE, &glut_win->attribs.depth_size);
    if (err) {
      printf("glXCreateContext error\n");
      goto error;
    }
  }

  free(glx_visual);

  return (int)glut_win;

error:
  if (glut_win->glx_ctx) {
    glXDestroyContext(glut_dpy->x11_dpy, glut_win->glx_ctx);
  }
  if (glut_win->x11_win) {
    destroy_window((int)glut_dpy->x11_dpy, glut_win->x11_win);
  }
  if (glx_visual) {
    free(glx_visual);
  }
  free(glut_win);
  return 0;
}

void SetWindow(int display, int window, int context)
{
  int err = 0;
  glutDisplay *glut_dpy = (glutDisplay *)display;
  glutWindow *glut_win = (glutWindow *)window;

  if (context) {
    err = glXMakeCurrent(glut_dpy->x11_dpy, glut_win->x11_win, glut_win->glx_ctx);
  }
  else {
    err = glXMakeCurrent(glut_dpy->x11_dpy, None, NULL);
  }
  if (!err) {
    printf("glXMakeCurrent error\n");
  }
}

void SwapBuffers(int display, int window)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;
  glutWindow *glut_win = (glutWindow *)window;

  glXSwapBuffers(glut_dpy->x11_dpy, glut_win->x11_win);
}

struct attributes *GetDisplayAttribs(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  return &glut_dpy->attribs;
}

struct attributes *GetWindowAttribs(int window)
{
  glutWindow *glut_win = (glutWindow *)window;

  return &glut_win->attribs;
}

void DestroyWindow(int display, int window)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;
  glutWindow *glut_win = (glutWindow *)window;

  glXDestroyContext(glut_dpy->x11_dpy, glut_win->glx_ctx);

  destroy_window((int)glut_dpy->x11_dpy, glut_win->x11_win);

  free(glut_win);
}

void Fini(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  fini((int)glut_dpy->x11_dpy);

  free(glut_dpy);
}

int GetEvent(int display, int *type, int *key, int *x, int *y)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  return get_event((int)glut_dpy->x11_dpy, type, key, x, y);
}
