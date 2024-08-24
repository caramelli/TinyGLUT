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

project('TinyGLUT', 'c', version: '0.6')

cc = meson.get_compiler('c')

pkgconfig = import('pkgconfig')

#########################
# Configuration options #
#########################

enable_dummy = get_option('dummy')
enable_x11 = get_option('x11')
enable_directfb = get_option('directfb')
enable_fbdev = get_option('fbdev')
enable_wayland = get_option('wayland')

enable_egl = get_option('egl')
enable_glx = get_option('glx')
enable_dfbgl = get_option('dfbgl')
enable_glfbdev = get_option('glfbdev')

enable_tests = get_option('tests')

#############
# Platforms #
#############

if enable_x11
  x11_dep = dependency('x11', required: false)
  if not x11_dep.found()
    enable_x11 = false
  endif
endif

if enable_directfb
  directfb_dep = dependency('directfb', required: false)
  if not directfb_dep.found()
    enable_directfb = false
  endif
endif

if enable_fbdev
  if not cc.has_header('linux/fb.h') or not cc.has_header('linux/input.h')
    enable_fbdev = false
  endif
endif

if enable_wayland
  wayland_dep = [dependency('wayland-client', required: false), dependency('xkbcommon', required: false)]
  foreach dep : wayland_dep
    if not dep.found()
      enable_wayland = false
    endif
  endforeach
endif

if not enable_dummy and not enable_x11 and not enable_directfb and not enable_fbdev and not enable_wayland
  error('No platforms found')
endif

############
# Backends #
############

if enable_egl
  egl_dep = dependency('egl', required: false)
  if egl_dep.found()
    egl_dep = declare_dependency(compile_args: '-DEGL_NO_PLATFORM_SPECIFIC_TYPES', dependencies: egl_dep)
  else
    enable_egl = false
  endif
endif

if enable_dummy
  if not enable_egl
    enable_dummy = false
  endif
endif

if enable_x11
  if enable_glx
    libdir = run_command('pkg-config', '--variable=libdir', 'gl', check: true).stdout().strip()
    if not cc.has_function('glXCreateContext', dependencies: cc.find_library('GL', dirs: libdir))
      enable_glx = false
    endif
  endif
  if not enable_glx and not enable_egl
    enable_x11 = false
  endif
else
  enable_glx = false
endif

if enable_directfb
  if enable_dfbgl
    message('Checking for DirectFBGL module')
    moduledir = run_command('pkg-config', '--variable=moduledir', 'directfb-internal', check: false).stdout().strip()
    if import('fs').is_dir(moduledir + '/interfaces/IDirectFBGL')
      message('DirectFBGL found')
    else
      message('DirectFBGL not found')
      enable_dfbgl = false
    endif
  endif
  if not enable_dfbgl and not enable_egl
    enable_directfb = false
  endif
else
  enable_dfbgl = false
endif

if enable_fbdev
  if enable_glfbdev
    libdir = run_command('pkg-config', '--variable=libdir', 'gl', check: true).stdout().strip()
    if not cc.has_function('glFBDevCreateContext', dependencies: cc.find_library('GL', dirs: libdir))
      enable_glfbdev = false
    endif
  endif
  if not enable_glfbdev and not enable_egl
    enable_fbdev = false
  endif
else
  enable_glfbdev = false
endif

if enable_wayland
  if not enable_egl
    enable_wayland = false
  endif
endif

if enable_glx or enable_glfbdev
  gl_dep = dependency('gl', required: false)
  if not gl_dep.found()
    enable_glx = false
    enable_glfbdev = false
  endif
endif

if not enable_egl and not enable_glx and not enable_dfbgl and not enable_glfbdev
  error('No backends found')
endif

#########
# Tests #
#########

if enable_tests
  check_dep = dependency('check', required: false)
  libfiu_dep = dependency('libfiu', required: false)
  if not check_dep.found() or not libfiu_dep.found()
    enable_tests = false
  else
    add_global_arguments('-DFIU_ENABLE', language: 'c')
  endif
endif

#######################
# Build configuration #
#######################

