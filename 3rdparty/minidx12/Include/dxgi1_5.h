

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

#ifndef __dxgi1_5_h__
#define __dxgi1_5_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIOutput5_FWD_DEFINED__
#define __IDXGIOutput5_FWD_DEFINED__
typedef interface IDXGIOutput5 IDXGIOutput5;

#endif 	/* __IDXGIOutput5_FWD_DEFINED__ */


#ifndef __IDXGISwapChain4_FWD_DEFINED__
#define __IDXGISwapChain4_FWD_DEFINED__
typedef interface IDXGISwapChain4 IDXGISwapChain4;

#endif 	/* __IDXGISwapChain4_FWD_DEFINED__ */


#ifndef __IDXGIDevice4_FWD_DEFINED__
#define __IDXGIDevice4_FWD_DEFINED__
typedef interface IDXGIDevice4 IDXGIDevice4;

#endif 	/* __IDXGIDevice4_FWD_DEFINED__ */


#ifndef __IDXGIFactory5_FWD_DEFINED__
#define __IDXGIFactory5_FWD_DEFINED__
typedef interface IDXGIFactory5 IDXGIFactory5;

#endif 	/* __IDXGIFactory5_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi1_4.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_5_0000_0000 */
/* [local] */ 

#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
typedef 
enum DXGI_OUTDUPL_FLAG
    {
        DXGI_OUTDUPL_COMPOSITED_UI_CAPTURE_ONLY	= 1
    } 	DXGI_OUTDUPL_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIOutput5_INTERFACE_DEFINED__
#define __IDXGIOutput5_INTERFACE_DEFINED__

/* interface IDXGIOutput5 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80A07424-AB52-42EB-833C-0C42FD282D98")
    IDXGIOutput5 : public IDXGIOutput4
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DuplicateOutput1( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */ 
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */ 
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput5 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput5 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */ 
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */ 
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        END_INTERFACE
    } IDXGIOutput5Vtbl;

    interface IDXGIOutput5
    {
        CONST_VTBL struct IDXGIOutput5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput5_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput5_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput5_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput5_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput5_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput5_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput5_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput5_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput5_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput5_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput5_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput5_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput5_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput5_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput5_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput5_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput5_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput5_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput5_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput5_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput5_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput5_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 


#define IDXGIOutput5_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) ) 


#define IDXGIOutput5_DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput5_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0001 */
/* [local] */ 

typedef 
enum DXGI_HDR_METADATA_TYPE
    {
        DXGI_HDR_METADATA_TYPE_NONE	= 0,
        DXGI_HDR_METADATA_TYPE_HDR10	= 1
    } 	DXGI_HDR_METADATA_TYPE;

typedef struct DXGI_HDR_METADATA_HDR10
    {
    UINT16 RedPrimary[ 2 ];
    UINT16 GreenPrimary[ 2 ];
    UINT16 BluePrimary[ 2 ];
    UINT16 WhitePoint[ 2 ];
    UINT MaxMasteringLuminance;
    UINT MinMasteringLuminance;
    UINT16 MaxContentLightLevel;
    UINT16 MaxFrameAverageLightLevel;
    } 	DXGI_HDR_METADATA_HDR10;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGISwapChain4_INTERFACE_DEFINED__
#define __IDXGISwapChain4_INTERFACE_DEFINED__

/* interface IDXGISwapChain4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D585D5A-BD4A-489E-B1F4-3DBCB6452FFB")
    IDXGISwapChain4 : public IDXGISwapChain3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData( 
            /* [annotation][in] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation][in] */ 
            _In_  UINT Size,
            /* [annotation][size_is][in] */ 
            _In_reads_opt_(Size)  void *pMetaData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain4 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
            IDXGISwapChain4 * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGISwapChain4 * This,
            UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
            IDXGISwapChain4 * This,
            const DXGI_MATRIX_3X2_F *pMatrix);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix);
        
        UINT ( STDMETHODCALLTYPE *GetCurrentBackBufferIndex )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckColorSpaceSupport )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorSpace1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  UINT BufferCount,
            /* [annotation][in] */ 
            _In_  UINT Width,
            /* [annotation][in] */ 
            _In_  UINT Height,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  UINT SwapChainFlags,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  const UINT *pCreationNodeMask,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  IUnknown *const *ppPresentQueue);
        
        HRESULT ( STDMETHODCALLTYPE *SetHDRMetaData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation][in] */ 
            _In_  UINT Size,
            /* [annotation][size_is][in] */ 
            _In_reads_opt_(Size)  void *pMetaData);
        
        END_INTERFACE
    } IDXGISwapChain4Vtbl;

    interface IDXGISwapChain4
    {
        CONST_VTBL struct IDXGISwapChain4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain4_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain4_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain4_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain4_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain4_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain4_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain4_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain4_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain4_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain4_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain4_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain4_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain4_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain4_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain4_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain4_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain4_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain4_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain4_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain4_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain4_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain4_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain4_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain4_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain4_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain4_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain4_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain4_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 


#define IDXGISwapChain4_GetCurrentBackBufferIndex(This)	\
    ( (This)->lpVtbl -> GetCurrentBackBufferIndex(This) ) 

#define IDXGISwapChain4_CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport)	\
    ( (This)->lpVtbl -> CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport) ) 

