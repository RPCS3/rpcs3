/*-------------------------------------------------------------------------------------
 *
 * Copyright (c) Microsoft Corporation
 *
 *-------------------------------------------------------------------------------------*/


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

#ifndef __d3d11_4_h__
#define __d3d11_4_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11Device4_FWD_DEFINED__
#define __ID3D11Device4_FWD_DEFINED__
typedef interface ID3D11Device4 ID3D11Device4;

#endif 	/* __ID3D11Device4_FWD_DEFINED__ */


#ifndef __ID3D11Device5_FWD_DEFINED__
#define __ID3D11Device5_FWD_DEFINED__
typedef interface ID3D11Device5 ID3D11Device5;

#endif 	/* __ID3D11Device5_FWD_DEFINED__ */


#ifndef __ID3D11Multithread_FWD_DEFINED__
#define __ID3D11Multithread_FWD_DEFINED__
typedef interface ID3D11Multithread ID3D11Multithread;

#endif 	/* __ID3D11Multithread_FWD_DEFINED__ */


#ifndef __ID3D11VideoContext2_FWD_DEFINED__
#define __ID3D11VideoContext2_FWD_DEFINED__
typedef interface ID3D11VideoContext2 ID3D11VideoContext2;

#endif 	/* __ID3D11VideoContext2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgi1_5.h"
#include "d3dcommon.h"
#include "d3d11_3.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d11_4_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d3d11_3.h" //
#ifdef __cplusplus
extern "C"{
#endif
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)


extern RPC_IF_HANDLE __MIDL_itf_d3d11_4_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_4_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11Device4_INTERFACE_DEFINED__
#define __ID3D11Device4_INTERFACE_DEFINED__

/* interface ID3D11Device4 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Device4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8992ab71-02e6-4b8d-ba48-b056dcda42c4")
    ID3D11Device4 : public ID3D11Device3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterDeviceRemovedEvent( 
            /* [annotation] */ 
            _In_  HANDLE hEvent,
            /* [annotation] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual void STDMETHODCALLTYPE UnregisterDeviceRemoved( 
            /* [annotation] */ 
            _In_  DWORD dwCookie) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Device4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Device4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Device4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Device4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT )  UINT NumElements,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecodeWithInputSignature,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11InputLayout **ppInputLayout);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVertexShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_reads_opt_(NumEntries)  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT )  UINT NumEntries,
            /* [annotation] */ 
            _In_reads_opt_(NumStrides)  const UINT *pBufferStrides,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumStrides,
            /* [annotation] */ 
            _In_  UINT RasterizedStream,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePixelShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query **ppQuery);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Counter **ppCounter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
            ID3D11Device4 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pDesc,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_TYPE *pType,
            /* [annotation] */ 
            _Out_  UINT *pActiveCounters,
            /* [annotation] */ 
            _Out_writes_opt_(*pNameLength)  LPSTR szName,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNameLength,
            /* [annotation] */ 
            _Out_writes_opt_(*pUnitsLength)  LPSTR szUnits,
            /* [annotation] */ 
            _Inout_opt_  UINT *pUnitsLength,
            /* [annotation] */ 
            _Out_writes_opt_(*pDescriptionLength)  LPSTR szDescription,
            /* [annotation] */ 
            _Inout_opt_  UINT *pDescriptionLength);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
            ID3D11Device4 * This,
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
            ID3D11Device4 * This);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            ID3D11Device4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
            ID3D11Device4 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
            ID3D11Device4 * This,
            UINT RaiseFlags);
        
        UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
            ID3D11Device4 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
            ID3D11Device4 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
            ID3D11Device4 * This,
            UINT Flags,
            /* [annotation] */ 
            _In_reads_( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            REFIID EmulatedInterface,
            /* [annotation] */ 
            _Out_opt_  D3D_FEATURE_LEVEL *pChosenFeatureLevel,
            /* [annotation] */ 
            _Out_opt_  ID3DDeviceContextState **ppContextState);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext2 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext2 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext2 )( 
            ID3D11Device4 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *GetResourceTiling )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _Out_opt_  UINT *pNumTilesForEntireResource,
            /* [annotation] */ 
            _Out_opt_  D3D11_PACKED_MIP_DESC *pPackedMipDesc,
            /* [annotation] */ 
            _Out_opt_  D3D11_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumSubresourceTilings,
            /* [annotation] */ 
            _In_  UINT FirstSubresourceTilingToGet,
            /* [annotation] */ 
            _Out_writes_(*pNumSubresourceTilings)  D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState2 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery1 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query1 **ppQuery1);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext3 )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext3 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext3 )( 
            ID3D11Device4 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *WriteToSubresource )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pDstBox,
            /* [annotation] */ 
            _In_  const void *pSrcData,
            /* [annotation] */ 
            _In_  UINT SrcRowPitch,
            /* [annotation] */ 
            _In_  UINT SrcDepthPitch);
        
        void ( STDMETHODCALLTYPE *ReadFromSubresource )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _Out_  void *pDstData,
            /* [annotation] */ 
            _In_  UINT DstRowPitch,
            /* [annotation] */ 
            _In_  UINT DstDepthPitch,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDeviceRemovedEvent )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  HANDLE hEvent,
            /* [annotation] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterDeviceRemoved )( 
            ID3D11Device4 * This,
            /* [annotation] */ 
            _In_  DWORD dwCookie);
        
        END_INTERFACE
    } ID3D11Device4Vtbl;

    interface ID3D11Device4
    {
        CONST_VTBL struct ID3D11Device4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Device4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device4_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device4_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device4_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device4_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device4_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device4_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device4_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device4_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device4_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device4_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device4_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device4_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device4_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device4_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device4_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device4_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device4_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device4_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device4_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device4_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device4_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device4_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device4_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device4_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device4_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device4_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device4_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device4_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device4_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device4_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device4_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device4_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device4_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device4_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device4_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device4_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device4_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device4_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device4_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device4_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device4_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device4_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device4_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device4_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device4_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device4_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device4_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 


#define ID3D11Device4_GetImmediateContext2(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext2(This,ppImmediateContext) ) 

#define ID3D11Device4_CreateDeferredContext2(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext2(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device4_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D11Device4_CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels) ) 


#define ID3D11Device4_CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D) ) 

#define ID3D11Device4_CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D) ) 

#define ID3D11Device4_CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device4_CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1)	\
    ( (This)->lpVtbl -> CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1) ) 

