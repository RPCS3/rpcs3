#ifndef __wglext_h_
#define __wglext_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
** Copyright (c) 2007 The Khronos Group Inc.
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
** 
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/*************************************************************/

/* Header file version number */
/* wglext.h last updated 2009/03/03 */
/* Current version at http://www.opengl.org/registry/ */
#define WGL_WGLEXT_VERSION 12

#ifndef WGL_ARB_buffer_region
#define WGL_FRONT_COLOR_BUFFER_BIT_ARB 0x00000001
#define WGL_BACK_COLOR_BUFFER_BIT_ARB  0x00000002
#define WGL_DEPTH_BUFFER_BIT_ARB       0x00000004
#define WGL_STENCIL_BUFFER_BIT_ARB     0x00000008
#endif

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042
#endif

#ifndef WGL_ARB_extensions_string
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_ARB   0x2043
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB        0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB     0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB      0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB     0x2030
#define WGL_PBUFFER_LARGEST_ARB        0x2033
#define WGL_PBUFFER_WIDTH_ARB          0x2034
#define WGL_PBUFFER_HEIGHT_ARB         0x2035
#define WGL_PBUFFER_LOST_ARB           0x2036
#endif

#ifndef WGL_ARB_render_texture
#define WGL_BIND_TO_TEXTURE_RGB_ARB    0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB   0x2071
#define WGL_TEXTURE_FORMAT_ARB         0x2072
#define WGL_TEXTURE_TARGET_ARB         0x2073
#define WGL_MIPMAP_TEXTURE_ARB         0x2074
#define WGL_TEXTURE_RGB_ARB            0x2075
#define WGL_TEXTURE_RGBA_ARB           0x2076
#define WGL_NO_TEXTURE_ARB             0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB       0x2078
#define WGL_TEXTURE_1D_ARB             0x2079
#define WGL_TEXTURE_2D_ARB             0x207A
#define WGL_MIPMAP_LEVEL_ARB           0x207B
#define WGL_CUBE_MAP_FACE_ARB          0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#define WGL_FRONT_LEFT_ARB             0x2083
#define WGL_FRONT_RIGHT_ARB            0x2084
#define WGL_BACK_LEFT_ARB              0x2085
#define WGL_BACK_RIGHT_ARB             0x2086
#define WGL_AUX0_ARB                   0x2087
#define WGL_AUX1_ARB                   0x2088
#define WGL_AUX2_ARB                   0x2089
#define WGL_AUX3_ARB                   0x208A
#define WGL_AUX4_ARB                   0x208B
#define WGL_AUX5_ARB                   0x208C
#define WGL_AUX6_ARB                   0x208D
#define WGL_AUX7_ARB                   0x208E
#define WGL_AUX8_ARB                   0x208F
#define WGL_AUX9_ARB                   0x2090
#endif

#ifndef WGL_ARB_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ARB        0x21A0
#endif

#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_DEBUG_BIT_ARB      0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB    0x2093
#define WGL_CONTEXT_FLAGS_ARB          0x2094
#define ERROR_INVALID_VERSION_ARB      0x2095
#endif

#ifndef WGL_EXT_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_EXT   0x2043
#endif

