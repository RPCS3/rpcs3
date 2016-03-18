
/*=========================================================================*\

    Copyright (c) Microsoft Corporation.  All rights reserved.

    File: D2D1_2Helper.h

    Module Name: D2D

    Description: Helper files over the D2D interfaces and APIs.

\*=========================================================================*/

#ifdef _MSC_VER
#pragma once
#endif  // _MSC_VER

#ifndef _D2D1_2HELPER_H_
#define _D2D1_2HELPER_H_

#if NTDDI_VERSION >= NTDDI_WINBLUE

#ifndef _D2D1_2_H_
#include <d2d1_2.h>
#endif // #ifndef _D2D1_2_H_

#ifndef D2D_USE_C_DEFINITIONS

#include <winapifamily.h>

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

namespace D2D1
{
    COM_DECLSPEC_NOTHROW
    D2D1FORCEINLINE
    FLOAT
    ComputeFlatteningTolerance(
        _In_ CONST D2D1_MATRIX_3X2_F &matrix,
        FLOAT dpiX = 96.0f,
        FLOAT dpiY = 96.0f,
        FLOAT maxZoomFactor = 1.0f
        )
    {
        D2D1_MATRIX_3X2_F dpiDependentTransform =
            matrix * D2D1::Matrix3x2F::Scale(dpiX / 96.0f, dpiY / 96.0f);
 
        FLOAT absMaxZoomFactor = (maxZoomFactor > 0) ? maxZoomFactor : -maxZoomFactor;

        return D2D1_DEFAULT_FLATTENING_TOLERANCE / 
            (absMaxZoomFactor * D2D1ComputeMaximumScaleFactor(&dpiDependentTransform));
    }

} // namespace D2D1

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#endif // #ifndef D2D_USE_C_DEFINITIONS

#endif // #if NTDDI_VERSION >= NTDDI_WINBLUE

#endif // #ifndef _D2D1_HELPER_H_