#define ID3D11Device4_CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1) ) 

#define ID3D11Device4_CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1)	\
    ( (This)->lpVtbl -> CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1) ) 

#define ID3D11Device4_CreateQuery1(This,pQueryDesc1,ppQuery1)	\
    ( (This)->lpVtbl -> CreateQuery1(This,pQueryDesc1,ppQuery1) ) 

#define ID3D11Device4_GetImmediateContext3(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext3(This,ppImmediateContext) ) 

#define ID3D11Device4_CreateDeferredContext3(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext3(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device4_WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11Device4_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox) ) 


#define ID3D11Device4_RegisterDeviceRemovedEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterDeviceRemovedEvent(This,hEvent,pdwCookie) ) 

#define ID3D11Device4_UnregisterDeviceRemoved(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterDeviceRemoved(This,dwCookie) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device4_INTERFACE_DEFINED__ */


#ifndef __ID3D11Device5_INTERFACE_DEFINED__
#define __ID3D11Device5_INTERFACE_DEFINED__

/* interface ID3D11Device5 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Device5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8ffde202-a0e7-45df-9e01-e837801b5ea0")
    ID3D11Device5 : public ID3D11Device4
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenSharedFence( 
            /* [annotation] */ 
            _In_  HANDLE hFence,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppFence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFence( 
            /* [annotation] */ 
            _In_  UINT64 InitialValue,
            /* [annotation] */ 
            _In_  D3D11_FENCE_FLAG Flags,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppFence) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Device5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Device5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Device5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Device5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT )  UINT NumElements,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecodeWithInputSignature,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11InputLayout **ppInputLayout);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVertexShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_reads_opt_(NumEntries)  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT )  UINT NumEntries,
            /* [annotation] */ 
            _In_reads_opt_(NumStrides)  const UINT *pBufferStrides,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumStrides,
            /* [annotation] */ 
            _In_  UINT RasterizedStream,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePixelShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query **ppQuery);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Counter **ppCounter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
            ID3D11Device5 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pDesc,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_TYPE *pType,
            /* [annotation] */ 
            _Out_  UINT *pActiveCounters,
            /* [annotation] */ 
            _Out_writes_opt_(*pNameLength)  LPSTR szName,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNameLength,
            /* [annotation] */ 
            _Out_writes_opt_(*pUnitsLength)  LPSTR szUnits,
            /* [annotation] */ 
            _Inout_opt_  UINT *pUnitsLength,
            /* [annotation] */ 
            _Out_writes_opt_(*pDescriptionLength)  LPSTR szDescription,
            /* [annotation] */ 
            _Inout_opt_  UINT *pDescriptionLength);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
            ID3D11Device5 * This,
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
            ID3D11Device5 * This);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            ID3D11Device5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
            ID3D11Device5 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
            ID3D11Device5 * This,
            UINT RaiseFlags);
        
        UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
            ID3D11Device5 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
            ID3D11Device5 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
            ID3D11Device5 * This,
            UINT Flags,
            /* [annotation] */ 
            _In_reads_( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            REFIID EmulatedInterface,
            /* [annotation] */ 
            _Out_opt_  D3D_FEATURE_LEVEL *pChosenFeatureLevel,
            /* [annotation] */ 
            _Out_opt_  ID3DDeviceContextState **ppContextState);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext2 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext2 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext2 )( 
            ID3D11Device5 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *GetResourceTiling )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _Out_opt_  UINT *pNumTilesForEntireResource,
            /* [annotation] */ 
            _Out_opt_  D3D11_PACKED_MIP_DESC *pPackedMipDesc,
            /* [annotation] */ 
            _Out_opt_  D3D11_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumSubresourceTilings,
            /* [annotation] */ 
            _In_  UINT FirstSubresourceTilingToGet,
            /* [annotation] */ 
            _Out_writes_(*pNumSubresourceTilings)  D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState2 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery1 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query1 **ppQuery1);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext3 )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext3 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext3 )( 
            ID3D11Device5 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *WriteToSubresource )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pDstBox,
            /* [annotation] */ 
            _In_  const void *pSrcData,
            /* [annotation] */ 
            _In_  UINT SrcRowPitch,
            /* [annotation] */ 
            _In_  UINT SrcDepthPitch);
        
        void ( STDMETHODCALLTYPE *ReadFromSubresource )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _Out_  void *pDstData,
            /* [annotation] */ 
            _In_  UINT DstRowPitch,
            /* [annotation] */ 
            _In_  UINT DstDepthPitch,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDeviceRemovedEvent )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  HANDLE hEvent,
            /* [annotation] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterDeviceRemoved )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedFence )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  HANDLE hFence,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppFence);
        
        HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
            ID3D11Device5 * This,
            /* [annotation] */ 
            _In_  UINT64 InitialValue,
            /* [annotation] */ 
            _In_  D3D11_FENCE_FLAG Flags,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppFence);
        
        END_INTERFACE
    } ID3D11Device5Vtbl;

    interface ID3D11Device5
    {
        CONST_VTBL struct ID3D11Device5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Device5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device5_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device5_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device5_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device5_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device5_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device5_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device5_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device5_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device5_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device5_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device5_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device5_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device5_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device5_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device5_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device5_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device5_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device5_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device5_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device5_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device5_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device5_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device5_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device5_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device5_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device5_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device5_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device5_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device5_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device5_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device5_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device5_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device5_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device5_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device5_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device5_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device5_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device5_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device5_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device5_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device5_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device5_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device5_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device5_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device5_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device5_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device5_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 


#define ID3D11Device5_GetImmediateContext2(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext2(This,ppImmediateContext) ) 

#define ID3D11Device5_CreateDeferredContext2(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext2(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device5_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D11Device5_CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels) ) 


#define ID3D11Device5_CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D) ) 

#define ID3D11Device5_CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D) ) 

#define ID3D11Device5_CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device5_CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1)	\
    ( (This)->lpVtbl -> CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1) ) 

