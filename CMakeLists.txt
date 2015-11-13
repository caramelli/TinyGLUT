cmake_minimum_required(VERSION 2.8.11)

set(TINYGLUT_VERSION 0.3)

find_package(PkgConfig)

include(CheckIncludeFiles)
include(CheckLibraryExists)

add_definitions(-Wall)
set(CMAKE_MODULE_LINKER_FLAGS -Wl,--no-undefined)
set(CMAKE_SHARED_LINKER_FLAGS -Wl,--no-undefined)

###########
# Options #
###########

option(ENABLE_DUMMY "Enable/Disable Dummy platform" ON)
option(ENABLE_DIRECTFB "Enable/Disable DirectFB platform" ON)
option(ENABLE_X11 "Enable/Disable X11 platform" ON)
option(ENABLE_FBDEV "Enable/Disable Linux FBDev platform" ON)

option(ENABLE_EGL "Enable/Disable EGL backend" ON)
option(ENABLE_GLX "Enable/Disable OpenGL Extension to X11 backend" ON)
option(ENABLE_DFBGL "Enable/Disable disable OpenGL Extension to DirectFB backend" ON)
option(ENABLE_GLFBDEV "Enable/Disable disable OpenGL Extension to Linux FBDev backend" ON)

option(ENABLE_TESTS "Enable/Disable unit testing" ON)

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
  CHECK_INCLUDE_FILES(linux/fb.h FB_FOUND)
  CHECK_INCLUDE_FILES(linux/input.h INPUT_FOUND)
  if(NOT FB_FOUND OR NOT INPUT_FOUND)
    set(ENABLE_FBDEV OFF)
  endif()
endif()

if(NOT ENABLE_DUMMY AND NOT ENABLE_X11 AND NOT ENABLE_X11 AND NOT ENABLE_FBDEV)
  message(FATAL_ERROR "No platforms found")
endif()

############
# Backends #
############

if(ENABLE_EGL)
  pkg_check_modules(EGL egl)
  if(NOT EGL_FOUND)
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
  endif()
endif()

##########
# Output #
##########

message("")
message("Backends:")
message("")
message("  EGL                              " ${ENABLE_EGL})
message("  OpenGL Extension to X11          " ${ENABLE_GLX})
message("  OpenGL Extension to DirectFB     " ${ENABLE_DFBGL})
message("  OpenGL Extension to Linux FBDev  " ${ENABLE_GLFBDEV})
message("")
message("Tests: " ${ENABLE_TESTS})
message("")

#####################
# Platforms plugins #
#####################

set(PLATFORMS_DIR ${CMAKE_INSTALL_PREFIX}/lib/glut/platforms)

if(ENABLE_DUMMY)
  list(APPEND PLATFORMS_TARGETS dummy_plugin)
  add_library(dummy_plugin MODULE dummy.c)
endif()

if(ENABLE_X11)
  list(APPEND PLATFORMS_TARGETS x11_plugin)
  add_library(x11_plugin MODULE x11.c)
  target_include_directories(x11_plugin PRIVATE ${X11_INCLUDE_DIRS})
  target_link_libraries(x11_plugin ${X11_LDFLAGS})
endif()

if(ENABLE_DIRECTFB)
  list(APPEND PLATFORMS_TARGETS directfb_plugin)
  add_library(directfb_plugin MODULE directfb.c)
  target_include_directories(directfb_plugin PRIVATE ${DIRECTFB_INCLUDE_DIRS})
  target_link_libraries(directfb_plugin ${DIRECTFB_LDFLAGS})
endif()

if(ENABLE_FBDEV)
  list(APPEND PLATFORMS_TARGETS fbdev_plugin)
  add_library(fbdev_plugin MODULE fbdev.c)
  target_link_libraries(fbdev_plugin -lpthread)
endif()

set_target_properties(${PLATFORMS_TARGETS} PROPERTIES PREFIX "")

install(TARGETS ${PLATFORMS_TARGETS} DESTINATION ${PLATFORMS_DIR})

