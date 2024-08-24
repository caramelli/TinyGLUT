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

#ifndef GLUT_H
#define GLUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Errors */
#define GLUT_SUCCESS             0x0000
#define GLUT_DISPLAY_EXIST       0x0001
#define GLUT_WINDOW_EXIST        0x0002
#define GLUT_BAD_BACKEND         0x0003
#define GLUT_BAD_DISPLAY         0x0004
#define GLUT_BAD_WINDOW          0x0005
#define GLUT_BAD_VALUE           0x0006
#define GLUT_BAD_ALLOC           0x0007

/* Display mode */
#define GLUT_DOUBLE              0x0002
#define GLUT_DEPTH               0x0010

/* Get query */
#define GLUT_WINDOW_X            0x0064
#define GLUT_WINDOW_Y            0x0065
#define GLUT_WINDOW_WIDTH        0x0066
#define GLUT_WINDOW_HEIGHT       0x0067
#define GLUT_WINDOW_DEPTH_SIZE   0x006A
#define GLUT_WINDOW_DOUBLEBUFFER 0x0073
#define GLUT_SCREEN_WIDTH        0x00C8
#define GLUT_SCREEN_HEIGHT       0x00C9
#define GLUT_INIT_WINDOW_X       0x01F4
#define GLUT_INIT_WINDOW_Y       0x01F5
#define GLUT_INIT_WINDOW_WIDTH   0x01F6
#define GLUT_INIT_WINDOW_HEIGHT  0x01F7
#define GLUT_INIT_DISPLAY_MODE   0x01F8
#define GLUT_ELAPSED_TIME        0x02BC

/* Special key */
#define GLUT_KEY_F1              0x0001
#define GLUT_KEY_F2              0x0002
#define GLUT_KEY_F3              0x0003
#define GLUT_KEY_F4              0x0004
#define GLUT_KEY_F5              0x0005
#define GLUT_KEY_F6              0x0006
#define GLUT_KEY_F7              0x0007
#define GLUT_KEY_F8              0x0008
#define GLUT_KEY_F9              0x0009
#define GLUT_KEY_F10             0x000A
#define GLUT_KEY_F11             0x000B
#define GLUT_KEY_F12             0x000C
#define GLUT_KEY_LEFT            0x0064
#define GLUT_KEY_UP              0x0065
#define GLUT_KEY_RIGHT           0x0066
#define GLUT_KEY_DOWN            0x0067
#define GLUT_KEY_PAGE_UP         0x0068
#define GLUT_KEY_PAGE_DOWN       0x0069

/* Flag for OpenGL profile */
#define GLUT_ES_PROFILE          0x0004

/* Functions */
int glutGetError();
void glutInit(int *argc, char **argv);
void glutInitWindowPosition(int posx, int posy);
void glutInitWindowSize(int width, int height);
void glutInitDisplayMode(unsigned int mode);
void glutInitContextProfile(int profile);
int glutCreateWindow(const char *title);
void glutSetWindow(int window);
void glutSetWindowData(void *data);
void *glutGetWindowData();
void glutReshapeFunc(void (*func)(int width, int height));
void glutDisplayFunc(void (*func)());
void glutIdleFunc(void (*func)());
void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void glutSpecialFunc(void (*func)(int key, int x, int y));
void glutPassiveMotionFunc(void (*func)(int x, int y));
void glutSwapBuffers();
void glutPostRedisplay();
int glutGet(int query);
void glutDestroyWindow(int window);
void glutExit();
void glutLeaveMainLoop();
void glutMainLoop();

#ifdef __cplusplus
}
#endif

#endif
