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

#ifndef __d3d11_3_h__
#define __d3d11_3_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11Texture2D1_FWD_DEFINED__
#define __ID3D11Texture2D1_FWD_DEFINED__
typedef interface ID3D11Texture2D1 ID3D11Texture2D1;

#endif 	/* __ID3D11Texture2D1_FWD_DEFINED__ */


#ifndef __ID3D11Texture3D1_FWD_DEFINED__
#define __ID3D11Texture3D1_FWD_DEFINED__
typedef interface ID3D11Texture3D1 ID3D11Texture3D1;

#endif 	/* __ID3D11Texture3D1_FWD_DEFINED__ */


#ifndef __ID3D11RasterizerState2_FWD_DEFINED__
#define __ID3D11RasterizerState2_FWD_DEFINED__
typedef interface ID3D11RasterizerState2 ID3D11RasterizerState2;

#endif 	/* __ID3D11RasterizerState2_FWD_DEFINED__ */


#ifndef __ID3D11ShaderResourceView1_FWD_DEFINED__
#define __ID3D11ShaderResourceView1_FWD_DEFINED__
typedef interface ID3D11ShaderResourceView1 ID3D11ShaderResourceView1;

#endif 	/* __ID3D11ShaderResourceView1_FWD_DEFINED__ */


#ifndef __ID3D11RenderTargetView1_FWD_DEFINED__
#define __ID3D11RenderTargetView1_FWD_DEFINED__
typedef interface ID3D11RenderTargetView1 ID3D11RenderTargetView1;

#endif 	/* __ID3D11RenderTargetView1_FWD_DEFINED__ */


#ifndef __ID3D11UnorderedAccessView1_FWD_DEFINED__
#define __ID3D11UnorderedAccessView1_FWD_DEFINED__
typedef interface ID3D11UnorderedAccessView1 ID3D11UnorderedAccessView1;

#endif 	/* __ID3D11UnorderedAccessView1_FWD_DEFINED__ */


#ifndef __ID3D11Query1_FWD_DEFINED__
#define __ID3D11Query1_FWD_DEFINED__
typedef interface ID3D11Query1 ID3D11Query1;

#endif 	/* __ID3D11Query1_FWD_DEFINED__ */


#ifndef __ID3D11DeviceContext3_FWD_DEFINED__
#define __ID3D11DeviceContext3_FWD_DEFINED__
typedef interface ID3D11DeviceContext3 ID3D11DeviceContext3;

#endif 	/* __ID3D11DeviceContext3_FWD_DEFINED__ */


#ifndef __ID3D11Fence_FWD_DEFINED__
#define __ID3D11Fence_FWD_DEFINED__
typedef interface ID3D11Fence ID3D11Fence;

#endif 	/* __ID3D11Fence_FWD_DEFINED__ */


#ifndef __ID3D11DeviceContext4_FWD_DEFINED__
#define __ID3D11DeviceContext4_FWD_DEFINED__
typedef interface ID3D11DeviceContext4 ID3D11DeviceContext4;

#endif 	/* __ID3D11DeviceContext4_FWD_DEFINED__ */


#ifndef __ID3D11Device3_FWD_DEFINED__
#define __ID3D11Device3_FWD_DEFINED__
typedef interface ID3D11Device3 ID3D11Device3;

