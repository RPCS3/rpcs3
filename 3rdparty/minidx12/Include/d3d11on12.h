/*-------------------------------------------------------------------------------------
 *
 * Copyright (c) Microsoft Corporation
 *
 *-------------------------------------------------------------------------------------*/


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */



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

#ifndef __d3d11on12_h__
#define __d3d11on12_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11On12Device_FWD_DEFINED__
#define __ID3D11On12Device_FWD_DEFINED__
typedef interface ID3D11On12Device ID3D11On12Device;

#endif 	/* __ID3D11On12Device_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "d3d11.h"
#include "d3d12.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d11on12_0000_0000 */
/* [local] */ 

#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

///////////////////////////////////////////////////////////////////////////
// D3D11On12CreateDevice
// ------------------
//
// pDevice
//      Specifies a pre-existing D3D12 device to use for D3D11 interop.
//      May not be NULL.
// Flags
//      Any of those documented for D3D11CreateDeviceAndSwapChain.
// pFeatureLevels
//      Array of any of the following:
//          D3D_FEATURE_LEVEL_12_1
//          D3D_FEATURE_LEVEL_12_0
//          D3D_FEATURE_LEVEL_11_1
//          D3D_FEATURE_LEVEL_11_0
//          D3D_FEATURE_LEVEL_10_1
//          D3D_FEATURE_LEVEL_10_0
//          D3D_FEATURE_LEVEL_9_3
//          D3D_FEATURE_LEVEL_9_2
//          D3D_FEATURE_LEVEL_9_1
//       The first feature level which is less than or equal to the
//       D3D12 device's feature level will be used to perform D3D11 validation.
//       Creation will fail if no acceptable feature levels are provided.
//       Providing NULL will default to the D3D12 device's feature level.
// FeatureLevels
//      Size of feature levels array.
// ppCommandQueues
//      Array of unique queues for D3D11On12 to use. Valid queue types:
//          3D command queue.
//      Flags must be compatible with device flags, and its NodeMask must
//      be a subset of the NodeMask provided to this API.
// NumQueues
//      Size of command queue array.
// NodeMask
//      Which node of the D3D12 device to use.  Only 1 bit may be set.
// ppDevice
//      Pointer to returned interface. May be NULL.
// ppImmediateContext
//      Pointer to returned interface. May be NULL.
// pChosenFeatureLevel
//      Pointer to returned feature level. May be NULL.
//
// Return Values
//  Any of those documented for 
//          D3D11CreateDevice
//
///////////////////////////////////////////////////////////////////////////
typedef HRESULT (WINAPI* PFN_D3D11ON12_CREATE_DEVICE)( _In_ IUnknown*, UINT, 
    _In_reads_opt_( FeatureLevels ) CONST D3D_FEATURE_LEVEL*, UINT FeatureLevels, 
    _In_reads_opt_( NumQueues ) IUnknown* CONST*, UINT NumQueues, 
    UINT, _COM_Outptr_opt_ ID3D11Device**, _COM_Outptr_opt_ ID3D11DeviceContext**, 
    _Out_opt_ D3D_FEATURE_LEVEL* );

HRESULT WINAPI D3D11On12CreateDevice(
    _In_ IUnknown* pDevice,
    UINT Flags,
    _In_reads_opt_( FeatureLevels ) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    _In_reads_opt_( NumQueues ) IUnknown* CONST* ppCommandQueues,
    UINT NumQueues,
    UINT NodeMask,
    _COM_Outptr_opt_ ID3D11Device** ppDevice,
    _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext,
    _Out_opt_ D3D_FEATURE_LEVEL* pChosenFeatureLevel );

typedef struct D3D11_RESOURCE_FLAGS
    {
    UINT BindFlags;
    UINT MiscFlags;
    UINT CPUAccessFlags;
    UINT StructureByteStride;
    } 	D3D11_RESOURCE_FLAGS;



extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11On12Device_INTERFACE_DEFINED__
#define __ID3D11On12Device_INTERFACE_DEFINED__

/* interface ID3D11On12Device */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11On12Device;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85611e73-70a9-490e-9614-a9e302777904")
    ID3D11On12Device : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateWrappedResource( 
            _In_  IUnknown *pResource12,
            _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
            D3D12_RESOURCE_STATES InState,
            D3D12_RESOURCE_STATES OutState,
            REFIID riid,
            _COM_Outptr_opt_  void **ppResource11) = 0;
        
        virtual void STDMETHODCALLTYPE ReleaseWrappedResources( 
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources) = 0;
        
        virtual void STDMETHODCALLTYPE AcquireWrappedResources( 
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11On12DeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11On12Device * This,
            REFIID riid,
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11On12Device * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11On12Device * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateWrappedResource )( 
            ID3D11On12Device * This,
            _In_  IUnknown *pResource12,
            _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
            D3D12_RESOURCE_STATES InState,
            D3D12_RESOURCE_STATES OutState,
            REFIID riid,
            _COM_Outptr_opt_  void **ppResource11);
        
        void ( STDMETHODCALLTYPE *ReleaseWrappedResources )( 
            ID3D11On12Device * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        void ( STDMETHODCALLTYPE *AcquireWrappedResources )( 
            ID3D11On12Device * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        END_INTERFACE
    } ID3D11On12DeviceVtbl;

    interface ID3D11On12Device
    {
        CONST_VTBL struct ID3D11On12DeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11On12Device_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11On12Device_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11On12Device_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11On12Device_CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11)	\
    ( (This)->lpVtbl -> CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11) ) 

#define ID3D11On12Device_ReleaseWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> ReleaseWrappedResources(This,ppResources,NumResources) ) 

#define ID3D11On12Device_AcquireWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> AcquireWrappedResources(This,ppResources,NumResources) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11On12Device_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11on12_0000_0001 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_ID3D11On12Device,0x85611e73,0x70a9,0x490e,0x96,0x14,0xa9,0xe3,0x02,0x77,0x79,0x04);


extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0001_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