#define IDXGISwapChain4_SetColorSpace1(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace1(This,ColorSpace) ) 

#define IDXGISwapChain4_ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue)	\
    ( (This)->lpVtbl -> ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue) ) 


#define IDXGISwapChain4_SetHDRMetaData(This,Type,Size,pMetaData)	\
    ( (This)->lpVtbl -> SetHDRMetaData(This,Type,Size,pMetaData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0002 */
/* [local] */ 

typedef 
enum _DXGI_OFFER_RESOURCE_FLAGS
    {
        DXGI_OFFER_RESOURCE_FLAG_ALLOW_DECOMMIT	= 0x1
    } 	DXGI_OFFER_RESOURCE_FLAGS;

typedef 
enum _DXGI_RECLAIM_RESOURCE_RESULTS
    {
        DXGI_RECLAIM_RESOURCE_RESULT_OK	= 0,
        DXGI_RECLAIM_RESOURCE_RESULT_DISCARDED	= 1,
        DXGI_RECLAIM_RESOURCE_RESULT_NOT_COMMITTED	= 2
    } 	DXGI_RECLAIM_RESOURCE_RESULTS;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0002_v0_0_s_ifspec;

#ifndef __IDXGIDevice4_INTERFACE_DEFINED__
#define __IDXGIDevice4_INTERFACE_DEFINED__

/* interface IDXGIDevice4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDevice4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("95B4F95F-D8DA-4CA4-9EE6-3B76D5968A10")
    IDXGIDevice4 : public IDXGIDevice3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OfferResources1( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority,
            /* [annotation][in] */ 
            _In_  UINT Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReclaimResources1( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(NumResources)  DXGI_RECLAIM_RESOURCE_RESULTS *pResults) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDevice4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDevice4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDevice4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDevice4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice4 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
            IDXGIDevice4 * This,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IUnknown *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
            /* [in] */ UINT NumResources);
        
        HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
            IDXGIDevice4 * This,
            /* [in] */ INT Priority);
        
        HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
            IDXGIDevice4 * This,
            /* [annotation][retval][out] */ 
            _Out_  INT *pPriority);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGIDevice4 * This,
            /* [in] */ UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGIDevice4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);
        
        HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent);
        
        void ( STDMETHODCALLTYPE *Trim )( 
            IDXGIDevice4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources1 )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority,
            /* [annotation][in] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources1 )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(NumResources)  DXGI_RECLAIM_RESOURCE_RESULTS *pResults);
        
        END_INTERFACE
    } IDXGIDevice4Vtbl;

    interface IDXGIDevice4
    {
        CONST_VTBL struct IDXGIDevice4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDevice4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice4_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice4_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice4_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice4_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice4_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice4_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice4_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice4_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice4_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice4_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 


#define IDXGIDevice4_Trim(This)	\
    ( (This)->lpVtbl -> Trim(This) ) 


#define IDXGIDevice4_OfferResources1(This,NumResources,ppResources,Priority,Flags)	\
    ( (This)->lpVtbl -> OfferResources1(This,NumResources,ppResources,Priority,Flags) ) 

#define IDXGIDevice4_ReclaimResources1(This,NumResources,ppResources,pResults)	\
    ( (This)->lpVtbl -> ReclaimResources1(This,NumResources,ppResources,pResults) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0003 */