#endif 	/* __ID3D11Device3_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgi1_3.h"
#include "d3dcommon.h"
#include "d3d11_2.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d11_3_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d3d11_2.h" //
#ifdef __cplusplus
extern "C"{
#endif
typedef 
enum D3D11_CONTEXT_TYPE
    {
        D3D11_CONTEXT_TYPE_ALL	= 0,
        D3D11_CONTEXT_TYPE_3D	= 1,
        D3D11_CONTEXT_TYPE_COMPUTE	= 2,
        D3D11_CONTEXT_TYPE_COPY	= 3,
        D3D11_CONTEXT_TYPE_VIDEO	= 4
    } 	D3D11_CONTEXT_TYPE;

typedef 
enum D3D11_TEXTURE_LAYOUT
    {
        D3D11_TEXTURE_LAYOUT_UNDEFINED	= 0,
        D3D11_TEXTURE_LAYOUT_ROW_MAJOR	= 1,
        D3D11_TEXTURE_LAYOUT_64K_STANDARD_SWIZZLE	= 2
    } 	D3D11_TEXTURE_LAYOUT;

typedef struct D3D11_TEXTURE2D_DESC1
    {
    UINT Width;
    UINT Height;
    UINT MipLevels;
    UINT ArraySize;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    D3D11_TEXTURE_LAYOUT TextureLayout;
    } 	D3D11_TEXTURE2D_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_TEXTURE2D_DESC1 : public D3D11_TEXTURE2D_DESC1
{
    CD3D11_TEXTURE2D_DESC1()
    {}
    explicit CD3D11_TEXTURE2D_DESC1( const D3D11_TEXTURE2D_DESC1& o ) :
        D3D11_TEXTURE2D_DESC1( o )
    {}
    explicit CD3D11_TEXTURE2D_DESC1(
        DXGI_FORMAT format,
        UINT width,
        UINT height,
        UINT arraySize = 1,
        UINT mipLevels = 0,
        UINT bindFlags = D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
        UINT cpuaccessFlags = 0,
        UINT sampleCount = 1,
        UINT sampleQuality = 0,
        UINT miscFlags = 0,
        D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
    {
        Width = width;
        Height = height;
        MipLevels = mipLevels;
        ArraySize = arraySize;
        Format = format;
        SampleDesc.Count = sampleCount;
        SampleDesc.Quality = sampleQuality;
        Usage = usage;
        BindFlags = bindFlags;
        CPUAccessFlags = cpuaccessFlags;
        MiscFlags = miscFlags;
        TextureLayout = textureLayout;
    }
    explicit CD3D11_TEXTURE2D_DESC1(
        const D3D11_TEXTURE2D_DESC &desc,
        D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
    {
        Width = desc.Width;
        Height = desc.Height;
        MipLevels = desc.MipLevels;
        ArraySize = desc.ArraySize;
        Format = desc.Format;
        SampleDesc.Count = desc.SampleDesc.Count;
        SampleDesc.Quality = desc. SampleDesc.Quality;
        Usage = desc.Usage;
        BindFlags = desc.BindFlags;
        CPUAccessFlags = desc.CPUAccessFlags;
        MiscFlags = desc.MiscFlags;
        TextureLayout = textureLayout;
    }
    ~CD3D11_TEXTURE2D_DESC1() {}
    operator const D3D11_TEXTURE2D_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11Texture2D1_INTERFACE_DEFINED__
#define __ID3D11Texture2D1_INTERFACE_DEFINED__

/* interface ID3D11Texture2D1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Texture2D1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51218251-1E33-4617-9CCB-4D3A4367E7BB")
    ID3D11Texture2D1 : public ID3D11Texture2D
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE2D_DESC1 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Texture2D1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Texture2D1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Texture2D1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Texture2D1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetType )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RESOURCE_DIMENSION *pResourceDimension);
        
        void ( STDMETHODCALLTYPE *SetEvictionPriority )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _In_  UINT EvictionPriority);
        
        UINT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
            ID3D11Texture2D1 * This);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE2D_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11Texture2D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE2D_DESC1 *pDesc);
        
        END_INTERFACE
    } ID3D11Texture2D1Vtbl;

    interface ID3D11Texture2D1
    {
        CONST_VTBL struct ID3D11Texture2D1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Texture2D1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Texture2D1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Texture2D1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Texture2D1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Texture2D1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Texture2D1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Texture2D1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Texture2D1_GetType(This,pResourceDimension)	\
    ( (This)->lpVtbl -> GetType(This,pResourceDimension) ) 

#define ID3D11Texture2D1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define ID3D11Texture2D1_GetEvictionPriority(This)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This) ) 


#define ID3D11Texture2D1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Texture2D1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Texture2D1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0001 */
/* [local] */ 

typedef struct D3D11_TEXTURE3D_DESC1
    {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    DXGI_FORMAT Format;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    D3D11_TEXTURE_LAYOUT TextureLayout;
    } 	D3D11_TEXTURE3D_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_TEXTURE3D_DESC1 : public D3D11_TEXTURE3D_DESC1
{
    CD3D11_TEXTURE3D_DESC1()
    {}
    explicit CD3D11_TEXTURE3D_DESC1( const D3D11_TEXTURE3D_DESC1& o ) :
        D3D11_TEXTURE3D_DESC1( o )
    {}
    explicit CD3D11_TEXTURE3D_DESC1(
        DXGI_FORMAT format,
        UINT width,
        UINT height,
        UINT depth,
        UINT mipLevels = 0,
        UINT bindFlags = D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
        UINT cpuaccessFlags = 0,
        UINT miscFlags = 0,
        D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
    {
        Width = width;
        Height = height;
        Depth = depth;
        MipLevels = mipLevels;
        Format = format;
        Usage = usage;
        BindFlags = bindFlags;
        CPUAccessFlags = cpuaccessFlags;
        MiscFlags = miscFlags;
        TextureLayout = textureLayout;
    }
    explicit CD3D11_TEXTURE3D_DESC1(
        const D3D11_TEXTURE3D_DESC &desc,
        D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
    {
        Width = desc.Width;
        Height = desc.Height;
        Depth = desc.Depth;
        MipLevels = desc.MipLevels;
        Format = desc.Format;
        Usage = desc.Usage;
        BindFlags = desc.BindFlags;
        CPUAccessFlags = desc.CPUAccessFlags;
        MiscFlags = desc.MiscFlags;
        TextureLayout = textureLayout;
    }
    ~CD3D11_TEXTURE3D_DESC1() {}
    operator const D3D11_TEXTURE3D_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0001_v0_0_s_ifspec;

#ifndef __ID3D11Texture3D1_INTERFACE_DEFINED__
#define __ID3D11Texture3D1_INTERFACE_DEFINED__

/* interface ID3D11Texture3D1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Texture3D1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C711683-2853-4846-9BB0-F3E60639E46A")
    ID3D11Texture3D1 : public ID3D11Texture3D
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE3D_DESC1 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Texture3D1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Texture3D1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Texture3D1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Texture3D1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetType )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RESOURCE_DIMENSION *pResourceDimension);
        
        void ( STDMETHODCALLTYPE *SetEvictionPriority )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _In_  UINT EvictionPriority);
        
        UINT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
            ID3D11Texture3D1 * This);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE3D_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11Texture3D1 * This,
            /* [annotation] */ 
            _Out_  D3D11_TEXTURE3D_DESC1 *pDesc);
        
        END_INTERFACE
    } ID3D11Texture3D1Vtbl;

    interface ID3D11Texture3D1
    {
        CONST_VTBL struct ID3D11Texture3D1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Texture3D1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Texture3D1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Texture3D1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Texture3D1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Texture3D1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Texture3D1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Texture3D1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Texture3D1_GetType(This,pResourceDimension)	\
    ( (This)->lpVtbl -> GetType(This,pResourceDimension) ) 

#define ID3D11Texture3D1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define ID3D11Texture3D1_GetEvictionPriority(This)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This) ) 


#define ID3D11Texture3D1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Texture3D1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Texture3D1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0002 */
/* [local] */ 

typedef 
enum D3D11_CONSERVATIVE_RASTERIZATION_MODE
    {
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
    } 	D3D11_CONSERVATIVE_RASTERIZATION_MODE;

typedef struct D3D11_RASTERIZER_DESC2
    {
    D3D11_FILL_MODE FillMode;
    D3D11_CULL_MODE CullMode;
    BOOL FrontCounterClockwise;
    INT DepthBias;
    FLOAT DepthBiasClamp;
    FLOAT SlopeScaledDepthBias;
    BOOL DepthClipEnable;
    BOOL ScissorEnable;
    BOOL MultisampleEnable;
    BOOL AntialiasedLineEnable;
    UINT ForcedSampleCount;
    D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    } 	D3D11_RASTERIZER_DESC2;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_RASTERIZER_DESC2 : public D3D11_RASTERIZER_DESC2
{
    CD3D11_RASTERIZER_DESC2()
    {}
    explicit CD3D11_RASTERIZER_DESC2( const D3D11_RASTERIZER_DESC2& o ) :
        D3D11_RASTERIZER_DESC2( o )
    {}
    explicit CD3D11_RASTERIZER_DESC2( CD3D11_DEFAULT )
    {
        FillMode = D3D11_FILL_SOLID;
        CullMode = D3D11_CULL_BACK;
        FrontCounterClockwise = FALSE;
        DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        DepthClipEnable = TRUE;
        ScissorEnable = FALSE;
        MultisampleEnable = FALSE;
        AntialiasedLineEnable = FALSE;
        ForcedSampleCount = 0;
        ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
    explicit CD3D11_RASTERIZER_DESC2(
        D3D11_FILL_MODE fillMode,
        D3D11_CULL_MODE cullMode,
        BOOL frontCounterClockwise,
        INT depthBias,
        FLOAT depthBiasClamp,
        FLOAT slopeScaledDepthBias,
        BOOL depthClipEnable,
        BOOL scissorEnable,
        BOOL multisampleEnable,
        BOOL antialiasedLineEnable, 
        UINT forcedSampleCount, 
        D3D11_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster )
    {
        FillMode = fillMode;
        CullMode = cullMode;
        FrontCounterClockwise = frontCounterClockwise;
        DepthBias = depthBias;
        DepthBiasClamp = depthBiasClamp;
        SlopeScaledDepthBias = slopeScaledDepthBias;
        DepthClipEnable = depthClipEnable;
        ScissorEnable = scissorEnable;
        MultisampleEnable = multisampleEnable;
        AntialiasedLineEnable = antialiasedLineEnable;
        ForcedSampleCount = forcedSampleCount;
        ConservativeRaster = conservativeRaster;
    }
    ~CD3D11_RASTERIZER_DESC2() {}
    operator const D3D11_RASTERIZER_DESC2&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0002_v0_0_s_ifspec;

#ifndef __ID3D11RasterizerState2_INTERFACE_DEFINED__
#define __ID3D11RasterizerState2_INTERFACE_DEFINED__

/* interface ID3D11RasterizerState2 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11RasterizerState2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6fbd02fb-209f-46c4-b059-2ed15586a6ac")
    ID3D11RasterizerState2 : public ID3D11RasterizerState1
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc2( 
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC2 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11RasterizerState2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11RasterizerState2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11RasterizerState2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11RasterizerState2 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC1 *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc2 )( 
            ID3D11RasterizerState2 * This,
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC2 *pDesc);
        
        END_INTERFACE
    } ID3D11RasterizerState2Vtbl;

    interface ID3D11RasterizerState2
    {
        CONST_VTBL struct ID3D11RasterizerState2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11RasterizerState2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11RasterizerState2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11RasterizerState2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11RasterizerState2_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11RasterizerState2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11RasterizerState2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11RasterizerState2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11RasterizerState2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11RasterizerState2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define ID3D11RasterizerState2_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11RasterizerState2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0003 */
/* [local] */ 

typedef struct D3D11_TEX2D_SRV1
    {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_SRV1;

typedef struct D3D11_TEX2D_ARRAY_SRV1
    {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_ARRAY_SRV1;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC1
    {
    DXGI_FORMAT Format;
    D3D11_SRV_DIMENSION ViewDimension;
    union 
        {
        D3D11_BUFFER_SRV Buffer;
        D3D11_TEX1D_SRV Texture1D;
        D3D11_TEX1D_ARRAY_SRV Texture1DArray;
        D3D11_TEX2D_SRV1 Texture2D;
        D3D11_TEX2D_ARRAY_SRV1 Texture2DArray;
        D3D11_TEX2DMS_SRV Texture2DMS;
        D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
        D3D11_TEX3D_SRV Texture3D;
        D3D11_TEXCUBE_SRV TextureCube;
        D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
        D3D11_BUFFEREX_SRV BufferEx;
        } 	;
    } 	D3D11_SHADER_RESOURCE_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_SHADER_RESOURCE_VIEW_DESC1 : public D3D11_SHADER_RESOURCE_VIEW_DESC1
{
    CD3D11_SHADER_RESOURCE_VIEW_DESC1()
    {}
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1( const D3D11_SHADER_RESOURCE_VIEW_DESC1& o ) :
        D3D11_SHADER_RESOURCE_VIEW_DESC1( o )
    {}
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
        D3D11_SRV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mostDetailedMip = 0, // FirstElement for BUFFER
        UINT mipLevels = -1, // NumElements for BUFFER
        UINT firstArraySlice = 0, // First2DArrayFace for TEXTURECUBEARRAY
        UINT arraySize = -1, // NumCubes for TEXTURECUBEARRAY
        UINT flags = 0, // BUFFEREX only
        UINT planeSlice = 0 ) // Texture2D and Texture2DArray only
    {
        Format = format;
        ViewDimension = viewDimension;
        switch (viewDimension)
        {
        case D3D11_SRV_DIMENSION_BUFFER:
            Buffer.FirstElement = mostDetailedMip;
            Buffer.NumElements = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE1D:
            Texture1D.MostDetailedMip = mostDetailedMip;
            Texture1D.MipLevels = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MostDetailedMip = mostDetailedMip;
            Texture1DArray.MipLevels = mipLevels;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2D:
            Texture2D.MostDetailedMip = mostDetailedMip;
            Texture2D.MipLevels = mipLevels;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MostDetailedMip = mostDetailedMip;
            Texture2DArray.MipLevels = mipLevels;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DMS:
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
            Texture2DMSArray.FirstArraySlice = firstArraySlice;
            Texture2DMSArray.ArraySize = arraySize;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE3D:
            Texture3D.MostDetailedMip = mostDetailedMip;
            Texture3D.MipLevels = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBE:
            TextureCube.MostDetailedMip = mostDetailedMip;
            TextureCube.MipLevels = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
            TextureCubeArray.MostDetailedMip = mostDetailedMip;
            TextureCubeArray.MipLevels = mipLevels;
            TextureCubeArray.First2DArrayFace = firstArraySlice;
            TextureCubeArray.NumCubes = arraySize;
            break;
        case D3D11_SRV_DIMENSION_BUFFEREX:
            BufferEx.FirstElement = mostDetailedMip;
            BufferEx.NumElements = mipLevels;
            BufferEx.Flags = flags;
            break;
        default: break;
        }
    }
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
        _In_ ID3D11Buffer*,
        DXGI_FORMAT format,
        UINT firstElement,
        UINT numElements,
        UINT flags = 0 )
    {
        Format = format;
        ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        BufferEx.FirstElement = firstElement;
        BufferEx.NumElements = numElements;
        BufferEx.Flags = flags;
    }
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
        _In_ ID3D11Texture1D* pTex1D,
        D3D11_SRV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mostDetailedMip = 0,
        UINT mipLevels = -1,
        UINT firstArraySlice = 0,
        UINT arraySize = -1 )
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format || -1 == mipLevels ||
            (-1 == arraySize && D3D11_SRV_DIMENSION_TEXTURE1DARRAY == viewDimension))
        {
            D3D11_TEXTURE1D_DESC TexDesc;
            pTex1D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
            if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_SRV_DIMENSION_TEXTURE1D:
            Texture1D.MostDetailedMip = mostDetailedMip;
            Texture1D.MipLevels = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MostDetailedMip = mostDetailedMip;
            Texture1DArray.MipLevels = mipLevels;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
        _In_ ID3D11Texture2D* pTex2D,
        D3D11_SRV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mostDetailedMip = 0,
        UINT mipLevels = -1,
        UINT firstArraySlice = 0, // First2DArrayFace for TEXTURECUBEARRAY
        UINT arraySize = -1,  // NumCubes for TEXTURECUBEARRAY
        UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D or TEXTURE2DARRAY
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format || 
            (-1 == mipLevels &&
                D3D11_SRV_DIMENSION_TEXTURE2DMS != viewDimension &&
                D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY != viewDimension) ||
            (-1 == arraySize &&
                (D3D11_SRV_DIMENSION_TEXTURE2DARRAY == viewDimension ||
                D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY == viewDimension ||
                D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension)))
        {
            D3D11_TEXTURE2D_DESC TexDesc;
            pTex2D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
            if (-1 == arraySize)
            {
                arraySize = TexDesc.ArraySize - firstArraySlice;
                if (D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension) arraySize /= 6;
            }
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_SRV_DIMENSION_TEXTURE2D:
            Texture2D.MostDetailedMip = mostDetailedMip;
            Texture2D.MipLevels = mipLevels;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MostDetailedMip = mostDetailedMip;
            Texture2DArray.MipLevels = mipLevels;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DMS:
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
            Texture2DMSArray.FirstArraySlice = firstArraySlice;
            Texture2DMSArray.ArraySize = arraySize;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBE:
            TextureCube.MostDetailedMip = mostDetailedMip;
            TextureCube.MipLevels = mipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
            TextureCubeArray.MostDetailedMip = mostDetailedMip;
            TextureCubeArray.MipLevels = mipLevels;
            TextureCubeArray.First2DArrayFace = firstArraySlice;
            TextureCubeArray.NumCubes = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
        _In_ ID3D11Texture3D* pTex3D,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mostDetailedMip = 0,
        UINT mipLevels = -1 )
    {
        ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        if (DXGI_FORMAT_UNKNOWN == format || -1 == mipLevels)
        {
            D3D11_TEXTURE3D_DESC TexDesc;
            pTex3D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
        }
        Format = format;
        Texture3D.MostDetailedMip = mostDetailedMip;
        Texture3D.MipLevels = mipLevels;
    }
    ~CD3D11_SHADER_RESOURCE_VIEW_DESC1() {}
    operator const D3D11_SHADER_RESOURCE_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0003_v0_0_s_ifspec;

#ifndef __ID3D11ShaderResourceView1_INTERFACE_DEFINED__
#define __ID3D11ShaderResourceView1_INTERFACE_DEFINED__

/* interface ID3D11ShaderResourceView1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11ShaderResourceView1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("91308b87-9040-411d-8c67-c39253ce3802")
    ID3D11ShaderResourceView1 : public ID3D11ShaderResourceView
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11ShaderResourceView1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11ShaderResourceView1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11ShaderResourceView1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11ShaderResourceView1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetResource )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Resource **ppResource);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11ShaderResourceView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1);
        
        END_INTERFACE
    } ID3D11ShaderResourceView1Vtbl;

    interface ID3D11ShaderResourceView1
    {
        CONST_VTBL struct ID3D11ShaderResourceView1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11ShaderResourceView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11ShaderResourceView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11ShaderResourceView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11ShaderResourceView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11ShaderResourceView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11ShaderResourceView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11ShaderResourceView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11ShaderResourceView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11ShaderResourceView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11ShaderResourceView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11ShaderResourceView1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0004 */
/* [local] */ 

typedef struct D3D11_TEX2D_RTV1
    {
    UINT MipSlice;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_RTV1;

typedef struct D3D11_TEX2D_ARRAY_RTV1
    {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_ARRAY_RTV1;

typedef struct D3D11_RENDER_TARGET_VIEW_DESC1
    {
    DXGI_FORMAT Format;
    D3D11_RTV_DIMENSION ViewDimension;
    union 
        {
        D3D11_BUFFER_RTV Buffer;
        D3D11_TEX1D_RTV Texture1D;
        D3D11_TEX1D_ARRAY_RTV Texture1DArray;
        D3D11_TEX2D_RTV1 Texture2D;
        D3D11_TEX2D_ARRAY_RTV1 Texture2DArray;
        D3D11_TEX2DMS_RTV Texture2DMS;
        D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
        D3D11_TEX3D_RTV Texture3D;
        } 	;
    } 	D3D11_RENDER_TARGET_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_RENDER_TARGET_VIEW_DESC1 : public D3D11_RENDER_TARGET_VIEW_DESC1
{
    CD3D11_RENDER_TARGET_VIEW_DESC1()
    {}
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1( const D3D11_RENDER_TARGET_VIEW_DESC1& o ) :
        D3D11_RENDER_TARGET_VIEW_DESC1( o )
    {}
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
        D3D11_RTV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0, // FirstElement for BUFFER
        UINT firstArraySlice = 0, // NumElements for BUFFER, FirstWSlice for TEXTURE3D
        UINT arraySize = -1, // WSize for TEXTURE3D
        UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D and TEXTURE2DARRAY
    {
        Format = format;
        ViewDimension = viewDimension;
        switch (viewDimension)
        {
        case D3D11_RTV_DIMENSION_BUFFER:
            Buffer.FirstElement = mipSlice;
            Buffer.NumElements = firstArraySlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE1D:
            Texture1D.MipSlice = mipSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MipSlice = mipSlice;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2D:
            Texture2D.MipSlice = mipSlice;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MipSlice = mipSlice;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DMS:
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
            Texture2DMSArray.FirstArraySlice = firstArraySlice;
            Texture2DMSArray.ArraySize = arraySize;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE3D:
            Texture3D.MipSlice = mipSlice;
            Texture3D.FirstWSlice = firstArraySlice;
            Texture3D.WSize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
        _In_ ID3D11Buffer*,
        DXGI_FORMAT format,
        UINT firstElement,
        UINT numElements )
    {
        Format = format;
        ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
        Buffer.FirstElement = firstElement;
        Buffer.NumElements = numElements;
    }
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
        _In_ ID3D11Texture1D* pTex1D,
        D3D11_RTV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstArraySlice = 0,
        UINT arraySize = -1 )
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format ||
            (-1 == arraySize && D3D11_RTV_DIMENSION_TEXTURE1DARRAY == viewDimension))
        {
            D3D11_TEXTURE1D_DESC TexDesc;
            pTex1D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_RTV_DIMENSION_TEXTURE1D:
            Texture1D.MipSlice = mipSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MipSlice = mipSlice;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
        _In_ ID3D11Texture2D* pTex2D,
        D3D11_RTV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstArraySlice = 0,
        UINT arraySize = -1,
        UINT planeSlice = 0 )
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format || 
            (-1 == arraySize &&
                (D3D11_RTV_DIMENSION_TEXTURE2DARRAY == viewDimension ||
                D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY == viewDimension)))
        {
            D3D11_TEXTURE2D_DESC TexDesc;
            pTex2D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_RTV_DIMENSION_TEXTURE2D:
            Texture2D.MipSlice = mipSlice;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MipSlice = mipSlice;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DMS:
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
            Texture2DMSArray.FirstArraySlice = firstArraySlice;
            Texture2DMSArray.ArraySize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
        _In_ ID3D11Texture3D* pTex3D,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstWSlice = 0,
        UINT wSize = -1 )
    {
        ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        if (DXGI_FORMAT_UNKNOWN == format || -1 == wSize)
        {
            D3D11_TEXTURE3D_DESC TexDesc;
            pTex3D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == wSize) wSize = TexDesc.Depth - firstWSlice;
        }
        Format = format;
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstWSlice;
        Texture3D.WSize = wSize;
    }
    ~CD3D11_RENDER_TARGET_VIEW_DESC1() {}
    operator const D3D11_RENDER_TARGET_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0004_v0_0_s_ifspec;

#ifndef __ID3D11RenderTargetView1_INTERFACE_DEFINED__
#define __ID3D11RenderTargetView1_INTERFACE_DEFINED__

/* interface ID3D11RenderTargetView1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11RenderTargetView1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ffbe2e23-f011-418a-ac56-5ceed7c5b94b")
    ID3D11RenderTargetView1 : public ID3D11RenderTargetView
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11RenderTargetView1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11RenderTargetView1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11RenderTargetView1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11RenderTargetView1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetResource )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Resource **ppResource);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RENDER_TARGET_VIEW_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11RenderTargetView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1);
        
        END_INTERFACE
    } ID3D11RenderTargetView1Vtbl;

    interface ID3D11RenderTargetView1
    {
        CONST_VTBL struct ID3D11RenderTargetView1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11RenderTargetView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11RenderTargetView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11RenderTargetView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11RenderTargetView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11RenderTargetView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11RenderTargetView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11RenderTargetView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11RenderTargetView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11RenderTargetView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11RenderTargetView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11RenderTargetView1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0005 */
/* [local] */ 

typedef struct D3D11_TEX2D_UAV1
    {
    UINT MipSlice;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_UAV1;

typedef struct D3D11_TEX2D_ARRAY_UAV1
    {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
    } 	D3D11_TEX2D_ARRAY_UAV1;

typedef struct D3D11_UNORDERED_ACCESS_VIEW_DESC1
    {
    DXGI_FORMAT Format;
    D3D11_UAV_DIMENSION ViewDimension;
    union 
        {
        D3D11_BUFFER_UAV Buffer;
        D3D11_TEX1D_UAV Texture1D;
        D3D11_TEX1D_ARRAY_UAV Texture1DArray;
        D3D11_TEX2D_UAV1 Texture2D;
        D3D11_TEX2D_ARRAY_UAV1 Texture2DArray;
        D3D11_TEX3D_UAV Texture3D;
        } 	;
    } 	D3D11_UNORDERED_ACCESS_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_UNORDERED_ACCESS_VIEW_DESC1 : public D3D11_UNORDERED_ACCESS_VIEW_DESC1
{
    CD3D11_UNORDERED_ACCESS_VIEW_DESC1()
    {}
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1( const D3D11_UNORDERED_ACCESS_VIEW_DESC1& o ) :
        D3D11_UNORDERED_ACCESS_VIEW_DESC1( o )
    {}
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
        D3D11_UAV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0, // FirstElement for BUFFER
        UINT firstArraySlice = 0, // NumElements for BUFFER, FirstWSlice for TEXTURE3D
        UINT arraySize = -1, // WSize for TEXTURE3D
        UINT flags = 0, // BUFFER only
        UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D and TEXTURE2DARRAY
    {
        Format = format;
        ViewDimension = viewDimension;
        switch (viewDimension)
        {
        case D3D11_UAV_DIMENSION_BUFFER:
            Buffer.FirstElement = mipSlice;
            Buffer.NumElements = firstArraySlice;
            Buffer.Flags = flags;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE1D:
            Texture1D.MipSlice = mipSlice;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MipSlice = mipSlice;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE2D:
            Texture2D.MipSlice = mipSlice;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MipSlice = mipSlice;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE3D:
            Texture3D.MipSlice = mipSlice;
            Texture3D.FirstWSlice = firstArraySlice;
            Texture3D.WSize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
        _In_ ID3D11Buffer*,
        DXGI_FORMAT format,
        UINT firstElement,
        UINT numElements,
        UINT flags = 0 )
    {
        Format = format;
        ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        Buffer.FirstElement = firstElement;
        Buffer.NumElements = numElements;
        Buffer.Flags = flags;
    }
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
        _In_ ID3D11Texture1D* pTex1D,
        D3D11_UAV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstArraySlice = 0,
        UINT arraySize = -1 )
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format ||
            (-1 == arraySize && D3D11_UAV_DIMENSION_TEXTURE1DARRAY == viewDimension))
        {
            D3D11_TEXTURE1D_DESC TexDesc;
            pTex1D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_UAV_DIMENSION_TEXTURE1D:
            Texture1D.MipSlice = mipSlice;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
            Texture1DArray.MipSlice = mipSlice;
            Texture1DArray.FirstArraySlice = firstArraySlice;
            Texture1DArray.ArraySize = arraySize;
            break;
        default: break;
        }
    }
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
        _In_ ID3D11Texture2D* pTex2D,
        D3D11_UAV_DIMENSION viewDimension,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstArraySlice = 0,
        UINT arraySize = -1, 
        UINT planeSlice = 0 ) 
    {
        ViewDimension = viewDimension;
        if (DXGI_FORMAT_UNKNOWN == format || 
            (-1 == arraySize && D3D11_UAV_DIMENSION_TEXTURE2DARRAY == viewDimension))
        {
            D3D11_TEXTURE2D_DESC TexDesc;
            pTex2D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
        }
        Format = format;
        switch (viewDimension)
        {
        case D3D11_UAV_DIMENSION_TEXTURE2D:
            Texture2D.MipSlice = mipSlice;
            Texture2D.PlaneSlice = planeSlice;
            break;
        case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
            Texture2DArray.MipSlice = mipSlice;
            Texture2DArray.FirstArraySlice = firstArraySlice;
            Texture2DArray.ArraySize = arraySize;
            Texture2DArray.PlaneSlice = planeSlice;
            break;
        default: break;
        }
    }
    explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
        _In_ ID3D11Texture3D* pTex3D,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
        UINT mipSlice = 0,
        UINT firstWSlice = 0,
        UINT wSize = -1 )
    {
        ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        if (DXGI_FORMAT_UNKNOWN == format || -1 == wSize)
        {
            D3D11_TEXTURE3D_DESC TexDesc;
            pTex3D->GetDesc( &TexDesc );
            if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
            if (-1 == wSize) wSize = TexDesc.Depth - firstWSlice;
        }
        Format = format;
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstWSlice;
        Texture3D.WSize = wSize;
    }
    ~CD3D11_UNORDERED_ACCESS_VIEW_DESC1() {}
    operator const D3D11_UNORDERED_ACCESS_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0005_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0005_v0_0_s_ifspec;

#ifndef __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__
#define __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__

/* interface ID3D11UnorderedAccessView1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11UnorderedAccessView1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7b3b6153-a886-4544-ab37-6537c8500403")
    ID3D11UnorderedAccessView1 : public ID3D11UnorderedAccessView
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11UnorderedAccessView1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11UnorderedAccessView1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11UnorderedAccessView1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11UnorderedAccessView1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetResource )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Resource **ppResource);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11UnorderedAccessView1 * This,
            /* [annotation] */ 
            _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1);
        
        END_INTERFACE
    } ID3D11UnorderedAccessView1Vtbl;

    interface ID3D11UnorderedAccessView1
    {
        CONST_VTBL struct ID3D11UnorderedAccessView1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11UnorderedAccessView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11UnorderedAccessView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11UnorderedAccessView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11UnorderedAccessView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11UnorderedAccessView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11UnorderedAccessView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11UnorderedAccessView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11UnorderedAccessView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11UnorderedAccessView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11UnorderedAccessView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0006 */
/* [local] */ 

typedef struct D3D11_QUERY_DESC1
    {
    D3D11_QUERY Query;
    UINT MiscFlags;
    D3D11_CONTEXT_TYPE ContextType;
    } 	D3D11_QUERY_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_QUERY_DESC1 : public D3D11_QUERY_DESC1
{
    CD3D11_QUERY_DESC1()
    {}
    explicit CD3D11_QUERY_DESC1( const D3D11_QUERY_DESC1& o ) :
        D3D11_QUERY_DESC1( o )
    {}
    explicit CD3D11_QUERY_DESC1(
        D3D11_QUERY query,
        UINT miscFlags = 0,
        D3D11_CONTEXT_TYPE contextType = D3D11_CONTEXT_TYPE_ALL )
    {
        Query = query;
        MiscFlags = miscFlags;
        ContextType = contextType;
    }
    ~CD3D11_QUERY_DESC1() {}
    operator const D3D11_QUERY_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0006_v0_0_s_ifspec;

#ifndef __ID3D11Query1_INTERFACE_DEFINED__
#define __ID3D11Query1_INTERFACE_DEFINED__

/* interface ID3D11Query1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Query1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("631b4766-36dc-461d-8db6-c47e13e60916")
    ID3D11Query1 : public ID3D11Query
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_QUERY_DESC1 *pDesc1) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Query1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Query1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Query1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Query1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        UINT ( STDMETHODCALLTYPE *GetDataSize )( 
            ID3D11Query1 * This);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _Out_  D3D11_QUERY_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11Query1 * This,
            /* [annotation] */ 
            _Out_  D3D11_QUERY_DESC1 *pDesc1);
        
        END_INTERFACE
    } ID3D11Query1Vtbl;

    interface ID3D11Query1
    {
        CONST_VTBL struct ID3D11Query1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Query1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Query1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Query1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Query1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Query1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Query1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Query1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Query1_GetDataSize(This)	\
    ( (This)->lpVtbl -> GetDataSize(This) ) 


#define ID3D11Query1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Query1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Query1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0007 */
/* [local] */ 

typedef 
enum D3D11_FENCE_FLAG
    {
        D3D11_FENCE_FLAG_NONE	= 0x1,
        D3D11_FENCE_FLAG_SHARED	= 0x2,
        D3D11_FENCE_FLAG_SHARED_CROSS_ADAPTER	= 0x4
    } 	D3D11_FENCE_FLAG;

DEFINE_ENUM_FLAG_OPERATORS(D3D11_FENCE_FLAG);


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0007_v0_0_s_ifspec;

#ifndef __ID3D11DeviceContext3_INTERFACE_DEFINED__
#define __ID3D11DeviceContext3_INTERFACE_DEFINED__

/* interface ID3D11DeviceContext3 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11DeviceContext3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b4e3c01d-e79e-4637-91b2-510e9f4c9b8f")
    ID3D11DeviceContext3 : public ID3D11DeviceContext2
    {
    public:
        virtual void STDMETHODCALLTYPE Flush1( 
            D3D11_CONTEXT_TYPE ContextType,
            /* [annotation] */ 
            _In_opt_  HANDLE hEvent) = 0;
        
        virtual void STDMETHODCALLTYPE SetHardwareProtectionState( 
            /* [annotation] */ 
            _In_  BOOL HwProtectionEnable) = 0;
        
        virtual void STDMETHODCALLTYPE GetHardwareProtectionState( 
            /* [annotation] */ 
            _Out_  BOOL *pHwProtectionEnable) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11DeviceContext3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11DeviceContext3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11DeviceContext3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11DeviceContext3 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11PixelShader *pPixelShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11VertexShader *pVertexShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DrawIndexed )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT IndexCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation);
        
        void ( STDMETHODCALLTYPE *Draw )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT VertexCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation);
        
        HRESULT ( STDMETHODCALLTYPE *Map )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource,
            /* [annotation] */ 
            _In_  D3D11_MAP MapType,
            /* [annotation] */ 
            _In_  UINT MapFlags,
            /* [annotation] */ 
            _Out_opt_  D3D11_MAPPED_SUBRESOURCE *pMappedResource);
        
        void ( STDMETHODCALLTYPE *Unmap )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IASetInputLayout )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11InputLayout *pInputLayout);
        
        void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pStrides,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT IndexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *DrawInstanced )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT VertexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);
        
        void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *Begin )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        void ( STDMETHODCALLTYPE *End )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags);
        
        void ( STDMETHODCALLTYPE *SetPredication )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue);
        
        void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT NumRTVs,
            /* [annotation] */ 
            _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT UAVStartSlot,
            /* [annotation] */ 
            _In_  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *OMSetBlendState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11BlendState *pBlendState,
            /* [annotation] */ 
            _In_opt_  const FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _In_  UINT SampleMask);
        
        void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
            /* [annotation] */ 
            _In_  UINT StencilRef);
        
        void ( STDMETHODCALLTYPE *SOSetTargets )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *DrawAuto )( 
            ID3D11DeviceContext3 * This);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *Dispatch )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountX,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountY,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountZ);
        
        void ( STDMETHODCALLTYPE *DispatchIndirect )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *RSSetState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11RasterizerState *pRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSSetViewports )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            /* [annotation] */ 
            _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  UINT DstX,
            /* [annotation] */ 
            _In_  UINT DstY,
            /* [annotation] */ 
            _In_  UINT DstZ,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox);
        
        void ( STDMETHODCALLTYPE *CopyResource )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource )( 
            ID3D11DeviceContext3 * This,
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
        
        void ( STDMETHODCALLTYPE *CopyStructureCount )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pDstBuffer,
            /* [annotation] */ 
            _In_  UINT DstAlignedByteOffset,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pSrcView);
        
        void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11RenderTargetView *pRenderTargetView,
            /* [annotation] */ 
            _In_  const FLOAT ColorRGBA[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const UINT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const FLOAT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_  UINT ClearFlags,
            /* [annotation] */ 
            _In_  FLOAT Depth,
            /* [annotation] */ 
            _In_  UINT8 Stencil);
        
        void ( STDMETHODCALLTYPE *GenerateMips )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11ShaderResourceView *pShaderResourceView);
        
        void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            FLOAT MinLOD);
        
        FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *ResolveSubresource )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format);
        
        void ( STDMETHODCALLTYPE *ExecuteCommandList )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11CommandList *pCommandList,
            BOOL RestoreContextState);
        
        void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11HullShader *pHullShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DomainShader *pDomainShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *CSSetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11ComputeShader *pComputeShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSSetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);
        
        void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pStrides,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *IAGetIndexBuffer )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
            /* [annotation] */ 
            _Out_opt_  DXGI_FORMAT *Format,
            /* [annotation] */ 
            _Out_opt_  UINT *Offset);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);
        
        void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *GetPredication )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
            /* [annotation] */ 
            _Out_opt_  BOOL *pPredicateValue);
        
        void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumRTVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT UAVStartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *OMGetBlendState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
            /* [annotation] */ 
            _Out_opt_  FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _Out_opt_  UINT *pSampleMask);
        
        void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef);
        
        void ( STDMETHODCALLTYPE *SOGetTargets )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);
        
        void ( STDMETHODCALLTYPE *RSGetState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSGetViewports )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *CSGetShader )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSGetSamplers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *ClearState )( 
            ID3D11DeviceContext3 * This);
        
        void ( STDMETHODCALLTYPE *Flush )( 
            ID3D11DeviceContext3 * This);
        
        D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
            ID3D11DeviceContext3 * This);
        
        UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
            ID3D11DeviceContext3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
            ID3D11DeviceContext3 * This,
            BOOL RestoreDeferredContextState,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  UINT DstX,
            /* [annotation] */ 
            _In_  UINT DstY,
            /* [annotation] */ 
            _In_  UINT DstZ,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox,
            /* [annotation] */ 
            _In_  UINT CopyFlags);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource1 )( 
            ID3D11DeviceContext3 * This,
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
            _In_  UINT SrcDepthPitch,
            /* [annotation] */ 
            _In_  UINT CopyFlags);
        
        void ( STDMETHODCALLTYPE *DiscardResource )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *DiscardView )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *SwapDeviceContextState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3DDeviceContextState *pState,
            /* [annotation] */ 
            _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);
        
        void ( STDMETHODCALLTYPE *ClearView )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pView,
            /* [annotation] */ 
            _In_  const FLOAT Color[ 4 ],
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
            UINT NumRects);
        
        void ( STDMETHODCALLTYPE *DiscardView1 )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
            UINT NumRects);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateTileMappings )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _In_  UINT NumTiledResourceRegions,
            /* [annotation] */ 
            _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
            /* [annotation] */ 
            _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT NumRanges,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pRangeFlags,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pTilePoolStartOffsets,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *CopyTileMappings )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDestTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSourceTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        void ( STDMETHODCALLTYPE *CopyTiles )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBuffer,
            /* [annotation] */ 
            _In_  UINT64 BufferStartOffsetInBytes,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        void ( STDMETHODCALLTYPE *UpdateTiles )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDestTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
            /* [annotation] */ 
            _In_  const void *pSourceTileData,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTilePool )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT64 NewSizeInBytes);
        
        void ( STDMETHODCALLTYPE *TiledResourceBarrier )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier);
        
        BOOL ( STDMETHODCALLTYPE *IsAnnotationEnabled )( 
            ID3D11DeviceContext3 * This);
        
        void ( STDMETHODCALLTYPE *SetMarkerInt )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *BeginEventInt )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *EndEvent )( 
            ID3D11DeviceContext3 * This);
        
        void ( STDMETHODCALLTYPE *Flush1 )( 
            ID3D11DeviceContext3 * This,
            D3D11_CONTEXT_TYPE ContextType,
            /* [annotation] */ 
            _In_opt_  HANDLE hEvent);
        
        void ( STDMETHODCALLTYPE *SetHardwareProtectionState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _In_  BOOL HwProtectionEnable);
        
        void ( STDMETHODCALLTYPE *GetHardwareProtectionState )( 
            ID3D11DeviceContext3 * This,
            /* [annotation] */ 
            _Out_  BOOL *pHwProtectionEnable);
        
        END_INTERFACE
    } ID3D11DeviceContext3Vtbl;

    interface ID3D11DeviceContext3
    {
        CONST_VTBL struct ID3D11DeviceContext3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11DeviceContext3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext3_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext3_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext3_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext3_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext3_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext3_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext3_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext3_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext3_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext3_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext3_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext3_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext3_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext3_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext3_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext3_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext3_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext3_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext3_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext3_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext3_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext3_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext3_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext3_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext3_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext3_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext3_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext3_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext3_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext3_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext3_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext3_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext3_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext3_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext3_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext3_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext3_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext3_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext3_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext3_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext3_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext3_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext3_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext3_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext3_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext3_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext3_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext3_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext3_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext3_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext3_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext3_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext3_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext3_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext3_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext3_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext3_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext3_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext3_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext3_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext3_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext3_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext3_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext3_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext3_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 


#define ID3D11DeviceContext3_UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D11DeviceContext3_CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags) ) 

#define ID3D11DeviceContext3_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D11DeviceContext3_UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags)	\
    ( (This)->lpVtbl -> UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags) ) 

