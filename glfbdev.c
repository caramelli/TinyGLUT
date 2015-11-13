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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glfbdev.h>
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
  int fbdev_dpy;
  struct attributes attribs;
} glutDisplay;

typedef struct {
  int fbdev_win;
  GLFBDevContextPtr glfbdev_ctx;
  void *fbdev_buffer;
  void *glfbdev_buffer;
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

  glut_dpy->fbdev_dpy = init(&glut_dpy->attribs.dpy_width, &glut_dpy->attribs.dpy_height, &err);
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
  GLFBDevVisualPtr glfbdev_visual = NULL;
  struct fb_fix_screeninfo fbdev_finfo;
  struct fb_var_screeninfo fbdev_vinfo;
  int glfbdev_visual_attr[4];
  int i = 0;

  glut_win = calloc(1, sizeof(glutWindow));
  FIU_CHECK(glut_win);
  if (!glut_win) {
    printf("glut_win calloc error\n");
    return 0;
  }

  memcpy(&glut_win->attribs, &glut_dpy->attribs, sizeof(struct attributes));

  memset(&fbdev_finfo, 0, sizeof(struct fb_fix_screeninfo));
  err = ioctl(glut_dpy->fbdev_dpy, FBIOGET_FSCREENINFO, &fbdev_finfo);
  if (err == -1) {
    printf("ioctl FBIOGET_FSCREENINFO error: %s\n", strerror(errno));
    goto error;
  }

  memset(&fbdev_vinfo, 0, sizeof(struct fb_var_screeninfo));
  err = ioctl(glut_dpy->fbdev_dpy, FBIOGET_VSCREENINFO, &fbdev_vinfo);
  if (err == -1) {
    printf("ioctl FBIOGET_VSCREENINFO error: %s\n", strerror(errno));
    goto error;
  }

  memset(glfbdev_visual_attr, 0, sizeof(glfbdev_visual_attr));
  if (glut_win->attribs.double_buffer) {
    glfbdev_visual_attr[i++] = GLFBDEV_DOUBLE_BUFFER;
  }
  if (glut_win->attribs.depth_size) {
    glfbdev_visual_attr[i++] = GLFBDEV_DEPTH_SIZE;
    glfbdev_visual_attr[i++] = glut_win->attribs.depth_size;
  }
  glfbdev_visual_attr[i] = GLFBDEV_NONE;
  glfbdev_visual = glFBDevCreateVisual(&fbdev_finfo, &fbdev_vinfo, glfbdev_visual_attr);
  if (!glfbdev_visual) {
    printf("glFBDevCreateVisual error\n");
    goto error;
  }

  glut_win->fbdev_win = create_window(glut_dpy->fbdev_dpy, glut_win->attribs.win_posx, glut_win->attribs.win_posy, glut_win->attribs.win_width, glut_win->attribs.win_height, 0, &err);
  if (err == -1) {
    goto error;
  }

  glut_win->fbdev_buffer = mmap(NULL, fbdev_finfo.smem_len, PROT_WRITE, MAP_SHARED, glut_dpy->fbdev_dpy, 0);
  if (glut_win->fbdev_buffer == MAP_FAILED) {
    printf("mmap error: %s\n", strerror(errno));
    goto error;
  }

  glut_win->glfbdev_buffer = glFBDevCreateBuffer(&fbdev_finfo, &fbdev_vinfo, glfbdev_visual, glut_win->fbdev_buffer, NULL, fbdev_finfo.smem_len);
  if (!glut_win->glfbdev_buffer) {
    printf("glFBDevCreateBuffer error\n");
    goto error;
  }

  glFBDevSetWindow(glut_win->glfbdev_buffer, (void *)glut_win->fbdev_win);

  glut_win->glfbdev_ctx = glFBDevCreateContext(glfbdev_visual, NULL);
  if (!glut_win->glfbdev_ctx) {
    printf("glFBDevCreateContext error\n");
    goto error;
  }

  if (glut_win->attribs.depth_size) {
    glut_win->attribs.depth_size = glFBDevGetVisualAttrib(glfbdev_visual, GLFBDEV_DEPTH_SIZE);
    if (glut_win->attribs.depth_size == -1) {
      printf("glFBDevGetVisualAttrib error\n");
      goto error;
    }
  }

  glFBDevDestroyVisual(glfbdev_visual);

  return (int)glut_win;

error:
  if (glut_win->glfbdev_ctx) {
    glFBDevDestroyContext(glut_win->glfbdev_ctx);
  }
  if (glut_win->glfbdev_buffer) {
    glFBDevDestroyBuffer(glut_win->glfbdev_buffer);
  }
  if (glut_win->fbdev_buffer) {
    munmap(glut_win->fbdev_buffer, fbdev_finfo.smem_len);
  }
  if (glut_win->fbdev_win) {
    destroy_window(glut_dpy->fbdev_dpy, glut_win->fbdev_win);
  }
  if (glfbdev_visual) {
    glFBDevDestroyVisual(glfbdev_visual);
  }
  free(glut_win);
  return 0;
}

void SetWindow(int display, int window, int context)
{
  int err = 0;
  glutWindow *glut_win = (glutWindow *)window;

  if (context) {
    err = glFBDevMakeCurrent(glut_win->glfbdev_ctx, glut_win->glfbdev_buffer, glut_win->glfbdev_buffer);
  }
  else {
    err = glFBDevMakeCurrent(glut_win->glfbdev_ctx, NULL, NULL);
  }
  if (!err) {
    printf("glFBDevMakeCurrent error\n");
  }
}

void SwapBuffers(int display, int window)
{
  glutWindow *glut_win = (glutWindow *)window;

  glFBDevSwapBuffers(glut_win->glfbdev_buffer);
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
  struct fb_fix_screeninfo fbdev_finfo;

  glFBDevDestroyContext(glut_win->glfbdev_ctx);

  glFBDevDestroyBuffer(glut_win->glfbdev_buffer);

  ioctl(glut_dpy->fbdev_dpy, FBIOGET_FSCREENINFO, &fbdev_finfo);
  munmap(glut_win->fbdev_buffer, fbdev_finfo.smem_len);

  destroy_window(glut_dpy->fbdev_dpy, glut_win->fbdev_win);

  free(glut_win);
}

void Fini(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  fini(glut_dpy->fbdev_dpy);

  free(glut_dpy);
}

int GetEvent(int display, int *type, int *key, int *x, int *y)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  return get_event(glut_dpy->fbdev_dpy, type, key, x, y);
}