#define ID3D11Device5_CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1) ) 

#define ID3D11Device5_CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1)	\
    ( (This)->lpVtbl -> CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1) ) 

#define ID3D11Device5_CreateQuery1(This,pQueryDesc1,ppQuery1)	\
    ( (This)->lpVtbl -> CreateQuery1(This,pQueryDesc1,ppQuery1) ) 

#define ID3D11Device5_GetImmediateContext3(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext3(This,ppImmediateContext) ) 

#define ID3D11Device5_CreateDeferredContext3(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext3(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device5_WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11Device5_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox) ) 


#define ID3D11Device5_RegisterDeviceRemovedEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterDeviceRemovedEvent(This,hEvent,pdwCookie) ) 

#define ID3D11Device5_UnregisterDeviceRemoved(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterDeviceRemoved(This,dwCookie) ) 


#define ID3D11Device5_OpenSharedFence(This,hFence,ReturnedInterface,ppFence)	\
    ( (This)->lpVtbl -> OpenSharedFence(This,hFence,ReturnedInterface,ppFence) ) 

#define ID3D11Device5_CreateFence(This,InitialValue,Flags,ReturnedInterface,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,ReturnedInterface,ppFence) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device5_INTERFACE_DEFINED__ */


#ifndef __ID3D11Multithread_INTERFACE_DEFINED__
#define __ID3D11Multithread_INTERFACE_DEFINED__

