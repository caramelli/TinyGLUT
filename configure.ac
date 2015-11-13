dnl Process this file with autoconf to create configure.

AC_INIT(TinyGLUT, 0.1)

AM_INIT_AUTOMAKE

LT_INIT(disable-static)

AC_PROG_CC

PKG_PROG_PKG_CONFIG

CFLAGS="$CFLAGS -Wall"
LDFLAGS="$LDFLAGS -Wl,--no-undefined"

dnl
dnl Options
dnl

with_dummy=no with_x11=no with_directfb=no with_fbdev=no

AC_ARG_WITH(platforms,
            AS_HELP_STRING(--with-platforms=ARG, comma delimited platforms),
            with_platforms=$withval, [with_platforms=dummy,x11,directfb,fbdev])

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

dnl
dnl Platforms
dnl

for platform in `IFS=,; echo $with_platforms`; do
  case $platform in
    dummy)
      with_dummy=yes
      ;;
    x11)
      with_x11=yes
      ;;
    directfb)
      with_directfb=yes
      ;;
    fbdev)
      with_fbdev=yes
      ;;
    *)
      AC_MSG_ERROR(Platform $platform unknown)
      ;;
  esac
done

if test x$with_x11 = xyes; then
  PKG_CHECK_MODULES(X11, x11, , with_x11=no)
fi

if test x$with_directfb = xyes; then
  PKG_CHECK_MODULES(DIRECTFB, directfb, , with_directfb=no)
fi

if test x$with_fbdev = xyes; then
  AC_CHECK_HEADERS(linux/fb.h linux/input.h, , with_fbdev=no)
fi

if test x$with_dummy = xno -a x$with_x11 = xno -a x$with_directfb = xno -a x$with_fbdev = xno; then
  AC_MSG_ERROR(No platforms found)
fi

dnl
dnl Backends
dnl

if test x$enable_egl = xyes; then
  PKG_CHECK_MODULES(EGL, egl, , enable_egl=no)
fi

if test x$with_dummy = xyes; then
  if test x$enable_egl = xno; then
    with_dummy=no
  fi
fi

if test x$with_x11 = xyes; then
  if test x$enable_glx = xyes; then
    AC_CHECK_LIB(GL, glXCreateContext, , enable_glx=no)
  fi
  if test x$enable_glx = xno -a x$enable_egl = xno; then
    with_x11=no
  fi
else
  enable_glx=no
fi

if test x$with_directfb = xyes; then
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
    with_directfb=no
  fi
else
  enable_dfbgl=no
fi

if test x$with_fbdev = xyes; then
  if test x$enable_glfbdev = xyes; then
    AC_CHECK_LIB(GL, glFBDevCreateContext, , enable_glfbdev=no)
  fi
  if test x$enable_glfbdev = xno -a x$enable_egl = xno; then
    with_fbdev=no
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
dnl Output
dnl

AM_CONDITIONAL(DUMMY, test x$with_dummy = xyes)
AM_CONDITIONAL(X11, test x$with_x11 = xyes)
AM_CONDITIONAL(DIRECTFB, test x$with_directfb = xyes)
AM_CONDITIONAL(FBDEV, test x$with_fbdev = xyes)

AM_CONDITIONAL(EGL, test x$enable_egl = xyes)
AM_CONDITIONAL(GLX, test x$enable_glx = xyes)
AM_CONDITIONAL(DFBGL, test x$enable_dfbgl = xyes)
AM_CONDITIONAL(GLFBDEV, test x$enable_glfbdev = xyes)

AC_OUTPUT(Makefile glut.pc)

echo
echo "Backends:"
echo
echo "  EGL                              $enable_egl"
echo "  OpenGL Extension to X11          $enable_glx"
echo "  OpenGL Extension to DirectFB     $enable_dfbgl"
echo "  OpenGL Extension to Linux FBDev  $enable_glfbdev"
echo
