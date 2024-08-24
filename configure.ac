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

AC_INIT(TinyGLUT, 0.6)

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
LT_INIT(disable-static)
AC_PROG_CC
CFLAGS="$CFLAGS -Wall"

PKG_PROG_PKG_CONFIG

#########################
# Configuration options #
#########################

AC_ARG_ENABLE(dummy,
              AS_HELP_STRING(--disable-dummy, disable Dummy platform),
              enable_dummy=no, enable_dummy=yes)
AC_ARG_ENABLE(x11,
              AS_HELP_STRING(--disable-x11, disable X11 platform),
              enable_x11=no, enable_x11=yes)
AC_ARG_ENABLE(directfb,
              AS_HELP_STRING(--disable-directfb, disable DirectFB platform),
              enable_directfb=no, enable_directfb=yes)
AC_ARG_ENABLE(fbdev,
              AS_HELP_STRING(--disable-fbdev, disable FBDev platform),
              enable_fbdev=no, enable_fbdev=yes)
AC_ARG_ENABLE(wayland,
              AS_HELP_STRING(--disable-wayland, disable Wayland platform),
              enable_wayland=no, enable_wayland=yes)

AC_ARG_ENABLE(egl,
              AS_HELP_STRING(--disable-egl, disable EGL backend),
              enable_egl=no, enable_egl=yes)
AC_ARG_ENABLE(glx,
              AS_HELP_STRING(--disable-glx, disable OpenGL Extension to X11 backend),
              enable_glx=no, enable_glx=yes)
AC_ARG_ENABLE(dfbgl,
              AS_HELP_STRING(--disable-dfbgl, disable OpenGL Extension to DirectFB backend),
              enable_dfbgl=no, enable_dfbgl=yes)
AC_ARG_ENABLE(glfbdev,
              AS_HELP_STRING(--disable-glfbdev, disable OpenGL Extension to FBDev backend),
              enable_glfbdev=no, enable_glfbdev=yes)

AC_ARG_ENABLE(tests,
              AS_HELP_STRING(--disable-tests, disable unit testing),
              enable_tests=no, enable_tests=yes)

#############
# Platforms #
#############

if test x$enable_x11 = xyes; then
  PKG_CHECK_MODULES(X11, x11, , enable_x11=no)
fi

if test x$enable_directfb = xyes; then
  PKG_CHECK_MODULES(DIRECTFB, directfb, , enable_directfb=no)
fi

if test x$enable_fbdev = xyes; then
  AC_CHECK_HEADERS(linux/fb.h linux/input.h, , enable_fbdev=no)
fi

if test x$enable_wayland = xyes; then
  PKG_CHECK_MODULES(WAYLAND, wayland-client xkbcommon, , enable_wayland=no)
fi

if test x$enable_dummy = xno -a x$enable_x11 = xno -a x$enable_directfb = xno -a x$enable_fbdev = xno -a x$enable_wayland = xno; then
  AC_MSG_ERROR(No platforms found)
fi

############
# Backends #
############

if test x$enable_egl = xyes; then
  PKG_CHECK_MODULES(EGL, egl, , enable_egl=no)
  if test x$enable_egl = xyes; then
    EGL_CFLAGS="$EGL_CFLAGS -DEGL_NO_PLATFORM_SPECIFIC_TYPES"
  fi
fi

if test x$enable_dummy = xyes; then
  if test x$enable_egl = xno; then
    enable_dummy=no
  fi
fi

if test x$enable_x11 = xyes; then
  if test x$enable_glx = xyes; then
    save_LIBS=$LIBS
    LIBS="$LIBS `$PKG_CONFIG --libs-only-L gl`"
    AC_CHECK_LIB(GL, glXCreateContext, , enable_glx=no)
    LIBS=$save_LIBS
  fi
  if test x$enable_glx = xno -a x$enable_egl = xno; then
    enable_x11=no
  fi
else
  enable_glx=no
fi