message('')
message('GLUT backends:')
message('')
message('  EGL     (Native Platform Graphics Interface)  @0@'.format(enable_egl))
if enable_egl
message('')
message('           EGL platforms:')
message('                               X11              @0@'.format(enable_x11))
message('                               DirectFB         @0@'.format(enable_directfb))
message('                               FBDev            @0@'.format(enable_fbdev))
message('                               Wayland          @0@'.format(enable_wayland))
message('')
endif
message('  GLX     (OpenGL Extension to X11)             @0@'.format(enable_glx))
message('  DFBGL   (OpenGL Extension to DirectFB)        @0@'.format(enable_dfbgl))
message('  GLFBDev (OpenGL Extension to FBDev)           @0@'.format(enable_glfbdev))
message('')
message('Tests: @0@'.format(enable_tests))
message('')

###############
# Build rules #
###############

# Platforms plugins

platformsdir = join_paths(get_option('prefix'), get_option('libdir'), 'glut/platforms')

if enable_dummy
  library('dummy_plugin', 'dummy.c',
          name_prefix: '',
          install: true,
          install_dir: platformsdir)
endif

if enable_x11
  x11_plugin = library('x11_plugin', 'x11.c',
                       dependencies: x11_dep,
                       name_prefix: '',
                       install: true,
                       install_dir: platformsdir)
endif

if enable_directfb
  directfb_plugin = library('directfb_plugin', 'directfb.c',
                            dependencies: directfb_dep,
                            name_prefix: '',
                            install: true,
                            install_dir: platformsdir)
endif

if enable_fbdev
  fbdev_plugin = library('fbdev_plugin', 'fbdev.c',
                         dependencies: dependency('threads'),
                         name_prefix: '',
                         install: true,
                         install_dir: platformsdir)
endif

if enable_wayland
  library('wayland_plugin', 'wayland.c',
          dependencies: wayland_dep,
          name_prefix: '',
          install: true,
          install_dir: platformsdir)
endif

# Backends plugins

backendsdir = join_paths(get_option('prefix'), get_option('libdir'), 'glut/backends')

if enable_egl
  library('egl_plugin', 'egl.c',
          c_args: '-DPLATFORMSDIR="' + platformsdir + '"',
          dependencies: [egl_dep, libfiu_dep, dependency('dl')],
          name_prefix: '',
          install: true,
          install_dir: backendsdir)
endif

if enable_glx
  library('glx_plugin', 'glx.c',
          build_rpath: platformsdir,
          dependencies: [gl_dep, libfiu_dep],
          link_with: x11_plugin,
          name_prefix: '',
          install: true,
          install_dir: backendsdir)
endif

if enable_dfbgl
  library('dfbgl_plugin', 'dfbgl.c',
          build_rpath: platformsdir,
          dependencies: [directfb_dep, libfiu_dep],
          link_with: directfb_plugin,
          name_prefix: '',
          install: true,
          install_dir: backendsdir)
endif

if enable_glfbdev
  library('glfbdev_plugin', 'glfbdev.c',
          build_rpath: platformsdir,
          dependencies: [gl_dep, libfiu_dep],
          link_with: fbdev_plugin,
          name_prefix: '',
          install: true,
          install_dir: backendsdir)
endif

# GLUT library

libglut = library('glut', 'glut.c',
                  c_args: '-DBACKENDSDIR="' + backendsdir + '"',
                  dependencies: [libfiu_dep, dependency('dl')],
                  version: '3.0.0',
                  install: true)

install_headers('glut.h', subdir: 'GL')

pkgconfig.generate(filebase: 'glut',
                   name: 'TinyGLUT',
                   description: 'Small implementation of GLUT (OpenGL Utility Toolkit)',
                   libraries: '-L${libdir} -lglut')

if enable_tests
  glut_tests = executable('glut-tests', 'glut-tests.c',
                          build_rpath: join_paths(get_option('prefix'), get_option('libdir')),
                          link_with: libglut,
                          dependencies: [check_dep, libfiu_dep])
  test('glut-tests', glut_tests)
endif