#ifndef WGL_EXT_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_EXT   0x2000
#define WGL_DRAW_TO_WINDOW_EXT         0x2001
#define WGL_DRAW_TO_BITMAP_EXT         0x2002
#define WGL_ACCELERATION_EXT           0x2003
#define WGL_NEED_PALETTE_EXT           0x2004
#define WGL_NEED_SYSTEM_PALETTE_EXT    0x2005
#define WGL_SWAP_LAYER_BUFFERS_EXT     0x2006
#define WGL_SWAP_METHOD_EXT            0x2007
#define WGL_NUMBER_OVERLAYS_EXT        0x2008
#define WGL_NUMBER_UNDERLAYS_EXT       0x2009
#define WGL_TRANSPARENT_EXT            0x200A
#define WGL_TRANSPARENT_VALUE_EXT      0x200B
#define WGL_SHARE_DEPTH_EXT            0x200C
#define WGL_SHARE_STENCIL_EXT          0x200D
#define WGL_SHARE_ACCUM_EXT            0x200E
#define WGL_SUPPORT_GDI_EXT            0x200F
#define WGL_SUPPORT_OPENGL_EXT         0x2010
#define WGL_DOUBLE_BUFFER_EXT          0x2011
#define WGL_STEREO_EXT                 0x2012
#define WGL_PIXEL_TYPE_EXT             0x2013
#define WGL_COLOR_BITS_EXT             0x2014
#define WGL_RED_BITS_EXT               0x2015
#define WGL_RED_SHIFT_EXT              0x2016
#define WGL_GREEN_BITS_EXT             0x2017
#define WGL_GREEN_SHIFT_EXT            0x2018
#define WGL_BLUE_BITS_EXT              0x2019
#define WGL_BLUE_SHIFT_EXT             0x201A
#define WGL_ALPHA_BITS_EXT             0x201B
#define WGL_ALPHA_SHIFT_EXT            0x201C
#define WGL_ACCUM_BITS_EXT             0x201D
#define WGL_ACCUM_RED_BITS_EXT         0x201E
#define WGL_ACCUM_GREEN_BITS_EXT       0x201F
#define WGL_ACCUM_BLUE_BITS_EXT        0x2020
#define WGL_ACCUM_ALPHA_BITS_EXT       0x2021
#define WGL_DEPTH_BITS_EXT             0x2022
#define WGL_STENCIL_BITS_EXT           0x2023
#define WGL_AUX_BUFFERS_EXT            0x2024
#define WGL_NO_ACCELERATION_EXT        0x2025
#define WGL_GENERIC_ACCELERATION_EXT   0x2026
#define WGL_FULL_ACCELERATION_EXT      0x2027
#define WGL_SWAP_EXCHANGE_EXT          0x2028
#define WGL_SWAP_COPY_EXT              0x2029
#define WGL_SWAP_UNDEFINED_EXT         0x202A
#define WGL_TYPE_RGBA_EXT              0x202B
#define WGL_TYPE_COLORINDEX_EXT        0x202C
#endif

#ifndef WGL_EXT_pbuffer
#define WGL_DRAW_TO_PBUFFER_EXT        0x202D
#define WGL_MAX_PBUFFER_PIXELS_EXT     0x202E
#define WGL_MAX_PBUFFER_WIDTH_EXT      0x202F
#define WGL_MAX_PBUFFER_HEIGHT_EXT     0x2030
#define WGL_OPTIMAL_PBUFFER_WIDTH_EXT  0x2031
#define WGL_OPTIMAL_PBUFFER_HEIGHT_EXT 0x2032
#define WGL_PBUFFER_LARGEST_EXT        0x2033
#define WGL_PBUFFER_WIDTH_EXT          0x2034
#define WGL_PBUFFER_HEIGHT_EXT         0x2035
#endif

#ifndef WGL_EXT_depth_float
#define WGL_DEPTH_FLOAT_EXT            0x2040
#endif

#ifndef WGL_3DFX_multisample
#define WGL_SAMPLE_BUFFERS_3DFX        0x2060
#define WGL_SAMPLES_3DFX               0x2061
#endif

#ifndef WGL_EXT_multisample
#define WGL_SAMPLE_BUFFERS_EXT         0x2041
#define WGL_SAMPLES_EXT                0x2042
#endif

#ifndef WGL_I3D_digital_video_control
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D 0x2050
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D 0x2051
#define WGL_DIGITAL_VIDEO_CURSOR_INCLUDED_I3D 0x2052
#define WGL_DIGITAL_VIDEO_GAMMA_CORRECTED_I3D 0x2053
#endif

#ifndef WGL_I3D_gamma
#define WGL_GAMMA_TABLE_SIZE_I3D       0x204E
#define WGL_GAMMA_EXCLUDE_DESKTOP_I3D  0x204F
#endif

#ifndef WGL_I3D_genlock
#define WGL_GENLOCK_SOURCE_MULTIVIEW_I3D 0x2044
#define WGL_GENLOCK_SOURCE_EXTENAL_SYNC_I3D 0x2045
#define WGL_GENLOCK_SOURCE_EXTENAL_FIELD_I3D 0x2046
#define WGL_GENLOCK_SOURCE_EXTENAL_TTL_I3D 0x2047
#define WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D 0x2048
#define WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D 0x2049
#define WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D 0x204A
#define WGL_GENLOCK_SOURCE_EDGE_RISING_I3D 0x204B
#define WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D 0x204C
#endif

#ifndef WGL_I3D_image_buffer
#define WGL_IMAGE_BUFFER_MIN_ACCESS_I3D 0x00000001
#define WGL_IMAGE_BUFFER_LOCK_I3D      0x00000002
#endif

#ifndef WGL_I3D_swap_frame_lock
#endif

#ifndef WGL_NV_render_depth_texture
#define WGL_BIND_TO_TEXTURE_DEPTH_NV   0x20A3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV 0x20A4
#define WGL_DEPTH_TEXTURE_FORMAT_NV    0x20A5
#define WGL_TEXTURE_DEPTH_COMPONENT_NV 0x20A6
#define WGL_DEPTH_COMPONENT_NV         0x20A7
#endif