/* [local] */ 

typedef 
enum DXGI_FEATURE
    {
        DXGI_FEATURE_PRESENT_ALLOW_TEARING	= 0
    } 	DXGI_FEATURE;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0003_v0_0_s_ifspec;

#ifndef __IDXGIFactory5_INTERFACE_DEFINED__
#define __IDXGIFactory5_INTERFACE_DEFINED__

/* interface IDXGIFactory5 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIFactory5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7632e1f5-ee65-4dca-87fd-84cd75f8838d")
    IDXGIFactory5 : public IDXGIFactory4
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport( 
            DXGI_FEATURE Feature,
            /* [annotation] */ 
            _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIFactory5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIFactory5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIFactory5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIFactory5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory5 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
            IDXGIFactory5 * This,
            HWND WindowHandle,
            UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
            IDXGIFactory5 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pWindowHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory5 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory5 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
        BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
            IDXGIFactory5 * This);
        
        BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
            IDXGIFactory5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  HWND hWnd,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCoreWindow )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  IUnknown *pWindow,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *GetSharedResourceAdapterLuid )( 
            IDXGIFactory5 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _Out_  LUID *pLuid);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
            IDXGIFactory5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            IDXGIFactory5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapterByLuid )( 
            IDXGIFactory5 * This,
            /* [annotation] */ 
            _In_  LUID AdapterLuid,
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumWarpAdapter )( 
            IDXGIFactory5 * This,
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
            IDXGIFactory5 * This,
            DXGI_FEATURE Feature,
            /* [annotation] */ 
            _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        END_INTERFACE
    } IDXGIFactory5Vtbl;

    interface IDXGIFactory5
    {
        CONST_VTBL struct IDXGIFactory5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIFactory5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory5_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory5_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory5_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory5_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory5_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory5_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory5_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory5_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory5_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory5_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory5_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory5_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory5_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory5_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory5_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory5_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory5_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory5_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory5_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory5_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory5_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory5_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 


#define IDXGIFactory5_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 


#define IDXGIFactory5_EnumAdapterByLuid(This,AdapterLuid,riid,ppvAdapter)	\
    ( (This)->lpVtbl -> EnumAdapterByLuid(This,AdapterLuid,riid,ppvAdapter) ) 

#define IDXGIFactory5_EnumWarpAdapter(This,riid,ppvAdapter)	\
    ( (This)->lpVtbl -> EnumWarpAdapter(This,riid,ppvAdapter) ) 


#define IDXGIFactory5_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory5_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0004 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_IDXGIOutput5,0x80A07424,0xAB52,0x42EB,0x83,0x3C,0x0C,0x42,0xFD,0x28,0x2D,0x98);
DEFINE_GUID(IID_IDXGISwapChain4,0x3D585D5A,0xBD4A,0x489E,0xB1,0xF4,0x3D,0xBC,0xB6,0x45,0x2F,0xFB);
DEFINE_GUID(IID_IDXGIDevice4,0x95B4F95F,0xD8DA,0x4CA4,0x9E,0xE6,0x3B,0x76,0xD5,0x96,0x8A,0x10);
DEFINE_GUID(IID_IDXGIFactory5,0x7632e1f5,0xee65,0x4dca,0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


