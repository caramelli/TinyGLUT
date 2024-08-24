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

cmake_minimum_required(VERSION 3.0)
project(TinyGLUT VERSION 0.6)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(GNUInstallDirs)
add_definitions(-Wall)

find_package(PkgConfig)

#########################
# Configuration options #
#########################

option(ENABLE_DUMMY "Dummy platform" ON)
option(ENABLE_X11 "X11 platform" ON)
option(ENABLE_DIRECTFB "DirectFB platform" ON)
option(ENABLE_FBDEV "FBDev platform" ON)
option(ENABLE_WAYLAND "Wayland platform" ON)

option(ENABLE_EGL "EGL backend" ON)
option(ENABLE_GLX "OpenGL Extension to X11 backend" ON)
option(ENABLE_DFBGL "OpenGL Extension to DirectFB backend" ON)
option(ENABLE_GLFBDEV "OpenGL Extension to FBDev backend" ON)

option(ENABLE_TESTS "unit testing" ON)

#############
# Platforms #
#############

if(ENABLE_X11)
  pkg_check_modules(X11 x11)
  if(NOT X11_FOUND)
    set(ENABLE_X11 OFF)
  endif()
endif()

if(ENABLE_DIRECTFB)
  pkg_check_modules(DIRECTFB directfb)
  if(NOT DIRECTFB_FOUND)
    set(ENABLE_DIRECTFB OFF)
  endif()
endif()

if(ENABLE_FBDEV)
  check_include_files(linux/fb.h FB_FOUND)
  check_include_files(linux/input.h INPUT_FOUND)
  if(NOT FB_FOUND OR NOT INPUT_FOUND)
    set(ENABLE_FBDEV OFF)
  endif()
endif()

if(ENABLE_WAYLAND)
  pkg_check_modules(WAYLAND wayland-client xkbcommon)
  if(NOT WAYLAND_FOUND)
    set(ENABLE_WAYLAND OFF)
  endif()
endif()

if(NOT ENABLE_DUMMY AND NOT ENABLE_X11 AND NOT ENABLE_X11 AND NOT ENABLE_FBDEV AND NOT ENABLE_WAYLAND)
  message(FATAL_ERROR "No platforms found")
endif()

############
# Backends #
############

if(ENABLE_EGL)
  pkg_check_modules(EGL egl)
  if(EGL_FOUND)
    list(APPEND EGL_CFLAGS -DEGL_NO_PLATFORM_SPECIFIC_TYPES)
  else()
    set(ENABLE_EGL OFF)
  endif()
endif()

if(ENABLE_DUMMY)
  if(NOT ENABLE_EGL)
    set(ENABLE_DUMMY OFF)
  endif()
endif()

if(ENABLE_X11)
  if(ENABLE_GLX)
    execute_process(COMMAND pkg-config --libs-only-L gl OUTPUT_VARIABLE LIBDIR)
    string(REPLACE "-L" "" LIBDIR ${LIBDIR})
    string(REPLACE "\n" "" LIBDIR ${LIBDIR})
    check_library_exists(GL glXCreateContext ${LIBDIR} GLX_FOUND)
    if(NOT GLX_FOUND)
      set(ENABLE_GLX OFF)
    endif()
  endif()
  if(NOT ENABLE_GLX AND NOT ENABLE_EGL)
    set(ENABLE_X11 OFF)
  endif()
else()
  set(ENABLE_GLX OFF)
endif()

if(ENABLE_DIRECTFB)
  if(ENABLE_DFBGL)
    message(STATUS "checking for DirectFBGL module")
    execute_process(COMMAND pkg-config --variable=moduledir directfb-internal OUTPUT_VARIABLE MODULEDIR)
    string(REPLACE "\n" "" MODULEDIR ${MODULEDIR})
    if(EXISTS ${MODULEDIR}/interfaces/IDirectFBGL)
      message(STATUS "  found")
    else()
      message(STATUS "  not found")
      set(ENABLE_DFBGL OFF)
    endif()
  endif()
  if(NOT ENABLE_DFBGL AND NOT ENABLE_EGL)
    set(ENABLE_DIRECTFB OFF)
  endif()
