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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "event.h"
#include "keys.h"

struct wl_window {
  long version;
  int width;
  int height;
  int dx;
  int dy;
  int attached_width;
  int attached_height;
  void *private;
  void (*resize_callback)(struct wl_window *, void *);
  void (*destroy_window_callback)(void *);
  struct wl_surface *surface;
  struct wl_shell_surface *shell_surface;
};

enum {
  WL_EVENT_EXPOSE,
  WL_EVENT_KEYBOARD,
  WL_EVENT_POINTER
};

struct wl_event {
  int type;
  xkb_keysym_t keysym;
  int x;
  int y;
  struct wl_window *window;
  struct wl_list link;
};

struct wl_user_data {
  int width;
  int height;
  struct wl_registry *registry;
  struct wl_output *output;
  struct wl_compositor *compositor;
  struct wl_shell *shell;
  struct wl_seat *seat;
  struct wl_keyboard *keyboard;
  struct xkb_context *context;
  struct xkb_keymap *keymap;
  struct xkb_state *state;
  struct wl_pointer *pointer;
  struct wl_window *window;
  struct wl_list event_list;
};

static void wl_output_handle_geometry(void *data, struct wl_output *output, int x, int y, int physical_width, int physical_height, int subpixel, const char *make, const char *model, int transform)
{
}

static void wl_output_handle_mode(void *data, struct wl_output *output, unsigned int flags, int width, int height, int refresh)
{
  struct wl_user_data *user_data = data;

  user_data->width = width;
  user_data->height = height;
}

static void wl_output_handle_done(void *data, struct wl_output *output)
{
}

static void wl_output_handle_scale(void *data, struct wl_output *output, int factor)
{
}

static struct wl_output_listener wl_output_listener = {
  wl_output_handle_geometry,
  wl_output_handle_mode,
  wl_output_handle_done,
  wl_output_handle_scale
};

static void wl_keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, unsigned int format, int fd, unsigned int size)
{
  struct wl_user_data *user_data = data;
  char *string;

  string = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  user_data->keymap = xkb_map_new_from_string(user_data->context, string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_MAP_COMPILE_NO_FLAGS);
  munmap(string, size);
  close(fd);

  user_data->state = xkb_state_new(user_data->keymap);
}

static void wl_keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, unsigned int serial, struct wl_surface *surface, struct wl_array *keys)
{
  struct wl_user_data *user_data = data;

  user_data->window = wl_surface_get_user_data(surface);
}

static void wl_keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, unsigned int serial, struct wl_surface *surface)
{
}

static void wl_keyboard_handle_key(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int time, unsigned int key, unsigned int state)
{
  struct wl_user_data *user_data = data;
  const xkb_keysym_t *syms;
  struct wl_event *event = NULL;

  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    return;

  xkb_key_get_syms(user_data->state, key + 8, &syms);

  event = calloc(1, sizeof(struct wl_event));
  if (!event) {
    printf("event calloc failed\n");
    return;
  }

  event->type = WL_EVENT_KEYBOARD;
  event->keysym = syms[0];
  event->window = user_data->window;
  wl_list_insert(&user_data->event_list, &event->link);
}

static void wl_keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int mods_depressed, unsigned int mods_latched, unsigned int mods_locked, unsigned int group)
{
}

static struct wl_keyboard_listener wl_keyboard_listener = {
  wl_keyboard_handle_keymap,
  wl_keyboard_handle_enter,
  wl_keyboard_handle_leave,
  wl_keyboard_handle_key,
  wl_keyboard_handle_modifiers
};

static void wl_pointer_handle_enter(void *data, struct wl_pointer *pointer, unsigned int serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
  struct wl_user_data *user_data = data;

  user_data->window = wl_surface_get_user_data(surface);
}

static void wl_pointer_handle_leave(void *data, struct wl_pointer *pointer, unsigned int serial, struct wl_surface *surface)
{
}

static void wl_pointer_handle_motion(void *data, struct wl_pointer *pointer, unsigned int time, wl_fixed_t sx, wl_fixed_t sy)
{
  struct wl_user_data *user_data = data;
  struct wl_event *event = NULL;

  event = calloc(1, sizeof(struct wl_event));
  if (!event) {
    printf("event calloc failed\n");
    return;
  }

  event->type = WL_EVENT_POINTER;
  event->x = wl_fixed_to_int(sx);
  event->y = wl_fixed_to_int(sy);
  event->window = user_data->window;
  wl_list_insert(&user_data->event_list, &event->link);
}

static void wl_pointer_handle_button(void *data, struct wl_pointer *pointer, unsigned int serial, unsigned int time, unsigned int button, unsigned int state)
{
}