#ifndef WGL_NV_render_texture_rectangle
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV 0x20A0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV 0x20A1
#define WGL_TEXTURE_RECTANGLE_NV       0x20A2
#endif

#ifndef WGL_ATI_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ATI        0x21A0
#endif

#ifndef WGL_NV_float_buffer
#define WGL_FLOAT_COMPONENTS_NV        0x20B0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV 0x20B1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV 0x20B2
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV 0x20B3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV 0x20B4
#define WGL_TEXTURE_FLOAT_R_NV         0x20B5
#define WGL_TEXTURE_FLOAT_RG_NV        0x20B6
#define WGL_TEXTURE_FLOAT_RGB_NV       0x20B7
#define WGL_TEXTURE_FLOAT_RGBA_NV      0x20B8
#endif

#ifndef WGL_3DL_stereo_control
#define WGL_STEREO_EMITTER_ENABLE_3DL  0x2055
#define WGL_STEREO_EMITTER_DISABLE_3DL 0x2056
#define WGL_STEREO_POLARITY_NORMAL_3DL 0x2057
#define WGL_STEREO_POLARITY_INVERT_3DL 0x2058
#endif

#ifndef WGL_EXT_pixel_format_packed_float
#define WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT 0x20A8
#endif

#ifndef WGL_EXT_framebuffer_sRGB
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9
#endif

#ifndef WGL_NV_present_video
#define WGL_NUM_VIDEO_SLOTS_NV         0x20F0
#endif

#ifndef WGL_NV_video_out
#define WGL_BIND_TO_VIDEO_RGB_NV       0x20C0
#define WGL_BIND_TO_VIDEO_RGBA_NV      0x20C1
#define WGL_BIND_TO_VIDEO_RGB_AND_DEPTH_NV 0x20C2
#define WGL_VIDEO_OUT_COLOR_NV         0x20C3
#define WGL_VIDEO_OUT_ALPHA_NV         0x20C4
#define WGL_VIDEO_OUT_DEPTH_NV         0x20C5
#define WGL_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define WGL_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define WGL_VIDEO_OUT_FRAME            0x20C8
#define WGL_VIDEO_OUT_FIELD_1          0x20C9
#define WGL_VIDEO_OUT_FIELD_2          0x20CA
#define WGL_VIDEO_OUT_STACKED_FIELDS_1_2 0x20CB
#define WGL_VIDEO_OUT_STACKED_FIELDS_2_1 0x20CC
#endif

#ifndef WGL_NV_swap_group
#endif

#ifndef WGL_NV_gpu_affinity
#define WGL_ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV 0x20D0
#define WGL_ERROR_MISSING_AFFINITY_MASK_NV 0x20D1
#endif

#ifndef WGL_AMD_gpu_association
#define WGL_GPU_VENDOR_AMD             0x1F00
#define WGL_GPU_RENDERER_STRING_AMD    0x1F01
#define WGL_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define WGL_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define WGL_GPU_RAM_AMD                0x21A3
#define WGL_GPU_CLOCK_AMD              0x21A4
#define WGL_GPU_NUM_PIPES_AMD          0x21A5
#define WGL_GPU_NUM_SIMD_AMD           0x21A6
#define WGL_GPU_NUM_RB_AMD             0x21A7
#define WGL_GPU_NUM_SPI_AMD            0x21A8
#endif


/*************************************************************/

#ifndef WGL_ARB_pbuffer
DECLARE_HANDLE(HPBUFFERARB);
#endif
#ifndef WGL_EXT_pbuffer
DECLARE_HANDLE(HPBUFFEREXT);
#endif
#ifndef WGL_NV_present_video
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
#endif
#ifndef WGL_NV_video_out
DECLARE_HANDLE(HPVIDEODEV);
#endif
#ifndef WGL_NV_gpu_affinity
DECLARE_HANDLE(HPGPUNV);
DECLARE_HANDLE(HGPUNV);

typedef struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
} GPU_DEVICE, *PGPU_DEVICE;
#endif