#define ID3D11DeviceContext3_ResizeTilePool(This,pTilePool,NewSizeInBytes)	\
    ( (This)->lpVtbl -> ResizeTilePool(This,pTilePool,NewSizeInBytes) ) 

#define ID3D11DeviceContext3_TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier)	\
    ( (This)->lpVtbl -> TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier) ) 

#define ID3D11DeviceContext3_IsAnnotationEnabled(This)	\
    ( (This)->lpVtbl -> IsAnnotationEnabled(This) ) 

#define ID3D11DeviceContext3_SetMarkerInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> SetMarkerInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext3_BeginEventInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> BeginEventInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext3_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 


#define ID3D11DeviceContext3_Flush1(This,ContextType,hEvent)	\
    ( (This)->lpVtbl -> Flush1(This,ContextType,hEvent) ) 

#define ID3D11DeviceContext3_SetHardwareProtectionState(This,HwProtectionEnable)	\
    ( (This)->lpVtbl -> SetHardwareProtectionState(This,HwProtectionEnable) ) 

#define ID3D11DeviceContext3_GetHardwareProtectionState(This,pHwProtectionEnable)	\
    ( (This)->lpVtbl -> GetHardwareProtectionState(This,pHwProtectionEnable) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext3_INTERFACE_DEFINED__ */


#ifndef __ID3D11Fence_INTERFACE_DEFINED__
#define __ID3D11Fence_INTERFACE_DEFINED__

/* interface ID3D11Fence */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Fence;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("affde9d1-1df7-4bb7-8a34-0f46251dab80")
    ID3D11Fence : public ID3D11DeviceChild
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle( 
            /* [annotation] */ 
            _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
            /* [annotation] */ 
            _In_  DWORD dwAccess,
            /* [annotation] */ 
            _In_opt_  LPCWSTR lpName,
            /* [annotation] */ 
            _Out_  HANDLE *pHandle) = 0;
        
        virtual UINT64 STDMETHODCALLTYPE GetCompletedValue( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion( 
            /* [annotation] */ 
            _In_  UINT64 Value,
            /* [annotation] */ 
            _In_  HANDLE hEvent) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11FenceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Fence * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Fence * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Fence * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
            /* [annotation] */ 
            _In_  DWORD dwAccess,
            /* [annotation] */ 
            _In_opt_  LPCWSTR lpName,
            /* [annotation] */ 
            _Out_  HANDLE *pHandle);
        
        UINT64 ( STDMETHODCALLTYPE *GetCompletedValue )( 
            ID3D11Fence * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetEventOnCompletion )( 
            ID3D11Fence * This,
            /* [annotation] */ 
            _In_  UINT64 Value,
            /* [annotation] */ 
            _In_  HANDLE hEvent);
        
        END_INTERFACE
    } ID3D11FenceVtbl;

    interface ID3D11Fence
    {
        CONST_VTBL struct ID3D11FenceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Fence_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Fence_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Fence_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Fence_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Fence_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Fence_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Fence_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Fence_CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle) ) 