/* interface ID3D11Multithread */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Multithread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9B7E4E00-342C-4106-A19F-4F2704F689F0")
    ID3D11Multithread : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Enter( void) = 0;
        
        virtual void STDMETHODCALLTYPE Leave( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE SetMultithreadProtected( 
            /* [annotation] */ 
            _In_  BOOL bMTProtect) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetMultithreadProtected( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11MultithreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Multithread * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Multithread * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Multithread * This);
        
        void ( STDMETHODCALLTYPE *Enter )( 
            ID3D11Multithread * This);
        
        void ( STDMETHODCALLTYPE *Leave )( 
            ID3D11Multithread * This);
        
        BOOL ( STDMETHODCALLTYPE *SetMultithreadProtected )( 
            ID3D11Multithread * This,
            /* [annotation] */ 
            _In_  BOOL bMTProtect);
        
        BOOL ( STDMETHODCALLTYPE *GetMultithreadProtected )( 
            ID3D11Multithread * This);
        
        END_INTERFACE
    } ID3D11MultithreadVtbl;

    interface ID3D11Multithread
    {
        CONST_VTBL struct ID3D11MultithreadVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Multithread_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Multithread_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Multithread_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Multithread_Enter(This)	\
    ( (This)->lpVtbl -> Enter(This) ) 

#define ID3D11Multithread_Leave(This)	\
    ( (This)->lpVtbl -> Leave(This) ) 

#define ID3D11Multithread_SetMultithreadProtected(This,bMTProtect)	\
    ( (This)->lpVtbl -> SetMultithreadProtected(This,bMTProtect) ) 

#define ID3D11Multithread_GetMultithreadProtected(This)	\
    ( (This)->lpVtbl -> GetMultithreadProtected(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Multithread_INTERFACE_DEFINED__ */


#ifndef __ID3D11VideoContext2_INTERFACE_DEFINED__
#define __ID3D11VideoContext2_INTERFACE_DEFINED__

/* interface ID3D11VideoContext2 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11VideoContext2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4E7374C-6243-4D1B-AE87-52B4F740E261")
    ID3D11VideoContext2 : public ID3D11VideoContext1
    {
    public:
        virtual void STDMETHODCALLTYPE VideoProcessorSetOutputHDRMetaData( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _In_reads_bytes_opt_(Size)  const void *pHDRMetaData) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetOutputHDRMetaData( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  DXGI_HDR_METADATA_TYPE *pType,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(Size)  void *pMetaData) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorSetStreamHDRMetaData( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _In_reads_bytes_opt_(Size)  const void *pHDRMetaData) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetStreamHDRMetaData( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  DXGI_HDR_METADATA_TYPE *pType,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(Size)  void *pMetaData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11VideoContext2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11VideoContext2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11VideoContext2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11VideoContext2 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecoderBuffer )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            D3D11_VIDEO_DECODER_BUFFER_TYPE Type,
            /* [annotation] */ 
            _Out_  UINT *pBufferSize,
            /* [annotation] */ 
            _Outptr_result_bytebuffer_(*pBufferSize)  void **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseDecoderBuffer )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_DECODER_BUFFER_TYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderBeginFrame )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoderOutputView *pView,
            UINT ContentKeySize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(ContentKeySize)  const void *pContentKey);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderEndFrame )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder);
        
        HRESULT ( STDMETHODCALLTYPE *SubmitDecoderBuffers )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_(NumBuffers)  const D3D11_VIDEO_DECODER_BUFFER_DESC *pBufferDesc);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *DecoderExtension )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_EXTENSION *pExtensionData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputTargetRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputBackgroundColor )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL YCbCr,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_COLOR *pColor);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputColorSpace )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputAlphaFillMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE AlphaFillMode,
            /* [annotation] */ 
            _In_  UINT StreamIndex);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputConstriction )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  SIZE Size);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputStereoMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorSetOutputExtension )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  void *pData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputTargetRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *Enabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputBackgroundColor )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pYCbCr,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_COLOR *pColor);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputColorSpace )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputAlphaFillMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE *pAlphaFillMode,
            /* [annotation] */ 
            _Out_  UINT *pStreamIndex);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputConstriction )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  SIZE *pSize);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputStereoMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorGetOutputExtension )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Out_writes_bytes_(DataSize)  void *pData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamFrameFormat )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_FRAME_FORMAT FrameFormat);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamColorSpace )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamOutputRate )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE OutputRate,
            /* [annotation] */ 
            _In_  BOOL RepeatFrame,
            /* [annotation] */ 
            _In_opt_  const DXGI_RATIONAL *pCustomRate);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamSourceRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamDestRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamAlpha )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  FLOAT Alpha);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamPalette )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  UINT Count,
            /* [annotation] */ 
            _In_reads_opt_(Count)  const UINT *pEntries);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamPixelAspectRatio )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const DXGI_RATIONAL *pSourceAspectRatio,
            /* [annotation] */ 
            _In_opt_  const DXGI_RATIONAL *pDestinationAspectRatio);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamLumaKey )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  FLOAT Lower,
            /* [annotation] */ 
            _In_  FLOAT Upper);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamStereoFormat )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT Format,
            /* [annotation] */ 
            _In_  BOOL LeftViewFrame0,
            /* [annotation] */ 
            _In_  BOOL BaseViewFrame0,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE FlipMode,
            /* [annotation] */ 
            _In_  int MonoOffset);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamAutoProcessingMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamFilter )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_FILTER Filter,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  int Level);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorSetStreamExtension )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  void *pData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamFrameFormat )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_FRAME_FORMAT *pFrameFormat);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamColorSpace )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamOutputRate )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE *pOutputRate,
            /* [annotation] */ 
            _Out_  BOOL *pRepeatFrame,
            /* [annotation] */ 
            _Out_  DXGI_RATIONAL *pCustomRate);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamSourceRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamDestRect )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamAlpha )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  FLOAT *pAlpha);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamPalette )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  UINT Count,
            /* [annotation] */ 
            _Out_writes_(Count)  UINT *pEntries);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamPixelAspectRatio )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  DXGI_RATIONAL *pSourceAspectRatio,
            /* [annotation] */ 
            _Out_  DXGI_RATIONAL *pDestinationAspectRatio);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamLumaKey )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  FLOAT *pLower,
            /* [annotation] */ 
            _Out_  FLOAT *pUpper);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamStereoFormat )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnable,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT *pFormat,
            /* [annotation] */ 
            _Out_  BOOL *pLeftViewFrame0,
            /* [annotation] */ 
            _Out_  BOOL *pBaseViewFrame0,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE *pFlipMode,
            /* [annotation] */ 
            _Out_  int *MonoOffset);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamAutoProcessingMode )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamFilter )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_FILTER Filter,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  int *pLevel);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorGetStreamExtension )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Out_writes_bytes_(DataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *VideoProcessorBlt )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessorOutputView *pView,
            /* [annotation] */ 
            _In_  UINT OutputFrame,
            /* [annotation] */ 
            _In_  UINT StreamCount,
            /* [annotation] */ 
            _In_reads_(StreamCount)  const D3D11_VIDEO_PROCESSOR_STREAM *pStreams);
        
        HRESULT ( STDMETHODCALLTYPE *NegotiateCryptoSessionKeyExchange )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Inout_updates_bytes_(DataSize)  void *pData);
        
        void ( STDMETHODCALLTYPE *EncryptionBlt )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  ID3D11Texture2D *pSrcSurface,
            /* [annotation] */ 
            _In_  ID3D11Texture2D *pDstSurface,
            /* [annotation] */ 
            _In_  UINT IVSize,
            /* [annotation] */ 
            _Inout_opt_bytecount_(IVSize)  void *pIV);
        
        void ( STDMETHODCALLTYPE *DecryptionBlt )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  ID3D11Texture2D *pSrcSurface,
            /* [annotation] */ 
            _In_  ID3D11Texture2D *pDstSurface,
            /* [annotation] */ 
            _In_opt_  D3D11_ENCRYPTED_BLOCK_INFO *pEncryptedBlockInfo,
            /* [annotation] */ 
            _In_  UINT ContentKeySize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(ContentKeySize)  const void *pContentKey,
            /* [annotation] */ 
            _In_  UINT IVSize,
            /* [annotation] */ 
            _Inout_opt_bytecount_(IVSize)  void *pIV);
        
        void ( STDMETHODCALLTYPE *StartSessionKeyRefresh )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT RandomNumberSize,
            /* [annotation] */ 
            _Out_writes_bytes_(RandomNumberSize)  void *pRandomNumber);
        
        void ( STDMETHODCALLTYPE *FinishSessionKeyRefresh )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetEncryptionBltKey )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT KeySize,
            /* [annotation] */ 
            _Out_writes_bytes_(KeySize)  void *pReadbackKey);
        
        HRESULT ( STDMETHODCALLTYPE *NegotiateAuthenticatedChannelKeyExchange )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11AuthenticatedChannel *pChannel,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Inout_updates_bytes_(DataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *QueryAuthenticatedChannel )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11AuthenticatedChannel *pChannel,
            /* [annotation] */ 
            _In_  UINT InputSize,
            /* [annotation] */ 
            _In_reads_bytes_(InputSize)  const void *pInput,
            /* [annotation] */ 
            _In_  UINT OutputSize,
            /* [annotation] */ 
            _Out_writes_bytes_(OutputSize)  void *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureAuthenticatedChannel )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11AuthenticatedChannel *pChannel,
            /* [annotation] */ 
            _In_  UINT InputSize,
            /* [annotation] */ 
            _In_reads_bytes_(InputSize)  const void *pInput,
            /* [annotation] */ 
            _Out_  D3D11_AUTHENTICATED_CONFIGURE_OUTPUT *pOutput);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamRotation )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_ROTATION Rotation);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamRotation )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnable,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SubmitDecoderBuffers1 )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_(NumBuffers)  const D3D11_VIDEO_DECODER_BUFFER_DESC1 *pBufferDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataForNewHardwareKey )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT PrivateInputSize,
            /* [annotation] */ 
            _In_reads_(PrivateInputSize)  const void *pPrivatInputData,
            /* [annotation] */ 
            _Out_  UINT64 *pPrivateOutputData);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCryptoSessionStatus )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _Out_  D3D11_CRYPTO_SESSION_STATUS *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderEnableDownsampling )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc,
            /* [annotation] */ 
            _In_  UINT ReferenceFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderUpdateDownsampling )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputColorSpace1 )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputShaderUsage )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL ShaderUsage);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputColorSpace1 )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputShaderUsage )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pShaderUsage);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamColorSpace1 )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamMirror )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  BOOL FlipHorizontal,
            /* [annotation] */ 
            _In_  BOOL FlipVertical);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamColorSpace1 )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamMirror )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnable,
            /* [annotation] */ 
            _Out_  BOOL *pFlipHorizontal,
            /* [annotation] */ 
            _Out_  BOOL *pFlipVertical);
        
        HRESULT ( STDMETHODCALLTYPE *VideoProcessorGetBehaviorHints )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT OutputWidth,
            /* [annotation] */ 
            _In_  UINT OutputHeight,
            /* [annotation] */ 
            _In_  DXGI_FORMAT OutputFormat,
            /* [annotation] */ 
            _In_  UINT StreamCount,
            /* [annotation] */ 
            _In_reads_(StreamCount)  const D3D11_VIDEO_PROCESSOR_STREAM_BEHAVIOR_HINT *pStreams,
            /* [annotation] */ 
            _Out_  UINT *pBehaviorHints);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputHDRMetaData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _In_reads_bytes_opt_(Size)  const void *pHDRMetaData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputHDRMetaData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  DXGI_HDR_METADATA_TYPE *pType,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(Size)  void *pMetaData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamHDRMetaData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _In_reads_bytes_opt_(Size)  const void *pHDRMetaData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamHDRMetaData )( 
            ID3D11VideoContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  DXGI_HDR_METADATA_TYPE *pType,
            /* [annotation] */ 
            _In_  UINT Size,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(Size)  void *pMetaData);
        
        END_INTERFACE
    } ID3D11VideoContext2Vtbl;

    interface ID3D11VideoContext2
    {
        CONST_VTBL struct ID3D11VideoContext2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11VideoContext2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11VideoContext2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11VideoContext2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11VideoContext2_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11VideoContext2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11VideoContext2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11VideoContext2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11VideoContext2_GetDecoderBuffer(This,pDecoder,Type,pBufferSize,ppBuffer)	\
    ( (This)->lpVtbl -> GetDecoderBuffer(This,pDecoder,Type,pBufferSize,ppBuffer) ) 

#define ID3D11VideoContext2_ReleaseDecoderBuffer(This,pDecoder,Type)	\
    ( (This)->lpVtbl -> ReleaseDecoderBuffer(This,pDecoder,Type) ) 

#define ID3D11VideoContext2_DecoderBeginFrame(This,pDecoder,pView,ContentKeySize,pContentKey)	\
    ( (This)->lpVtbl -> DecoderBeginFrame(This,pDecoder,pView,ContentKeySize,pContentKey) ) 

#define ID3D11VideoContext2_DecoderEndFrame(This,pDecoder)	\
    ( (This)->lpVtbl -> DecoderEndFrame(This,pDecoder) ) 

#define ID3D11VideoContext2_SubmitDecoderBuffers(This,pDecoder,NumBuffers,pBufferDesc)	\
    ( (This)->lpVtbl -> SubmitDecoderBuffers(This,pDecoder,NumBuffers,pBufferDesc) ) 

#define ID3D11VideoContext2_DecoderExtension(This,pDecoder,pExtensionData)	\
    ( (This)->lpVtbl -> DecoderExtension(This,pDecoder,pExtensionData) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputTargetRect(This,pVideoProcessor,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputTargetRect(This,pVideoProcessor,Enable,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputBackgroundColor(This,pVideoProcessor,YCbCr,pColor)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputBackgroundColor(This,pVideoProcessor,YCbCr,pColor) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputColorSpace(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputColorSpace(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputAlphaFillMode(This,pVideoProcessor,AlphaFillMode,StreamIndex)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputAlphaFillMode(This,pVideoProcessor,AlphaFillMode,StreamIndex) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputConstriction(This,pVideoProcessor,Enable,Size)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputConstriction(This,pVideoProcessor,Enable,Size) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputStereoMode(This,pVideoProcessor,Enable)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputStereoMode(This,pVideoProcessor,Enable) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputTargetRect(This,pVideoProcessor,Enabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputTargetRect(This,pVideoProcessor,Enabled,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputBackgroundColor(This,pVideoProcessor,pYCbCr,pColor)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputBackgroundColor(This,pVideoProcessor,pYCbCr,pColor) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputColorSpace(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputColorSpace(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputAlphaFillMode(This,pVideoProcessor,pAlphaFillMode,pStreamIndex)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputAlphaFillMode(This,pVideoProcessor,pAlphaFillMode,pStreamIndex) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputConstriction(This,pVideoProcessor,pEnabled,pSize)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputConstriction(This,pVideoProcessor,pEnabled,pSize) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputStereoMode(This,pVideoProcessor,pEnabled)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputStereoMode(This,pVideoProcessor,pEnabled) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamFrameFormat(This,pVideoProcessor,StreamIndex,FrameFormat)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamFrameFormat(This,pVideoProcessor,StreamIndex,FrameFormat) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamOutputRate(This,pVideoProcessor,StreamIndex,OutputRate,RepeatFrame,pCustomRate)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamOutputRate(This,pVideoProcessor,StreamIndex,OutputRate,RepeatFrame,pCustomRate) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamSourceRect(This,pVideoProcessor,StreamIndex,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamSourceRect(This,pVideoProcessor,StreamIndex,Enable,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamDestRect(This,pVideoProcessor,StreamIndex,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamDestRect(This,pVideoProcessor,StreamIndex,Enable,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamAlpha(This,pVideoProcessor,StreamIndex,Enable,Alpha)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamAlpha(This,pVideoProcessor,StreamIndex,Enable,Alpha) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,Enable,pSourceAspectRatio,pDestinationAspectRatio)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,Enable,pSourceAspectRatio,pDestinationAspectRatio) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamLumaKey(This,pVideoProcessor,StreamIndex,Enable,Lower,Upper)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamLumaKey(This,pVideoProcessor,StreamIndex,Enable,Lower,Upper) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamStereoFormat(This,pVideoProcessor,StreamIndex,Enable,Format,LeftViewFrame0,BaseViewFrame0,FlipMode,MonoOffset)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamStereoFormat(This,pVideoProcessor,StreamIndex,Enable,Format,LeftViewFrame0,BaseViewFrame0,FlipMode,MonoOffset) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,Enable)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,Enable) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,Enable,Level)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,Enable,Level) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamFrameFormat(This,pVideoProcessor,StreamIndex,pFrameFormat)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamFrameFormat(This,pVideoProcessor,StreamIndex,pFrameFormat) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamOutputRate(This,pVideoProcessor,StreamIndex,pOutputRate,pRepeatFrame,pCustomRate)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamOutputRate(This,pVideoProcessor,StreamIndex,pOutputRate,pRepeatFrame,pCustomRate) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamSourceRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamSourceRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamDestRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamDestRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamAlpha(This,pVideoProcessor,StreamIndex,pEnabled,pAlpha)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamAlpha(This,pVideoProcessor,StreamIndex,pEnabled,pAlpha) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,pEnabled,pSourceAspectRatio,pDestinationAspectRatio)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,pEnabled,pSourceAspectRatio,pDestinationAspectRatio) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamLumaKey(This,pVideoProcessor,StreamIndex,pEnabled,pLower,pUpper)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamLumaKey(This,pVideoProcessor,StreamIndex,pEnabled,pLower,pUpper) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamStereoFormat(This,pVideoProcessor,StreamIndex,pEnable,pFormat,pLeftViewFrame0,pBaseViewFrame0,pFlipMode,MonoOffset)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamStereoFormat(This,pVideoProcessor,StreamIndex,pEnable,pFormat,pLeftViewFrame0,pBaseViewFrame0,pFlipMode,MonoOffset) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,pEnabled)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,pEnabled) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,pEnabled,pLevel)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,pEnabled,pLevel) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext2_VideoProcessorBlt(This,pVideoProcessor,pView,OutputFrame,StreamCount,pStreams)	\
    ( (This)->lpVtbl -> VideoProcessorBlt(This,pVideoProcessor,pView,OutputFrame,StreamCount,pStreams) ) 

#define ID3D11VideoContext2_NegotiateCryptoSessionKeyExchange(This,pCryptoSession,DataSize,pData)	\
    ( (This)->lpVtbl -> NegotiateCryptoSessionKeyExchange(This,pCryptoSession,DataSize,pData) ) 

#define ID3D11VideoContext2_EncryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,IVSize,pIV)	\
    ( (This)->lpVtbl -> EncryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,IVSize,pIV) ) 

#define ID3D11VideoContext2_DecryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,pEncryptedBlockInfo,ContentKeySize,pContentKey,IVSize,pIV)	\
    ( (This)->lpVtbl -> DecryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,pEncryptedBlockInfo,ContentKeySize,pContentKey,IVSize,pIV) ) 