####################
# Backends plugins #
####################

set(BACKENDS_DIR ${CMAKE_INSTALL_PREFIX}/lib/glut/backends)

if(ENABLE_EGL)
  list(APPEND BACKENDS_TARGETS egl_plugin)
  add_library(egl_plugin MODULE egl.c)
  target_compile_definitions(egl_plugin PRIVATE -DPLATFORMSDIR="${PLATFORMS_DIR}")
  target_include_directories(egl_plugin PRIVATE ${EGL_INCLUDE_DIRS} ${LIBFIU_INCLUDE_DIRS})
  target_link_libraries(egl_plugin ${EGL_LDFLAGS} ${LIBFIU_LDFLAGS} -ldl)
endif()

if(ENABLE_GLX)
  list(APPEND BACKENDS_TARGETS glx_plugin)
  add_library(glx_plugin MODULE glx.c)
  target_include_directories(glx_plugin PRIVATE ${GL_INCLUDE_DIRS} ${LIBFIU_INCLUDE_DIRS})
  add_dependencies(glx_plugin x11_plugin)
  target_link_libraries(glx_plugin ${GL_LDFLAGS} ${LIBFIU_LDFLAGS} -Wl,x11_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

if(ENABLE_DFBGL)
  list(APPEND BACKENDS_TARGETS dfbgl_plugin)
  add_library(dfbgl_plugin MODULE dfbgl.c)
  target_include_directories(dfbgl_plugin PRIVATE ${DIRECTFB_INCLUDE_DIRS} ${LIBFIU_INCLUDE_DIRS})
  add_dependencies(dfbgl_plugin directfb_plugin)
  target_link_libraries(dfbgl_plugin ${LIBFIU_LDFLAGS} -Wl,directfb_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

if(ENABLE_GLFBDEV)
  list(APPEND BACKENDS_TARGETS glfbdev_plugin)
  add_library(glfbdev_plugin MODULE glfbdev.c)
  target_include_directories(glfbdev_plugin PRIVATE ${GL_INCLUDE_DIRS} ${LIBFIU_INCLUDE_DIRS})
  add_dependencies(glfbdev_plugin fbdev_plugin)
  target_link_libraries(glfbdev_plugin ${GL_LDFLAGS} ${LIBFIU_LDFLAGS} -Wl,fbdev_plugin.so -Wl,-rpath,${PLATFORMS_DIR})
endif()

set_target_properties(${BACKENDS_TARGETS} PROPERTIES PREFIX "")

install(TARGETS ${BACKENDS_TARGETS} DESTINATION ${BACKENDS_DIR})

################
# GLUT library #
################

add_library(glut SHARED glut.c)
target_compile_definitions(glut PRIVATE -DBACKENDSDIR="${BACKENDS_DIR}")
target_include_directories(glut PRIVATE ${LIBFIU_INCLUDE_DIRS})
target_link_libraries(glut ${LIBFIU_LDFLAGS} -ldl)
set_target_properties(glut PROPERTIES VERSION 3.0.0 SOVERSION 3)

install(TARGETS glut DESTINATION lib)
install(FILES glut.h DESTINATION include/GL)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${exec_prefix}/lib)
set(includedir \${prefix}/include)
set(PACKAGE_VERSION ${TINYGLUT_VERSION})
configure_file(glut.pc.in glut.pc @ONLY)
install(FILES glut.pc DESTINATION lib/pkgconfig)

##############
# GLUT tests #
##############

if(ENABLE_TESTS)
  add_definitions(-DFIU_ENABLE)
  add_executable(glut-tests glut-tests.c)
  target_include_directories(glut-tests PRIVATE ${CHECK_INCLUDE_DIRS} ${LIBFIU_INCLUDE_DIRS})
  target_link_libraries(glut-tests glut ${CHECK_LDFLAGS} ${LIBFIU_LDFLAGS})
  enable_testing()
  add_test(glut-tests glut-tests)
endif()