#ifndef WGL_ARB_buffer_region
#define WGL_ARB_buffer_region 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HANDLE WINAPI wglCreateBufferRegionARB (HDC, int, UINT);
extern VOID WINAPI wglDeleteBufferRegionARB (HANDLE);
extern BOOL WINAPI wglSaveBufferRegionARB (HANDLE, int, int, int, int);
extern BOOL WINAPI wglRestoreBufferRegionARB (HANDLE, int, int, int, int, int, int);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef HANDLE (WINAPI * PFNWGLCREATEBUFFERREGIONARBPROC) (HDC hDC, int iLayerPlane, UINT uType);
typedef VOID (WINAPI * PFNWGLDELETEBUFFERREGIONARBPROC) (HANDLE hRegion);
typedef BOOL (WINAPI * PFNWGLSAVEBUFFERREGIONARBPROC) (HANDLE hRegion, int x, int y, int width, int height);
typedef BOOL (WINAPI * PFNWGLRESTOREBUFFERREGIONARBPROC) (HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
#endif

#ifndef WGL_ARB_multisample
#define WGL_ARB_multisample 1
#endif

#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern const char * WINAPI wglGetExtensionsStringARB (HDC);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetPixelFormatAttribivARB (HDC, int, int, UINT, const int *, int *);
extern BOOL WINAPI wglGetPixelFormatAttribfvARB (HDC, int, int, UINT, const int *, FLOAT *);
extern BOOL WINAPI wglChoosePixelFormatARB (HDC, const int *, const FLOAT *, UINT, int *, UINT *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBFVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
#endif

#ifndef WGL_ARB_make_current_read
#define WGL_ARB_make_current_read 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglMakeContextCurrentARB (HDC, HDC, HGLRC);
extern HDC WINAPI wglGetCurrentReadDCARB (void);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLMAKECONTEXTCURRENTARBPROC) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
typedef HDC (WINAPI * PFNWGLGETCURRENTREADDCARBPROC) (void);
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HPBUFFERARB WINAPI wglCreatePbufferARB (HDC, int, int, int, const int *);
extern HDC WINAPI wglGetPbufferDCARB (HPBUFFERARB);
extern int WINAPI wglReleasePbufferDCARB (HPBUFFERARB, HDC);
extern BOOL WINAPI wglDestroyPbufferARB (HPBUFFERARB);
extern BOOL WINAPI wglQueryPbufferARB (HPBUFFERARB, int, int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef HPBUFFERARB (WINAPI * PFNWGLCREATEPBUFFERARBPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (WINAPI * PFNWGLGETPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer);
typedef int (WINAPI * PFNWGLRELEASEPBUFFERDCARBPROC) (HPBUFFERARB hPbuffer, HDC hDC);
typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFERARBPROC) (HPBUFFERARB hPbuffer);
typedef BOOL (WINAPI * PFNWGLQUERYPBUFFERARBPROC) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
#endif

#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglBindTexImageARB (HPBUFFERARB, int);
extern BOOL WINAPI wglReleaseTexImageARB (HPBUFFERARB, int);
extern BOOL WINAPI wglSetPbufferAttribARB (HPBUFFERARB, const int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLBINDTEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLRELEASETEXIMAGEARBPROC) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLSETPBUFFERATTRIBARBPROC) (HPBUFFERARB hPbuffer, const int *piAttribList);
#endif

#ifndef WGL_ARB_pixel_format_float
#define WGL_ARB_pixel_format_float 1
#endif

#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HGLRC WINAPI wglCreateContextAttribsARB (HDC, HGLRC, const int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
#endif

#ifndef WGL_EXT_display_color_table
#define WGL_EXT_display_color_table 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern GLboolean WINAPI wglCreateDisplayColorTableEXT (GLushort);
extern GLboolean WINAPI wglLoadDisplayColorTableEXT (const GLushort *, GLuint);
extern GLboolean WINAPI wglBindDisplayColorTableEXT (GLushort);
extern VOID WINAPI wglDestroyDisplayColorTableEXT (GLushort);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef GLboolean (WINAPI * PFNWGLCREATEDISPLAYCOLORTABLEEXTPROC) (GLushort id);
typedef GLboolean (WINAPI * PFNWGLLOADDISPLAYCOLORTABLEEXTPROC) (const GLushort *table, GLuint length);
typedef GLboolean (WINAPI * PFNWGLBINDDISPLAYCOLORTABLEEXTPROC) (GLushort id);
typedef VOID (WINAPI * PFNWGLDESTROYDISPLAYCOLORTABLEEXTPROC) (GLushort id);
#endif

#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern const char * WINAPI wglGetExtensionsStringEXT (void);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
#endif

#ifndef WGL_EXT_make_current_read
#define WGL_EXT_make_current_read 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglMakeContextCurrentEXT (HDC, HDC, HGLRC);
extern HDC WINAPI wglGetCurrentReadDCEXT (void);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLMAKECONTEXTCURRENTEXTPROC) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
typedef HDC (WINAPI * PFNWGLGETCURRENTREADDCEXTPROC) (void);
#endif