#define ID3D11Fence_GetCompletedValue(This)	\
    ( (This)->lpVtbl -> GetCompletedValue(This) ) 

#define ID3D11Fence_SetEventOnCompletion(This,Value,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnCompletion(This,Value,hEvent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Fence_INTERFACE_DEFINED__ */


#ifndef __ID3D11DeviceContext4_INTERFACE_DEFINED__
#define __ID3D11DeviceContext4_INTERFACE_DEFINED__

/* interface ID3D11DeviceContext4 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11DeviceContext4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("917600da-f58c-4c33-98d8-3e15b390fa24")
    ID3D11DeviceContext4 : public ID3D11DeviceContext3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Signal( 
            /* [annotation] */ 
            _In_  ID3D11Fence *pFence,
            /* [annotation] */ 
            _In_  UINT64 Value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Wait( 
            /* [annotation] */ 
            _In_  ID3D11Fence *pFence,
            /* [annotation] */ 
            _In_  UINT64 Value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11DeviceContext4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11DeviceContext4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11DeviceContext4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11DeviceContext4 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11PixelShader *pPixelShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11VertexShader *pVertexShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DrawIndexed )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT IndexCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation);
        
        void ( STDMETHODCALLTYPE *Draw )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT VertexCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation);
        
        HRESULT ( STDMETHODCALLTYPE *Map )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource,
            /* [annotation] */ 
            _In_  D3D11_MAP MapType,
            /* [annotation] */ 
            _In_  UINT MapFlags,
            /* [annotation] */ 
            _Out_opt_  D3D11_MAPPED_SUBRESOURCE *pMappedResource);
        
        void ( STDMETHODCALLTYPE *Unmap )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IASetInputLayout )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11InputLayout *pInputLayout);
        
        void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pStrides,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT IndexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *DrawInstanced )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT VertexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);
        
        void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *Begin )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        void ( STDMETHODCALLTYPE *End )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags);
        
        void ( STDMETHODCALLTYPE *SetPredication )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue);
        
        void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT NumRTVs,
            /* [annotation] */ 
            _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT UAVStartSlot,
            /* [annotation] */ 
            _In_  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *OMSetBlendState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11BlendState *pBlendState,
            /* [annotation] */ 
            _In_opt_  const FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _In_  UINT SampleMask);
        
        void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
            /* [annotation] */ 
            _In_  UINT StencilRef);
        
        void ( STDMETHODCALLTYPE *SOSetTargets )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *DrawAuto )( 
            ID3D11DeviceContext4 * This);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *Dispatch )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountX,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountY,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountZ);
        
        void ( STDMETHODCALLTYPE *DispatchIndirect )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *RSSetState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11RasterizerState *pRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSSetViewports )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            /* [annotation] */ 
            _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  UINT DstX,
            /* [annotation] */ 
            _In_  UINT DstY,
            /* [annotation] */ 
            _In_  UINT DstZ,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox);
        
        void ( STDMETHODCALLTYPE *CopyResource )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource )( 
            ID3D11DeviceContext4 * This,
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
        
        void ( STDMETHODCALLTYPE *CopyStructureCount )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pDstBuffer,
            /* [annotation] */ 
            _In_  UINT DstAlignedByteOffset,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pSrcView);
        
        void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11RenderTargetView *pRenderTargetView,
            /* [annotation] */ 
            _In_  const FLOAT ColorRGBA[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const UINT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const FLOAT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_  UINT ClearFlags,
            /* [annotation] */ 
            _In_  FLOAT Depth,
            /* [annotation] */ 
            _In_  UINT8 Stencil);
        
        void ( STDMETHODCALLTYPE *GenerateMips )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11ShaderResourceView *pShaderResourceView);
        
        void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            FLOAT MinLOD);
        
        FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *ResolveSubresource )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format);
        
        void ( STDMETHODCALLTYPE *ExecuteCommandList )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11CommandList *pCommandList,
            BOOL RestoreContextState);
        
        void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11HullShader *pHullShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DomainShader *pDomainShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *CSSetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11ComputeShader *pComputeShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSSetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);
        
        void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pStrides,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *IAGetIndexBuffer )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
            /* [annotation] */ 
            _Out_opt_  DXGI_FORMAT *Format,
            /* [annotation] */ 
            _Out_opt_  UINT *Offset);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);
        
        void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *GetPredication )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
            /* [annotation] */ 
            _Out_opt_  BOOL *pPredicateValue);
        
        void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumRTVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT UAVStartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *OMGetBlendState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
            /* [annotation] */ 
            _Out_opt_  FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _Out_opt_  UINT *pSampleMask);
        
        void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef);
        
        void ( STDMETHODCALLTYPE *SOGetTargets )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);
        
        void ( STDMETHODCALLTYPE *RSGetState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSGetViewports )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *CSGetShader )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSGetSamplers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *ClearState )( 
            ID3D11DeviceContext4 * This);
        
        void ( STDMETHODCALLTYPE *Flush )( 
            ID3D11DeviceContext4 * This);
        
        D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
            ID3D11DeviceContext4 * This);
        
        UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
            ID3D11DeviceContext4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
            ID3D11DeviceContext4 * This,
            BOOL RestoreDeferredContextState,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  UINT DstX,
            /* [annotation] */ 
            _In_  UINT DstY,
            /* [annotation] */ 
            _In_  UINT DstZ,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_opt_  const D3D11_BOX *pSrcBox,
            /* [annotation] */ 
            _In_  UINT CopyFlags);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource1 )( 
            ID3D11DeviceContext4 * This,
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
            _In_  UINT SrcDepthPitch,
            /* [annotation] */ 
            _In_  UINT CopyFlags);
        
        void ( STDMETHODCALLTYPE *DiscardResource )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *DiscardView )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);
        
        void ( STDMETHODCALLTYPE *SwapDeviceContextState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3DDeviceContextState *pState,
            /* [annotation] */ 
            _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);
        
        void ( STDMETHODCALLTYPE *ClearView )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pView,
            /* [annotation] */ 
            _In_  const FLOAT Color[ 4 ],
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
            UINT NumRects);
        
        void ( STDMETHODCALLTYPE *DiscardView1 )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
            UINT NumRects);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateTileMappings )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _In_  UINT NumTiledResourceRegions,
            /* [annotation] */ 
            _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
            /* [annotation] */ 
            _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT NumRanges,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pRangeFlags,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pTilePoolStartOffsets,
            /* [annotation] */ 
            _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *CopyTileMappings )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDestTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSourceTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        void ( STDMETHODCALLTYPE *CopyTiles )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBuffer,
            /* [annotation] */ 
            _In_  UINT64 BufferStartOffsetInBytes,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        void ( STDMETHODCALLTYPE *UpdateTiles )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDestTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
            /* [annotation] */ 
            _In_  const void *pSourceTileData,
            /* [annotation] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTilePool )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT64 NewSizeInBytes);
        
        void ( STDMETHODCALLTYPE *TiledResourceBarrier )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier);
        
        BOOL ( STDMETHODCALLTYPE *IsAnnotationEnabled )( 
            ID3D11DeviceContext4 * This);
        
        void ( STDMETHODCALLTYPE *SetMarkerInt )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *BeginEventInt )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *EndEvent )( 
            ID3D11DeviceContext4 * This);
        
        void ( STDMETHODCALLTYPE *Flush1 )( 
            ID3D11DeviceContext4 * This,
            D3D11_CONTEXT_TYPE ContextType,
            /* [annotation] */ 
            _In_opt_  HANDLE hEvent);
        
        void ( STDMETHODCALLTYPE *SetHardwareProtectionState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  BOOL HwProtectionEnable);
        
        void ( STDMETHODCALLTYPE *GetHardwareProtectionState )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _Out_  BOOL *pHwProtectionEnable);
        
        HRESULT ( STDMETHODCALLTYPE *Signal )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Fence *pFence,
            /* [annotation] */ 
            _In_  UINT64 Value);
        
        HRESULT ( STDMETHODCALLTYPE *Wait )( 
            ID3D11DeviceContext4 * This,
            /* [annotation] */ 
            _In_  ID3D11Fence *pFence,
            /* [annotation] */ 
            _In_  UINT64 Value);
        
        END_INTERFACE
    } ID3D11DeviceContext4Vtbl;

    interface ID3D11DeviceContext4
    {
        CONST_VTBL struct ID3D11DeviceContext4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11DeviceContext4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext4_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext4_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext4_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext4_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext4_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext4_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext4_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext4_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext4_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext4_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext4_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext4_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext4_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext4_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext4_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext4_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext4_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext4_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext4_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext4_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext4_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext4_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext4_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext4_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext4_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext4_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext4_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext4_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext4_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext4_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext4_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext4_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext4_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext4_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext4_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext4_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext4_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext4_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext4_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext4_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext4_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext4_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext4_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext4_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext4_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext4_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext4_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext4_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext4_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext4_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext4_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext4_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext4_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext4_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext4_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext4_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext4_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext4_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext4_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext4_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext4_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext4_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext4_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext4_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext4_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext4_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext4_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext4_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 


#define ID3D11DeviceContext4_UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D11DeviceContext4_CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags) ) 

#define ID3D11DeviceContext4_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D11DeviceContext4_UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags)	\
    ( (This)->lpVtbl -> UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags) ) 

