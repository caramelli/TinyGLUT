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

#include <directfbgl.h>
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
  IDirectFB *directfb_dpy;
  IDirectFBGL *dfbgl_ctx;
  struct attributes attribs;
} glutDisplay;

typedef struct {
  IDirectFBSurface *directfb_win;
  IDirectFBGL *dfbgl_ctx;
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

  glut_dpy->directfb_dpy = (IDirectFB *)(long)init(&glut_dpy->attribs.dpy_width, &glut_dpy->attribs.dpy_height, &err);
  if (err == -1) {
    goto error;
  }

  glut_dpy->attribs.win_width = glut_dpy->attribs.dpy_width;
  glut_dpy->attribs.win_height = glut_dpy->attribs.dpy_height;

  return (long)glut_dpy;

error:
  free(glut_dpy);
  return 0;
}

void InitWindowPosition(int display, int posx, int posy)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  glut_dpy->attribs.win_posx = posx;
  glut_dpy->attribs.win_posy = posy;
}

void InitWindowSize(int display, int width, int height)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  glut_dpy->attribs.win_width = width;
  glut_dpy->attribs.win_height = height;
}

void InitDisplayMode(int display, int double_buffer, int depth_size)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  glut_dpy->attribs.double_buffer = double_buffer;
  glut_dpy->attribs.depth_size = depth_size;
}

void InitContextProfile(int display, int profile)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  if (profile) {
    glut_dpy->attribs.gles_version = 2;
  }
}

int CreateWindow(int display)
{
  int err = 0, opt = 0;
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;
  glutWindow *glut_win = NULL;
  DFBGLAttributes dfbgl_attribs;

  glut_win = calloc(1, sizeof(glutWindow));
  FIU_CHECK(glut_win);
  if (!glut_win) {
    printf("glut_win calloc error\n");
    return 0;
  }

  memcpy(&glut_win->attribs, &glut_dpy->attribs, sizeof(struct attributes));

  if (glut_win->attribs.double_buffer) {
    opt = DSCAPS_DOUBLE;
  }
  if (glut_win->attribs.depth_size) {
    opt |= DSCAPS_DEPTH;
  }

  glut_win->directfb_win = (IDirectFBSurface *)(long)create_window((long)glut_dpy->directfb_dpy, glut_win->attribs.win_posx, glut_win->attribs.win_posy, glut_win->attribs.win_width, glut_win->attribs.win_height, opt, &err);
  if (err == -1) {
    goto error;
  }

  DirectFBSetOption("gles", glut_dpy->attribs.gles_version ? "2" : "0");

  err = glut_win->directfb_win->GetGL(glut_win->directfb_win, &glut_win->dfbgl_ctx);
  if (err) {
    printf("GetGL error\n");
    goto error;
  }

  if (glut_win->attribs.depth_size) {
    memset(&dfbgl_attribs, 0, sizeof(DFBGLAttributes));
    err = glut_win->dfbgl_ctx->GetAttributes(glut_win->dfbgl_ctx, &dfbgl_attribs);
    if (err) {
      printf("GetAttributes error\n");
      goto error;
    }
    else {
      glut_win->attribs.depth_size = dfbgl_attribs.depth_size;
    }
  }

  return (long)glut_win;

error:
  if (glut_win->dfbgl_ctx) {
    glut_win->dfbgl_ctx->Release(glut_win->dfbgl_ctx);
  }
  if (glut_win->directfb_win) {
    glut_win->directfb_win->Release(glut_win->directfb_win);
  }
  free(glut_win);
  return 0;
}

void SetWindow(int display, int window, int context)
{
  int err = 0;
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;
  glutWindow *glut_win = (glutWindow *)(long)window;

  if (context) {
    if (glut_dpy->dfbgl_ctx) {
      err = glut_dpy->dfbgl_ctx->Unlock(glut_dpy->dfbgl_ctx);
    }
    err = glut_win->dfbgl_ctx->Lock(glut_win->dfbgl_ctx);
    glut_dpy->dfbgl_ctx = glut_win->dfbgl_ctx;
  }
  else {
    err = glut_win->dfbgl_ctx->Unlock(glut_win->dfbgl_ctx);
  }
  if (err) {
    printf("Lock/Unlock error\n");
  }
}

void SwapBuffers(int display, int window)
{
  glutWindow *glut_win = (glutWindow *)(long)window;

  glut_win->directfb_win->Flip(glut_win->directfb_win, NULL, DSFLIP_WAITFORSYNC);
}

struct attributes *GetDisplayAttribs(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  return &glut_dpy->attribs;
}

struct attributes *GetWindowAttribs(int window)
{
  glutWindow *glut_win = (glutWindow *)(long)window;

  return &glut_win->attribs;
}

void DestroyWindow(int display, int window)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;
  glutWindow *glut_win = (glutWindow *)(long)window;

  glut_win->dfbgl_ctx->Release(glut_win->dfbgl_ctx);

  destroy_window((long)glut_dpy->directfb_dpy, (long)glut_win->directfb_win);

  free(glut_win);
}

void Fini(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  fini((long)glut_dpy->directfb_dpy);

  free(glut_dpy);
}

int GetEvent(int display, int *type, int *key, int *x, int *y)
{
  glutDisplay *glut_dpy = (glutDisplay *)(long)display;

  return get_event((long)glut_dpy->directfb_dpy, type, key, x, y);
}