static void wl_pointer_handle_axis(void *data, struct wl_pointer *pointer, unsigned int time, unsigned int axis, wl_fixed_t value)
{
}

static struct wl_pointer_listener wl_pointer_listener = {
  wl_pointer_handle_enter,
  wl_pointer_handle_leave,
  wl_pointer_handle_motion,
  wl_pointer_handle_button,
  wl_pointer_handle_axis
};

static void wl_seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
  struct wl_user_data *user_data = data;

  user_data->keyboard = wl_seat_get_keyboard(user_data->seat);
  wl_keyboard_add_listener(user_data->keyboard, &wl_keyboard_listener, user_data);

  user_data->pointer = wl_seat_get_pointer(user_data->seat);
  wl_pointer_add_listener(user_data->pointer, &wl_pointer_listener, user_data);
}

static void wl_seat_handle_name(void *data, struct wl_seat *seat, const char *name)
{
}

static struct wl_seat_listener wl_seat_listener = {
  wl_seat_handle_capabilities,
  wl_seat_handle_name
};

static void wl_registry_handle_global(void *data, struct wl_registry *registry, unsigned int name, const char *interface, unsigned int version)
{
  int ret = 0;
  struct wl_user_data *user_data = data;

  if (!strcmp(interface, "wl_output")) {
    user_data->output = wl_registry_bind(registry, name, &wl_output_interface, 1);
    wl_output_add_listener(user_data->output, &wl_output_listener, user_data);
    if (ret == -1) {
      printf("wl_output_add_listener failed\n");
    }
  }
  else if (!strcmp(interface, "wl_compositor")) {
    user_data->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
  }
  else if (!strcmp(interface, "wl_shell")) {
    user_data->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
  }
  else if (!strcmp(interface, "wl_seat")) {
    user_data->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    ret = wl_seat_add_listener(user_data->seat, &wl_seat_listener, user_data);
    if (ret == -1) {
      printf("wl_seat_add_listener failed\n");
    }
  }
}

static void wl_registry_handle_global_remove(void *data, struct wl_registry *registry, unsigned int name)
{
}

static struct wl_registry_listener registry_listener = {
  wl_registry_handle_global,
  wl_registry_handle_global_remove
};

int init(int *width, int *height, int *err)
{
  int ret = 0;
  struct wl_display *display = NULL;
  struct wl_user_data *user_data = NULL;

  display = wl_display_connect(NULL);
  if (!display) {
    printf("wl_display_connect failed\n");
    goto fail;
  }

  user_data = calloc(1, sizeof(struct wl_user_data));
  if (!user_data) {
    printf("user_data calloc failed\n");
    goto fail;
  }

  wl_display_set_user_data(display, user_data);

  user_data->registry = wl_display_get_registry(display);
  if (!user_data->registry) {
    printf("wl_display_get_registry failed\n");
    goto fail;
  }

  ret = wl_registry_add_listener(user_data->registry, &registry_listener, user_data);
  if (ret == -1) {
    printf("wl_registry_add_listener failed\n");
    goto fail;
  }

  ret = wl_display_dispatch(display);
  if (ret == -1) {
    printf("wl_display_dispatch failed\n");
    goto fail;
  }

  ret = wl_display_roundtrip(display);
  if (ret == -1) {
    printf("wl_display_roundtrip failed\n");
    goto fail;
  }

  user_data->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!user_data->context) {
    printf("xkb_context_new failed\n");
    goto fail;
  }

  wl_list_init(&user_data->event_list);

  *width = user_data->width;
  *height = user_data->height;

  *err = 0;

  return (long)display;

fail:
  if (user_data) {
    if (user_data->registry) {
      wl_registry_destroy(user_data->registry);
    }
    free(user_data);
  }
  if (display) {
    wl_display_disconnect(display);
  }
  *err = -1;
  return 0;
}

int create_window(int dpy, int posx, int posy, int width, int height, int opt, int *err)
{
  struct wl_display *display = (struct wl_display *)(long)dpy;
  struct wl_user_data *user_data = wl_display_get_user_data(display);
  struct wl_window *window = NULL;
  struct wl_event *event = NULL;

  window = calloc(1, sizeof(struct wl_window));
  if (!window) {
    printf("wl_window calloc failed\n");
    goto fail;
  }
  else {
    window->width = width;
    window->height = height;
  }

  window->surface = wl_compositor_create_surface(user_data->compositor);
  if (!window->surface) {
    printf("wl_compositor_create_surface failed\n");
    goto fail;
  }
  else {
    wl_surface_set_user_data(window->surface, window);
  }

  window->shell_surface = wl_shell_get_shell_surface(user_data->shell, window->surface);
  if (!window->shell_surface) {
    printf("wl_shell_get_shell_surface failed\n");
    goto fail;
  }

  wl_shell_surface_set_toplevel(window->shell_surface);

  wl_shell_surface_set_position(window->shell_surface, posx, posy);

  event = calloc(1, sizeof(struct wl_event));
  if (!event) {
    printf("event calloc failed\n");
    goto fail;
  }

  event->type = WL_EVENT_EXPOSE;
  event->window = window;
  wl_list_insert(&user_data->event_list, &event->link);

  *err = 0;

  return (long)window;

fail:
  if (window) {
    if (window->shell_surface) {
      wl_shell_surface_destroy(window->shell_surface);
    }
    if (window->surface) {
      wl_surface_destroy(window->surface);
    }
    free(window);
  }
  *err = -1;
  return 0;
}