#define ID3D11VideoContext2_StartSessionKeyRefresh(This,pCryptoSession,RandomNumberSize,pRandomNumber)	\
    ( (This)->lpVtbl -> StartSessionKeyRefresh(This,pCryptoSession,RandomNumberSize,pRandomNumber) ) 

#define ID3D11VideoContext2_FinishSessionKeyRefresh(This,pCryptoSession)	\
    ( (This)->lpVtbl -> FinishSessionKeyRefresh(This,pCryptoSession) ) 

#define ID3D11VideoContext2_GetEncryptionBltKey(This,pCryptoSession,KeySize,pReadbackKey)	\
    ( (This)->lpVtbl -> GetEncryptionBltKey(This,pCryptoSession,KeySize,pReadbackKey) ) 

#define ID3D11VideoContext2_NegotiateAuthenticatedChannelKeyExchange(This,pChannel,DataSize,pData)	\
    ( (This)->lpVtbl -> NegotiateAuthenticatedChannelKeyExchange(This,pChannel,DataSize,pData) ) 

#define ID3D11VideoContext2_QueryAuthenticatedChannel(This,pChannel,InputSize,pInput,OutputSize,pOutput)	\
    ( (This)->lpVtbl -> QueryAuthenticatedChannel(This,pChannel,InputSize,pInput,OutputSize,pOutput) ) 

