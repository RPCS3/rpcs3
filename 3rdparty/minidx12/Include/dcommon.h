//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     Public API definitions shared by DWrite, D2D, and DImage.
//
//----------------------------------------------------------------------------
#pragma once

#ifndef DCOMMON_H_INCLUDED
#define DCOMMON_H_INCLUDED

#include <dxgiformat.h>

#ifndef DX_DECLARE_INTERFACE
#define DX_DECLARE_INTERFACE(x) DECLSPEC_UUID(x) DECLSPEC_NOVTABLE
#endif

#ifndef CHECKMETHOD
#define CHECKMETHOD(method) virtual DECLSPEC_NOTHROW _Must_inspect_result_ HRESULT STDMETHODCALLTYPE method
#endif

#pragma warning(push)
#pragma warning(disable:4201) // anonymous unions warning

//
// Forward declarations
//
struct IDXGISurface;

/// <summary>
/// The measuring method used for text layout.
/// </summary>
typedef enum DWRITE_MEASURING_MODE
{
    /// <summary>
    /// Text is measured using glyph ideal metrics whose values are independent to the current display resolution.
    /// </summary>
    DWRITE_MEASURING_MODE_NATURAL,

    /// <summary>
    /// Text is measured using glyph display compatible metrics whose values tuned for the current display resolution.
    /// </summary>
    DWRITE_MEASURING_MODE_GDI_CLASSIC,

    /// <summary>
    // Text is measured using the same glyph display metrics as text measured by GDI using a font
    // created with CLEARTYPE_NATURAL_QUALITY.
    /// </summary>
    DWRITE_MEASURING_MODE_GDI_NATURAL

} DWRITE_MEASURING_MODE;

/// <summary>
/// Qualifies how alpha is to be treated in a bitmap or render target containing
/// alpha.
/// </summary>
typedef enum D2D1_ALPHA_MODE
{
        
        /// <summary>
        /// Alpha mode should be determined implicitly. Some target surfaces do not supply
        /// or imply this information in which case alpha must be specified.
        /// </summary>
        D2D1_ALPHA_MODE_UNKNOWN = 0,
        
        /// <summary>
        /// Treat the alpha as premultipled.
        /// </summary>
        D2D1_ALPHA_MODE_PREMULTIPLIED = 1,
        
        /// <summary>
        /// Opacity is in the 'A' component only.
        /// </summary>
        D2D1_ALPHA_MODE_STRAIGHT = 2,
        
        /// <summary>
        /// Ignore any alpha channel information.
        /// </summary>
        D2D1_ALPHA_MODE_IGNORE = 3,

        D2D1_ALPHA_MODE_FORCE_DWORD = 0xffffffff

} D2D1_ALPHA_MODE;

/// <summary>
/// Description of a pixel format.
/// </summary>
typedef struct D2D1_PIXEL_FORMAT
{
    DXGI_FORMAT format;
    D2D1_ALPHA_MODE alphaMode;

} D2D1_PIXEL_FORMAT;

/// <summary>
/// Represents an x-coordinate and y-coordinate pair in two-dimensional space.
/// </summary>
typedef struct D2D_POINT_2U
{
    UINT32 x;
    UINT32 y;

} D2D_POINT_2U;

/// <summary>
/// Represents an x-coordinate and y-coordinate pair in two-dimensional space.
/// </summary>
typedef struct D2D_POINT_2F
{
    FLOAT x;
    FLOAT y;

} D2D_POINT_2F;

typedef POINT D2D_POINT_2L;

/// <summary>
/// A vector of 2 FLOAT values (x, y).
/// </summary>
typedef struct D2D_VECTOR_2F
{
    FLOAT x;
    FLOAT y;

} D2D_VECTOR_2F;


/// <summary>
/// A vector of 3 FLOAT values (x, y, z).
/// </summary>
typedef struct D2D_VECTOR_3F
{
    FLOAT x;
    FLOAT y;
    FLOAT z;

} D2D_VECTOR_3F;


/// <summary>
/// A vector of 4 FLOAT values (x, y, z, w).
/// </summary>
typedef struct D2D_VECTOR_4F
{
    FLOAT x;
    FLOAT y;
    FLOAT z;
    FLOAT w;

} D2D_VECTOR_4F;


