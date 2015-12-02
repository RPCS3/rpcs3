//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __dxgitype_h__
#define __dxgitype_h__

#include "dxgiformat.h"

#define _FACDXGI    0x87a
#define MAKE_DXGI_HRESULT(code) MAKE_HRESULT(1, _FACDXGI, code)
#define MAKE_DXGI_STATUS(code)  MAKE_HRESULT(0, _FACDXGI, code)

// DXGI error messages have moved to winerror.h

#define DXGI_CPU_ACCESS_NONE                    ( 0 )
#define DXGI_CPU_ACCESS_DYNAMIC                 ( 1 )
#define DXGI_CPU_ACCESS_READ_WRITE              ( 2 )
#define DXGI_CPU_ACCESS_SCRATCH                 ( 3 )
#define DXGI_CPU_ACCESS_FIELD                   15


typedef struct DXGI_RGB
{
    float Red;
    float Green;
    float Blue;
} DXGI_RGB;

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;

#define D3DCOLORVALUE_DEFINED
#endif

typedef D3DCOLORVALUE DXGI_RGBA;

typedef struct DXGI_GAMMA_CONTROL
{
    DXGI_RGB Scale;
    DXGI_RGB Offset;
    DXGI_RGB GammaCurve[ 1025 ];
} DXGI_GAMMA_CONTROL;

typedef struct DXGI_GAMMA_CONTROL_CAPABILITIES
{
    BOOL ScaleAndOffsetSupported;
    float MaxConvertedValue;
    float MinConvertedValue;
    UINT NumGammaControlPoints;
    float ControlPointPositions[1025];
} DXGI_GAMMA_CONTROL_CAPABILITIES;

typedef struct DXGI_RATIONAL
{
    UINT Numerator;
    UINT Denominator;
} DXGI_RATIONAL;

typedef enum DXGI_MODE_SCANLINE_ORDER
{
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED        = 0,
    DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE        = 1,
    DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST  = 2,
    DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST  = 3
} DXGI_MODE_SCANLINE_ORDER;

typedef enum DXGI_MODE_SCALING
{
    DXGI_MODE_SCALING_UNSPECIFIED   = 0,
    DXGI_MODE_SCALING_CENTERED      = 1,
    DXGI_MODE_SCALING_STRETCHED     = 2
} DXGI_MODE_SCALING;

typedef enum DXGI_MODE_ROTATION
{
    DXGI_MODE_ROTATION_UNSPECIFIED  = 0,
    DXGI_MODE_ROTATION_IDENTITY     = 1,
    DXGI_MODE_ROTATION_ROTATE90     = 2,
    DXGI_MODE_ROTATION_ROTATE180    = 3,
    DXGI_MODE_ROTATION_ROTATE270    = 4
} DXGI_MODE_ROTATION;

typedef struct DXGI_MODE_DESC
{
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

// The following values are used with DXGI_SAMPLE_DESC::Quality:
#define DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN 0xffffffff
#define DXGI_CENTER_MULTISAMPLE_QUALITY_PATTERN 0xfffffffe

typedef struct DXGI_SAMPLE_DESC
{
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

typedef enum DXGI_COLOR_SPACE_TYPE
{
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709             = 0,
    DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709             = 1,
    DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709           = 2,
    DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020          = 3,
    DXGI_COLOR_SPACE_RESERVED                           = 4,
    DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601      = 5,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601         = 6,
    DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601           = 7,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709         = 8,
    DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709           = 9,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020        = 10,
    DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020          = 11,
    DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020          = 12,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020      = 13,
    DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020        = 14,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020     = 15,
    DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020   = 16,
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020            = 17,
    DXGI_COLOR_SPACE_CUSTOM                             = 0xFFFFFFFF
} DXGI_COLOR_SPACE_TYPE;

typedef struct DXGI_JPEG_DC_HUFFMAN_TABLE
{
    BYTE CodeCounts[12];
    BYTE CodeValues[12];
} DXGI_JPEG_DC_HUFFMAN_TABLE;

typedef struct DXGI_JPEG_AC_HUFFMAN_TABLE
{
    BYTE CodeCounts[16];
    BYTE CodeValues[162];
} DXGI_JPEG_AC_HUFFMAN_TABLE;

typedef struct DXGI_JPEG_QUANTIZATION_TABLE
{
    BYTE Elements[64];
} DXGI_JPEG_QUANTIZATION_TABLE;

#endif // __dxgitype_h__

