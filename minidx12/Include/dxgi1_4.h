

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0613 */
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

#ifndef __dxgi1_4_h__
#define __dxgi1_4_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGISwapChain3_FWD_DEFINED__
#define __IDXGISwapChain3_FWD_DEFINED__
typedef interface IDXGISwapChain3 IDXGISwapChain3;

#endif 	/* __IDXGISwapChain3_FWD_DEFINED__ */


#ifndef __IDXGIOutput4_FWD_DEFINED__
#define __IDXGIOutput4_FWD_DEFINED__
typedef interface IDXGIOutput4 IDXGIOutput4;

#endif 	/* __IDXGIOutput4_FWD_DEFINED__ */


#ifndef __IDXGIFactory4_FWD_DEFINED__
#define __IDXGIFactory4_FWD_DEFINED__
typedef interface IDXGIFactory4 IDXGIFactory4;

#endif 	/* __IDXGIFactory4_FWD_DEFINED__ */


#ifndef __IDXGIAdapter3_FWD_DEFINED__
#define __IDXGIAdapter3_FWD_DEFINED__
typedef interface IDXGIAdapter3 IDXGIAdapter3;

#endif 	/* __IDXGIAdapter3_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi1_3.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_4_0000_0000 */
/* [local] */ 

#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
typedef 
enum DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG
    {
        DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT	= 0x1,
        DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_OVERLAY_PRESENT	= 0x2
    } 	DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGISwapChain3_INTERFACE_DEFINED__
#define __IDXGISwapChain3_INTERFACE_DEFINED__

/* interface IDXGISwapChain3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("94d99bdb-f1f8-4ab0-b236-7da0170edab1")
    IDXGISwapChain3 : public IDXGISwapChain2
    {
    public:
        virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport( 
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorSpace1( 
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1( 
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
            _In_reads_(BufferCount)  IUnknown *const *ppPresentQueue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain3 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
            IDXGISwapChain3 * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGISwapChain3 * This,
            UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
            IDXGISwapChain3 * This,
            const DXGI_MATRIX_3X2_F *pMatrix);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix);
        
        UINT ( STDMETHODCALLTYPE *GetCurrentBackBufferIndex )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckColorSpaceSupport )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorSpace1 )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers1 )( 
            IDXGISwapChain3 * This,
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
        
        END_INTERFACE
    } IDXGISwapChain3Vtbl;

    interface IDXGISwapChain3
    {
        CONST_VTBL struct IDXGISwapChain3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain3_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain3_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain3_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain3_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain3_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain3_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain3_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain3_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain3_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain3_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain3_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain3_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain3_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain3_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain3_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain3_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain3_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain3_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain3_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain3_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain3_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain3_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain3_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain3_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain3_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain3_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain3_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain3_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain3_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 


#define IDXGISwapChain3_GetCurrentBackBufferIndex(This)	\
    ( (This)->lpVtbl -> GetCurrentBackBufferIndex(This) ) 

#define IDXGISwapChain3_CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport)	\
    ( (This)->lpVtbl -> CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport) ) 

#define IDXGISwapChain3_SetColorSpace1(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace1(This,ColorSpace) ) 

#define IDXGISwapChain3_ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue)	\
    ( (This)->lpVtbl -> ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_4_0000_0001 */
/* [local] */ 

typedef 
enum DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG
    {
        DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG_PRESENT	= 0x1
    } 	DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutput4_INTERFACE_DEFINED__
#define __IDXGIOutput4_INTERFACE_DEFINED__

