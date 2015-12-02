

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

#ifndef __dxgi1_3_h__
#define __dxgi1_3_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIDevice3_FWD_DEFINED__
#define __IDXGIDevice3_FWD_DEFINED__
typedef interface IDXGIDevice3 IDXGIDevice3;

#endif 	/* __IDXGIDevice3_FWD_DEFINED__ */


#ifndef __IDXGISwapChain2_FWD_DEFINED__
#define __IDXGISwapChain2_FWD_DEFINED__
typedef interface IDXGISwapChain2 IDXGISwapChain2;

#endif 	/* __IDXGISwapChain2_FWD_DEFINED__ */


#ifndef __IDXGIOutput2_FWD_DEFINED__
#define __IDXGIOutput2_FWD_DEFINED__
typedef interface IDXGIOutput2 IDXGIOutput2;

#endif 	/* __IDXGIOutput2_FWD_DEFINED__ */


#ifndef __IDXGIFactory3_FWD_DEFINED__
#define __IDXGIFactory3_FWD_DEFINED__
typedef interface IDXGIFactory3 IDXGIFactory3;

#endif 	/* __IDXGIFactory3_FWD_DEFINED__ */


#ifndef __IDXGIDecodeSwapChain_FWD_DEFINED__
#define __IDXGIDecodeSwapChain_FWD_DEFINED__
typedef interface IDXGIDecodeSwapChain IDXGIDecodeSwapChain;

#endif 	/* __IDXGIDecodeSwapChain_FWD_DEFINED__ */


#ifndef __IDXGIFactoryMedia_FWD_DEFINED__
#define __IDXGIFactoryMedia_FWD_DEFINED__
typedef interface IDXGIFactoryMedia IDXGIFactoryMedia;

#endif 	/* __IDXGIFactoryMedia_FWD_DEFINED__ */


#ifndef __IDXGISwapChainMedia_FWD_DEFINED__
#define __IDXGISwapChainMedia_FWD_DEFINED__
typedef interface IDXGISwapChainMedia IDXGISwapChainMedia;

#endif 	/* __IDXGISwapChainMedia_FWD_DEFINED__ */


#ifndef __IDXGIOutput3_FWD_DEFINED__
#define __IDXGIOutput3_FWD_DEFINED__
typedef interface IDXGIOutput3 IDXGIOutput3;

#endif 	/* __IDXGIOutput3_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi1_2.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_3_0000_0000 */
/* [local] */ 

#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define DXGI_CREATE_FACTORY_DEBUG 0x1
HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, _COM_Outptr_ void **ppFactory);
HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, _COM_Outptr_ void **pDebug);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIDevice3_INTERFACE_DEFINED__
#define __IDXGIDevice3_INTERFACE_DEFINED__

