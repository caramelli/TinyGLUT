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
#include "event.h"

static int expose = 0;

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

int create_window(int display, int width, int height, int opt, int *err)
{
  *err = 0;

  return 0;
}

void destroy_window(int display, int window)
{
}

void fini(int display)
{
  expose = 0;
}

int get_event(int display, int *type, int *key)
{
  int ret = 0;

  *type = EVENT_NONE;
  *key = 0;

  if (!expose) {
    *type = EVENT_DISPLAY;
    expose = 1;
    ret = 1;
  }

  return ret;
}