else()
  set(ENABLE_DFBGL OFF)
endif()

if(ENABLE_FBDEV)
  if(ENABLE_GLFBDEV)
    execute_process(COMMAND pkg-config --libs-only-L gl OUTPUT_VARIABLE LIBDIR)
    string(REPLACE "-L" "" LIBDIR ${LIBDIR})
    string(REPLACE "\n" "" LIBDIR ${LIBDIR})
    check_library_exists(GL glFBDevCreateContext ${LIBDIR} GLFBDEV_FOUND)
    if(NOT GLFBDEV_FOUND)
      set(ENABLE_GLFBDEV OFF)
    endif()
  endif()
  if(NOT ENABLE_GLFBDEV AND NOT ENABLE_EGL)
    set(ENABLE_FBDEV OFF)
  endif()
else()
  set(ENABLE_GLFBDEV OFF)
endif()

if(ENABLE_WAYLAND)
  if(NOT ENABLE_EGL)
    set(ENABLE_WAYLAND OFF)
  endif()
endif()

if(ENABLE_GLX OR ENABLE_GLFBDEV)
  pkg_check_modules(GL gl)
  if(NOT GL_FOUND)
    set(ENABLE_GLX OFF)
    set(ENABLE_GLFBDEV OFF)
  endif()
endif()

if(NOT ENABLE_EGL AND NOT ENABLE_GLX AND NOT ENABLE_DFBGL AND NOT ENABLE_GLFBDEV)
  message(FATAL_ERROR "No backends found")
endif()

#########
# Tests #
#########

if(ENABLE_TESTS)
  pkg_check_modules(CHECK check)
  pkg_check_modules(LIBFIU libfiu)
  if(NOT CHECK_FOUND OR NOT LIBFIU_FOUND)
    set(ENABLE_TESTS OFF)
  else()
    add_definitions(-DFIU_ENABLE)
  endif()
endif()

#######################
# Build configuration #
#######################

message("")
message("GLUT backends:")
message("")
message("  EGL     (Native Platform Graphics Interface)  ${ENABLE_EGL}")
if(ENABLE_EGL)
message("")
message("           EGL platforms:")
message("                               X11              ${ENABLE_X11}")
message("                               DirectFB         ${ENABLE_DIRECTFB}")
message("                               FBDev            ${ENABLE_FBDEV}")
message("                               Wayland          ${ENABLE_WAYLAND}")
message("")
endif()
message("  GLX     (OpenGL Extension to X11)             ${ENABLE_GLX}")
message("  DFBGL   (OpenGL Extension to DirectFB)        ${ENABLE_DFBGL}")
message("  GLFBDev (OpenGL Extension to FBDev)           ${ENABLE_GLFBDEV}")
message("")
message("Tests: ${ENABLE_TESTS}")
message("")

###############
# Build rules #
###############

# Platforms plugins

set(PLATFORMS_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/glut/platforms)

if(ENABLE_DUMMY)
  list(APPEND PLATFORMS_TARGETS dummy_plugin)
  add_library(dummy_plugin MODULE dummy.c)
endif()

if(ENABLE_X11)
  list(APPEND PLATFORMS_TARGETS x11_plugin)
  add_library(x11_plugin MODULE x11.c)
  target_compile_options(x11_plugin PRIVATE ${X11_CFLAGS})
  target_link_libraries(x11_plugin ${X11_LDFLAGS})
endif()

if(ENABLE_DIRECTFB)
  list(APPEND PLATFORMS_TARGETS directfb_plugin)
  add_library(directfb_plugin MODULE directfb.c)
  target_compile_options(directfb_plugin PRIVATE ${DIRECTFB_CFLAGS})
  target_link_libraries(directfb_plugin ${DIRECTFB_LDFLAGS})
endif()

if(ENABLE_FBDEV)
  list(APPEND PLATFORMS_TARGETS fbdev_plugin)
  add_library(fbdev_plugin MODULE fbdev.c)
  target_link_libraries(fbdev_plugin -lpthread)
