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
#include <EGL/egl.h>
#include "attributes.h"

typedef struct {
  EGLNativeDisplayType native_dpy;
  EGLDisplay egl_dpy;
  struct attributes attribs;
  void *platform;
  int (*init)(int *width, int *height, int *err);
  int (*create_window)(int dpy, int width, int height, int opt, int *err);
  void (*destroy_window)(int dpy, int win);
  void (*fini)(int dpy);
  int (*get_event)(int dpy, int *type, int *key);
} glutDisplay;

typedef struct {
  EGLNativeWindowType native_win;
  EGLSurface egl_win;
  EGLContext egl_ctx;
  struct attributes attribs;
} glutWindow;

int Init()
{
  int err = 0;
  char platform_path[PATH_MAX];
  glutDisplay *glut_dpy = NULL;
  EGLint egl_glapi = 0;

  glut_dpy = calloc(1, sizeof(glutDisplay));
  if (!glut_dpy) {
    printf("glut_dpy calloc error\n");
    return 0;
  }

  if (!getenv("EGL_PLATFORM")) {
    printf("EGL_PLATFORM is not set\n");
    goto error;
  }
  else {
    sprintf(platform_path, "%s/%s.so", PLATFORMSDIR, getenv("EGL_PLATFORM"));
  }

  if (!getenv("EGL_GLAPI")) {
    printf("EGL_GLAPI is not set\n");
    goto error;
  }
  else {
    if (!strcmp(getenv("EGL_GLAPI"), "gl")) {
      egl_glapi = EGL_OPENGL_API;
    }
    else if (!strcmp(getenv("EGL_GLAPI"), "glesv1_cm") || !strcmp(getenv("EGL_GLAPI"), "glesv2")) {
      egl_glapi = EGL_OPENGL_ES_API;
    }
    else {
      printf("Bad engine\n");
      goto error;
    }
  }

  glut_dpy->platform = dlopen(platform_path, RTLD_LAZY);
  if (!glut_dpy->platform) {
    printf("dlopen %s error\n", getenv("EGL_PLATFORM"));
    goto error;
  }

  #define FINDSYM(sym) glut_dpy->sym = dlsym(glut_dpy->platform, #sym); \
  if (!glut_dpy->sym) { \
    printf("dlsym %s error\n", #sym); \
    goto error; \
  }

  FINDSYM(init);
  FINDSYM(create_window);
  FINDSYM(get_event);
  FINDSYM(destroy_window);
  FINDSYM(fini);

  glut_dpy->native_dpy = (EGLNativeDisplayType)glut_dpy->init(&glut_dpy->attribs.dpy_width, &glut_dpy->attribs.dpy_height, &err);
  if (err == -1) {
    goto error;
  }

  glut_dpy->attribs.win_width = glut_dpy->attribs.dpy_width;
  glut_dpy->attribs.win_height = glut_dpy->attribs.dpy_height;

  glut_dpy->egl_dpy = eglGetDisplay(glut_dpy->native_dpy);
  if (!glut_dpy->egl_dpy) {
    printf("eglGetDisplay error: 0x%x\n", eglGetError());
    goto error;
  }

  err = eglInitialize(glut_dpy->egl_dpy, NULL, NULL);
  if (!err) {
    printf("eglInitialize error: 0x%x\n", eglGetError());
    goto error;
  }

  err = eglBindAPI(egl_glapi);
  if (!err) {
    printf("eglBindAPI error: 0x%x\n", eglGetError());
    goto error;
  }

  return (int)glut_dpy;

error:
  if (glut_dpy->egl_dpy) {
    eglTerminate(glut_dpy->egl_dpy);
  }
  if (glut_dpy->native_dpy) {
    glut_dpy->fini((int)glut_dpy->native_dpy);
  }
  if (glut_dpy->platform) {
    dlclose(glut_dpy->platform);
  }
  free(glut_dpy);
  return 0;
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
  EGLConfig egl_config = NULL;
  EGLint egl_config_attr[5], egl_win_attr[3], egl_ctx_attr[3];
  EGLint i = 0, egl_renderable_type = 0, egl_gles_version = 0;

  glut_win = calloc(1, sizeof(glutWindow));
  if (!glut_win) {
    printf("glut_win calloc error\n");
    return 0;
  }

  memcpy(&glut_win->attribs, &glut_dpy->attribs, sizeof(struct attributes));

  if (!getenv("EGL_GLAPI")) {
    printf("EGL_GLAPI is not set\n");
    goto error;
  }
  else {
    if (!strcmp(getenv("EGL_GLAPI"), "gl")) {
      egl_renderable_type = EGL_OPENGL_BIT;
    }
    else if (!strcmp(getenv("EGL_GLAPI"), "glesv1_cm")) {
      egl_renderable_type = EGL_OPENGL_ES_BIT;
      egl_gles_version = 1;
    }
    else if (!strcmp(getenv("EGL_GLAPI"), "glesv2")) {
      egl_renderable_type = EGL_OPENGL_ES2_BIT;
      egl_gles_version = 2;
    }
    else {
      printf("Bad engine\n");
      goto error;
    }
  }

  memset(egl_config_attr, 0, sizeof(egl_config_attr));
  if (glut_win->attribs.depth_size) {
    egl_config_attr[i++] = EGL_DEPTH_SIZE;
    egl_config_attr[i++] = glut_win->attribs.depth_size;
  }
  egl_config_attr[i++] = EGL_RENDERABLE_TYPE;
  egl_config_attr[i++] = egl_renderable_type;
  egl_config_attr[i] = EGL_NONE;
  err = eglChooseConfig(glut_dpy->egl_dpy, egl_config_attr, &egl_config, 1, &i);
  if (!err) {
    printf("eglChooseConfig error: 0x%x\n", eglGetError());
    goto error;
  }

  glut_win->native_win = (EGLNativeWindowType)glut_dpy->create_window((int)glut_dpy->native_dpy, glut_win->attribs.win_width, glut_win->attribs.win_height, 0, &err);
  if (err == -1) {
    goto error;
  }

  memset(egl_win_attr, 0, sizeof(egl_win_attr));
  egl_win_attr[0] = EGL_RENDER_BUFFER;
  if (glut_win->attribs.double_buffer) {
    egl_win_attr[1] = EGL_BACK_BUFFER;
  }
  else {
    egl_win_attr[1] = EGL_SINGLE_BUFFER;
  }
  egl_win_attr[2] = EGL_NONE;
  glut_win->egl_win = eglCreateWindowSurface(glut_dpy->egl_dpy, egl_config, glut_win->native_win, egl_win_attr);
  if (!glut_win->egl_win) {
    printf("eglCreateWindowSurface error: 0x%x\n", eglGetError());
    goto error;
  }

  i = 0;
  memset(egl_ctx_attr, 0, sizeof(egl_ctx_attr));
  if (egl_gles_version == 2) {
    egl_ctx_attr[i++] = EGL_CONTEXT_CLIENT_VERSION;
    egl_ctx_attr[i++] = egl_gles_version;
  }
  egl_ctx_attr[i] = EGL_NONE;

  glut_win->egl_ctx = eglCreateContext(glut_dpy->egl_dpy, egl_config, EGL_NO_CONTEXT, egl_ctx_attr);
  if (!glut_win->egl_ctx) {
    printf("eglCreateContext error: 0x%x\n", eglGetError());
    goto error;
  }

  if (glut_win->attribs.depth_size) {
    err = eglGetConfigAttrib(glut_dpy->egl_dpy, egl_config, EGL_DEPTH_SIZE, &glut_win->attribs.depth_size);
    if (!err) {
      printf("eglGetConfigAttrib error: 0x%x\n", eglGetError());
    }
  }

  err = eglMakeCurrent(glut_dpy->egl_dpy, glut_win->egl_win, glut_win->egl_win, glut_win->egl_ctx);
  if (!err) {
    printf("eglMakeCurrent error: 0x%x\n", eglGetError());
    goto error;
  }

  return (int)glut_win;

error:
  if (glut_win->egl_ctx) {
    eglDestroyContext(glut_dpy->egl_dpy, glut_win->egl_ctx);
  }
  if (glut_win->egl_win) {
    eglDestroySurface(glut_dpy->egl_dpy, glut_win->egl_win);
  }
  if (glut_win->native_win) {
    glut_dpy->destroy_window((int)glut_dpy->native_dpy, (int)glut_win->native_win);
  }
  free(glut_win);
  return 0;
}

void SwapBuffers(int display, int window)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;
  glutWindow *glut_win = (glutWindow *)window;

  eglSwapBuffers(glut_dpy->egl_dpy, glut_win->egl_win);
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

  eglMakeCurrent(glut_dpy->egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  eglDestroyContext(glut_dpy->egl_dpy, glut_win->egl_ctx);

  eglDestroySurface(glut_dpy->egl_dpy, glut_win->egl_win);

  glut_dpy->destroy_window((int)glut_dpy->native_dpy, (int)glut_win->native_win);

  free(glut_win);
}

void Fini(int display)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  eglTerminate(glut_dpy->egl_dpy);

  glut_dpy->fini((int)glut_dpy->native_dpy);

  dlclose(glut_dpy->platform);

  free(glut_dpy);
}

int GetEvent(int display, int *type, int *key)
{
  glutDisplay *glut_dpy = (glutDisplay *)display;

  return glut_dpy->get_event((int)glut_dpy->native_dpy, type, key);
}