/// <summary>
/// Represents a rectangle defined by the coordinates of the upper-left corner
/// (left, top) and the coordinates of the lower-right corner (right, bottom).
/// </summary>
typedef struct D2D_RECT_F
{
    FLOAT left;
    FLOAT top;
    FLOAT right;
    FLOAT bottom;

} D2D_RECT_F;


/// <summary>
/// Represents a rectangle defined by the coordinates of the upper-left corner
/// (left, top) and the coordinates of the lower-right corner (right, bottom).
/// </summary>
typedef struct D2D_RECT_U
{
    UINT32 left;
    UINT32 top;
    UINT32 right;
    UINT32 bottom;

} D2D_RECT_U;

typedef RECT D2D_RECT_L;

/// <summary>
/// Stores an ordered pair of floats, typically the width and height of a rectangle.
/// </summary>
typedef struct D2D_SIZE_F
{
    FLOAT width;
    FLOAT height;

} D2D_SIZE_F;


/// <summary>
/// Stores an ordered pair of integers, typically the width and height of a
/// rectangle.
/// </summary>
typedef struct D2D_SIZE_U
{
    UINT32 width;
    UINT32 height;

} D2D_SIZE_U;


/// <summary>
/// Represents a 3-by-2 matrix.
/// </summary>
typedef struct D2D_MATRIX_3X2_F
{
    union
    {
        struct
        {
            /// <summary>
            /// Horizontal scaling / cosine of rotation
            /// </summary>
            FLOAT m11;

            /// <summary>
            /// Vertical shear / sine of rotation
            /// </summary>
            FLOAT m12;

            /// <summary>
            /// Horizontal shear / negative sine of rotation
            /// </summary>
            FLOAT m21;

            /// <summary>
            /// Vertical scaling / cosine of rotation
            /// </summary>
            FLOAT m22;

            /// <summary>
            /// Horizontal shift (always orthogonal regardless of rotation)
            /// </summary>
            FLOAT dx;

            /// <summary>
            /// Vertical shift (always orthogonal regardless of rotation)
            /// </summary>
            FLOAT dy;
        };

        struct
        {
            FLOAT _11, _12;
            FLOAT _21, _22;
            FLOAT _31, _32;
        };

        FLOAT m[3][2];
    };

} D2D_MATRIX_3X2_F;



/// <summary>
/// Represents a 4-by-3 matrix.
/// </summary>
typedef struct D2D_MATRIX_4X3_F
{
    union
    {
        struct
        {
            FLOAT _11, _12, _13;
            FLOAT _21, _22, _23;
            FLOAT _31, _32, _33;
            FLOAT _41, _42, _43;
        };

        FLOAT m[4][3];
    };

} D2D_MATRIX_4X3_F;


/// <summary>
/// Represents a 4-by-4 matrix.
/// </summary>
typedef struct D2D_MATRIX_4X4_F
{
    union
    {
        struct 
        {
            FLOAT _11, _12, _13, _14;
            FLOAT _21, _22, _23, _24;
            FLOAT _31, _32, _33, _34;
            FLOAT _41, _42, _43, _44;
        };

        FLOAT m[4][4];
    };

} D2D_MATRIX_4X4_F;


/// <summary>
/// Represents a 5-by-4 matrix.
/// </summary>
typedef struct D2D_MATRIX_5X4_F
{
    union
    {
        struct 
        {
            FLOAT _11, _12, _13, _14;
            FLOAT _21, _22, _23, _24;
            FLOAT _31, _32, _33, _34;
            FLOAT _41, _42, _43, _44;
            FLOAT _51, _52, _53, _54;
        };

        FLOAT m[5][4];
    };

} D2D_MATRIX_5X4_F;


typedef D2D_POINT_2F D2D1_POINT_2F;
typedef D2D_POINT_2U D2D1_POINT_2U;
typedef D2D_POINT_2L D2D1_POINT_2L;
typedef D2D_RECT_F D2D1_RECT_F;
typedef D2D_RECT_U D2D1_RECT_U;
typedef D2D_RECT_L D2D1_RECT_L;
typedef D2D_SIZE_F D2D1_SIZE_F;
typedef D2D_SIZE_U D2D1_SIZE_U;
typedef D2D_MATRIX_3X2_F D2D1_MATRIX_3X2_F;


#pragma warning(pop)

#endif /* DCOMMON_H_INCLUDED */