#define ID3D11VideoContext2_ConfigureAuthenticatedChannel(This,pChannel,InputSize,pInput,pOutput)	\
    ( (This)->lpVtbl -> ConfigureAuthenticatedChannel(This,pChannel,InputSize,pInput,pOutput) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamRotation(This,pVideoProcessor,StreamIndex,Enable,Rotation)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamRotation(This,pVideoProcessor,StreamIndex,Enable,Rotation) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamRotation(This,pVideoProcessor,StreamIndex,pEnable,pRotation)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamRotation(This,pVideoProcessor,StreamIndex,pEnable,pRotation) ) 


#define ID3D11VideoContext2_SubmitDecoderBuffers1(This,pDecoder,NumBuffers,pBufferDesc)	\
    ( (This)->lpVtbl -> SubmitDecoderBuffers1(This,pDecoder,NumBuffers,pBufferDesc) ) 

#define ID3D11VideoContext2_GetDataForNewHardwareKey(This,pCryptoSession,PrivateInputSize,pPrivatInputData,pPrivateOutputData)	\
    ( (This)->lpVtbl -> GetDataForNewHardwareKey(This,pCryptoSession,PrivateInputSize,pPrivatInputData,pPrivateOutputData) ) 

#define ID3D11VideoContext2_CheckCryptoSessionStatus(This,pCryptoSession,pStatus)	\
    ( (This)->lpVtbl -> CheckCryptoSessionStatus(This,pCryptoSession,pStatus) ) 