/* interface IDXGIDevice3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDevice3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6007896c-3244-4afd-bf18-a6d3beda5023")
    IDXGIDevice3 : public IDXGIDevice2
    {
    public:
        virtual void STDMETHODCALLTYPE Trim( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDevice3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDevice3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDevice3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDevice3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice3 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
            IDXGIDevice3 * This,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IUnknown *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
            /* [in] */ UINT NumResources);
        
        HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
            IDXGIDevice3 * This,
            /* [in] */ INT Priority);
        
        HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
            IDXGIDevice3 * This,
            /* [annotation][retval][out] */ 
            _Out_  INT *pPriority);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGIDevice3 * This,
            /* [in] */ UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGIDevice3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);
        
        HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent);
        
        void ( STDMETHODCALLTYPE *Trim )( 
            IDXGIDevice3 * This);
        
        END_INTERFACE
    } IDXGIDevice3Vtbl;

    interface IDXGIDevice3
    {
        CONST_VTBL struct IDXGIDevice3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDevice3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice3_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice3_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice3_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice3_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice3_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice3_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice3_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice3_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice3_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice3_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 


#define IDXGIDevice3_Trim(This)	\
    ( (This)->lpVtbl -> Trim(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_3_0000_0001 */
/* [local] */ 

typedef struct DXGI_MATRIX_3X2_F
    {
    FLOAT _11;
    FLOAT _12;
    FLOAT _21;
    FLOAT _22;
    FLOAT _31;
    FLOAT _32;
    } 	DXGI_MATRIX_3X2_F;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGISwapChain2_INTERFACE_DEFINED__
#define __IDXGISwapChain2_INTERFACE_DEFINED__

/* interface IDXGISwapChain2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a8be2ac4-199f-4946-b331-79599fb98de7")
    IDXGISwapChain2 : public IDXGISwapChain1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSourceSize( 
            UINT Width,
            UINT Height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceSize( 
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency( 
            UINT MaxLatency) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency( 
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency) = 0;
        
        virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform( 
            const DXGI_MATRIX_3X2_F *pMatrix) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform( 
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain2 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain2 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain2 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain2 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain2 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
            IDXGISwapChain2 * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGISwapChain2 * This,
            UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
            IDXGISwapChain2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
            IDXGISwapChain2 * This,
            const DXGI_MATRIX_3X2_F *pMatrix);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
            IDXGISwapChain2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix);
        
        END_INTERFACE
    } IDXGISwapChain2Vtbl;

    interface IDXGISwapChain2
    {
        CONST_VTBL struct IDXGISwapChain2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain2_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain2_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain2_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain2_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain2_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain2_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain2_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain2_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain2_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain2_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain2_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain2_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain2_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain2_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain2_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain2_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain2_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain2_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain2_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain2_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain2_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain2_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain2_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain2_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain2_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain2_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain2_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain2_INTERFACE_DEFINED__ */


#ifndef __IDXGIOutput2_INTERFACE_DEFINED__
#define __IDXGIOutput2_INTERFACE_DEFINED__

/* interface IDXGIOutput2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("595e39d1-2724-4663-99b1-da969de28364")
    IDXGIOutput2 : public IDXGIOutput1
    {
    public:
        virtual BOOL STDMETHODCALLTYPE SupportsOverlays( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput2 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput2 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput2 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput2 * This);
        
        END_INTERFACE
    } IDXGIOutput2Vtbl;

    interface IDXGIOutput2
    {
        CONST_VTBL struct IDXGIOutput2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput2_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput2_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput2_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput2_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput2_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput2_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput2_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput2_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput2_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput2_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput2_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput2_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput2_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput2_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput2_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput2_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput2_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactory3_INTERFACE_DEFINED__
#define __IDXGIFactory3_INTERFACE_DEFINED__

/* interface IDXGIFactory3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIFactory3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("25483823-cd46-4c7d-86ca-47aa95b837bd")
    IDXGIFactory3 : public IDXGIFactory2
    {
    public:
        virtual UINT STDMETHODCALLTYPE GetCreationFlags( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIFactory3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIFactory3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIFactory3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIFactory3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory3 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
            IDXGIFactory3 * This,
            HWND WindowHandle,
            UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
            IDXGIFactory3 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pWindowHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory3 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory3 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
        BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
            IDXGIFactory3 * This);
        
        BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
            IDXGIFactory3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
            IDXGIFactory3 * This,
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
            IDXGIFactory3 * This,
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
            IDXGIFactory3 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _Out_  LUID *pLuid);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
            IDXGIFactory3 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            IDXGIFactory3 * This);
        
        END_INTERFACE
    } IDXGIFactory3Vtbl;

    interface IDXGIFactory3
    {
        CONST_VTBL struct IDXGIFactory3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIFactory3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory3_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory3_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory3_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory3_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory3_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory3_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory3_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory3_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory3_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory3_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory3_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory3_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory3_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory3_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory3_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory3_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory3_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory3_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 


#define IDXGIFactory3_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_3_0000_0004 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef struct DXGI_DECODE_SWAP_CHAIN_DESC
    {
    UINT Flags;
    } 	DXGI_DECODE_SWAP_CHAIN_DESC;

typedef 
enum DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS
    {
        DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_NOMINAL_RANGE	= 0x1,
        DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_BT709	= 0x2,
        DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_xvYCC	= 0x4
    } 	DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0004_v0_0_s_ifspec;

#ifndef __IDXGIDecodeSwapChain_INTERFACE_DEFINED__
#define __IDXGIDecodeSwapChain_INTERFACE_DEFINED__

/* interface IDXGIDecodeSwapChain */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDecodeSwapChain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2633066b-4514-4c7a-8fd8-12ea98059d18")
    IDXGIDecodeSwapChain : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PresentBuffer( 
            UINT BufferToPresent,
            UINT SyncInterval,
            UINT Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSourceRect( 
            const RECT *pRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTargetRect( 
            const RECT *pRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDestSize( 
            UINT Width,
            UINT Height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceRect( 
            /* [annotation][out] */ 
            _Out_  RECT *pRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTargetRect( 
            /* [annotation][out] */ 
            _Out_  RECT *pRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDestSize( 
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorSpace( 
            DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ColorSpace) = 0;
        
        virtual DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS STDMETHODCALLTYPE GetColorSpace( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDecodeSwapChainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDecodeSwapChain * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDecodeSwapChain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDecodeSwapChain * This);
        
        HRESULT ( STDMETHODCALLTYPE *PresentBuffer )( 
            IDXGIDecodeSwapChain * This,
            UINT BufferToPresent,
            UINT SyncInterval,
            UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceRect )( 
            IDXGIDecodeSwapChain * This,
            const RECT *pRect);
        
        HRESULT ( STDMETHODCALLTYPE *SetTargetRect )( 
            IDXGIDecodeSwapChain * This,
            const RECT *pRect);
        
        HRESULT ( STDMETHODCALLTYPE *SetDestSize )( 
            IDXGIDecodeSwapChain * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceRect )( 
            IDXGIDecodeSwapChain * This,
            /* [annotation][out] */ 
            _Out_  RECT *pRect);
        
        HRESULT ( STDMETHODCALLTYPE *GetTargetRect )( 
            IDXGIDecodeSwapChain * This,
            /* [annotation][out] */ 
            _Out_  RECT *pRect);
        
        HRESULT ( STDMETHODCALLTYPE *GetDestSize )( 
            IDXGIDecodeSwapChain * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorSpace )( 
            IDXGIDecodeSwapChain * This,
            DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ColorSpace);
        
        DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ( STDMETHODCALLTYPE *GetColorSpace )( 
            IDXGIDecodeSwapChain * This);
        
        END_INTERFACE
    } IDXGIDecodeSwapChainVtbl;

    interface IDXGIDecodeSwapChain
    {
        CONST_VTBL struct IDXGIDecodeSwapChainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDecodeSwapChain_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDecodeSwapChain_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDecodeSwapChain_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDecodeSwapChain_PresentBuffer(This,BufferToPresent,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> PresentBuffer(This,BufferToPresent,SyncInterval,Flags) ) 

#define IDXGIDecodeSwapChain_SetSourceRect(This,pRect)	\
    ( (This)->lpVtbl -> SetSourceRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_SetTargetRect(This,pRect)	\
    ( (This)->lpVtbl -> SetTargetRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_SetDestSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetDestSize(This,Width,Height) ) 

#define IDXGIDecodeSwapChain_GetSourceRect(This,pRect)	\
    ( (This)->lpVtbl -> GetSourceRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_GetTargetRect(This,pRect)	\
    ( (This)->lpVtbl -> GetTargetRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_GetDestSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetDestSize(This,pWidth,pHeight) ) 

#define IDXGIDecodeSwapChain_SetColorSpace(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace(This,ColorSpace) ) 

#define IDXGIDecodeSwapChain_GetColorSpace(This)	\
    ( (This)->lpVtbl -> GetColorSpace(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDecodeSwapChain_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactoryMedia_INTERFACE_DEFINED__
#define __IDXGIFactoryMedia_INTERFACE_DEFINED__

/* interface IDXGIFactoryMedia */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIFactoryMedia;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41e7d1f2-a591-4f7b-a2e5-fa9c843e1c12")
    IDXGIFactoryMedia : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCompositionSurfaceHandle( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDecodeSwapChainForCompositionSurfaceHandle( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pYuvDecodeBuffers,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIDecodeSwapChain **ppSwapChain) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIFactoryMediaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIFactoryMedia * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIFactoryMedia * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIFactoryMedia * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCompositionSurfaceHandle )( 
            IDXGIFactoryMedia * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDecodeSwapChainForCompositionSurfaceHandle )( 
            IDXGIFactoryMedia * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pYuvDecodeBuffers,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIDecodeSwapChain **ppSwapChain);
        
        END_INTERFACE
    } IDXGIFactoryMediaVtbl;

    interface IDXGIFactoryMedia
    {
        CONST_VTBL struct IDXGIFactoryMediaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIFactoryMedia_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactoryMedia_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactoryMedia_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactoryMedia_CreateSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactoryMedia_CreateDecodeSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pYuvDecodeBuffers,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateDecodeSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pYuvDecodeBuffers,pRestrictToOutput,ppSwapChain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactoryMedia_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_3_0000_0006 */
/* [local] */ 

typedef 
enum DXGI_FRAME_PRESENTATION_MODE
    {
        DXGI_FRAME_PRESENTATION_MODE_COMPOSED	= 0,
        DXGI_FRAME_PRESENTATION_MODE_OVERLAY	= 1,
        DXGI_FRAME_PRESENTATION_MODE_NONE	= 2,
        DXGI_FRAME_PRESENTATION_MODE_COMPOSITION_FAILURE	= 3
    } 	DXGI_FRAME_PRESENTATION_MODE;

typedef struct DXGI_FRAME_STATISTICS_MEDIA
    {
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    LARGE_INTEGER SyncGPUTime;
    DXGI_FRAME_PRESENTATION_MODE CompositionMode;
    UINT ApprovedPresentDuration;
    } 	DXGI_FRAME_STATISTICS_MEDIA;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0006_v0_0_s_ifspec;

#ifndef __IDXGISwapChainMedia_INTERFACE_DEFINED__
#define __IDXGISwapChainMedia_INTERFACE_DEFINED__

/* interface IDXGISwapChainMedia */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChainMedia;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dd95b90b-f05f-4f6a-bd65-25bfb264bd84")
    IDXGISwapChainMedia : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameStatisticsMedia( 
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS_MEDIA *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPresentDuration( 
            UINT Duration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckPresentDurationSupport( 
            UINT DesiredPresentDuration,
            /* [annotation][out] */ 
            _Out_  UINT *pClosestSmallerPresentDuration,
            /* [annotation][out] */ 
            _Out_  UINT *pClosestLargerPresentDuration) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChainMediaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChainMedia * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChainMedia * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChainMedia * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatisticsMedia )( 
            IDXGISwapChainMedia * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS_MEDIA *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetPresentDuration )( 
            IDXGISwapChainMedia * This,
            UINT Duration);
        
        HRESULT ( STDMETHODCALLTYPE *CheckPresentDurationSupport )( 
            IDXGISwapChainMedia * This,
            UINT DesiredPresentDuration,
            /* [annotation][out] */ 
            _Out_  UINT *pClosestSmallerPresentDuration,
            /* [annotation][out] */ 
            _Out_  UINT *pClosestLargerPresentDuration);
        
        END_INTERFACE
    } IDXGISwapChainMediaVtbl;

    interface IDXGISwapChainMedia
    {
        CONST_VTBL struct IDXGISwapChainMediaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChainMedia_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChainMedia_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChainMedia_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChainMedia_GetFrameStatisticsMedia(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatisticsMedia(This,pStats) ) 

#define IDXGISwapChainMedia_SetPresentDuration(This,Duration)	\
    ( (This)->lpVtbl -> SetPresentDuration(This,Duration) ) 

#define IDXGISwapChainMedia_CheckPresentDurationSupport(This,DesiredPresentDuration,pClosestSmallerPresentDuration,pClosestLargerPresentDuration)	\
    ( (This)->lpVtbl -> CheckPresentDurationSupport(This,DesiredPresentDuration,pClosestSmallerPresentDuration,pClosestLargerPresentDuration) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChainMedia_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_3_0000_0007 */
/* [local] */ 

typedef 
enum DXGI_OVERLAY_SUPPORT_FLAG
    {
        DXGI_OVERLAY_SUPPORT_FLAG_DIRECT	= 0x1,
        DXGI_OVERLAY_SUPPORT_FLAG_SCALING	= 0x2
    } 	DXGI_OVERLAY_SUPPORT_FLAG;

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0007_v0_0_s_ifspec;

#ifndef __IDXGIOutput3_INTERFACE_DEFINED__
#define __IDXGIOutput3_INTERFACE_DEFINED__

/* interface IDXGIOutput3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8a6bb301-7e7e-41F4-a8e0-5b32f7f99b18")
    IDXGIOutput3 : public IDXGIOutput2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckOverlaySupport( 
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput3 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput3 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        END_INTERFACE
    } IDXGIOutput3Vtbl;

    interface IDXGIOutput3
    {
        CONST_VTBL struct IDXGIOutput3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput3_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput3_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput3_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput3_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput3_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput3_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput3_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput3_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput3_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput3_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput3_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput3_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput3_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput3_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput3_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput3_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput3_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput3_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_3_0000_0008 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_IDXGIDevice3,0x6007896c,0x3244,0x4afd,0xbf,0x18,0xa6,0xd3,0xbe,0xda,0x50,0x23);
DEFINE_GUID(IID_IDXGISwapChain2,0xa8be2ac4,0x199f,0x4946,0xb3,0x31,0x79,0x59,0x9f,0xb9,0x8d,0xe7);
DEFINE_GUID(IID_IDXGIOutput2,0x595e39d1,0x2724,0x4663,0x99,0xb1,0xda,0x96,0x9d,0xe2,0x83,0x64);
DEFINE_GUID(IID_IDXGIFactory3,0x25483823,0xcd46,0x4c7d,0x86,0xca,0x47,0xaa,0x95,0xb8,0x37,0xbd);
DEFINE_GUID(IID_IDXGIDecodeSwapChain,0x2633066b,0x4514,0x4c7a,0x8f,0xd8,0x12,0xea,0x98,0x05,0x9d,0x18);
DEFINE_GUID(IID_IDXGIFactoryMedia,0x41e7d1f2,0xa591,0x4f7b,0xa2,0xe5,0xfa,0x9c,0x84,0x3e,0x1c,0x12);
DEFINE_GUID(IID_IDXGISwapChainMedia,0xdd95b90b,0xf05f,0x4f6a,0xbd,0x65,0x25,0xbf,0xb2,0x64,0xbd,0x84);
DEFINE_GUID(IID_IDXGIOutput3,0x8a6bb301,0x7e7e,0x41F4,0xa8,0xe0,0x5b,0x32,0xf7,0xf9,0x9b,0x18);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0008_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


