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

#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "attributes.h"
#include "event.h"
#include "glut.h"

#ifdef FIU_ENABLE
#include <fiu-local.h>
#define FIU_CHECK(ptr) \
  fiu_init(0); \
  if (fiu_fail("ENOMEM")) { \
    free(ptr); \
    ptr = NULL; \
  }
#else
#define FIU_CHECK(ptr)
#endif

typedef struct glutList {
  struct glutList *next;
  struct glutList *prev;
} glutList;

typedef struct {
  glutList entry;
  int win;
  void *data;
  void (*reshape_cb)();
  void (*display_cb)();
  void (*keyboard_cb)(unsigned char, int, int);
  void (*special_cb)(int, int, int);
  void (*passive_motion_cb)(int, int);
} glutWindowContext;

static void *backend_handle = NULL;

static int t0 = 0;

static int glut_dpy = 0, glut_win = 0, glut_err = 0, glut_loop = 0;
static glutList glut_win_list = { &glut_win_list, &glut_win_list };

#define DISPLAY_CHECK() \
  if (!glut_dpy) { \
    printf("%s: display is NULL\n", __FUNCTION__); \
    glut_err = GLUT_BAD_DISPLAY; \
  }

#define WINDOW_CHECK() \
  if (!glut_win) { \
    printf("%s: window is NULL\n", __FUNCTION__); \
    glut_err = GLUT_BAD_WINDOW; \
  }

#define WINDOW_CONTEXT_GET(window) \
  glutWindowContext *glut_win_ctx = NULL; \
  glutList *glut_win_entry = NULL; \
  for (glut_win_entry = glut_win_list.next; glut_win_entry != &glut_win_list; glut_win_entry = glut_win_entry->next) { \
    glut_win_ctx = (glutWindowContext *)glut_win_entry; \
    if (glut_win_ctx->win == window) { \
      break; \
    } \
  }

#define WINDOW_SET() \
  if (glut_win != glut_win_ctx->win) { \
    glut_win = glut_win_ctx->win; \
    SetWindowProc(glut_dpy, glut_win, 1); \
  }

static int (*InitProc)() = NULL;
static void (*InitWindowPositionProc)(int, int, int) = NULL;
static void (*InitWindowSizeProc)(int, int, int) = NULL;
static void (*InitDisplayModeProc)(int, int, int) = NULL;
static void (*InitContextProfileProc)(int, int) = NULL;
static int (*CreateWindowProc)(int) = NULL;
static void (*SetWindowProc)(int, int, int) = NULL;
static void (*SwapBuffersProc)(int, int) = NULL;
static struct attributes *(*GetDisplayAttribsProc)(int) = NULL;
static struct attributes *(*GetWindowAttribsProc)(int) = NULL;
static void (*DestroyWindowProc)(int, int) = NULL;
static void (*FiniProc)() = NULL;
static int (*GetEventProc)(int, int *, int *, int *, int *) = NULL;

static void (*IdleCb)() = NULL;

int glutGetError()
{
  return glut_err;
}

