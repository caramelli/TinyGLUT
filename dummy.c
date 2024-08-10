/*
  TinyGLUT                 Small implementation of GLUT (OpenGL Utility Toolkit)
  Copyright (c) 2015-2021, Nicolas Caramelli
  
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

#include <stdio.h>
#include <stdlib.h>
#include "event.h"

static int expose;

int init(int *width, int *height, int *err)
{
  if (!getenv("WIDTH") || !getenv("HEIGHT")) {
    printf("WIDTH or HEIGHT is not set\n");
    goto fail;
  }

  *width = atoi(getenv("WIDTH"));
  *height = atoi(getenv("HEIGHT"));

  *err = 0;

  return 0;

fail:
  *err = -1;
  return 0;
}

int create_window(int display, int posx, int posy, int width, int height, int opt, int *err)
{
  expose = 0;

  *err = 0;

  return 0;
}

void destroy_window(int display, int window)
{
  expose = 0;
}

void fini(int display)
{
}

int get_event(int display, int *type, int *key, int *x, int *y)
{
  int ret = 0;

  *type = EVENT_NONE;
  *key = *x = *y = 0;

  if (!expose) {
    *type = EVENT_DISPLAY;
    expose = 1;
    ret = 1;
  }

  return ret;
}