/* interface IDXGIOutput4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc7dca35-2196-414d-9F53-617884032a60")
    IDXGIOutput4 : public IDXGIOutput3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckOverlayColorSpaceSupport( 
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput4 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput4 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        END_INTERFACE
    } IDXGIOutput4Vtbl;

    interface IDXGIOutput4
    {
        CONST_VTBL struct IDXGIOutput4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput4_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput4_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput4_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput4_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput4_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput4_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput4_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput4_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput4_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput4_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput4_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput4_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput4_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput4_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput4_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput4_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput4_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 


#define IDXGIOutput4_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput4_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactory4_INTERFACE_DEFINED__
#define __IDXGIFactory4_INTERFACE_DEFINED__

/* interface IDXGIFactory4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIFactory4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
    IDXGIFactory4 : public IDXGIFactory3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid( 
            /* [annotation] */ 
            _In_  LUID AdapterLuid,
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter( 
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIFactory4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIFactory4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIFactory4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIFactory4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory4 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
            IDXGIFactory4 * This,
            HWND WindowHandle,
            UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
            IDXGIFactory4 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pWindowHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory4 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory4 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
        BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
            IDXGIFactory4 * This);
        
        BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
            IDXGIFactory4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
            IDXGIFactory4 * This,
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
            IDXGIFactory4 * This,
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
            IDXGIFactory4 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _Out_  LUID *pLuid);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
            IDXGIFactory4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            IDXGIFactory4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapterByLuid )( 
            IDXGIFactory4 * This,
            /* [annotation] */ 
            _In_  LUID AdapterLuid,
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumWarpAdapter )( 
            IDXGIFactory4 * This,
            /* [annotation] */ 
            _In_  REFIID riid,
            /* [annotation] */ 
            _COM_Outptr_  void **ppvAdapter);
        
        END_INTERFACE
    } IDXGIFactory4Vtbl;

    interface IDXGIFactory4
    {
        CONST_VTBL struct IDXGIFactory4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIFactory4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory4_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory4_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory4_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory4_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory4_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory4_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory4_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory4_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory4_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory4_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory4_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory4_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory4_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory4_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory4_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory4_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory4_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory4_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 


#define IDXGIFactory4_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 


#define IDXGIFactory4_EnumAdapterByLuid(This,AdapterLuid,riid,ppvAdapter)	\
    ( (This)->lpVtbl -> EnumAdapterByLuid(This,AdapterLuid,riid,ppvAdapter) ) 

#define IDXGIFactory4_EnumWarpAdapter(This,riid,ppvAdapter)	\
    ( (This)->lpVtbl -> EnumWarpAdapter(This,riid,ppvAdapter) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_4_0000_0003 */
/* [local] */ 

typedef 
enum DXGI_MEMORY_SEGMENT_GROUP
    {
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL	= 0,
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL	= 1
    } 	DXGI_MEMORY_SEGMENT_GROUP;

typedef struct DXGI_QUERY_VIDEO_MEMORY_INFO
    {
    UINT64 Budget;
    UINT64 CurrentUsage;
    UINT64 AvailableForReservation;
    UINT64 CurrentReservation;
    } 	DXGI_QUERY_VIDEO_MEMORY_INFO;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0003_v0_0_s_ifspec;

#ifndef __IDXGIAdapter3_INTERFACE_DEFINED__
#define __IDXGIAdapter3_INTERFACE_DEFINED__

/* interface IDXGIAdapter3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIAdapter3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("645967A4-1392-4310-A798-8053CE3E93FD")
    IDXGIAdapter3 : public IDXGIAdapter2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterHardwareContentProtectionTeardownStatusEvent( 
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual void STDMETHODCALLTYPE UnregisterHardwareContentProtectionTeardownStatus( 
            /* [annotation][in] */ 
            _In_  DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryVideoMemoryInfo( 
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][out] */ 
            _Out_  DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoMemoryReservation( 
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][in] */ 
            _In_  UINT64 Reservation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterVideoMemoryBudgetChangeNotificationEvent( 
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual void STDMETHODCALLTYPE UnregisterVideoMemoryBudgetChangeNotification( 
            /* [annotation][in] */ 
            _In_  DWORD dwCookie) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIAdapter3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIAdapter3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIAdapter3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIAdapter3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
            IDXGIAdapter3 * This,
            /* [in] */ UINT Output,
            /* [annotation][out][in] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIAdapter3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID InterfaceName,
            /* [annotation][out] */ 
            _Out_  LARGE_INTEGER *pUMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGIAdapter3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc2 )( 
            IDXGIAdapter3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC2 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterHardwareContentProtectionTeardownStatusEvent )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterHardwareContentProtectionTeardownStatus )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *QueryVideoMemoryInfo )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][out] */ 
            _Out_  DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoMemoryReservation )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  UINT NodeIndex,
            /* [annotation][in] */ 
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][in] */ 
            _In_  UINT64 Reservation);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterVideoMemoryBudgetChangeNotificationEvent )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterVideoMemoryBudgetChangeNotification )( 
            IDXGIAdapter3 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        END_INTERFACE
    } IDXGIAdapter3Vtbl;

    interface IDXGIAdapter3
    {
        CONST_VTBL struct IDXGIAdapter3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIAdapter3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter3_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter3_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter3_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 


#define IDXGIAdapter3_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define IDXGIAdapter3_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 


#define IDXGIAdapter3_RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIAdapter3_UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie) ) 

#define IDXGIAdapter3_QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo)	\
    ( (This)->lpVtbl -> QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo) ) 

#define IDXGIAdapter3_SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation)	\
    ( (This)->lpVtbl -> SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation) ) 

#define IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie) ) 

#define IDXGIAdapter3_UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_4_0000_0004 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_IDXGISwapChain3,0x94d99bdb,0xf1f8,0x4ab0,0xb2,0x36,0x7d,0xa0,0x17,0x0e,0xda,0xb1);
DEFINE_GUID(IID_IDXGIOutput4,0xdc7dca35,0x2196,0x414d,0x9F,0x53,0x61,0x78,0x84,0x03,0x2a,0x60);
DEFINE_GUID(IID_IDXGIFactory4,0x1bc6ea02,0xef36,0x464f,0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a);
DEFINE_GUID(IID_IDXGIAdapter3,0x645967A4,0x1392,0x4310,0xA7,0x98,0x80,0x53,0xCE,0x3E,0x93,0xFD);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


