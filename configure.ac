dnl Process this file with autoconf to create configure.

AC_INIT(TinyGLUT, 0.2)

AM_INIT_AUTOMAKE

LT_INIT(disable-static)

AC_PROG_CC

AM_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

CFLAGS="$CFLAGS -Wall"
LDFLAGS="$LDFLAGS -Wl,--no-undefined"

dnl
dnl Options
dnl

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
              AS_HELP_STRING(--disable-fbdev, disable Linux FBDev platform),
              enable_fbdev=no, enable_fbdev=yes)

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
              AS_HELP_STRING(--disable-glfbdev, disable OpenGL Extension to Linux FBDev backend),
              enable_glfbdev=no, enable_glfbdev=yes)

AC_ARG_ENABLE(tests,
              AS_HELP_STRING(--disable-tests, disable unit testing),
              enable_tests=no, enable_tests=yes)

dnl
dnl Platforms
dnl

if test x$enable_x11 = xyes; then
  PKG_CHECK_MODULES(X11, x11, , enable_x11=no)
fi

if test x$enable_directfb = xyes; then
  PKG_CHECK_MODULES(DIRECTFB, directfb, , enable_directfb=no)
fi

if test x$enable_fbdev = xyes; then
  AC_CHECK_HEADERS(linux/fb.h linux/input.h, , enable_fbdev=no)
fi

if test x$enable_dummy = xno -a x$enable_x11 = xno -a x$enable_directfb = xno -a x$enable_fbdev = xno; then
  AC_MSG_ERROR(No platforms found)
fi

dnl
dnl Backends
dnl

if test x$enable_egl = xyes; then
  PKG_CHECK_MODULES(EGL, egl, , enable_egl=no)
fi

if test x$enable_dummy = xyes; then
  if test x$enable_egl = xno; then
    enable_dummy=no
  fi
fi

if test x$enable_x11 = xyes; then
  if test x$enable_glx = xyes; then
    AC_CHECK_LIB(GL, glXCreateContext, , enable_glx=no)
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
    AC_CHECK_LIB(GL, glFBDevCreateContext, , enable_glfbdev=no)
  fi
  if test x$enable_glfbdev = xno -a x$enable_egl = xno; then
    enable_fbdev=no
  fi
else
  enable_glfbdev=no
fi

if test x$enable_glx = xyes || test x$enable_glfbdev = xyes; then
  PKG_CHECK_MODULES(GL, gl, , enable_glx=no enable_glfbdev=no)
fi

if test x$enable_egl = xno -a x$enable_glx = xno -a x$enable_dfbgl = xno -a x$enable_glfbdev = xno; then
  AC_MSG_ERROR(No backends found)
fi

dnl
dnl Tests
dnl

if test x$enable_tests = xyes; then
  PKG_CHECK_MODULES(CHECK, check, , enable_tests=no)
  PKG_CHECK_MODULES(LIBFIU, libfiu, , enable_tests=no)
  if test x$enable_tests = xyes; then
    CFLAGS="$CFLAGS -DFIU_ENABLE"
  fi
fi

dnl
dnl Output
dnl

AM_CONDITIONAL(DUMMY, test x$enable_dummy = xyes)
AM_CONDITIONAL(X11, test x$enable_x11 = xyes)
AM_CONDITIONAL(DIRECTFB, test x$enable_directfb = xyes)
AM_CONDITIONAL(FBDEV, test x$enable_fbdev = xyes)

AM_CONDITIONAL(EGL, test x$enable_egl = xyes)
AM_CONDITIONAL(GLX, test x$enable_glx = xyes)
AM_CONDITIONAL(DFBGL, test x$enable_dfbgl = xyes)
AM_CONDITIONAL(GLFBDEV, test x$enable_glfbdev = xyes)

AM_CONDITIONAL(TESTS, test x$enable_tests = xyes)

AC_OUTPUT(Makefile glut.pc)

echo
echo "Backends:"
echo
echo "  EGL                              $enable_egl"
echo "  OpenGL Extension to X11          $enable_glx"
echo "  OpenGL Extension to DirectFB     $enable_dfbgl"
echo "  OpenGL Extension to Linux FBDev  $enable_glfbdev"
echo
echo "Tests: $enable_tests"
echo
