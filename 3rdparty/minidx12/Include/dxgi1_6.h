

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxgi1_6_h__
#define __dxgi1_6_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIAdapter4_FWD_DEFINED__
#define __IDXGIAdapter4_FWD_DEFINED__
typedef interface IDXGIAdapter4 IDXGIAdapter4;

#endif 	/* __IDXGIAdapter4_FWD_DEFINED__ */


#ifndef __IDXGIOutput6_FWD_DEFINED__
#define __IDXGIOutput6_FWD_DEFINED__
typedef interface IDXGIOutput6 IDXGIOutput6;

#endif 	/* __IDXGIOutput6_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi1_5.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_6_0000_0000 */
/* [local] */ 

// Copyright (c) Microsoft Corporation.  All Rights Reserved
#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
typedef 
enum DXGI_ADAPTER_FLAG3
    {
        DXGI_ADAPTER_FLAG3_NONE	= 0,
        DXGI_ADAPTER_FLAG3_REMOTE	= 1,
        DXGI_ADAPTER_FLAG3_SOFTWARE	= 2,
        DXGI_ADAPTER_FLAG3_ACG_COMPATIBLE	= 4,
        DXGI_ADAPTER_FLAG3_FORCE_DWORD	= 0xffffffff
    } 	DXGI_ADAPTER_FLAG3;

DEFINE_ENUM_FLAG_OPERATORS( DXGI_ADAPTER_FLAG3 );
typedef struct DXGI_ADAPTER_DESC3
    {
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
    DXGI_ADAPTER_FLAG3 Flags;
    DXGI_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    DXGI_COMPUTE_PREEMPTION_GRANULARITY ComputePreemptionGranularity;
    } 	DXGI_ADAPTER_DESC3;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIAdapter4_INTERFACE_DEFINED__
#define __IDXGIAdapter4_INTERFACE_DEFINED__

/* interface IDXGIAdapter4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIAdapter4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3c8d99d1-4fbf-4181-a82c-af66bf7bd24e")
    IDXGIAdapter4 : public IDXGIAdapter3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc3( 
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC3 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIAdapter4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIAdapter4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIAdapter4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIAdapter4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
            IDXGIAdapter4 * This,
            /* [in] */ UINT Output,
            /* [annotation][out][in] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIAdapter4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID InterfaceName,
            /* [annotation][out] */ 
            _Out_  LARGE_INTEGER *pUMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGIAdapter4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc2 )( 
            IDXGIAdapter4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC2 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterHardwareContentProtectionTeardownStatusEvent )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterHardwareContentProtectionTeardownStatus )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *QueryVideoMemoryInfo )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][out] */ 
            _Out_  DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoMemoryReservation )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][in] */ 
            _In_  UINT64 Reservation);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterVideoMemoryBudgetChangeNotificationEvent )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterVideoMemoryBudgetChangeNotification )( 
            IDXGIAdapter4 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc3 )( 
            IDXGIAdapter4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC3 *pDesc);
        
        END_INTERFACE
    } IDXGIAdapter4Vtbl;

    interface IDXGIAdapter4
    {
        CONST_VTBL struct IDXGIAdapter4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIAdapter4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter4_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter4_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 


#define IDXGIAdapter4_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define IDXGIAdapter4_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 


#define IDXGIAdapter4_RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIAdapter4_UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie) ) 

#define IDXGIAdapter4_QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo)	\
    ( (This)->lpVtbl -> QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo) ) 

#define IDXGIAdapter4_SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation)	\
    ( (This)->lpVtbl -> SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation) ) 

#define IDXGIAdapter4_RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie) ) 

#define IDXGIAdapter4_UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie) ) 


#define IDXGIAdapter4_GetDesc3(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc3(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_6_0000_0001 */
/* [local] */ 

typedef struct DXGI_OUTPUT_DESC1
    {
    WCHAR DeviceName[ 32 ];
    RECT DesktopCoordinates;
    BOOL AttachedToDesktop;
    DXGI_MODE_ROTATION Rotation;
    HMONITOR Monitor;
    UINT BitsPerColor;
    DXGI_COLOR_SPACE_TYPE ColorSpace;
    FLOAT RedPrimary[ 2 ];
    FLOAT GreenPrimary[ 2 ];
    FLOAT BluePrimary[ 2 ];
    FLOAT WhitePoint[ 2 ];
    FLOAT MinLuminance;
    FLOAT MaxLuminance;
    FLOAT MaxFullFrameLuminance;
    } 	DXGI_OUTPUT_DESC1;

typedef 
enum DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS
    {
        DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_FULLSCREEN	= 1,
        DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED	= 2,
        DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_CURSOR_STRETCHED	= 4
    } 	DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS;

DEFINE_ENUM_FLAG_OPERATORS( DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS );


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutput6_INTERFACE_DEFINED__
#define __IDXGIOutput6_INTERFACE_DEFINED__

/* interface IDXGIOutput6 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput6;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068346e8-aaec-4b84-add7-137f513f77a1")
    IDXGIOutput6 : public IDXGIOutput5
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc1( 
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC1 *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckHardwareCompositionSupport( 
            /* [annotation][out] */ 
            _Out_  UINT *pFlags) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput6 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput6 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput1 )( 
            IDXGIOutput6 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */ 
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */ 
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CheckHardwareCompositionSupport )( 
            IDXGIOutput6 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        END_INTERFACE
    } IDXGIOutput6Vtbl;

    interface IDXGIOutput6
    {
        CONST_VTBL struct IDXGIOutput6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput6_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput6_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput6_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput6_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput6_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput6_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput6_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput6_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput6_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput6_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput6_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput6_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput6_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput6_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput6_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput6_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput6_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput6_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput6_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput6_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput6_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput6_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 


#define IDXGIOutput6_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) ) 


#define IDXGIOutput6_DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication) ) 


#define IDXGIOutput6_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGIOutput6_CheckHardwareCompositionSupport(This,pFlags)	\
    ( (This)->lpVtbl -> CheckHardwareCompositionSupport(This,pFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput6_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_6_0000_0002 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_IDXGIAdapter4,0x3c8d99d1,0x4fbf,0x4181,0xa8,0x2c,0xaf,0x66,0xbf,0x7b,0xd2,0x4e);
DEFINE_GUID(IID_IDXGIOutput6,0x068346e8,0xaaec,0x4b84,0xad,0xd7,0x13,0x7f,0x51,0x3f,0x77,0xa1);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0002_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