if test x$enable_directfb = xyes; then
  if test x$enable_dfbgl = xyes; then
    AC_MSG_CHECKING(for DirectFBGL module)
    MODULEDIR=`$PKG_CONFIG --variable=moduledir directfb-internal`
    if test -d $MODULEDIR/interfaces/IDirectFBGL; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
      enable_dfbgl=no
    fi
  fi
  if test x$enable_dfbgl = xno -a x$enable_egl = xno; then
    enable_directfb=no
  fi
else
  enable_dfbgl=no
fi

if test x$enable_fbdev = xyes; then
  if test x$enable_glfbdev = xyes; then
    save_LIBS=$LIBS
    LIBS="$LIBS `$PKG_CONFIG --libs-only-L gl`"
    AC_CHECK_LIB(GL, glFBDevCreateContext, , enable_glfbdev=no)
    LIBS=$save_LIBS
  fi
  if test x$enable_glfbdev = xno -a x$enable_egl = xno; then
    enable_fbdev=no
  fi
else
  enable_glfbdev=no
fi

if test x$enable_wayland = xyes; then
  if test x$enable_egl = xno; then
    enable_wayland=no
  fi
fi

if test x$enable_glx = xyes || test x$enable_glfbdev = xyes; then
  PKG_CHECK_MODULES(GL, gl, , enable_glx=no enable_glfbdev=no)
fi

if test x$enable_egl = xno -a x$enable_glx = xno -a x$enable_dfbgl = xno -a x$enable_glfbdev = xno; then
  AC_MSG_ERROR(No backends found)
fi

#########
# Tests #
#########

if test x$enable_tests = xyes; then
  PKG_CHECK_MODULES(CHECK, check, , enable_tests=no)
  PKG_CHECK_MODULES(LIBFIU, libfiu, , enable_tests=no)
  if test x$enable_tests = xyes; then
    CFLAGS="$CFLAGS -DFIU_ENABLE"
  fi
fi

#######################
# Build configuration #
#######################

echo
echo "GLUT backends:"
echo
echo "  EGL     (Native Platform Graphics Interface)  $enable_egl"
if test x$enable_egl = xyes; then
echo
echo "           EGL platforms:"
echo "                               X11              $enable_x11"
echo "                               DirectFB         $enable_directfb"
echo "                               FBDev            $enable_fbdev"
echo "                               Wayland          $enable_wayland"
echo
fi
echo "  GLX     (OpenGL Extension to X11)             $enable_glx"
echo "  DFBGL   (OpenGL Extension to DirectFB)        $enable_dfbgl"
echo "  GLFBDev (OpenGL Extension to FBDev)           $enable_glfbdev"
echo
echo "Tests: $enable_tests"
echo

###############
# Build rules #
###############

# Platforms plugins

PLATFORMS_DIR=$libdir/glut/platforms
AC_SUBST(PLATFORMS_DIR)

AM_CONDITIONAL(DUMMY, test x$enable_dummy = xyes)

AM_CONDITIONAL(X11, test x$enable_x11 = xyes)

AM_CONDITIONAL(DIRECTFB, test x$enable_directfb = xyes)

AM_CONDITIONAL(FBDEV, test x$enable_fbdev = xyes)

AM_CONDITIONAL(WAYLAND, test x$enable_wayland = xyes)

# Backends plugins

BACKENDS_DIR=$libdir/glut/backends
AC_SUBST(BACKENDS_DIR)

AM_CONDITIONAL(EGL, test x$enable_egl = xyes)

AM_CONDITIONAL(GLX, test x$enable_glx = xyes)

AM_CONDITIONAL(DFBGL, test x$enable_dfbgl = xyes)

AM_CONDITIONAL(GLFBDEV, test x$enable_glfbdev = xyes)

# GLUT library

AC_CONFIG_FILES(Makefile glut.pc)

AC_OUTPUT

AM_CONDITIONAL(TESTS, test x$enable_tests = xyes)