endif()

if(ENABLE_WAYLAND)
  list(APPEND PLATFORMS_TARGETS wayland_plugin)
  add_library(wayland_plugin MODULE wayland.c)
  target_compile_options(wayland_plugin PRIVATE ${WAYLAND_CFLAGS})
  target_link_libraries(wayland_plugin ${WAYLAND_LDFLAGS})
endif()

set_target_properties(${PLATFORMS_TARGETS} PROPERTIES PREFIX "")

install(TARGETS ${PLATFORMS_TARGETS} DESTINATION ${PLATFORMS_DIR})

# Backends plugins

set(BACKENDS_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/glut/backends)

if(ENABLE_EGL)
  list(APPEND BACKENDS_TARGETS egl_plugin)
  add_library(egl_plugin MODULE egl.c)
  target_compile_definitions(egl_plugin PRIVATE -DPLATFORMSDIR="${PLATFORMS_DIR}")
  target_compile_options(egl_plugin PRIVATE ${EGL_CFLAGS} ${LIBFIU_CFLAGS})
  target_link_libraries(egl_plugin ${EGL_LDFLAGS} ${LIBFIU_LDFLAGS} -ldl)
endif()

if(ENABLE_GLX)
  list(APPEND BACKENDS_TARGETS glx_plugin)
  add_library(glx_plugin MODULE glx.c)
  target_compile_options(glx_plugin PRIVATE ${GL_CFLAGS} ${LIBFIU_CFLAGS})
  add_dependencies(glx_plugin x11_plugin)
  target_link_libraries(glx_plugin ${GL_LDFLAGS} ${LIBFIU_LDFLAGS} -Wl,x11_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

if(ENABLE_DFBGL)
  list(APPEND BACKENDS_TARGETS dfbgl_plugin)
  add_library(dfbgl_plugin MODULE dfbgl.c)
  target_compile_options(dfbgl_plugin PRIVATE ${DIRECTFB_CFLAGS} ${LIBFIU_CFLAGS})
  add_dependencies(dfbgl_plugin directfb_plugin)
  target_link_libraries(dfbgl_plugin ${LIBFIU_LDFLAGS} -Wl,directfb_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

if(ENABLE_GLFBDEV)
  list(APPEND BACKENDS_TARGETS glfbdev_plugin)
  add_library(glfbdev_plugin MODULE glfbdev.c)
  target_compile_options(glfbdev_plugin PRIVATE ${GL_CFLAGS} ${LIBFIU_CFLAGS})
  add_dependencies(glfbdev_plugin fbdev_plugin)
  target_link_libraries(glfbdev_plugin ${GL_LDFLAGS} ${LIBFIU_LDFLAGS} -Wl,fbdev_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

set_target_properties(${BACKENDS_TARGETS} PROPERTIES PREFIX "")

install(TARGETS ${BACKENDS_TARGETS} DESTINATION ${BACKENDS_DIR})

# GLUT library

add_library(glut SHARED glut.c)
target_compile_definitions(glut PRIVATE -DBACKENDSDIR="${BACKENDS_DIR}")
target_compile_options(glut PRIVATE ${LIBFIU_CFLAGS})
target_link_libraries(glut ${LIBFIU_LDFLAGS} -ldl)
set_target_properties(glut PROPERTIES VERSION 3.0.0 SOVERSION 3)

install(TARGETS glut DESTINATION lib)
install(FILES glut.h DESTINATION include/GL)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${exec_prefix}/lib)
set(includedir \${prefix}/include)
set(PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
configure_file(glut.pc.in glut.pc @ONLY)
install(FILES glut.pc DESTINATION lib/pkgconfig)

if(ENABLE_TESTS)
  add_executable(glut-tests glut-tests.c)
  target_compile_options(glut-tests PRIVATE ${CHECK_CFLAGS} ${LIBFIU_CFLAGS})
  target_link_libraries(glut-tests glut ${CHECK_LDFLAGS} ${LIBFIU_LDFLAGS})
  enable_testing()
  add_test(glut-tests glut-tests)
endif()
