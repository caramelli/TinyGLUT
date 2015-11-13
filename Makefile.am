#####################
# Platforms plugins #
#####################

platformsdir = $(libdir)/glut/platforms
platforms_LTLIBRARIES =

if DUMMY
platforms_LTLIBRARIES += dummy.la
dummy_la_SOURCES = dummy.c
dummy_la_LDFLAGS = -module -avoid-version
endif

if X11
platforms_LTLIBRARIES += x11.la
x11_la_SOURCES = x11.c
x11_la_CFLAGS = @X11_CFLAGS@
x11_la_LIBADD = @X11_LIBS@
x11_la_LDFLAGS = -module -avoid-version
endif

if DIRECTFB
platforms_LTLIBRARIES += directfb.la
directfb_la_SOURCES = directfb.c
directfb_la_CFLAGS = @DIRECTFB_CFLAGS@
directfb_la_LIBADD = @DIRECTFB_LIBS@
directfb_la_LDFLAGS = -module -avoid-version
endif

if FBDEV
platforms_LTLIBRARIES += fbdev.la
fbdev_la_SOURCES = fbdev.c
fbdev_la_LDFLAGS = -module -avoid-version
endif

####################
# Backends plugins #
####################

backendsdir = $(libdir)/glut/backends
backends_LTLIBRARIES =

if EGL
backends_LTLIBRARIES += egl.la
egl_la_SOURCES = egl.c
egl_la_CFLAGS = -DPLATFORMSDIR=\"$(platformsdir)\" @EGL_CFLAGS@
egl_la_LIBADD = @EGL_LIBS@ -ldl
egl_la_LDFLAGS = -module -avoid-version
endif

if GLX
backends_LTLIBRARIES += glx.la
glx_la_SOURCES = glx.c
glx_la_CFLAGS = @GL_CFLAGS@
glx_la_LIBADD = @GL_LIBS@
glx_la_DEPENDENCIES = x11.la
glx_la_LDFLAGS = -module -avoid-version -Wl,.libs/x11.so -Wl,-rpath,$(libdir)/glut/platforms
endif

if DFBGL
backends_LTLIBRARIES += dfbgl.la
dfbgl_la_SOURCES = dfbgl.c
dfbgl_la_CFLAGS = @DIRECTFB_CFLAGS@
dfbgl_la_DEPENDENCIES = directfb.la
dfbgl_la_LDFLAGS = -module -avoid-version -Wl,.libs/directfb.so -Wl,-rpath,$(libdir)/glut/platforms
endif

if GLFBDEV
backends_LTLIBRARIES += glfbdev.la
glfbdev_la_SOURCES = glfbdev.c
glfbdev_la_CFLAGS = @GL_CFLAGS@
glfbdev_la_LIBADD = @GL_LIBS@
glfbdev_la_DEPENDENCIES = fbdev.la
glfbdev_la_LDFLAGS = -module -avoid-version -Wl,.libs/fbdev.so -Wl,-rpath,$(libdir)/glut/platforms
endif

################
# GLUT library #
################

lib_LTLIBRARIES = libglut.la
libglut_la_SOURCES = glut.c
libglut_la_CFLAGS = -DBACKENDSDIR=\"$(backendsdir)\"
libglut_la_LIBADD = -ldl
libglut_la_LDFLAGS = -version-info 3:0:0

glutincludedir = $(includedir)/GL
glutinclude_HEADERS = glut.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glut.pc
