//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     Public API definitions for DWrite, D2D and DImage.
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

//
// Forward declarations
//
interface IDXGISurface;

//+----------------------------------------------------------------------------
//
//  Enum:
//      DWRITE_MEASURING_MODE
//
//  Synopsis:
//      The measuring method used for text layout.
//
//-------------------------------------------------------------------------------
typedef enum DWRITE_MEASURING_MODE
{
    //
    // Text is measured using glyph ideal metrics whose values are independent to the current display resolution.
    //
    DWRITE_MEASURING_MODE_NATURAL,

    //
    // Text is measured using glyph display compatible metrics whose values tuned for the current display resolution.
    //
    DWRITE_MEASURING_MODE_GDI_CLASSIC,

    //
    // Text is measured using the same glyph display metrics as text measured by GDI using a font
    // created with CLEARTYPE_NATURAL_QUALITY.
    //
    DWRITE_MEASURING_MODE_GDI_NATURAL

} DWRITE_MEASURING_MODE;

//+-----------------------------------------------------------------------------
//
//  Enum:
//      D2D1_ALPHA_MODE
//
//  Synopsis:
//      Qualifies how alpha is to be treated in a bitmap or render target containing
//      alpha.
//
//------------------------------------------------------------------------------
typedef enum D2D1_ALPHA_MODE
{
        
        //
        // Alpha mode should be determined implicitly. Some target surfaces do not supply
        // or imply this information in which case alpha must be specified.
        //
        D2D1_ALPHA_MODE_UNKNOWN = 0,
        
        //
        // Treat the alpha as premultipled.
        //
        D2D1_ALPHA_MODE_PREMULTIPLIED = 1,
        
        //
        // Opacity is in the 'A' component only.
        //
        D2D1_ALPHA_MODE_STRAIGHT = 2,
        
        //
        // Ignore any alpha channel information.
        //
        D2D1_ALPHA_MODE_IGNORE = 3,

        D2D1_ALPHA_MODE_FORCE_DWORD = 0xffffffff

} D2D1_ALPHA_MODE;

//+-----------------------------------------------------------------------------
//
//  Struct:
//      D2D1_PIXEL_FORMAT
//
//------------------------------------------------------------------------------
typedef struct D2D1_PIXEL_FORMAT
{
    DXGI_FORMAT format;
    D2D1_ALPHA_MODE alphaMode;

} D2D1_PIXEL_FORMAT;

#endif /* DCOMMON_H_INCLUDED */
