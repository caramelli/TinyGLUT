# TinyGLUT                 Small implementation of GLUT (OpenGL Utility Toolkit)
# Copyright (c) 2015-2024, Nicolas Caramelli
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Platforms plugins

platformsdir = @PLATFORMS_DIR@
platforms_LTLIBRARIES =

if DUMMY
platforms_LTLIBRARIES += dummy_plugin.la
dummy_plugin_la_SOURCES = dummy.c
dummy_plugin_la_LDFLAGS = -module -avoid-version
endif

if X11
platforms_LTLIBRARIES += x11_plugin.la
x11_plugin_la_SOURCES = x11.c
x11_plugin_la_CFLAGS = @X11_CFLAGS@
x11_plugin_la_LIBADD = @X11_LIBS@
x11_plugin_la_LDFLAGS = -module -avoid-version
endif

if XCB
platforms_LTLIBRARIES += xcb_plugin.la
xcb_plugin_la_SOURCES = xcb.c
xcb_plugin_la_CFLAGS = @XCB_CFLAGS@
xcb_plugin_la_LIBADD = @XCB_LIBS@
xcb_plugin_la_LDFLAGS = -module -avoid-version
endif

if DIRECTFB
platforms_LTLIBRARIES += directfb_plugin.la
directfb_plugin_la_SOURCES = directfb.c
directfb_plugin_la_CFLAGS = @DIRECTFB_CFLAGS@
directfb_plugin_la_LIBADD = @DIRECTFB_LIBS@
directfb_plugin_la_LDFLAGS = -module -avoid-version
endif

if FBDEV
platforms_LTLIBRARIES += fbdev_plugin.la
fbdev_plugin_la_SOURCES = fbdev.c
fbdev_plugin_la_LIBADD = -lpthread
fbdev_plugin_la_LDFLAGS = -module -avoid-version
endif

if WAYLAND
platforms_LTLIBRARIES += wayland_plugin.la
wayland_plugin_la_SOURCES = wayland.c
wayland_plugin_la_CFLAGS = @WAYLAND_CFLAGS@
wayland_plugin_la_LIBADD = @WAYLAND_LIBS@
wayland_plugin_la_LDFLAGS = -module -avoid-version
endif

# Backends plugins

backendsdir = @BACKENDS_DIR@
backends_LTLIBRARIES =

if EGL
backends_LTLIBRARIES += egl_plugin.la
egl_plugin_la_SOURCES = egl.c
egl_plugin_la_CFLAGS = -DPLATFORMSDIR=\"$(platformsdir)\" @EGL_CFLAGS@ @LIBFIU_CFLAGS@
egl_plugin_la_LIBADD = @EGL_LIBS@ @LIBFIU_LIBS@ -ldl
egl_plugin_la_LDFLAGS = -module -avoid-version
endif

if GLX
backends_LTLIBRARIES += glx_plugin.la
glx_plugin_la_SOURCES = glx.c
glx_plugin_la_CFLAGS = @GL_CFLAGS@ @LIBFIU_CFLAGS@
glx_plugin_la_LIBADD = @GL_LIBS@ @LIBFIU_LIBS@
glx_plugin_la_DEPENDENCIES = x11_plugin.la
glx_plugin_la_LDFLAGS = -module -avoid-version -Wl,.libs/x11_plugin.so -Wl,-rpath,$(libdir)/glut/platforms
endif

if DFBGL
backends_LTLIBRARIES += dfbgl_plugin.la
dfbgl_plugin_la_SOURCES = dfbgl.c
dfbgl_plugin_la_CFLAGS = @DIRECTFB_CFLAGS@ @LIBFIU_CFLAGS@
dfbgl_plugin_la_LIBADD = @LIBFIU_LIBS@
dfbgl_plugin_la_DEPENDENCIES = directfb_plugin.la
dfbgl_plugin_la_LDFLAGS = -module -avoid-version -Wl,.libs/directfb_plugin.so -Wl,-rpath,$(libdir)/glut/platforms
endif

if GLFBDEV
backends_LTLIBRARIES += glfbdev_plugin.la
glfbdev_plugin_la_SOURCES = glfbdev.c
glfbdev_plugin_la_CFLAGS = @GL_CFLAGS@ @LIBFIU_CFLAGS@
glfbdev_plugin_la_LIBADD = @GL_LIBS@ @LIBFIU_LIBS@
glfbdev_plugin_la_DEPENDENCIES = fbdev_plugin.la
glfbdev_plugin_la_LDFLAGS = -module -avoid-version -Wl,.libs/fbdev_plugin.so -Wl,-rpath,$(libdir)/glut/platforms
endif

# GLUT library

lib_LTLIBRARIES = libglut.la
libglut_la_SOURCES = glut.c
libglut_la_CFLAGS = -DBACKENDSDIR=\"$(backendsdir)\" @LIBFIU_CFLAGS@
libglut_la_LIBADD =  @LIBFIU_LIBS@ -ldl
libglut_la_LDFLAGS = -version-info 3:0:0

glutincludedir = $(includedir)/GL
glutinclude_HEADERS = glut.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glut.pc

if TESTS
check_PROGRAMS = glut-tests
glut_tests_SOURCES = glut-tests.c
glut_tests_CFLAGS = @CHECK_CFLAGS@ @LIBFIU_CFLAGS@
glut_tests_LDADD = libglut.la @CHECK_LIBS@ @LIBFIU_LIBS@
TESTS = glut-tests
endif
