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

#include <check.h>
#include <fcntl.h>
#include <fiu-control.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/uinput.h>
#include "glut.h"

static int glut_win = 0, uinput_keyboard = 0, uinput_mouse = 0;

static void sighandler_quit(int signum)
{
  glutLeaveMainLoop();
}

static void sighandler_input(int signum)
{
  struct input_event event;

  memset(&event, 0, sizeof(struct input_event));

  event.type = EV_REL;
  event.code = REL_X;
  event.value = 1;
  write(uinput_mouse, &event, sizeof(struct input_event));
  event.code = REL_Y;
  event.value = 1;
  write(uinput_mouse, &event, sizeof(struct input_event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  write(uinput_mouse, &event, sizeof(struct input_event));

  event.type = EV_KEY;
  event.code = KEY_F1;
  event.value = 1;
  write(uinput_keyboard, &event, sizeof(struct input_event));
  event.value = 0;
  write(uinput_keyboard, &event, sizeof(struct input_event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  write(uinput_keyboard, &event, sizeof(struct input_event));

  event.type = EV_KEY;
  event.code = KEY_ESC;
  event.value = 1;
  write(uinput_keyboard, &event, sizeof(struct input_event));
  event.value = 0;
  write(uinput_keyboard, &event, sizeof(struct input_event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  write(uinput_keyboard, &event, sizeof(struct input_event));
}

static void glutReshape(int width, int height)
{
}

static void glutDisplay()
{
}

static void glutIdle()
{
}

static void glutKeyboard(unsigned char key, int x, int y)
{
  switch (key) {
    case 27:
      glutDestroyWindow(glut_win);
      break;
  }
}

static void glutSpecial(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_F1:
      printf("F1 key\n");
      break;
  }
}

static void glutPassiveMotion(int x, int y)
{
  printf("x = %d, y = %d\n", x, y);
}

/* glutInit test */

START_TEST(test_glutInit)
{
  glutInit(NULL, NULL);
  glutInit(NULL, NULL);
  ck_assert_int_eq(glutGetError(), GLUT_DISPLAY_EXIST);
  glutExit();

  setenv("GLUT_BACKEND", "none", 1);
  glutInit(NULL, NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_BACKEND);
  unsetenv("GLUT_BACKEND");

  fiu_enable("BACKEND_ENOMEM", 1, NULL, FIU_ONETIME);
  glutInit(NULL, NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  glutExit();
}
END_TEST

/* glutInitWindowPosition test */

START_TEST(test_glutInitWindowPosition)
{
  glutInitWindowPosition(0, 0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  glutInitWindowPosition(0, 0);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutExit();
}
END_TEST

/* glutInitWindowSize test */

START_TEST(test_glutInitWindowSize)
{
  glutInitWindowSize(800, 600);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  glutInitWindowSize(800, 600);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutExit();
}
END_TEST

/* glutInitDisplayMode test */

START_TEST(test_glutInitDisplayMode)
{
  glutInitDisplayMode(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  setenv("DEPTH_SIZE", "24", 1);
  glutInitDisplayMode(GLUT_DEPTH);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  unsetenv("DEPTH_SIZE");

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutExit();
}
END_TEST

/* glutCreateWindow test */

START_TEST(test_glutCreateWindow)
{
  glutCreateWindow(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  fiu_enable("ENOMEM", 1, NULL, FIU_ONETIME);
  glutCreateWindow(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_ALLOC);

  fiu_enable("BACKEND_ENOMEM", 1, NULL, FIU_ONETIME);
  glutCreateWindow(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glut_win = glutCreateWindow(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  glutDestroyWindow(glut_win);

  glutExit();
}
END_TEST

/* glutSetWindow test */

START_TEST(test_glutSetWindow)
{
  glutSetWindow(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);
  int glut_win2 = glutCreateWindow(NULL);

  glutSetWindow(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_VALUE);

  glutSetWindow(glut_win);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutDestroyWindow(glut_win2);
  glutExit();
}
END_TEST

/* glutSetWindowData test */

START_TEST(test_glutSetWindowData)
{
  glutSetWindowData(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutSetWindowData(NULL);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutGetWindowData test */

START_TEST(test_glutGetWindowData)
{
  glutGetWindowData();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutGetWindowData();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutReshapeFunc test */

START_TEST(test_glutReshapeFunc)
{
  glutReshapeFunc(glutReshape);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutReshapeFunc(glutReshape);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutDisplayFunc test */

START_TEST(test_glutDisplayFunc)
{
  glutDisplayFunc(glutDisplay);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutDisplayFunc(glutDisplay);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutIdleFunc test */

START_TEST(test_glutIdleFunc)
{
  glutIdleFunc(glutIdle);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutIdleFunc(glutIdle);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutKeyboardFunc test */

START_TEST(test_glutKeyboardFunc)
{
  glutKeyboardFunc(glutKeyboard);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutKeyboardFunc(glutKeyboard);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutSpecialFunc test */

START_TEST(test_glutSpecialFunc)
{
  glutSpecialFunc(glutSpecial);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutSpecialFunc(glutSpecial);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutPassiveMotionFunc test */

START_TEST(test_glutPassiveMotionFunc)
{
  glutPassiveMotionFunc(glutPassiveMotion);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutPassiveMotionFunc(glutPassiveMotion);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutSwapBuffers test */

START_TEST(test_glutSwapBuffers)
{
  glutSwapBuffers();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutSwapBuffers();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutPostRedisplay test */

START_TEST(test_glutPostRedisplay)
{
  glutPostRedisplay();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutDisplayFunc(glutDisplay);
  glutPostRedisplay();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutGet test */

START_TEST(test_glutGet)
{
  glutGet(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_VALUE);

  glutGet(GLUT_ELAPSED_TIME);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  sleep(1);
  glutGet(GLUT_ELAPSED_TIME);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_DISPLAY_MODE);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  glutGet(GLUT_SCREEN_WIDTH);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_SCREEN_HEIGHT);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_WINDOW_X);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_WINDOW_Y);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_WINDOW_WIDTH);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_WINDOW_HEIGHT);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_INIT_DISPLAY_MODE);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_DEPTH_SIZE);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glut_win = glutCreateWindow(NULL);

  glutGet(GLUT_WINDOW_X);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_Y);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_WIDTH);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_HEIGHT);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_DOUBLEBUFFER);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutGet(GLUT_WINDOW_DEPTH_SIZE);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutDestroyWindow test */

START_TEST(test_glutDestroyWindow)
{
  glutDestroyWindow(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutDestroyWindow(0);
  ck_assert_int_eq(glutGetError(), GLUT_BAD_VALUE);

  glutDestroyWindow(glut_win);
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutExit();
}
END_TEST

/* glutExit test */

START_TEST(test_glutExit)
{
  glutExit();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_DISPLAY);

  glutInit(NULL, NULL);

  glut_win = glutCreateWindow(NULL);
  glutExit();
  ck_assert_int_eq(glutGetError(), GLUT_WINDOW_EXIST);
  glutDestroyWindow(glut_win);

  glutExit();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutExit();
}
END_TEST

/* glutLeaveMainLoop test */

START_TEST(test_glutLeaveMainLoop)
{
  glutLeaveMainLoop();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);

  glutLeaveMainLoop();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);

  glutDestroyWindow(glut_win);
  glutExit();
}
END_TEST

/* glutMainLoop test */

START_TEST(test_glutMainLoop)
{
  struct uinput_user_dev dev;
  int i;

  glutMainLoop();
  ck_assert_int_eq(glutGetError(), GLUT_BAD_WINDOW);

  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);
  signal(SIGALRM, sighandler_quit);
  alarm(1);
  glutMainLoop();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  glutDestroyWindow(glut_win);
  glutExit();

  memset(&dev, 0, sizeof(struct uinput_user_dev));
  uinput_keyboard = open("/dev/uinput", O_WRONLY);
  strcpy(dev.name, "uinput-keyboard");
  write(uinput_keyboard, &dev, sizeof(struct uinput_user_dev));
  ioctl(uinput_keyboard, UI_SET_EVBIT, EV_KEY);
  for (i = KEY_Q; i <= KEY_M; i++) {
    ioctl(uinput_keyboard, UI_SET_KEYBIT, i);
  }
  ioctl(uinput_keyboard, UI_SET_KEYBIT, KEY_ESC);
  ioctl(uinput_keyboard, UI_SET_KEYBIT, KEY_F1);
  ioctl(uinput_keyboard, UI_DEV_CREATE);
  uinput_mouse = open("/dev/uinput", O_WRONLY);
  strcpy(dev.name, "uinput-mouse");
  write(uinput_mouse, &dev, sizeof(struct uinput_user_dev));
  ioctl(uinput_mouse, UI_SET_EVBIT, EV_REL);
  ioctl(uinput_mouse, UI_SET_RELBIT, REL_X);
  ioctl(uinput_mouse, UI_SET_RELBIT, REL_Y);
  ioctl(uinput_mouse, UI_DEV_CREATE);
  glutInit(NULL, NULL);
  glut_win = glutCreateWindow(NULL);
  glutReshapeFunc(glutReshape);
  glutDisplayFunc(glutDisplay);
  glutIdleFunc(glutIdle);
  glutKeyboardFunc(glutKeyboard);
  glutSpecialFunc(glutSpecial);
  glutPassiveMotionFunc(glutPassiveMotion);
  signal(SIGALRM, sighandler_input);
  alarm(1);
  glutMainLoop();
  ck_assert_int_eq(glutGetError(), GLUT_SUCCESS);
  ioctl(uinput_mouse, UI_DEV_DESTROY);
  close(uinput_mouse);
  ioctl(uinput_keyboard, UI_DEV_DESTROY);
  close(uinput_keyboard);
}
END_TEST

/* main */

int main()
{
  Suite *s;
  TCase *tc;
  SRunner *sr;
  int n;

  s = suite_create("TinyGLUT");
  tc = tcase_create("Tests");
  tcase_set_timeout(tc, 10);
  tcase_add_test(tc, test_glutInit);
  tcase_add_test(tc, test_glutInitWindowPosition);
  tcase_add_test(tc, test_glutInitWindowSize);
  tcase_add_test(tc, test_glutInitDisplayMode);
  tcase_add_test(tc, test_glutCreateWindow);
  tcase_add_test(tc, test_glutSetWindow);
  tcase_add_test(tc, test_glutSetWindowData);
  tcase_add_test(tc, test_glutGetWindowData);
  tcase_add_test(tc, test_glutReshapeFunc);
  tcase_add_test(tc, test_glutDisplayFunc);
  tcase_add_test(tc, test_glutIdleFunc);
  tcase_add_test(tc, test_glutKeyboardFunc);
  tcase_add_test(tc, test_glutSpecialFunc);
  tcase_add_test(tc, test_glutPassiveMotionFunc);
  tcase_add_test(tc, test_glutSwapBuffers);
  tcase_add_test(tc, test_glutPostRedisplay);
  tcase_add_test(tc, test_glutGet);
  tcase_add_test(tc, test_glutDestroyWindow);
  tcase_add_test(tc, test_glutExit);
  tcase_add_test(tc, test_glutLeaveMainLoop);
  tcase_add_test(tc, test_glutMainLoop);
  suite_add_tcase(s, tc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  n = srunner_ntests_failed(sr);

  return n == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