void glutInit(int *argc, char **argv)
{
  char backend_path[PATH_MAX];

  glut_err = 0;

  if (glut_dpy) {
    printf("display already initialized\n");
    glut_err = GLUT_DISPLAY_EXIST;
    return;
  }

  if (!getenv("GLUT_BACKEND")) {
    DIR *dir;
    struct dirent *backend;

    dir = opendir(BACKENDSDIR);
    if (!dir) {
      glut_err = GLUT_BAD_BACKEND;
      goto out;
    }

    while ((backend = readdir(dir))) {
      if (backend->d_type == DT_REG && strstr(backend->d_name, ".so")) {
        sprintf(backend_path, "%s/%s", BACKENDSDIR, backend->d_name);
        break;
      }
    }

    closedir(dir);
  }
  else {
    sprintf(backend_path, "%s/%s_plugin.so", BACKENDSDIR, getenv("GLUT_BACKEND"));
  }

  backend_handle = dlopen(backend_path, RTLD_LAZY);
  if (!backend_handle) {
    printf("%s backend not found\n", getenv("GLUT_BACKEND"));
    glut_err = GLUT_BAD_BACKEND;
    goto out;
  }

  #define DLSYM(sym) sym##Proc = dlsym(backend_handle, #sym); \
  if (!sym##Proc) { \
    printf("%s not found\n", #sym); \
    glut_err = GLUT_BAD_BACKEND; \
    goto out; \
  }

  DLSYM(Init);
  DLSYM(InitWindowPosition);
  DLSYM(InitWindowSize);
  DLSYM(InitDisplayMode);
  DLSYM(InitContextProfile);
  DLSYM(CreateWindow);
  DLSYM(SetWindow);
  DLSYM(SwapBuffers);
  DLSYM(GetDisplayAttribs);
  DLSYM(GetWindowAttribs);
  DLSYM(DestroyWindow);
  DLSYM(Fini);
  DLSYM(GetEvent);

  glut_dpy = InitProc();
  if (!glut_dpy) {
    glut_err = GLUT_BAD_DISPLAY;
    goto out;
  }

  return;

out:
  if (backend_handle) {
    dlclose(backend_handle);
    backend_handle = NULL;
  }
}

void glutInitWindowPosition(int posx, int posy)
{
  glut_err = 0;

  DISPLAY_CHECK();
  if (glut_err) {
    return;
  }

  InitWindowPositionProc(glut_dpy, posx, posy);
}

void glutInitWindowSize(int width, int height)
{
  glut_err = 0;

  DISPLAY_CHECK();
  if (glut_err) {
    return;
  }

  InitWindowSizeProc(glut_dpy, width, height);
}

void glutInitDisplayMode(unsigned int mode)
{
  int double_buffer = 0, depth_size = 0;

  glut_err = 0;

  DISPLAY_CHECK();
  if (glut_err) {
    return;
  }

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

void glutInitContextProfile(int profile)
{
  glut_err = 0;

  DISPLAY_CHECK();
  if (glut_err) {
    return;
  }

  InitContextProfileProc(glut_dpy, profile == GLUT_ES_PROFILE ? 1 : 0);
}

int glutCreateWindow(const char *title)
{
  glut_err = 0;
  glutWindowContext *glut_win_ctx = NULL;
  glutList *glut_win_entry = NULL;

  DISPLAY_CHECK();
  if (glut_err) {
    return 0;
  }

  glut_win_ctx = calloc(1, sizeof(glutWindowContext));
  FIU_CHECK(glut_win_ctx);
  if (!glut_win_ctx) {
    printf("glut_win_ctx calloc error\n");
    glut_err = GLUT_BAD_ALLOC;
    return 0;
  }

  glut_win_ctx->win = CreateWindowProc(glut_dpy);
  if (!glut_win_ctx->win) {
    glut_err = GLUT_BAD_WINDOW;
    goto out;
  }

  glut_win_entry = &glut_win_ctx->entry;
  glut_win_entry->next = glut_win_list.next;
  glut_win_entry->prev = &glut_win_list;
  glut_win_list.next->prev = glut_win_entry;
  glut_win_list.next = glut_win_entry;

  glut_win = glut_win_ctx->win;

  SetWindowProc(glut_dpy, glut_win, 1);

  return glut_win;

out:
  free(glut_win_ctx);
  return 0;
}

void glutSetWindow(int window)
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(window);

  if (glut_win_entry == &glut_win_list) {
    printf("Invalid window\n");
    glut_err = GLUT_BAD_VALUE;
    return;
  }

  WINDOW_SET();
}

void glutSetWindowData(void *data)
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->data = data;
}

void *glutGetWindowData()
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return NULL;
  }

  WINDOW_CONTEXT_GET(glut_win);

  return glut_win_ctx->data;
}

void glutReshapeFunc(void (*func)(int, int))
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->reshape_cb = func;
}

void glutDisplayFunc(void (*func)())
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->display_cb = func;
}

void glutIdleFunc(void (*func)())
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  IdleCb = func;
}

void glutKeyboardFunc(void (*func)(unsigned char, int, int))
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->keyboard_cb = func;
}

void glutSpecialFunc(void (*func)(int, int, int))
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->special_cb = func;
}

void glutPassiveMotionFunc(void (*func)(int, int))
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  glut_win_ctx->passive_motion_cb = func;
}

void glutSwapBuffers()
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  SwapBuffersProc(glut_dpy, glut_win);
}

void glutPostRedisplay()
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(glut_win);

  if (glut_win_ctx->display_cb) {
    glut_win_ctx->display_cb();
  }
}

int glutGet(int query)
{
  glut_err = 0;

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
  else if (query == GLUT_SCREEN_WIDTH || query == GLUT_SCREEN_HEIGHT || query == GLUT_INIT_WINDOW_X || query == GLUT_INIT_WINDOW_Y || query == GLUT_INIT_WINDOW_WIDTH || query == GLUT_INIT_WINDOW_HEIGHT || query == GLUT_INIT_DISPLAY_MODE) {
    DISPLAY_CHECK();
    if (glut_err) {
      return 0;
    }

    struct attributes *attribs = GetDisplayAttribsProc(glut_dpy);
    int mode = 0;

    switch (query) {
      case GLUT_SCREEN_WIDTH: return attribs->dpy_width;
      case GLUT_SCREEN_HEIGHT: return attribs->dpy_height;
      case GLUT_INIT_WINDOW_X: return attribs->win_posx;
      case GLUT_INIT_WINDOW_Y: return attribs->win_posy;
      case GLUT_INIT_WINDOW_WIDTH: return attribs->win_width;
      case GLUT_INIT_WINDOW_HEIGHT: return attribs->win_height;
      case GLUT_INIT_DISPLAY_MODE:
        if (attribs->double_buffer) mode |= GLUT_DOUBLE;
        if (attribs->depth_size) mode |= GLUT_DEPTH;
        return mode;
    }
  }
  else if (query == GLUT_WINDOW_X || query == GLUT_WINDOW_Y || query == GLUT_WINDOW_WIDTH || query == GLUT_WINDOW_HEIGHT || query == GLUT_WINDOW_DOUBLEBUFFER || query == GLUT_WINDOW_DEPTH_SIZE) {
    WINDOW_CHECK();
    if (glut_err) {
      return 0;
    }

    struct attributes *attribs = GetWindowAttribsProc(glut_win);

    switch (query) {
      case GLUT_WINDOW_X: return attribs->win_posx;
      case GLUT_WINDOW_Y: return attribs->win_posy;
      case GLUT_WINDOW_WIDTH: return attribs->win_width;
      case GLUT_WINDOW_HEIGHT: return attribs->win_height;
      case GLUT_WINDOW_DOUBLEBUFFER: return attribs->double_buffer;
      case GLUT_WINDOW_DEPTH_SIZE: return attribs->depth_size;
    }
  }
  else {
    glut_err = GLUT_BAD_VALUE;
  }

  return 0;
}