#define ID3D11VideoContext2_DecoderEnableDownsampling(This,pDecoder,InputColorSpace,pOutputDesc,ReferenceFrameCount)	\
    ( (This)->lpVtbl -> DecoderEnableDownsampling(This,pDecoder,InputColorSpace,pOutputDesc,ReferenceFrameCount) ) 

#define ID3D11VideoContext2_DecoderUpdateDownsampling(This,pDecoder,pOutputDesc)	\
    ( (This)->lpVtbl -> DecoderUpdateDownsampling(This,pDecoder,pOutputDesc) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputColorSpace1(This,pVideoProcessor,ColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputColorSpace1(This,pVideoProcessor,ColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorSetOutputShaderUsage(This,pVideoProcessor,ShaderUsage)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputShaderUsage(This,pVideoProcessor,ShaderUsage) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputColorSpace1(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputColorSpace1(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputShaderUsage(This,pVideoProcessor,pShaderUsage)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputShaderUsage(This,pVideoProcessor,pShaderUsage) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamColorSpace1(This,pVideoProcessor,StreamIndex,ColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamColorSpace1(This,pVideoProcessor,StreamIndex,ColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamMirror(This,pVideoProcessor,StreamIndex,Enable,FlipHorizontal,FlipVertical)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamMirror(This,pVideoProcessor,StreamIndex,Enable,FlipHorizontal,FlipVertical) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamColorSpace1(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamColorSpace1(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamMirror(This,pVideoProcessor,StreamIndex,pEnable,pFlipHorizontal,pFlipVertical)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamMirror(This,pVideoProcessor,StreamIndex,pEnable,pFlipHorizontal,pFlipVertical) ) 

#define ID3D11VideoContext2_VideoProcessorGetBehaviorHints(This,pVideoProcessor,OutputWidth,OutputHeight,OutputFormat,StreamCount,pStreams,pBehaviorHints)	\
    ( (This)->lpVtbl -> VideoProcessorGetBehaviorHints(This,pVideoProcessor,OutputWidth,OutputHeight,OutputFormat,StreamCount,pStreams,pBehaviorHints) ) 


#define ID3D11VideoContext2_VideoProcessorSetOutputHDRMetaData(This,pVideoProcessor,Type,Size,pHDRMetaData)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputHDRMetaData(This,pVideoProcessor,Type,Size,pHDRMetaData) ) 

#define ID3D11VideoContext2_VideoProcessorGetOutputHDRMetaData(This,pVideoProcessor,pType,Size,pMetaData)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputHDRMetaData(This,pVideoProcessor,pType,Size,pMetaData) ) 

#define ID3D11VideoContext2_VideoProcessorSetStreamHDRMetaData(This,pVideoProcessor,StreamIndex,Type,Size,pHDRMetaData)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamHDRMetaData(This,pVideoProcessor,StreamIndex,Type,Size,pHDRMetaData) ) 

#define ID3D11VideoContext2_VideoProcessorGetStreamHDRMetaData(This,pVideoProcessor,StreamIndex,pType,Size,pMetaData)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamHDRMetaData(This,pVideoProcessor,StreamIndex,pType,Size,pMetaData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11VideoContext2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_4_0000_0004 */
/* [local] */ 

typedef struct D3D11_FEATURE_DATA_D3D11_OPTIONS4
    {
    BOOL ExtendedNV12SharedTextureSupported;
    } 	D3D11_FEATURE_DATA_D3D11_OPTIONS4;

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_ID3D11Device4,0x8992ab71,0x02e6,0x4b8d,0xba,0x48,0xb0,0x56,0xdc,0xda,0x42,0xc4);
DEFINE_GUID(IID_ID3D11Device5,0x8ffde202,0xa0e7,0x45df,0x9e,0x01,0xe8,0x37,0x80,0x1b,0x5e,0xa0);
DEFINE_GUID(IID_ID3D11Multithread,0x9B7E4E00,0x342C,0x4106,0xA1,0x9F,0x4F,0x27,0x04,0xF6,0x89,0xF0);
DEFINE_GUID(IID_ID3D11VideoContext2,0xC4E7374C,0x6243,0x4D1B,0xAE,0x87,0x52,0xB4,0xF7,0x40,0xE2,0x61);


extern RPC_IF_HANDLE __MIDL_itf_d3d11_4_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_4_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