#ifndef WGL_EXT_pbuffer
#define WGL_EXT_pbuffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern HPBUFFEREXT WINAPI wglCreatePbufferEXT (HDC, int, int, int, const int *);
extern HDC WINAPI wglGetPbufferDCEXT (HPBUFFEREXT);
extern int WINAPI wglReleasePbufferDCEXT (HPBUFFEREXT, HDC);
extern BOOL WINAPI wglDestroyPbufferEXT (HPBUFFEREXT);
extern BOOL WINAPI wglQueryPbufferEXT (HPBUFFEREXT, int, int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef HPBUFFEREXT (WINAPI * PFNWGLCREATEPBUFFEREXTPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (WINAPI * PFNWGLGETPBUFFERDCEXTPROC) (HPBUFFEREXT hPbuffer);
typedef int (WINAPI * PFNWGLRELEASEPBUFFERDCEXTPROC) (HPBUFFEREXT hPbuffer, HDC hDC);
typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFEREXTPROC) (HPBUFFEREXT hPbuffer);
typedef BOOL (WINAPI * PFNWGLQUERYPBUFFEREXTPROC) (HPBUFFEREXT hPbuffer, int iAttribute, int *piValue);
#endif

#ifndef WGL_EXT_pixel_format
#define WGL_EXT_pixel_format 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetPixelFormatAttribivEXT (HDC, int, int, UINT, int *, int *);
extern BOOL WINAPI wglGetPixelFormatAttribfvEXT (HDC, int, int, UINT, int *, FLOAT *);
extern BOOL WINAPI wglChoosePixelFormatEXT (HDC, const int *, const FLOAT *, UINT, int *, UINT *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVEXTPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBFVEXTPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATEXTPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
#endif

#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglSwapIntervalEXT (int);
extern int WINAPI wglGetSwapIntervalEXT (void);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);
#endif

#ifndef WGL_EXT_depth_float
#define WGL_EXT_depth_float 1
#endif

#ifndef WGL_NV_vertex_array_range
#define WGL_NV_vertex_array_range 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern void* WINAPI wglAllocateMemoryNV (GLsizei, GLfloat, GLfloat, GLfloat);
extern void WINAPI wglFreeMemoryNV (void *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef void* (WINAPI * PFNWGLALLOCATEMEMORYNVPROC) (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
typedef void (WINAPI * PFNWGLFREEMEMORYNVPROC) (void *pointer);
#endif

#ifndef WGL_3DFX_multisample
#define WGL_3DFX_multisample 1
#endif

#ifndef WGL_EXT_multisample
#define WGL_EXT_multisample 1
#endif

#ifndef WGL_OML_sync_control
#define WGL_OML_sync_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetSyncValuesOML (HDC, INT64 *, INT64 *, INT64 *);
extern BOOL WINAPI wglGetMscRateOML (HDC, INT32 *, INT32 *);
extern INT64 WINAPI wglSwapBuffersMscOML (HDC, INT64, INT64, INT64);
extern INT64 WINAPI wglSwapLayerBuffersMscOML (HDC, int, INT64, INT64, INT64);
extern BOOL WINAPI wglWaitForMscOML (HDC, INT64, INT64, INT64, INT64 *, INT64 *, INT64 *);
extern BOOL WINAPI wglWaitForSbcOML (HDC, INT64, INT64 *, INT64 *, INT64 *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETSYNCVALUESOMLPROC) (HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc);
typedef BOOL (WINAPI * PFNWGLGETMSCRATEOMLPROC) (HDC hdc, INT32 *numerator, INT32 *denominator);
typedef INT64 (WINAPI * PFNWGLSWAPBUFFERSMSCOMLPROC) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
typedef INT64 (WINAPI * PFNWGLSWAPLAYERBUFFERSMSCOMLPROC) (HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
typedef BOOL (WINAPI * PFNWGLWAITFORMSCOMLPROC) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc);
typedef BOOL (WINAPI * PFNWGLWAITFORSBCOMLPROC) (HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc);
#endif

#ifndef WGL_I3D_digital_video_control
#define WGL_I3D_digital_video_control 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetDigitalVideoParametersI3D (HDC, int, int *);
extern BOOL WINAPI wglSetDigitalVideoParametersI3D (HDC, int, const int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETDIGITALVIDEOPARAMETERSI3DPROC) (HDC hDC, int iAttribute, int *piValue);
typedef BOOL (WINAPI * PFNWGLSETDIGITALVIDEOPARAMETERSI3DPROC) (HDC hDC, int iAttribute, const int *piValue);
#endif

#ifndef WGL_I3D_gamma
#define WGL_I3D_gamma 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetGammaTableParametersI3D (HDC, int, int *);
extern BOOL WINAPI wglSetGammaTableParametersI3D (HDC, int, const int *);
extern BOOL WINAPI wglGetGammaTableI3D (HDC, int, USHORT *, USHORT *, USHORT *);
extern BOOL WINAPI wglSetGammaTableI3D (HDC, int, const USHORT *, const USHORT *, const USHORT *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETGAMMATABLEPARAMETERSI3DPROC) (HDC hDC, int iAttribute, int *piValue);
typedef BOOL (WINAPI * PFNWGLSETGAMMATABLEPARAMETERSI3DPROC) (HDC hDC, int iAttribute, const int *piValue);
typedef BOOL (WINAPI * PFNWGLGETGAMMATABLEI3DPROC) (HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue);
typedef BOOL (WINAPI * PFNWGLSETGAMMATABLEI3DPROC) (HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue);
#endif

#ifndef WGL_I3D_genlock
#define WGL_I3D_genlock 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglEnableGenlockI3D (HDC);
extern BOOL WINAPI wglDisableGenlockI3D (HDC);
extern BOOL WINAPI wglIsEnabledGenlockI3D (HDC, BOOL *);
extern BOOL WINAPI wglGenlockSourceI3D (HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceI3D (HDC, UINT *);
extern BOOL WINAPI wglGenlockSourceEdgeI3D (HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceEdgeI3D (HDC, UINT *);
extern BOOL WINAPI wglGenlockSampleRateI3D (HDC, UINT);
extern BOOL WINAPI wglGetGenlockSampleRateI3D (HDC, UINT *);
extern BOOL WINAPI wglGenlockSourceDelayI3D (HDC, UINT);
extern BOOL WINAPI wglGetGenlockSourceDelayI3D (HDC, UINT *);
extern BOOL WINAPI wglQueryGenlockMaxSourceDelayI3D (HDC, UINT *, UINT *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLENABLEGENLOCKI3DPROC) (HDC hDC);
typedef BOOL (WINAPI * PFNWGLDISABLEGENLOCKI3DPROC) (HDC hDC);
typedef BOOL (WINAPI * PFNWGLISENABLEDGENLOCKI3DPROC) (HDC hDC, BOOL *pFlag);
typedef BOOL (WINAPI * PFNWGLGENLOCKSOURCEI3DPROC) (HDC hDC, UINT uSource);
typedef BOOL (WINAPI * PFNWGLGETGENLOCKSOURCEI3DPROC) (HDC hDC, UINT *uSource);
typedef BOOL (WINAPI * PFNWGLGENLOCKSOURCEEDGEI3DPROC) (HDC hDC, UINT uEdge);
typedef BOOL (WINAPI * PFNWGLGETGENLOCKSOURCEEDGEI3DPROC) (HDC hDC, UINT *uEdge);
typedef BOOL (WINAPI * PFNWGLGENLOCKSAMPLERATEI3DPROC) (HDC hDC, UINT uRate);
typedef BOOL (WINAPI * PFNWGLGETGENLOCKSAMPLERATEI3DPROC) (HDC hDC, UINT *uRate);
typedef BOOL (WINAPI * PFNWGLGENLOCKSOURCEDELAYI3DPROC) (HDC hDC, UINT uDelay);
typedef BOOL (WINAPI * PFNWGLGETGENLOCKSOURCEDELAYI3DPROC) (HDC hDC, UINT *uDelay);
typedef BOOL (WINAPI * PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC) (HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay);
#endif

#ifndef WGL_I3D_image_buffer
#define WGL_I3D_image_buffer 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern LPVOID WINAPI wglCreateImageBufferI3D (HDC, DWORD, UINT);
extern BOOL WINAPI wglDestroyImageBufferI3D (HDC, LPVOID);
extern BOOL WINAPI wglAssociateImageBufferEventsI3D (HDC, const HANDLE *, const LPVOID *, const DWORD *, UINT);
extern BOOL WINAPI wglReleaseImageBufferEventsI3D (HDC, const LPVOID *, UINT);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef LPVOID (WINAPI * PFNWGLCREATEIMAGEBUFFERI3DPROC) (HDC hDC, DWORD dwSize, UINT uFlags);
typedef BOOL (WINAPI * PFNWGLDESTROYIMAGEBUFFERI3DPROC) (HDC hDC, LPVOID pAddress);
typedef BOOL (WINAPI * PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3DPROC) (HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count);
typedef BOOL (WINAPI * PFNWGLRELEASEIMAGEBUFFEREVENTSI3DPROC) (HDC hDC, const LPVOID *pAddress, UINT count);
#endif

#ifndef WGL_I3D_swap_frame_lock
#define WGL_I3D_swap_frame_lock 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglEnableFrameLockI3D (void);
extern BOOL WINAPI wglDisableFrameLockI3D (void);
extern BOOL WINAPI wglIsEnabledFrameLockI3D (BOOL *);
extern BOOL WINAPI wglQueryFrameLockMasterI3D (BOOL *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLENABLEFRAMELOCKI3DPROC) (void);
typedef BOOL (WINAPI * PFNWGLDISABLEFRAMELOCKI3DPROC) (void);
typedef BOOL (WINAPI * PFNWGLISENABLEDFRAMELOCKI3DPROC) (BOOL *pFlag);
typedef BOOL (WINAPI * PFNWGLQUERYFRAMELOCKMASTERI3DPROC) (BOOL *pFlag);
#endif

#ifndef WGL_I3D_swap_frame_usage
#define WGL_I3D_swap_frame_usage 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetFrameUsageI3D (float *);
extern BOOL WINAPI wglBeginFrameTrackingI3D (void);
extern BOOL WINAPI wglEndFrameTrackingI3D (void);
extern BOOL WINAPI wglQueryFrameTrackingI3D (DWORD *, DWORD *, float *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETFRAMEUSAGEI3DPROC) (float *pUsage);
typedef BOOL (WINAPI * PFNWGLBEGINFRAMETRACKINGI3DPROC) (void);
typedef BOOL (WINAPI * PFNWGLENDFRAMETRACKINGI3DPROC) (void);
typedef BOOL (WINAPI * PFNWGLQUERYFRAMETRACKINGI3DPROC) (DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage);
#endif

#ifndef WGL_ATI_pixel_format_float
#define WGL_ATI_pixel_format_float 1
#endif

#ifndef WGL_NV_float_buffer
#define WGL_NV_float_buffer 1
#endif

#ifndef WGL_EXT_pixel_format_packed_float
#define WGL_EXT_pixel_format_packed_float 1
#endif

#ifndef WGL_EXT_framebuffer_sRGB
#define WGL_EXT_framebuffer_sRGB 1
#endif

#ifndef WGL_NV_present_video
#define WGL_NV_present_video 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern int WINAPI wglEnumerateVideoDevicesNV (HDC, HVIDEOOUTPUTDEVICENV *);
extern BOOL WINAPI wglBindVideoDeviceNV (HDC, unsigned int, HVIDEOOUTPUTDEVICENV, const int *);
extern BOOL WINAPI wglQueryCurrentContextNV (int, int *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef int (WINAPI * PFNWGLENUMERATEVIDEODEVICESNVPROC) (HDC hDC, HVIDEOOUTPUTDEVICENV *phDeviceList);
typedef BOOL (WINAPI * PFNWGLBINDVIDEODEVICENVPROC) (HDC hDC, unsigned int uVideoSlot, HVIDEOOUTPUTDEVICENV hVideoDevice, const int *piAttribList);
typedef BOOL (WINAPI * PFNWGLQUERYCURRENTCONTEXTNVPROC) (int iAttribute, int *piValue);
#endif

#ifndef WGL_NV_video_out
#define WGL_NV_video_out 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglGetVideoDeviceNV (HDC, int, HPVIDEODEV *);
extern BOOL WINAPI wglReleaseVideoDeviceNV (HPVIDEODEV);
extern BOOL WINAPI wglBindVideoImageNV (HPVIDEODEV, HPBUFFERARB, int);
extern BOOL WINAPI wglReleaseVideoImageNV (HPBUFFERARB, int);
extern BOOL WINAPI wglSendPbufferToVideoNV (HPBUFFERARB, int, unsigned long *, BOOL);
extern BOOL WINAPI wglGetVideoInfoNV (HPVIDEODEV, unsigned long *, unsigned long *);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLGETVIDEODEVICENVPROC) (HDC hDC, int numDevices, HPVIDEODEV *hVideoDevice);
typedef BOOL (WINAPI * PFNWGLRELEASEVIDEODEVICENVPROC) (HPVIDEODEV hVideoDevice);
typedef BOOL (WINAPI * PFNWGLBINDVIDEOIMAGENVPROC) (HPVIDEODEV hVideoDevice, HPBUFFERARB hPbuffer, int iVideoBuffer);
typedef BOOL (WINAPI * PFNWGLRELEASEVIDEOIMAGENVPROC) (HPBUFFERARB hPbuffer, int iVideoBuffer);
typedef BOOL (WINAPI * PFNWGLSENDPBUFFERTOVIDEONVPROC) (HPBUFFERARB hPbuffer, int iBufferType, unsigned long *pulCounterPbuffer, BOOL bBlock);
typedef BOOL (WINAPI * PFNWGLGETVIDEOINFONVPROC) (HPVIDEODEV hpVideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo);
#endif

#ifndef WGL_NV_swap_group
#define WGL_NV_swap_group 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglJoinSwapGroupNV (HDC, GLuint);
extern BOOL WINAPI wglBindSwapBarrierNV (GLuint, GLuint);
extern BOOL WINAPI wglQuerySwapGroupNV (HDC, GLuint *, GLuint *);
extern BOOL WINAPI wglQueryMaxSwapGroupsNV (HDC, GLuint *, GLuint *);
extern BOOL WINAPI wglQueryFrameCountNV (HDC, GLuint *);
extern BOOL WINAPI wglResetFrameCountNV (HDC);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLJOINSWAPGROUPNVPROC) (HDC hDC, GLuint group);
typedef BOOL (WINAPI * PFNWGLBINDSWAPBARRIERNVPROC) (GLuint group, GLuint barrier);
typedef BOOL (WINAPI * PFNWGLQUERYSWAPGROUPNVPROC) (HDC hDC, GLuint *group, GLuint *barrier);
typedef BOOL (WINAPI * PFNWGLQUERYMAXSWAPGROUPSNVPROC) (HDC hDC, GLuint *maxGroups, GLuint *maxBarriers);
typedef BOOL (WINAPI * PFNWGLQUERYFRAMECOUNTNVPROC) (HDC hDC, GLuint *count);
typedef BOOL (WINAPI * PFNWGLRESETFRAMECOUNTNVPROC) (HDC hDC);
#endif

#ifndef WGL_NV_gpu_affinity
#define WGL_NV_gpu_affinity 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern BOOL WINAPI wglEnumGpusNV (UINT, HGPUNV *);
extern BOOL WINAPI wglEnumGpuDevicesNV (HGPUNV, UINT, PGPU_DEVICE);
extern HDC WINAPI wglCreateAffinityDCNV (const HGPUNV *);
extern BOOL WINAPI wglEnumGpusFromAffinityDCNV (HDC, UINT, HGPUNV *);
extern BOOL WINAPI wglDeleteDCNV (HDC);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef BOOL (WINAPI * PFNWGLENUMGPUSNVPROC) (UINT iGpuIndex, HGPUNV *phGpu);
typedef BOOL (WINAPI * PFNWGLENUMGPUDEVICESNVPROC) (HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice);
typedef HDC (WINAPI * PFNWGLCREATEAFFINITYDCNVPROC) (const HGPUNV *phGpuList);
typedef BOOL (WINAPI * PFNWGLENUMGPUSFROMAFFINITYDCNVPROC) (HDC hAffinityDC, UINT iGpuIndex, HGPUNV *hGpu);
typedef BOOL (WINAPI * PFNWGLDELETEDCNVPROC) (HDC hdc);
#endif

#ifndef WGL_AMD_gpu_association
#define WGL_AMD_gpu_association 1
#ifdef WGL_WGLEXT_PROTOTYPES
extern UINT WINAPI wglGetGPUIDsAMD (UINT, UINT *);
extern INT WINAPI wglGetGPUInfoAMD (UINT, int, GLenum, UINT, void *);
extern UINT WINAPI wglGetContextGPUIDAMD (HGLRC);
extern HGLRC WINAPI wglCreateAssociatedContextAMD (UINT);
extern HGLRC WINAPI wglCreateAssociatedContextAttribsAMD (UINT, HGLRC, const int *);
extern BOOL WINAPI wglDeleteAssociatedContextAMD (HGLRC);
extern BOOL WINAPI wglMakeAssociatedContextCurrentAMD (HGLRC);
extern HGLRC WINAPI wglGetCurrentAssociatedContextAMD (void);
extern VOID WINAPI wglBlitContextFramebufferAMD (HGLRC, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
#endif /* WGL_WGLEXT_PROTOTYPES */
typedef UINT (WINAPI * PFNWGLGETGPUIDSAMDPROC) (UINT maxCount, UINT *ids);
typedef INT (WINAPI * PFNWGLGETGPUINFOAMDPROC) (UINT id, int property, GLenum dataType, UINT size, void *data);
typedef UINT (WINAPI * PFNWGLGETCONTEXTGPUIDAMDPROC) (HGLRC hglrc);
typedef HGLRC (WINAPI * PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC) (UINT id);
typedef HGLRC (WINAPI * PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC) (UINT id, HGLRC hShareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC) (HGLRC hglrc);
typedef BOOL (WINAPI * PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC) (HGLRC hglrc);
typedef HGLRC (WINAPI * PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC) (void);
typedef VOID (WINAPI * PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC) (HGLRC dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
#endif


#ifdef __cplusplus
}
#endif

#endif