#define ID3D11DeviceContext4_ResizeTilePool(This,pTilePool,NewSizeInBytes)	\
    ( (This)->lpVtbl -> ResizeTilePool(This,pTilePool,NewSizeInBytes) ) 

#define ID3D11DeviceContext4_TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier)	\
    ( (This)->lpVtbl -> TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier) ) 

#define ID3D11DeviceContext4_IsAnnotationEnabled(This)	\
    ( (This)->lpVtbl -> IsAnnotationEnabled(This) ) 

#define ID3D11DeviceContext4_SetMarkerInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> SetMarkerInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext4_BeginEventInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> BeginEventInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext4_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 


#define ID3D11DeviceContext4_Flush1(This,ContextType,hEvent)	\
    ( (This)->lpVtbl -> Flush1(This,ContextType,hEvent) ) 

#define ID3D11DeviceContext4_SetHardwareProtectionState(This,HwProtectionEnable)	\
    ( (This)->lpVtbl -> SetHardwareProtectionState(This,HwProtectionEnable) ) 

#define ID3D11DeviceContext4_GetHardwareProtectionState(This,pHwProtectionEnable)	\
    ( (This)->lpVtbl -> GetHardwareProtectionState(This,pHwProtectionEnable) ) 


#define ID3D11DeviceContext4_Signal(This,pFence,Value)	\
    ( (This)->lpVtbl -> Signal(This,pFence,Value) ) 