void destroy_window(int dpy, int win)
{
  struct wl_window *window = (struct wl_window *)(long)win;

  wl_shell_surface_destroy(window->shell_surface);
  wl_surface_destroy(window->surface);
  free(window);
}

void fini(int dpy)
{
  struct wl_display *display = (struct wl_display *)(long)dpy;
  struct wl_user_data *user_data = wl_display_get_user_data(display);
  struct wl_event *event, *tmp;

  wl_list_for_each_safe(event, tmp, &user_data->event_list, link) {
    wl_list_remove(&event->link);
    free(event);
  }
  if (user_data->pointer) wl_pointer_destroy(user_data->pointer);
  if (user_data->state) xkb_state_unref(user_data->state);
  if (user_data->keymap) xkb_map_unref(user_data->keymap);
  if (user_data->keyboard) wl_keyboard_destroy(user_data->keyboard);
  if (user_data->seat) wl_seat_destroy(user_data->seat);
  if (user_data->shell) wl_shell_destroy(user_data->shell);
  if (user_data->compositor) wl_compositor_destroy(user_data->compositor);
  if (user_data->output) wl_output_destroy(user_data->output);
  xkb_context_unref(user_data->context);
  wl_registry_destroy(user_data->registry);
  free(user_data);
  wl_display_disconnect(display);
}

int get_event(int dpy, int *type, int *key, int *x, int *y)
{
  struct wl_display *display = (struct wl_display *)(long)dpy;
  struct wl_user_data *user_data = NULL;
  struct wl_event *event = NULL;
  struct wl_list *event_link = NULL;
  int win = 0;

  user_data = wl_display_get_user_data(display);

  *type = EVENT_NONE;
  *key = *x = *y = 0;

  wl_display_dispatch_pending(display);

  event_link = &user_data->event_list;
  while (event_link->next != &user_data->event_list) {
    event_link = event_link->next;
  }

  if (event_link != &user_data->event_list) {
    event = (struct wl_event *)((char *)event_link  - (char *)&((struct wl_event *)NULL)->link);
    if (event->type == WL_EVENT_EXPOSE) {
      *type = EVENT_DISPLAY;
    }
    else if (event->type == WL_EVENT_KEYBOARD) {
      switch (event->keysym) {
        case XKB_KEY_F1:        *key = F1;        break;
        case XKB_KEY_F2:        *key = F2;        break;
        case XKB_KEY_F3:        *key = F3;        break;
        case XKB_KEY_F4:        *key = F4;        break;
        case XKB_KEY_F5:        *key = F5;        break;
        case XKB_KEY_F6:        *key = F6;        break;
        case XKB_KEY_F7:        *key = F7;        break;
        case XKB_KEY_F8:        *key = F8;        break;
        case XKB_KEY_F9:        *key = F9;        break;
        case XKB_KEY_F10:       *key = F10;       break;
        case XKB_KEY_F11:       *key = F11;       break;
        case XKB_KEY_F12:       *key = F12;       break;
        case XKB_KEY_Left:      *key = LEFT;      break;
        case XKB_KEY_Up:        *key = UP;        break;
        case XKB_KEY_Right:     *key = RIGHT;     break;
        case XKB_KEY_Down:      *key = DOWN;      break;
        case XKB_KEY_Page_Up:   *key = PAGE_UP;   break;
        case XKB_KEY_Page_Down: *key = PAGE_DOWN; break;
        default:                *key = 0;         break;
      }
      if (!*key) {
        char string[7];
        xkb_keysym_to_utf8(event->keysym, (char *)&string, sizeof(string));
        *key = string[0];
        *type = EVENT_KEYBOARD;
      }
      else {
        *type = EVENT_SPECIAL;
      }
    }
    else if (event->type == WL_EVENT_POINTER) {
      *x = event->x;
      *y = event->y;
      *type = EVENT_PASSIVEMOTION;
    }
  }

  if (*type) {
    win = (long)event->window;
    wl_list_remove(&event->link);
    free(event);
  }

  return win;
}
