# library name
set(Output pcsx2_SDL)

set (SDL_ROOT "${PROJECT_SOURCE_DIR}/3rdparty/SDL-1.3.0-5387")

# set common flags
set(CommonFlags 
    -DHAVE_LINUX_VERSION_H
    -D_REENTRANT 
    -D__LINUX__
	-pthread
    -mmmx
    -msse
    -msse2
    -fvisibility=hidden
	-pipe
    -w
    )

# set warning flags
set(DebugFlags
	-g
    )

# set optimization flags
set(OptimizationFlags
    -O3
    )

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(${CommonFlags} ${DebugFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

if(CMAKE_BUILD_TYPE STREQUAL Devel)
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

if(CMAKE_BUILD_TYPE STREQUAL Release)
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Release)


#Make on big arrary with all the source and header files
set(SDL_sources

   #Headers
   "${SDL_ROOT}/include/begin_code.h"
   "${SDL_ROOT}/include/close_code.h"
   "${SDL_ROOT}/include/SDL_assert.h"
   "${SDL_ROOT}/include/SDL_atomic.h"
   "${SDL_ROOT}/include/SDL_audio.h"
   "${SDL_ROOT}/include/SDL_blendmode.h"
   "${SDL_ROOT}/include/SDL_clipboard.h"
   "${SDL_ROOT}/include/SDL_compat.h"
   "${SDL_ROOT}/include/SDL_config.h"
   "${SDL_ROOT}/include/SDL_config_macosx.h"
   "${SDL_ROOT}/include/SDL_config_minimal.h"
   "${SDL_ROOT}/include/SDL_config_windows.h"
   "${SDL_ROOT}/include/SDL_config_wiz.h"
   "${SDL_ROOT}/include/SDL_copying.h"
   "${SDL_ROOT}/include/SDL_cpuinfo.h"
   "${SDL_ROOT}/include/SDL_endian.h"
   "${SDL_ROOT}/include/SDL_error.h"
   "${SDL_ROOT}/include/SDL_events.h"
   "${SDL_ROOT}/include/SDL_gesture.h"
   "${SDL_ROOT}/include/SDL.h"
   "${SDL_ROOT}/include/SDL_haptic.h"
   "${SDL_ROOT}/include/SDL_hints.h"
   "${SDL_ROOT}/include/SDL_input.h"
   "${SDL_ROOT}/include/SDL_joystick.h"
   "${SDL_ROOT}/include/SDL_keyboard.h"
   "${SDL_ROOT}/include/SDL_keycode.h"
   "${SDL_ROOT}/include/SDL_loadso.h"
   "${SDL_ROOT}/include/SDL_log.h"
   "${SDL_ROOT}/include/SDL_main.h"
   "${SDL_ROOT}/include/SDL_mouse.h"
   "${SDL_ROOT}/include/SDL_mutex.h"
   "${SDL_ROOT}/include/SDL_name.h"
   "${SDL_ROOT}/include/SDL_opengles2.h"
   "${SDL_ROOT}/include/SDL_opengles.h"
   "${SDL_ROOT}/include/SDL_opengl.h"
   "${SDL_ROOT}/include/SDL_pixels.h"
   "${SDL_ROOT}/include/SDL_platform.h"
   "${SDL_ROOT}/include/SDL_power.h"
   "${SDL_ROOT}/include/SDL_quit.h"
   "${SDL_ROOT}/include/SDL_rect.h"
   "${SDL_ROOT}/include/SDL_render.h"
   "${SDL_ROOT}/include/SDL_revision.h"
   "${SDL_ROOT}/include/SDL_rwops.h"
   "${SDL_ROOT}/include/SDL_scancode.h"
   "${SDL_ROOT}/include/SDL_shape.h"
   "${SDL_ROOT}/include/SDL_stdinc.h"
   "${SDL_ROOT}/include/SDL_surface.h"
   "${SDL_ROOT}/include/SDL_syswm.h"
   "${SDL_ROOT}/include/SDL_thread.h"
   "${SDL_ROOT}/include/SDL_timer.h"
   "${SDL_ROOT}/include/SDL_touch.h"
   "${SDL_ROOT}/include/SDL_types.h"
   "${SDL_ROOT}/include/SDL_version.h"
   "${SDL_ROOT}/include/SDL_video.h"

   # Source Files
   "${SDL_ROOT}/src/SDL_assert.c"
   "${SDL_ROOT}/src/SDL_assert_c.h"
   "${SDL_ROOT}/src/SDL.c"
   "${SDL_ROOT}/src/SDL_compat.c"
   "${SDL_ROOT}/src/SDL_error.c"
   "${SDL_ROOT}/src/SDL_error_c.h"
   "${SDL_ROOT}/src/SDL_fatal.c"
   "${SDL_ROOT}/src/SDL_fatal.h"
   "${SDL_ROOT}/src/SDL_hints.c"
   "${SDL_ROOT}/src/SDL_log.c"

   #c/atomic:
   # "${SDL_ROOT}/src/atomic/SDL_atomic.c"
   "${SDL_ROOT}/src/atomic/SDL_spinlock.c"

   #c/audio:
   "${SDL_ROOT}/src/audio/SDL_audio.c"
   "${SDL_ROOT}/src/audio/SDL_audio_c.h"
   "${SDL_ROOT}/src/audio/SDL_audiocvt.c"
   "${SDL_ROOT}/src/audio/SDL_audiodev.c"
   "${SDL_ROOT}/src/audio/SDL_audiodev_c.h"
   "${SDL_ROOT}/src/audio/SDL_audiomem.h"
   "${SDL_ROOT}/src/audio/SDL_audiotypecvt.c"
   "${SDL_ROOT}/src/audio/SDL_mixer.c"
   "${SDL_ROOT}/src/audio/SDL_mixer_m68k.c"
   "${SDL_ROOT}/src/audio/SDL_mixer_m68k.h"
   "${SDL_ROOT}/src/audio/SDL_mixer_MMX.c"
   "${SDL_ROOT}/src/audio/SDL_mixer_MMX.h"
   "${SDL_ROOT}/src/audio/SDL_mixer_MMX_VC.c"
   "${SDL_ROOT}/src/audio/SDL_mixer_MMX_VC.h"
   "${SDL_ROOT}/src/audio/SDL_sysaudio.h"
   "${SDL_ROOT}/src/audio/SDL_wave.c"
   "${SDL_ROOT}/src/audio/SDL_wave.h"

   #c/cpuinfo:
   "${SDL_ROOT}/src/cpuinfo/SDL_cpuinfo.c"

   #c/events:
   "${SDL_ROOT}/src/events/blank_cursor.h"
   "${SDL_ROOT}/src/events/default_cursor.h"
   "${SDL_ROOT}/src/events/scancodes_darwin.h"
   "${SDL_ROOT}/src/events/scancodes_linux.h"
   "${SDL_ROOT}/src/events/scancodes_windows.h"
   "${SDL_ROOT}/src/events/scancodes_xfree86.h"
   "${SDL_ROOT}/src/events/SDL_clipboardevents.c"
   "${SDL_ROOT}/src/events/SDL_clipboardevents_c.h"
   "${SDL_ROOT}/src/events/SDL_events.c"
   "${SDL_ROOT}/src/events/SDL_events_c.h"
   "${SDL_ROOT}/src/events/SDL_gesture.c"
   "${SDL_ROOT}/src/events/SDL_gesture_c.h"
   "${SDL_ROOT}/src/events/SDL_keyboard.c"
   "${SDL_ROOT}/src/events/SDL_keyboard_c.h"
   "${SDL_ROOT}/src/events/SDL_mouse.c"
   "${SDL_ROOT}/src/events/SDL_mouse_c.h"
   "${SDL_ROOT}/src/events/SDL_quit.c"
   "${SDL_ROOT}/src/events/SDL_sysevents.h"
   "${SDL_ROOT}/src/events/SDL_touch.c"
   "${SDL_ROOT}/src/events/SDL_touch_c.h"
   "${SDL_ROOT}/src/events/SDL_windowevents.c"
   "${SDL_ROOT}/src/events/SDL_windowevents_c.h"

   #c/file:
   "${SDL_ROOT}/src/file/SDL_rwops.c"

   #c/haptic:
   "${SDL_ROOT}/src/haptic/SDL_haptic.c"
   "${SDL_ROOT}/src/haptic/SDL_haptic_c.h"
   "${SDL_ROOT}/src/haptic/SDL_syshaptic.h"

   #c/joystick:
   "${SDL_ROOT}/src/joystick/SDL_joystick.c"
   "${SDL_ROOT}/src/joystick/SDL_joystick_c.h"
   "${SDL_ROOT}/src/joystick/SDL_sysjoystick.h"

   #c/libm:
   #"${SDL_ROOT}/src/libm/e_atan2.c"
   #"${SDL_ROOT}/src/libm/e_log.c"
   #"${SDL_ROOT}/src/libm/e_pow.c"
   #"${SDL_ROOT}/src/libm/e_rem_pio2.c"
   #"${SDL_ROOT}/src/libm/e_sqrt.c"
   #"${SDL_ROOT}/src/libm/k_cos.c"
   #"${SDL_ROOT}/src/libm/k_rem_pio2.c"
   #"${SDL_ROOT}/src/libm/k_sin.c"
   #"${SDL_ROOT}/src/libm/math.h"
   #"${SDL_ROOT}/src/libm/math_private.h"
   #"${SDL_ROOT}/src/libm/s_atan.c"
   #"${SDL_ROOT}/src/libm/s_copysign.c"
   #"${SDL_ROOT}/src/libm/s_cos.c"
   #"${SDL_ROOT}/src/libm/s_fabs.c"
   #"${SDL_ROOT}/src/libm/s_floor.c"
   #"${SDL_ROOT}/src/libm/s_scalbn.c"
   #"${SDL_ROOT}/src/libm/s_sin.c"

   #c/power:
   "${SDL_ROOT}/src/power/SDL_power.c"

   #c/render:
   "${SDL_ROOT}/src/render/mmx.h"
   "${SDL_ROOT}/src/render/SDL_render.c"
   "${SDL_ROOT}/src/render/SDL_sysrender.h"
   "${SDL_ROOT}/src/render/SDL_yuv_mmx.c"
   "${SDL_ROOT}/src/render/SDL_yuv_sw.c"
   "${SDL_ROOT}/src/render/SDL_yuv_sw_c.h"

   #c/stdlib:
   "${SDL_ROOT}/src/stdlib/SDL_getenv.c"
   "${SDL_ROOT}/src/stdlib/SDL_iconv.c"
   "${SDL_ROOT}/src/stdlib/SDL_malloc.c"
   "${SDL_ROOT}/src/stdlib/SDL_qsort.c"
   "${SDL_ROOT}/src/stdlib/SDL_stdlib.c"
   "${SDL_ROOT}/src/stdlib/SDL_string.c"

   #c/thread:
   "${SDL_ROOT}/src/thread/SDL_systhread.h"
   "${SDL_ROOT}/src/thread/SDL_thread.c"
   "${SDL_ROOT}/src/thread/SDL_thread_c.h"

   #c/timer:
   "${SDL_ROOT}/src/timer/SDL_timer.c"
   "${SDL_ROOT}/src/timer/SDL_timer_c.h"

   #c/video:
   "${SDL_ROOT}/src/video/SDL_blit_0.c"
   "${SDL_ROOT}/src/video/SDL_blit_1.c"
   "${SDL_ROOT}/src/video/SDL_blit_A.c"
   "${SDL_ROOT}/src/video/SDL_blit_auto.c"
   "${SDL_ROOT}/src/video/SDL_blit_auto.h"
   "${SDL_ROOT}/src/video/SDL_blit.c"
   "${SDL_ROOT}/src/video/SDL_blit_copy.c"
   "${SDL_ROOT}/src/video/SDL_blit_copy.h"
   "${SDL_ROOT}/src/video/SDL_blit.h"
   "${SDL_ROOT}/src/video/SDL_blit_N.c"
   "${SDL_ROOT}/src/video/SDL_blit_slow.c"
   "${SDL_ROOT}/src/video/SDL_blit_slow.h"
   "${SDL_ROOT}/src/video/SDL_bmp.c"
   "${SDL_ROOT}/src/video/SDL_clipboard.c"
   "${SDL_ROOT}/src/video/SDL_fillrect.c"
   "${SDL_ROOT}/src/video/SDL_pixels.c"
   "${SDL_ROOT}/src/video/SDL_pixels_c.h"
   "${SDL_ROOT}/src/video/SDL_rect.c"
   "${SDL_ROOT}/src/video/SDL_rect_c.h"
   "${SDL_ROOT}/src/video/SDL_RLEaccel.c"
   "${SDL_ROOT}/src/video/SDL_RLEaccel_c.h"
   "${SDL_ROOT}/src/video/SDL_shape.c"
   "${SDL_ROOT}/src/video/SDL_shape_internals.h"
   "${SDL_ROOT}/src/video/SDL_stretch.c"
   "${SDL_ROOT}/src/video/SDL_surface.c"
   "${SDL_ROOT}/src/video/SDL_sysvideo.h"
   "${SDL_ROOT}/src/video/SDL_video.c"

   #re deeply nestedfiles that may need some platform specific logic to determine inclusion
   "${SDL_ROOT}/src/audio/dummy/SDL_dummyaudio.c"
   "${SDL_ROOT}/src/audio/dummy/SDL_dummyaudio.h"
   "${SDL_ROOT}/src/haptic/dummy/SDL_syshaptic.c"
   "${SDL_ROOT}/src/haptic/linux/SDL_syshaptic.c"
   "${SDL_ROOT}/src/joystick/dummy/SDL_sysjoystick.c"
   "${SDL_ROOT}/src/joystick/linux/SDL_sysjoystick.c"
   "${SDL_ROOT}/src/joystick/linux/SDL_sysjoystick_c.h"
   "${SDL_ROOT}/src/loadso/dlopen/SDL_sysloadso.c"
   "${SDL_ROOT}/src/loadso/dummy/SDL_sysloadso.c"
   "${SDL_ROOT}/src/loadso/macosx/SDL_dlcompat.c"
   "${SDL_ROOT}/src/main/dummy/SDL_dummy_main.c"
   "${SDL_ROOT}/src/power/linux/SDL_syspower.c"
   "${SDL_ROOT}/src/render/direct3d/SDL_render_d3d.c"
   "${SDL_ROOT}/src/render/opengles2/SDL_render_gles2.c"
   "${SDL_ROOT}/src/render/opengles2/SDL_shaders_gles2.c"
   "${SDL_ROOT}/src/render/opengles2/SDL_shaders_gles2.h"
   "${SDL_ROOT}/src/render/opengles/SDL_render_gles.c"
   "${SDL_ROOT}/src/render/opengl/SDL_glfuncs.h"
   "${SDL_ROOT}/src/render/opengl/SDL_render_gl.c"
   "${SDL_ROOT}/src/render/opengl/SDL_shaders_gl.c"
   "${SDL_ROOT}/src/render/opengl/SDL_shaders_gl.h"
   "${SDL_ROOT}/src/render/software/SDL_blendfillrect.c"
   "${SDL_ROOT}/src/render/software/SDL_blendfillrect.h"
   "${SDL_ROOT}/src/render/software/SDL_blendline.c"
   "${SDL_ROOT}/src/render/software/SDL_blendline.h"
   "${SDL_ROOT}/src/render/software/SDL_blendpoint.c"
   "${SDL_ROOT}/src/render/software/SDL_blendpoint.h"
   "${SDL_ROOT}/src/render/software/SDL_draw.h"
   "${SDL_ROOT}/src/render/software/SDL_drawline.c"
   "${SDL_ROOT}/src/render/software/SDL_drawline.h"
   "${SDL_ROOT}/src/render/software/SDL_drawpoint.c"
   "${SDL_ROOT}/src/render/software/SDL_drawpoint.h"
   "${SDL_ROOT}/src/render/software/SDL_render_sw.c"
   "${SDL_ROOT}/src/render/software/SDL_render_sw_c.h"
   # "${SDL_ROOT}/src/thread/generic/SDL_syscond.c"
   # "${SDL_ROOT}/src/thread/generic/SDL_sysmutex.c"
   # "${SDL_ROOT}/src/thread/generic/SDL_sysmutex_c.h"
   # "${SDL_ROOT}/src/thread/generic/SDL_syssem.c"
   # "${SDL_ROOT}/src/thread/generic/SDL_systhread.c"
   # "${SDL_ROOT}/src/thread/generic/SDL_systhread_c.h"
   "${SDL_ROOT}/src/thread/pthread/SDL_syscond.c"
   "${SDL_ROOT}/src/thread/pthread/SDL_sysmutex.c"
   "${SDL_ROOT}/src/thread/pthread/SDL_sysmutex_c.h"
   "${SDL_ROOT}/src/thread/pthread/SDL_syssem.c"
   "${SDL_ROOT}/src/thread/pthread/SDL_systhread.c"
   "${SDL_ROOT}/src/thread/pthread/SDL_systhread_c.h"
   "${SDL_ROOT}/src/timer/dummy/SDL_systimer.c"
   "${SDL_ROOT}/src/timer/unix/SDL_systimer.c"
   # "${SDL_ROOT}/src/timer/wince/SDL_systimer.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_dyn.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_dyn.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_events.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_events.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_modes.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_modes.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_mouse.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_mouse.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_opengl.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_opengl.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_render.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_render.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_video.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_video.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_window.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_window.h"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_WM.c"
   # "${SDL_ROOT}/src/video/directfb/SDL_DirectFB_WM.h"
   "${SDL_ROOT}/src/video/dummy/SDL_nullevents.c"
   "${SDL_ROOT}/src/video/dummy/SDL_nullevents_c.h"
   "${SDL_ROOT}/src/video/dummy/SDL_nullframebuffer.c"
   "${SDL_ROOT}/src/video/dummy/SDL_nullframebuffer_c.h"
   "${SDL_ROOT}/src/video/dummy/SDL_nullvideo.c"
   "${SDL_ROOT}/src/video/dummy/SDL_nullvideo.h"
   "${SDL_ROOT}/src/video/uikit/jumphack.c"
   "${SDL_ROOT}/src/video/uikit/jumphack.h"
   "${SDL_ROOT}/src/video/uikit/keyinfotable.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitappdelegate.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitevents.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitkeyboard.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitopengles.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitopenglview.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitvideo.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitview.h"
   "${SDL_ROOT}/src/video/uikit/SDL_uikitwindow.h"
   "${SDL_ROOT}/src/video/x11/imKStoUCS.c"
   "${SDL_ROOT}/src/video/x11/imKStoUCS.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11clipboard.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11clipboard.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11dyn.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11dyn.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11events.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11events.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11framebuffer.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11framebuffer.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11keyboard.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11keyboard.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11modes.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11modes.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11mouse.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11mouse.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11opengl.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11opengles.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11opengles.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11opengl.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11shape.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11shape.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11sym.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11touch.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11touch.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11video.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11video.h"
   "${SDL_ROOT}/src/video/x11/SDL_x11window.c"
   "${SDL_ROOT}/src/video/x11/SDL_x11window.h"

   "${SDL_ROOT}/src/video/Xext/extensions/extutil.h"
   "${SDL_ROOT}/src/video/Xext/extensions/panoramiXext.h"
   "${SDL_ROOT}/src/video/Xext/extensions/panoramiXproto.h"
   "${SDL_ROOT}/src/video/Xext/extensions/Xext.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86dga1.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86dga1str.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86dga.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86dgastr.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86vmode.h"
   "${SDL_ROOT}/src/video/Xext/extensions/xf86vmstr.h"
   "${SDL_ROOT}/src/video/Xext/extensions/Xinerama.h"
   "${SDL_ROOT}/src/video/Xext/extensions/Xv.h"
   "${SDL_ROOT}/src/video/Xext/extensions/Xvlib.h"
   "${SDL_ROOT}/src/video/Xext/extensions/Xvproto.h"
   "${SDL_ROOT}/src/video/Xext/Xinerama/Xinerama.c"
   "${SDL_ROOT}/src/video/Xext/XME/xme.c"
   "${SDL_ROOT}/src/video/Xext/Xv/Xv.c"
   "${SDL_ROOT}/src/video/Xext/Xv/Xvlibint.h"
   "${SDL_ROOT}/src/video/Xext/Xxf86vm/XF86VMode.c"
)

#include_directories( ${SDL_ROOT}/include /usr/include/kde/artsc /usr/include/directfb)


# Hiden option. It is only a temporary workaround of issue 1003. The purpose is to LD_PRELOAD
# the library so it avoid any conflict with previous SDL1.2 symbol
if (DEFINED SHARED_SDL)
    add_library(${Output} SHARED ${SDL_sources} )

    # Seem to be needed when pre-loading on Fedora
    target_link_libraries(${Output} dl)
    target_link_libraries(${Output} m)

    # Install the library with others plugins
    if(PACKAGE_MODE)
        install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})
    else(PACKAGE_MODE)
        install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    endif(PACKAGE_MODE)

else (DEFINED SHARED_SDL)
    # Std build use static build
    add_library(${Output} STATIC ${SDL_sources} )
endif (DEFINED SHARED_SDL)


# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