#define ID3D11DeviceContext4_Wait(This,pFence,Value)	\
    ( (This)->lpVtbl -> Wait(This,pFence,Value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext4_INTERFACE_DEFINED__ */


#ifndef __ID3D11Device3_INTERFACE_DEFINED__
#define __ID3D11Device3_INTERFACE_DEFINED__

/* interface ID3D11Device3 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Device3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A05C8C37-D2C6-4732-B3A0-9CE0B0DC9AE6")
    ID3D11Device3 : public ID3D11Device2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTexture2D1( 
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTexture3D1( 
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState2( 
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView1( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView1( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView1( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateQuery1( 
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query1 **ppQuery1) = 0;
        
        virtual void STDMETHODCALLTYPE GetImmediateContext3( 
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext3 **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext3( 
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext) = 0;
        
        virtual void STDMETHODCALLTYPE WriteToSubresource( 
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
            _In_  UINT SrcDepthPitch) = 0;
        
        virtual void STDMETHODCALLTYPE ReadFromSubresource( 
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
            _In_opt_  const D3D11_BOX *pSrcBox) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Device3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Device3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Device3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Device3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query **ppQuery);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Counter **ppCounter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
            ID3D11Device3 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
            ID3D11Device3 * This);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            ID3D11Device3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
            ID3D11Device3 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
            ID3D11Device3 * This,
            UINT RaiseFlags);
        
        UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
            ID3D11Device3 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
            ID3D11Device3 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext2 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext2 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext2 )( 
            ID3D11Device3 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *GetResourceTiling )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState2 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery1 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query1 **ppQuery1);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext3 )( 
            ID3D11Device3 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext3 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext3 )( 
            ID3D11Device3 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *WriteToSubresource )( 
            ID3D11Device3 * This,
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
            ID3D11Device3 * This,
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
        
        END_INTERFACE
    } ID3D11Device3Vtbl;

    interface ID3D11Device3
    {
        CONST_VTBL struct ID3D11Device3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Device3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device3_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device3_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device3_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device3_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device3_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device3_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device3_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device3_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device3_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device3_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device3_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device3_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device3_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device3_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device3_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device3_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device3_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device3_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device3_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device3_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device3_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device3_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device3_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device3_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device3_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device3_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device3_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device3_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device3_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device3_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device3_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device3_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device3_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device3_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device3_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device3_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device3_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device3_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device3_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 


#define ID3D11Device3_GetImmediateContext2(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext2(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext2(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext2(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D11Device3_CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels) ) 


#define ID3D11Device3_CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D) ) 

#define ID3D11Device3_CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D) ) 

#define ID3D11Device3_CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1)	\
    ( (This)->lpVtbl -> CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1) ) 

#define ID3D11Device3_CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1) ) 

#define ID3D11Device3_CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1)	\
    ( (This)->lpVtbl -> CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1) ) 

#define ID3D11Device3_CreateQuery1(This,pQueryDesc1,ppQuery1)	\
    ( (This)->lpVtbl -> CreateQuery1(This,pQueryDesc1,ppQuery1) ) 

#define ID3D11Device3_GetImmediateContext3(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext3(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext3(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext3(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11Device3_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_3_0000_0011 */
/* [local] */ 

DEFINE_GUID(IID_ID3D11Texture2D1,0x51218251,0x1E33,0x4617,0x9C,0xCB,0x4D,0x3A,0x43,0x67,0xE7,0xBB);
DEFINE_GUID(IID_ID3D11Texture3D1,0x0C711683,0x2853,0x4846,0x9B,0xB0,0xF3,0xE6,0x06,0x39,0xE4,0x6A);
DEFINE_GUID(IID_ID3D11RasterizerState2,0x6fbd02fb,0x209f,0x46c4,0xb0,0x59,0x2e,0xd1,0x55,0x86,0xa6,0xac);
DEFINE_GUID(IID_ID3D11ShaderResourceView1,0x91308b87,0x9040,0x411d,0x8c,0x67,0xc3,0x92,0x53,0xce,0x38,0x02);
DEFINE_GUID(IID_ID3D11RenderTargetView1,0xffbe2e23,0xf011,0x418a,0xac,0x56,0x5c,0xee,0xd7,0xc5,0xb9,0x4b);
DEFINE_GUID(IID_ID3D11UnorderedAccessView1,0x7b3b6153,0xa886,0x4544,0xab,0x37,0x65,0x37,0xc8,0x50,0x04,0x03);
DEFINE_GUID(IID_ID3D11Query1,0x631b4766,0x36dc,0x461d,0x8d,0xb6,0xc4,0x7e,0x13,0xe6,0x09,0x16);
DEFINE_GUID(IID_ID3D11DeviceContext3,0xb4e3c01d,0xe79e,0x4637,0x91,0xb2,0x51,0x0e,0x9f,0x4c,0x9b,0x8f);
DEFINE_GUID(IID_ID3D11Fence,0xaffde9d1,0x1df7,0x4bb7,0x8a,0x34,0x0f,0x46,0x25,0x1d,0xab,0x80);
DEFINE_GUID(IID_ID3D11DeviceContext4,0x917600da,0xf58c,0x4c33,0x98,0xd8,0x3e,0x15,0xb3,0x90,0xfa,0x24);
DEFINE_GUID(IID_ID3D11Device3,0xA05C8C37,0xD2C6,0x4732,0xB3,0xA0,0x9C,0xE0,0xB0,0xDC,0x9A,0xE6);


extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0011_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