void glutDestroyWindow(int window)
{
  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  WINDOW_CONTEXT_GET(window);

  if (glut_win_entry == &glut_win_list) {
    printf("Invalid window\n");
    glut_err = GLUT_BAD_VALUE;
    return;
  }

  glut_win_entry->next->prev = glut_win_entry->prev;
  glut_win_entry->prev->next = glut_win_entry->next;

  if (glut_win_list.next == &glut_win_list) {
    glut_win = 0;
  }
  else if (glut_win == glut_win_ctx->win) {
    glut_win = ((glutWindowContext *)glut_win_list.next)->win;
  }

  SetWindowProc(glut_dpy, glut_win_ctx->win, 0);

  DestroyWindowProc(glut_dpy, glut_win_ctx->win);

  free(glut_win_ctx);

  if (glut_win) {
    SetWindowProc(glut_dpy, glut_win, 1);
  }
}

void glutExit()
{
  glut_err = 0;

  DISPLAY_CHECK();
  if (glut_err) {
    return;
  }

  if (glut_win_list.next != &glut_win_list) {
    printf("window is not NULL\n");
    glut_err = GLUT_WINDOW_EXIST;
    return;
  }

  FiniProc(glut_dpy);
  glut_dpy = 0;
  t0 = 0;
  dlclose(backend_handle);
  backend_handle = NULL;
}

void glutLeaveMainLoop()
{
  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  glut_loop = 0;
}

void glutMainLoop()
{
  int type = EVENT_NONE, key = 0, x = 0, y = 0;
  glutWindowContext *glut_win_ctx = NULL;
  glutList *glut_win_entry = NULL;

  glut_err = 0;

  WINDOW_CHECK();
  if (glut_err) {
    return;
  }

  for (glut_win_entry = glut_win_list.next; glut_win_entry != &glut_win_list; glut_win_entry = glut_win_entry->next) {
    glut_win_ctx = (glutWindowContext *)glut_win_entry;
    if (glut_win_ctx->reshape_cb) {
      WINDOW_SET();
      struct attributes *attribs = GetWindowAttribsProc(glut_win);
      glut_win_ctx->reshape_cb(attribs->win_width, attribs->win_height);
    }
  }

  glut_loop = 1;

  while (glut_loop && glut_win) {
    int native_win = GetEventProc(glut_dpy, &type, &key, &x, &y);
    if (native_win) {
      for (glut_win_entry = glut_win_list.next; glut_win_entry != &glut_win_list; glut_win_entry = glut_win_entry->next) {
        glut_win_ctx = (glutWindowContext *)glut_win_entry;
        if (*(int *)(long)glut_win_ctx->win == native_win)
          break;
      }
      switch (type) {
        case EVENT_DISPLAY:
          if (glut_win_ctx->display_cb) {
            WINDOW_SET();
            glut_win_ctx->display_cb();
          }
          break;
        case EVENT_KEYBOARD:
          if (glut_win_ctx->keyboard_cb) {
            WINDOW_SET();
            glut_win_ctx->keyboard_cb(key, 0, 0);
          }
          break;
        case EVENT_SPECIAL:
          if (glut_win_ctx->special_cb) {
            WINDOW_SET();
            glut_win_ctx->special_cb(key, 0, 0);
          }
          break;
        case EVENT_PASSIVEMOTION:
          if (glut_win_ctx->passive_motion_cb) {
            WINDOW_SET();
            glut_win_ctx->passive_motion_cb(x, y);
          }
          break;
        default:
          break;
      }
    }
    else if (IdleCb) {
      IdleCb();
    }
  }

  if (glut_loop) {
    FiniProc(glut_dpy);
    glut_dpy = 0;
    t0 = 0;
    dlclose(backend_handle);
    backend_handle = NULL;
  }
}
