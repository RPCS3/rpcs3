
/*=========================================================================*\

    Copyright (c) Microsoft Corporation.  All rights reserved.

    File: D2D1_3Helper.h

    Module Name: D2D

    Description: Helper files over the D2D interfaces and APIs.

\*=========================================================================*/

#ifdef _MSC_VER
#pragma once
#endif  // _MSC_VER

#ifndef _D2D1_3HELPER_H_
#define _D2D1_3HELPER_H_

#if NTDDI_VERSION >= NTDDI_WINTHRESHOLD

#ifndef _D2D1_3_H_
#include <d2d1_3.h>
#endif // #ifndef _D2D1_3_H_

#ifndef D2D_USE_C_DEFINITIONS

#include <winapifamily.h>

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

namespace D2D1
{
    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_GRADIENT_MESH_PATCH
    GradientMeshPatch( 
        D2D1_POINT_2F point00, 
        D2D1_POINT_2F point01, 
        D2D1_POINT_2F point02, 
        D2D1_POINT_2F point03, 
        D2D1_POINT_2F point10, 
        D2D1_POINT_2F point11, 
        D2D1_POINT_2F point12, 
        D2D1_POINT_2F point13, 
        D2D1_POINT_2F point20, 
        D2D1_POINT_2F point21, 
        D2D1_POINT_2F point22, 
        D2D1_POINT_2F point23, 
        D2D1_POINT_2F point30, 
        D2D1_POINT_2F point31, 
        D2D1_POINT_2F point32, 
        D2D1_POINT_2F point33, 
        D2D1_COLOR_F color00, 
        D2D1_COLOR_F color03, 
        D2D1_COLOR_F color30, 
        D2D1_COLOR_F color33,
        D2D1_PATCH_EDGE_MODE topEdgeMode,
        D2D1_PATCH_EDGE_MODE leftEdgeMode,
        D2D1_PATCH_EDGE_MODE bottomEdgeMode,
        D2D1_PATCH_EDGE_MODE rightEdgeMode
        )
    {
        D2D1_GRADIENT_MESH_PATCH newPatch;
        newPatch.point00 = point00;
        newPatch.point01 = point01;
        newPatch.point02 = point02;
        newPatch.point03 = point03; 
        newPatch.point10 = point10;
        newPatch.point11 = point11;
        newPatch.point12 = point12;
        newPatch.point13 = point13; 
        newPatch.point20 = point20;
        newPatch.point21 = point21;
        newPatch.point22 = point22;
        newPatch.point23 = point23; 
        newPatch.point30 = point30;
        newPatch.point31 = point31;
        newPatch.point32 = point32;
        newPatch.point33 = point33; 

        newPatch.color00 = color00;
        newPatch.color03 = color03;
        newPatch.color30 = color30;
        newPatch.color33 = color33;
        
        newPatch.topEdgeMode = topEdgeMode;
        newPatch.leftEdgeMode = leftEdgeMode;
        newPatch.bottomEdgeMode = bottomEdgeMode;
        newPatch.rightEdgeMode = rightEdgeMode;
        
        return newPatch;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_GRADIENT_MESH_PATCH
    GradientMeshPatchFromCoonsPatch(
        D2D1_POINT_2F point0,
        D2D1_POINT_2F point1,
        D2D1_POINT_2F point2,
        D2D1_POINT_2F point3,
        D2D1_POINT_2F point4,
        D2D1_POINT_2F point5,
        D2D1_POINT_2F point6,
        D2D1_POINT_2F point7,
        D2D1_POINT_2F point8,
        D2D1_POINT_2F point9,
        D2D1_POINT_2F point10,
        D2D1_POINT_2F point11,
        D2D1_COLOR_F color0,
        D2D1_COLOR_F color1,
        D2D1_COLOR_F color2,
        D2D1_COLOR_F color3,
        D2D1_PATCH_EDGE_MODE topEdgeMode,
        D2D1_PATCH_EDGE_MODE leftEdgeMode,
        D2D1_PATCH_EDGE_MODE bottomEdgeMode,
        D2D1_PATCH_EDGE_MODE rightEdgeMode
        )
    {
        D2D1_GRADIENT_MESH_PATCH newPatch;
        newPatch.point00 = point0;
        newPatch.point01 = point1;
        newPatch.point02 = point2;
        newPatch.point03 = point3;        
        newPatch.point13 = point4;
        newPatch.point23 = point5;
        newPatch.point33 = point6;
        newPatch.point32 = point7;
        newPatch.point31 = point8;
        newPatch.point30 = point9;
        newPatch.point20 = point10;
        newPatch.point10 = point11;
        
        D2D1GetGradientMeshInteriorPointsFromCoonsPatch(
            &point0,
            &point1,      
            &point2,
            &point3,                       
            &point4,
            &point5,      
            &point6,
            &point7,
            &point8,
            &point9,      
            &point10,
            &point11,
            &newPatch.point11,
            &newPatch.point12,
            &newPatch.point21,
            &newPatch.point22
            );                

        newPatch.color00 = color0;
        newPatch.color03 = color1;
        newPatch.color33 = color2;
        newPatch.color30 = color3;
        newPatch.topEdgeMode = topEdgeMode;
        newPatch.leftEdgeMode = leftEdgeMode;
        newPatch.bottomEdgeMode = bottomEdgeMode;
        newPatch.rightEdgeMode = rightEdgeMode;
        
        return newPatch;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_INK_POINT 
    InkPoint( 
        const D2D1_POINT_2F &point, 
        FLOAT radius 
        )
    {
        D2D1_INK_POINT inkPoint;

        inkPoint.x = point.x;
        inkPoint.y = point.y;
        inkPoint.radius = radius;

        return inkPoint;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_INK_BEZIER_SEGMENT 
    InkBezierSegment( 
        const D2D1_INK_POINT &point1, 
        const D2D1_INK_POINT &point2, 
        const D2D1_INK_POINT &point3 
        )
    {
        D2D1_INK_BEZIER_SEGMENT inkBezierSegment;

        inkBezierSegment.point1 = point1;
        inkBezierSegment.point2 = point2;
        inkBezierSegment.point3 = point3;

        return inkBezierSegment;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_INK_STYLE_PROPERTIES 
    InkStyleProperties( 
        D2D1_INK_NIB_SHAPE nibShape, 
        const D2D1_MATRIX_3X2_F &nibTransform 
        )
    {
        D2D1_INK_STYLE_PROPERTIES inkStyleProperties;

        inkStyleProperties.nibShape = nibShape;
        inkStyleProperties.nibTransform = nibTransform;

        return inkStyleProperties;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_RECT_U
    InfiniteRectU()
    {
        D2D1_RECT_U rect = { 0u, 0u, UINT_MAX, UINT_MAX };

        return rect;
    }

    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    D2D1_SIMPLE_COLOR_PROFILE
    SimpleColorProfile(
        const D2D1_POINT_2F &redPrimary,
        const D2D1_POINT_2F &greenPrimary,
        const D2D1_POINT_2F &bluePrimary,
        const D2D1_GAMMA1 gamma,
        const D2D1_POINT_2F &whitePointXZ
        )
    {
        D2D1_SIMPLE_COLOR_PROFILE simpleColorProfile;

        simpleColorProfile.redPrimary = redPrimary;
        simpleColorProfile.greenPrimary = greenPrimary;
        simpleColorProfile.bluePrimary = bluePrimary;
        simpleColorProfile.gamma = gamma;
        simpleColorProfile.whitePointXZ = whitePointXZ;

        return simpleColorProfile;
    }
} // namespace D2D1

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#endif // #ifndef D2D_USE_C_DEFINITIONS

#endif // #if NTDDI_VERSION >= NTDDI_WINTHRESHOLD

#endif // #ifndef _D2D1_HELPER_H_

