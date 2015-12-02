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

#ifndef __d3d11_1_h__
#define __d3d11_1_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11BlendState1_FWD_DEFINED__
#define __ID3D11BlendState1_FWD_DEFINED__
typedef interface ID3D11BlendState1 ID3D11BlendState1;

#endif 	/* __ID3D11BlendState1_FWD_DEFINED__ */


#ifndef __ID3D11RasterizerState1_FWD_DEFINED__
#define __ID3D11RasterizerState1_FWD_DEFINED__
typedef interface ID3D11RasterizerState1 ID3D11RasterizerState1;

#endif 	/* __ID3D11RasterizerState1_FWD_DEFINED__ */


#ifndef __ID3DDeviceContextState_FWD_DEFINED__
#define __ID3DDeviceContextState_FWD_DEFINED__
typedef interface ID3DDeviceContextState ID3DDeviceContextState;

#endif 	/* __ID3DDeviceContextState_FWD_DEFINED__ */


#ifndef __ID3D11DeviceContext1_FWD_DEFINED__
#define __ID3D11DeviceContext1_FWD_DEFINED__
typedef interface ID3D11DeviceContext1 ID3D11DeviceContext1;

#endif 	/* __ID3D11DeviceContext1_FWD_DEFINED__ */


#ifndef __ID3D11VideoContext1_FWD_DEFINED__
#define __ID3D11VideoContext1_FWD_DEFINED__
typedef interface ID3D11VideoContext1 ID3D11VideoContext1;

#endif 	/* __ID3D11VideoContext1_FWD_DEFINED__ */


#ifndef __ID3D11VideoDevice1_FWD_DEFINED__
#define __ID3D11VideoDevice1_FWD_DEFINED__
typedef interface ID3D11VideoDevice1 ID3D11VideoDevice1;

#endif 	/* __ID3D11VideoDevice1_FWD_DEFINED__ */


#ifndef __ID3D11VideoProcessorEnumerator1_FWD_DEFINED__
#define __ID3D11VideoProcessorEnumerator1_FWD_DEFINED__
typedef interface ID3D11VideoProcessorEnumerator1 ID3D11VideoProcessorEnumerator1;

#endif 	/* __ID3D11VideoProcessorEnumerator1_FWD_DEFINED__ */


#ifndef __ID3D11Device1_FWD_DEFINED__
#define __ID3D11Device1_FWD_DEFINED__
typedef interface ID3D11Device1 ID3D11Device1;

#endif 	/* __ID3D11Device1_FWD_DEFINED__ */


#ifndef __ID3DUserDefinedAnnotation_FWD_DEFINED__
#define __ID3DUserDefinedAnnotation_FWD_DEFINED__
typedef interface ID3DUserDefinedAnnotation ID3DUserDefinedAnnotation;

#endif 	/* __ID3DUserDefinedAnnotation_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgi1_2.h"
#include "d3dcommon.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d11_1_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d3d11.h" //
#ifdef __cplusplus
extern "C"{
#endif
typedef 
enum D3D11_COPY_FLAGS
    {
        D3D11_COPY_NO_OVERWRITE	= 0x1,
        D3D11_COPY_DISCARD	= 0x2
    } 	D3D11_COPY_FLAGS;

typedef 
enum D3D11_LOGIC_OP
    {
        D3D11_LOGIC_OP_CLEAR	= 0,
        D3D11_LOGIC_OP_SET	= ( D3D11_LOGIC_OP_CLEAR + 1 ) ,
        D3D11_LOGIC_OP_COPY	= ( D3D11_LOGIC_OP_SET + 1 ) ,
        D3D11_LOGIC_OP_COPY_INVERTED	= ( D3D11_LOGIC_OP_COPY + 1 ) ,
        D3D11_LOGIC_OP_NOOP	= ( D3D11_LOGIC_OP_COPY_INVERTED + 1 ) ,
        D3D11_LOGIC_OP_INVERT	= ( D3D11_LOGIC_OP_NOOP + 1 ) ,
        D3D11_LOGIC_OP_AND	= ( D3D11_LOGIC_OP_INVERT + 1 ) ,
        D3D11_LOGIC_OP_NAND	= ( D3D11_LOGIC_OP_AND + 1 ) ,
        D3D11_LOGIC_OP_OR	= ( D3D11_LOGIC_OP_NAND + 1 ) ,
        D3D11_LOGIC_OP_NOR	= ( D3D11_LOGIC_OP_OR + 1 ) ,
        D3D11_LOGIC_OP_XOR	= ( D3D11_LOGIC_OP_NOR + 1 ) ,
        D3D11_LOGIC_OP_EQUIV	= ( D3D11_LOGIC_OP_XOR + 1 ) ,
        D3D11_LOGIC_OP_AND_REVERSE	= ( D3D11_LOGIC_OP_EQUIV + 1 ) ,
        D3D11_LOGIC_OP_AND_INVERTED	= ( D3D11_LOGIC_OP_AND_REVERSE + 1 ) ,
        D3D11_LOGIC_OP_OR_REVERSE	= ( D3D11_LOGIC_OP_AND_INVERTED + 1 ) ,
        D3D11_LOGIC_OP_OR_INVERTED	= ( D3D11_LOGIC_OP_OR_REVERSE + 1 ) 
    } 	D3D11_LOGIC_OP;

typedef struct D3D11_RENDER_TARGET_BLEND_DESC1
    {
    BOOL BlendEnable;
    BOOL LogicOpEnable;
    D3D11_BLEND SrcBlend;
    D3D11_BLEND DestBlend;
    D3D11_BLEND_OP BlendOp;
    D3D11_BLEND SrcBlendAlpha;
    D3D11_BLEND DestBlendAlpha;
    D3D11_BLEND_OP BlendOpAlpha;
    D3D11_LOGIC_OP LogicOp;
    UINT8 RenderTargetWriteMask;
    } 	D3D11_RENDER_TARGET_BLEND_DESC1;

typedef struct D3D11_BLEND_DESC1
    {
    BOOL AlphaToCoverageEnable;
    BOOL IndependentBlendEnable;
    D3D11_RENDER_TARGET_BLEND_DESC1 RenderTarget[ 8 ];
    } 	D3D11_BLEND_DESC1;

/* Note, the array size for RenderTarget[] above is D3D11_SIMULTANEOUS_RENDERTARGET_COUNT. 
   IDL processing/generation of this header replaces the define; this comment is merely explaining what happened. */
#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_BLEND_DESC1 : public D3D11_BLEND_DESC1
{
    CD3D11_BLEND_DESC1()
    {}
    explicit CD3D11_BLEND_DESC1( const D3D11_BLEND_DESC1& o ) :
        D3D11_BLEND_DESC1( o )
    {}
    explicit CD3D11_BLEND_DESC1( CD3D11_DEFAULT )
    {
        AlphaToCoverageEnable = FALSE;
        IndependentBlendEnable = FALSE;
        const D3D11_RENDER_TARGET_BLEND_DESC1 defaultRenderTargetBlendDesc =
        {
            FALSE,FALSE,
            D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
            D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
            D3D11_LOGIC_OP_NOOP,
            D3D11_COLOR_WRITE_ENABLE_ALL,
        };
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            RenderTarget[ i ] = defaultRenderTargetBlendDesc;
    }
    ~CD3D11_BLEND_DESC1() {}
    operator const D3D11_BLEND_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11BlendState1_INTERFACE_DEFINED__
#define __ID3D11BlendState1_INTERFACE_DEFINED__

/* interface ID3D11BlendState1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11BlendState1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc86fabe-da55-401d-85e7-e3c9de2877e9")
    ID3D11BlendState1 : public ID3D11BlendState
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_BLEND_DESC1 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11BlendState1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11BlendState1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11BlendState1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11BlendState1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _Out_  D3D11_BLEND_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11BlendState1 * This,
            /* [annotation] */ 
            _Out_  D3D11_BLEND_DESC1 *pDesc);
        
        END_INTERFACE
    } ID3D11BlendState1Vtbl;

    interface ID3D11BlendState1
    {
        CONST_VTBL struct ID3D11BlendState1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11BlendState1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11BlendState1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11BlendState1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11BlendState1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11BlendState1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11BlendState1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11BlendState1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11BlendState1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11BlendState1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11BlendState1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_1_0000_0001 */
/* [local] */ 

typedef struct D3D11_RASTERIZER_DESC1
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
    } 	D3D11_RASTERIZER_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_RASTERIZER_DESC1 : public D3D11_RASTERIZER_DESC1
{
    CD3D11_RASTERIZER_DESC1()
    {}
    explicit CD3D11_RASTERIZER_DESC1( const D3D11_RASTERIZER_DESC1& o ) :
        D3D11_RASTERIZER_DESC1( o )
    {}
    explicit CD3D11_RASTERIZER_DESC1( CD3D11_DEFAULT )
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
    }
    explicit CD3D11_RASTERIZER_DESC1(
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
        UINT forcedSampleCount )
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
    }
    ~CD3D11_RASTERIZER_DESC1() {}
    operator const D3D11_RASTERIZER_DESC1&() const { return *this; }
};
extern "C"{
#endif


extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0001_v0_0_s_ifspec;

#ifndef __ID3D11RasterizerState1_INTERFACE_DEFINED__
#define __ID3D11RasterizerState1_INTERFACE_DEFINED__

/* interface ID3D11RasterizerState1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11RasterizerState1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1217d7a6-5039-418c-b042-9cbe256afd6e")
    ID3D11RasterizerState1 : public ID3D11RasterizerState
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc1( 
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC1 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11RasterizerState1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11RasterizerState1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11RasterizerState1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11RasterizerState1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC *pDesc);
        
        void ( STDMETHODCALLTYPE *GetDesc1 )( 
            ID3D11RasterizerState1 * This,
            /* [annotation] */ 
            _Out_  D3D11_RASTERIZER_DESC1 *pDesc);
        
        END_INTERFACE
    } ID3D11RasterizerState1Vtbl;

    interface ID3D11RasterizerState1
    {
        CONST_VTBL struct ID3D11RasterizerState1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11RasterizerState1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11RasterizerState1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11RasterizerState1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11RasterizerState1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11RasterizerState1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11RasterizerState1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11RasterizerState1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11RasterizerState1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11RasterizerState1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11RasterizerState1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_1_0000_0002 */
/* [local] */ 

typedef 
enum D3D11_1_CREATE_DEVICE_CONTEXT_STATE_FLAG
    {
        D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED	= 0x1
    } 	D3D11_1_CREATE_DEVICE_CONTEXT_STATE_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0002_v0_0_s_ifspec;

#ifndef __ID3DDeviceContextState_INTERFACE_DEFINED__
#define __ID3DDeviceContextState_INTERFACE_DEFINED__

/* interface ID3DDeviceContextState */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3DDeviceContextState;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5c1e0d8a-7c23-48f9-8c59-a92958ceff11")
    ID3DDeviceContextState : public ID3D11DeviceChild
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ID3DDeviceContextStateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3DDeviceContextState * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3DDeviceContextState * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3DDeviceContextState * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3DDeviceContextState * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3DDeviceContextState * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3DDeviceContextState * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3DDeviceContextState * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        END_INTERFACE
    } ID3DDeviceContextStateVtbl;

    interface ID3DDeviceContextState
    {
        CONST_VTBL struct ID3DDeviceContextStateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3DDeviceContextState_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3DDeviceContextState_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3DDeviceContextState_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3DDeviceContextState_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3DDeviceContextState_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3DDeviceContextState_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3DDeviceContextState_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3DDeviceContextState_INTERFACE_DEFINED__ */


#ifndef __ID3D11DeviceContext1_INTERFACE_DEFINED__
#define __ID3D11DeviceContext1_INTERFACE_DEFINED__

/* interface ID3D11DeviceContext1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11DeviceContext1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bb2c6faa-b5fb-4082-8e6b-388b8cfa90e1")
    ID3D11DeviceContext1 : public ID3D11DeviceContext
    {
    public:
        virtual void STDMETHODCALLTYPE CopySubresourceRegion1( 
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
            _In_  UINT CopyFlags) = 0;
        
        virtual void STDMETHODCALLTYPE UpdateSubresource1( 
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
            _In_  UINT CopyFlags) = 0;
        
        virtual void STDMETHODCALLTYPE DiscardResource( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource) = 0;
        
        virtual void STDMETHODCALLTYPE DiscardView( 
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView) = 0;
        
        virtual void STDMETHODCALLTYPE VSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE HSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE DSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE GSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE PSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE VSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE HSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE DSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE GSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE PSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetConstantBuffers1( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) = 0;
        
        virtual void STDMETHODCALLTYPE SwapDeviceContextState( 
            /* [annotation] */ 
            _In_  ID3DDeviceContextState *pState,
            /* [annotation] */ 
            _Outptr_opt_  ID3DDeviceContextState **ppPreviousState) = 0;
        
        virtual void STDMETHODCALLTYPE ClearView( 
            /* [annotation] */ 
            _In_  ID3D11View *pView,
            /* [annotation] */ 
            _In_  const FLOAT Color[ 4 ],
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
            UINT NumRects) = 0;
        
        virtual void STDMETHODCALLTYPE DiscardView1( 
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
            UINT NumRects) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11DeviceContext1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11DeviceContext1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11DeviceContext1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11DeviceContext1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11PixelShader *pPixelShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11VertexShader *pVertexShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DrawIndexed )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  UINT IndexCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation);
        
        void ( STDMETHODCALLTYPE *Draw )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  UINT VertexCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation);
        
        HRESULT ( STDMETHODCALLTYPE *Map )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IASetInputLayout )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11InputLayout *pInputLayout);
        
        void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  UINT VertexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);
        
        void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *Begin )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        void ( STDMETHODCALLTYPE *End )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags);
        
        void ( STDMETHODCALLTYPE *SetPredication )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue);
        
        void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11BlendState *pBlendState,
            /* [annotation] */ 
            _In_opt_  const FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _In_  UINT SampleMask);
        
        void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
            /* [annotation] */ 
            _In_  UINT StencilRef);
        
        void ( STDMETHODCALLTYPE *SOSetTargets )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *DrawAuto )( 
            ID3D11DeviceContext1 * This);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *Dispatch )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountX,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountY,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountZ);
        
        void ( STDMETHODCALLTYPE *DispatchIndirect )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *RSSetState )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11RasterizerState *pRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSSetViewports )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            /* [annotation] */ 
            _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pDstBuffer,
            /* [annotation] */ 
            _In_  UINT DstAlignedByteOffset,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pSrcView);
        
        void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11RenderTargetView *pRenderTargetView,
            /* [annotation] */ 
            _In_  const FLOAT ColorRGBA[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const UINT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const FLOAT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_  UINT ClearFlags,
            /* [annotation] */ 
            _In_  FLOAT Depth,
            /* [annotation] */ 
            _In_  UINT8 Stencil);
        
        void ( STDMETHODCALLTYPE *GenerateMips )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11ShaderResourceView *pShaderResourceView);
        
        void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            FLOAT MinLOD);
        
        FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *ResolveSubresource )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CommandList *pCommandList,
            BOOL RestoreContextState);
        
        void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11HullShader *pHullShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DomainShader *pDomainShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *CSSetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11ComputeShader *pComputeShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSSetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);
        
        void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
            /* [annotation] */ 
            _Out_opt_  DXGI_FORMAT *Format,
            /* [annotation] */ 
            _Out_opt_  UINT *Offset);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);
        
        void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *GetPredication )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
            /* [annotation] */ 
            _Out_opt_  BOOL *pPredicateValue);
        
        void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
            /* [annotation] */ 
            _Out_opt_  FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _Out_opt_  UINT *pSampleMask);
        
        void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef);
        
        void ( STDMETHODCALLTYPE *SOGetTargets )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);
        
        void ( STDMETHODCALLTYPE *RSGetState )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSGetViewports )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *CSGetShader )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSGetSamplers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *ClearState )( 
            ID3D11DeviceContext1 * This);
        
        void ( STDMETHODCALLTYPE *Flush )( 
            ID3D11DeviceContext1 * This);
        
        D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
            ID3D11DeviceContext1 * This);
        
        UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
            ID3D11DeviceContext1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
            ID3D11DeviceContext1 * This,
            BOOL RestoreDeferredContextState,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *DiscardView )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
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
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3DDeviceContextState *pState,
            /* [annotation] */ 
            _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);
        
        void ( STDMETHODCALLTYPE *ClearView )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pView,
            /* [annotation] */ 
            _In_  const FLOAT Color[ 4 ],
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
            UINT NumRects);
        
        void ( STDMETHODCALLTYPE *DiscardView1 )( 
            ID3D11DeviceContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
            UINT NumRects);
        
        END_INTERFACE
    } ID3D11DeviceContext1Vtbl;

    interface ID3D11DeviceContext1
    {
        CONST_VTBL struct ID3D11DeviceContext1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11DeviceContext1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext1_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext1_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext1_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext1_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext1_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext1_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext1_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext1_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext1_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext1_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext1_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext1_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext1_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext1_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext1_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext1_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext1_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext1_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext1_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext1_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext1_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext1_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext1_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext1_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext1_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext1_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext1_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext1_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext1_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext1_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext1_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext1_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext1_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext1_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext1_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext1_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext1_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext1_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext1_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext1_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext1_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext1_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext1_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext1_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext1_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext1_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext1_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext1_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext1_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext1_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext1_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext1_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext1_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext1_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext1_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext1_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext1_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext1_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext1_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext1_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext1_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext1_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext1_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext1_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext1_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext1_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext1_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext1_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext1_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext1_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext1_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext1_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext1_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_1_0000_0004 */
/* [local] */ 

typedef struct D3D11_VIDEO_DECODER_SUB_SAMPLE_MAPPING_BLOCK
    {
    UINT ClearSize;
    UINT EncryptedSize;
    } 	D3D11_VIDEO_DECODER_SUB_SAMPLE_MAPPING_BLOCK;

typedef struct D3D11_VIDEO_DECODER_BUFFER_DESC1
    {
    D3D11_VIDEO_DECODER_BUFFER_TYPE BufferType;
    UINT DataOffset;
    UINT DataSize;
    /* [annotation] */ 
    _Field_size_opt_(IVSize)  void *pIV;
    UINT IVSize;
    /* [annotation] */ 
    _Field_size_opt_(SubSampleMappingCount)  D3D11_VIDEO_DECODER_SUB_SAMPLE_MAPPING_BLOCK *pSubSampleMappingBlock;
    UINT SubSampleMappingCount;
    } 	D3D11_VIDEO_DECODER_BUFFER_DESC1;

typedef struct D3D11_VIDEO_DECODER_BEGIN_FRAME_CRYPTO_SESSION
    {
    ID3D11CryptoSession *pCryptoSession;
    UINT BlobSize;
    /* [annotation] */ 
    _Field_size_opt_(BlobSize)  void *pBlob;
    GUID *pKeyInfoId;
    UINT PrivateDataSize;
    /* [annotation] */ 
    _Field_size_opt_(PrivateDataSize)  void *pPrivateData;
    } 	D3D11_VIDEO_DECODER_BEGIN_FRAME_CRYPTO_SESSION;

typedef 
enum D3D11_VIDEO_DECODER_CAPS
    {
        D3D11_VIDEO_DECODER_CAPS_DOWNSAMPLE	= 0x1,
        D3D11_VIDEO_DECODER_CAPS_NON_REAL_TIME	= 0x2,
        D3D11_VIDEO_DECODER_CAPS_DOWNSAMPLE_DYNAMIC	= 0x4,
        D3D11_VIDEO_DECODER_CAPS_DOWNSAMPLE_REQUIRED	= 0x8,
        D3D11_VIDEO_DECODER_CAPS_UNSUPPORTED	= 0x10
    } 	D3D11_VIDEO_DECODER_CAPS;

typedef 
enum D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINTS
    {
        D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINT_MULTIPLANE_OVERLAY_ROTATION	= 0x1,
        D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINT_MULTIPLANE_OVERLAY_RESIZE	= 0x2,
        D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINT_MULTIPLANE_OVERLAY_COLOR_SPACE_CONVERSION	= 0x4,
        D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINT_TRIPLE_BUFFER_OUTPUT	= 0x8
    } 	D3D11_VIDEO_PROCESSOR_BEHAVIOR_HINTS;

typedef struct D3D11_VIDEO_PROCESSOR_STREAM_BEHAVIOR_HINT
    {
    BOOL Enable;
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    } 	D3D11_VIDEO_PROCESSOR_STREAM_BEHAVIOR_HINT;

typedef 
enum D3D11_CRYPTO_SESSION_STATUS
    {
        D3D11_CRYPTO_SESSION_STATUS_OK	= 0,
        D3D11_CRYPTO_SESSION_STATUS_KEY_LOST	= 1,
        D3D11_CRYPTO_SESSION_STATUS_KEY_AND_CONTENT_LOST	= 2
    } 	D3D11_CRYPTO_SESSION_STATUS;

typedef struct D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA
    {
    UINT PrivateDataSize;
    UINT HWProtectionDataSize;
    BYTE pbInput[ 4 ];
    } 	D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA;

typedef struct D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA
    {
    UINT PrivateDataSize;
    UINT MaxHWProtectionDataSize;
    UINT HWProtectionDataSize;
    UINT64 TransportTime;
    UINT64 ExecutionTime;
    BYTE pbOutput[ 4 ];
    } 	D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA;

typedef struct D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA
    {
    UINT HWProtectionFunctionID;
    D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA *pInputData;
    D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA *pOutputData;
    HRESULT Status;
    } 	D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA;

typedef struct D3D11_VIDEO_SAMPLE_DESC
    {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    DXGI_COLOR_SPACE_TYPE ColorSpace;
    } 	D3D11_VIDEO_SAMPLE_DESC;



extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0004_v0_0_s_ifspec;

#ifndef __ID3D11VideoContext1_INTERFACE_DEFINED__
#define __ID3D11VideoContext1_INTERFACE_DEFINED__

/* interface ID3D11VideoContext1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11VideoContext1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A7F026DA-A5F8-4487-A564-15E34357651E")
    ID3D11VideoContext1 : public ID3D11VideoContext
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SubmitDecoderBuffers1( 
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_(NumBuffers)  const D3D11_VIDEO_DECODER_BUFFER_DESC1 *pBufferDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataForNewHardwareKey( 
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT PrivateInputSize,
            /* [annotation] */ 
            _In_reads_(PrivateInputSize)  const void *pPrivatInputData,
            /* [annotation] */ 
            _Out_  UINT64 *pPrivateOutputData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckCryptoSessionStatus( 
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _Out_  D3D11_CRYPTO_SESSION_STATUS *pStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DecoderEnableDownsampling( 
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc,
            /* [annotation] */ 
            _In_  UINT ReferenceFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DecoderUpdateDownsampling( 
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorSetOutputColorSpace1( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorSetOutputShaderUsage( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL ShaderUsage) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetOutputColorSpace1( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetOutputShaderUsage( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pShaderUsage) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorSetStreamColorSpace1( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorSetStreamMirror( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  BOOL FlipHorizontal,
            /* [annotation] */ 
            _In_  BOOL FlipVertical) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetStreamColorSpace1( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace) = 0;
        
        virtual void STDMETHODCALLTYPE VideoProcessorGetStreamMirror( 
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnable,
            /* [annotation] */ 
            _Out_  BOOL *pFlipHorizontal,
            /* [annotation] */ 
            _Out_  BOOL *pFlipVertical) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoProcessorGetBehaviorHints( 
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
            _Out_  UINT *pBehaviorHints) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11VideoContext1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11VideoContext1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11VideoContext1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11VideoContext1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecoderBuffer )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            D3D11_VIDEO_DECODER_BUFFER_TYPE Type,
            /* [annotation] */ 
            _Out_  UINT *pBufferSize,
            /* [annotation] */ 
            _Outptr_result_bytebuffer_(*pBufferSize)  void **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseDecoderBuffer )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_DECODER_BUFFER_TYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderBeginFrame )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoderOutputView *pView,
            UINT ContentKeySize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(ContentKeySize)  const void *pContentKey);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderEndFrame )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder);
        
        HRESULT ( STDMETHODCALLTYPE *SubmitDecoderBuffers )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_(NumBuffers)  const D3D11_VIDEO_DECODER_BUFFER_DESC *pBufferDesc);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *DecoderExtension )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_EXTENSION *pExtensionData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputTargetRect )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputBackgroundColor )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL YCbCr,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_COLOR *pColor);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputColorSpace )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputAlphaFillMode )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE AlphaFillMode,
            /* [annotation] */ 
            _In_  UINT StreamIndex);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputConstriction )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  SIZE Size);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputStereoMode )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL Enable);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorSetOutputExtension )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  void *pData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputTargetRect )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *Enabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputBackgroundColor )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pYCbCr,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_COLOR *pColor);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputColorSpace )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputAlphaFillMode )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE *pAlphaFillMode,
            /* [annotation] */ 
            _Out_  UINT *pStreamIndex);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputConstriction )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  SIZE *pSize);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputStereoMode )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled);
        
        APP_DEPRECATED_HRESULT ( STDMETHODCALLTYPE *VideoProcessorGetOutputExtension )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  const GUID *pExtensionGuid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Out_writes_bytes_(DataSize)  void *pData);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamFrameFormat )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_FRAME_FORMAT FrameFormat);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamColorSpace )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamOutputRate )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamDestRect )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_opt_  const RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamAlpha )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  FLOAT Alpha);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamPalette )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  UINT Count,
            /* [annotation] */ 
            _In_reads_opt_(Count)  const UINT *pEntries);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamPixelAspectRatio )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamFilter )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_FRAME_FORMAT *pFrameFormat);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamColorSpace )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_COLOR_SPACE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamOutputRate )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamDestRect )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  RECT *pRect);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamAlpha )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled,
            /* [annotation] */ 
            _Out_  FLOAT *pAlpha);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamPalette )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  UINT Count,
            /* [annotation] */ 
            _Out_writes_(Count)  UINT *pEntries);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamPixelAspectRatio )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnabled);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamFilter )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Inout_updates_bytes_(DataSize)  void *pData);
        
        void ( STDMETHODCALLTYPE *EncryptionBlt )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT RandomNumberSize,
            /* [annotation] */ 
            _Out_writes_bytes_(RandomNumberSize)  void *pRandomNumber);
        
        void ( STDMETHODCALLTYPE *FinishSessionKeyRefresh )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetEncryptionBltKey )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT KeySize,
            /* [annotation] */ 
            _Out_writes_bytes_(KeySize)  void *pReadbackKey);
        
        HRESULT ( STDMETHODCALLTYPE *NegotiateAuthenticatedChannelKeyExchange )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11AuthenticatedChannel *pChannel,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _Inout_updates_bytes_(DataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *QueryAuthenticatedChannel )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11AuthenticatedChannel *pChannel,
            /* [annotation] */ 
            _In_  UINT InputSize,
            /* [annotation] */ 
            _In_reads_bytes_(InputSize)  const void *pInput,
            /* [annotation] */ 
            _Out_  D3D11_AUTHENTICATED_CONFIGURE_OUTPUT *pOutput);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamRotation )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  BOOL Enable,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_ROTATION Rotation);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamRotation )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  BOOL *pEnable,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SubmitDecoderBuffers1 )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_(NumBuffers)  const D3D11_VIDEO_DECODER_BUFFER_DESC1 *pBufferDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataForNewHardwareKey )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _In_  UINT PrivateInputSize,
            /* [annotation] */ 
            _In_reads_(PrivateInputSize)  const void *pPrivatInputData,
            /* [annotation] */ 
            _Out_  UINT64 *pPrivateOutputData);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCryptoSessionStatus )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11CryptoSession *pCryptoSession,
            /* [annotation] */ 
            _Out_  D3D11_CRYPTO_SESSION_STATUS *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderEnableDownsampling )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc,
            /* [annotation] */ 
            _In_  UINT ReferenceFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *DecoderUpdateDownsampling )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoDecoder *pDecoder,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputColorSpace1 )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetOutputShaderUsage )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  BOOL ShaderUsage);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputColorSpace1 )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetOutputShaderUsage )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _Out_  BOOL *pShaderUsage);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamColorSpace1 )( 
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorSetStreamMirror )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessor *pVideoProcessor,
            /* [annotation] */ 
            _In_  UINT StreamIndex,
            /* [annotation] */ 
            _Out_  DXGI_COLOR_SPACE_TYPE *pColorSpace);
        
        void ( STDMETHODCALLTYPE *VideoProcessorGetStreamMirror )( 
            ID3D11VideoContext1 * This,
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
            ID3D11VideoContext1 * This,
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
        
        END_INTERFACE
    } ID3D11VideoContext1Vtbl;

    interface ID3D11VideoContext1
    {
        CONST_VTBL struct ID3D11VideoContext1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11VideoContext1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11VideoContext1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11VideoContext1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11VideoContext1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11VideoContext1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11VideoContext1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11VideoContext1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11VideoContext1_GetDecoderBuffer(This,pDecoder,Type,pBufferSize,ppBuffer)	\
    ( (This)->lpVtbl -> GetDecoderBuffer(This,pDecoder,Type,pBufferSize,ppBuffer) ) 

#define ID3D11VideoContext1_ReleaseDecoderBuffer(This,pDecoder,Type)	\
    ( (This)->lpVtbl -> ReleaseDecoderBuffer(This,pDecoder,Type) ) 

#define ID3D11VideoContext1_DecoderBeginFrame(This,pDecoder,pView,ContentKeySize,pContentKey)	\
    ( (This)->lpVtbl -> DecoderBeginFrame(This,pDecoder,pView,ContentKeySize,pContentKey) ) 

#define ID3D11VideoContext1_DecoderEndFrame(This,pDecoder)	\
    ( (This)->lpVtbl -> DecoderEndFrame(This,pDecoder) ) 

#define ID3D11VideoContext1_SubmitDecoderBuffers(This,pDecoder,NumBuffers,pBufferDesc)	\
    ( (This)->lpVtbl -> SubmitDecoderBuffers(This,pDecoder,NumBuffers,pBufferDesc) ) 

#define ID3D11VideoContext1_DecoderExtension(This,pDecoder,pExtensionData)	\
    ( (This)->lpVtbl -> DecoderExtension(This,pDecoder,pExtensionData) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputTargetRect(This,pVideoProcessor,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputTargetRect(This,pVideoProcessor,Enable,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputBackgroundColor(This,pVideoProcessor,YCbCr,pColor)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputBackgroundColor(This,pVideoProcessor,YCbCr,pColor) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputColorSpace(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputColorSpace(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputAlphaFillMode(This,pVideoProcessor,AlphaFillMode,StreamIndex)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputAlphaFillMode(This,pVideoProcessor,AlphaFillMode,StreamIndex) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputConstriction(This,pVideoProcessor,Enable,Size)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputConstriction(This,pVideoProcessor,Enable,Size) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputStereoMode(This,pVideoProcessor,Enable)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputStereoMode(This,pVideoProcessor,Enable) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputTargetRect(This,pVideoProcessor,Enabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputTargetRect(This,pVideoProcessor,Enabled,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputBackgroundColor(This,pVideoProcessor,pYCbCr,pColor)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputBackgroundColor(This,pVideoProcessor,pYCbCr,pColor) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputColorSpace(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputColorSpace(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputAlphaFillMode(This,pVideoProcessor,pAlphaFillMode,pStreamIndex)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputAlphaFillMode(This,pVideoProcessor,pAlphaFillMode,pStreamIndex) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputConstriction(This,pVideoProcessor,pEnabled,pSize)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputConstriction(This,pVideoProcessor,pEnabled,pSize) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputStereoMode(This,pVideoProcessor,pEnabled)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputStereoMode(This,pVideoProcessor,pEnabled) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputExtension(This,pVideoProcessor,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamFrameFormat(This,pVideoProcessor,StreamIndex,FrameFormat)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamFrameFormat(This,pVideoProcessor,StreamIndex,FrameFormat) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamOutputRate(This,pVideoProcessor,StreamIndex,OutputRate,RepeatFrame,pCustomRate)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamOutputRate(This,pVideoProcessor,StreamIndex,OutputRate,RepeatFrame,pCustomRate) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamSourceRect(This,pVideoProcessor,StreamIndex,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamSourceRect(This,pVideoProcessor,StreamIndex,Enable,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamDestRect(This,pVideoProcessor,StreamIndex,Enable,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamDestRect(This,pVideoProcessor,StreamIndex,Enable,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamAlpha(This,pVideoProcessor,StreamIndex,Enable,Alpha)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamAlpha(This,pVideoProcessor,StreamIndex,Enable,Alpha) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,Enable,pSourceAspectRatio,pDestinationAspectRatio)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,Enable,pSourceAspectRatio,pDestinationAspectRatio) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamLumaKey(This,pVideoProcessor,StreamIndex,Enable,Lower,Upper)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamLumaKey(This,pVideoProcessor,StreamIndex,Enable,Lower,Upper) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamStereoFormat(This,pVideoProcessor,StreamIndex,Enable,Format,LeftViewFrame0,BaseViewFrame0,FlipMode,MonoOffset)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamStereoFormat(This,pVideoProcessor,StreamIndex,Enable,Format,LeftViewFrame0,BaseViewFrame0,FlipMode,MonoOffset) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,Enable)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,Enable) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,Enable,Level)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,Enable,Level) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamFrameFormat(This,pVideoProcessor,StreamIndex,pFrameFormat)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamFrameFormat(This,pVideoProcessor,StreamIndex,pFrameFormat) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamColorSpace(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamOutputRate(This,pVideoProcessor,StreamIndex,pOutputRate,pRepeatFrame,pCustomRate)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamOutputRate(This,pVideoProcessor,StreamIndex,pOutputRate,pRepeatFrame,pCustomRate) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamSourceRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamSourceRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamDestRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamDestRect(This,pVideoProcessor,StreamIndex,pEnabled,pRect) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamAlpha(This,pVideoProcessor,StreamIndex,pEnabled,pAlpha)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamAlpha(This,pVideoProcessor,StreamIndex,pEnabled,pAlpha) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamPalette(This,pVideoProcessor,StreamIndex,Count,pEntries) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,pEnabled,pSourceAspectRatio,pDestinationAspectRatio)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamPixelAspectRatio(This,pVideoProcessor,StreamIndex,pEnabled,pSourceAspectRatio,pDestinationAspectRatio) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamLumaKey(This,pVideoProcessor,StreamIndex,pEnabled,pLower,pUpper)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamLumaKey(This,pVideoProcessor,StreamIndex,pEnabled,pLower,pUpper) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamStereoFormat(This,pVideoProcessor,StreamIndex,pEnable,pFormat,pLeftViewFrame0,pBaseViewFrame0,pFlipMode,MonoOffset)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamStereoFormat(This,pVideoProcessor,StreamIndex,pEnable,pFormat,pLeftViewFrame0,pBaseViewFrame0,pFlipMode,MonoOffset) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,pEnabled)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamAutoProcessingMode(This,pVideoProcessor,StreamIndex,pEnabled) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,pEnabled,pLevel)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamFilter(This,pVideoProcessor,StreamIndex,Filter,pEnabled,pLevel) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamExtension(This,pVideoProcessor,StreamIndex,pExtensionGuid,DataSize,pData) ) 

#define ID3D11VideoContext1_VideoProcessorBlt(This,pVideoProcessor,pView,OutputFrame,StreamCount,pStreams)	\
    ( (This)->lpVtbl -> VideoProcessorBlt(This,pVideoProcessor,pView,OutputFrame,StreamCount,pStreams) ) 

#define ID3D11VideoContext1_NegotiateCryptoSessionKeyExchange(This,pCryptoSession,DataSize,pData)	\
    ( (This)->lpVtbl -> NegotiateCryptoSessionKeyExchange(This,pCryptoSession,DataSize,pData) ) 

#define ID3D11VideoContext1_EncryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,IVSize,pIV)	\
    ( (This)->lpVtbl -> EncryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,IVSize,pIV) ) 

#define ID3D11VideoContext1_DecryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,pEncryptedBlockInfo,ContentKeySize,pContentKey,IVSize,pIV)	\
    ( (This)->lpVtbl -> DecryptionBlt(This,pCryptoSession,pSrcSurface,pDstSurface,pEncryptedBlockInfo,ContentKeySize,pContentKey,IVSize,pIV) ) 

#define ID3D11VideoContext1_StartSessionKeyRefresh(This,pCryptoSession,RandomNumberSize,pRandomNumber)	\
    ( (This)->lpVtbl -> StartSessionKeyRefresh(This,pCryptoSession,RandomNumberSize,pRandomNumber) ) 

#define ID3D11VideoContext1_FinishSessionKeyRefresh(This,pCryptoSession)	\
    ( (This)->lpVtbl -> FinishSessionKeyRefresh(This,pCryptoSession) ) 

#define ID3D11VideoContext1_GetEncryptionBltKey(This,pCryptoSession,KeySize,pReadbackKey)	\
    ( (This)->lpVtbl -> GetEncryptionBltKey(This,pCryptoSession,KeySize,pReadbackKey) ) 

#define ID3D11VideoContext1_NegotiateAuthenticatedChannelKeyExchange(This,pChannel,DataSize,pData)	\
    ( (This)->lpVtbl -> NegotiateAuthenticatedChannelKeyExchange(This,pChannel,DataSize,pData) ) 

#define ID3D11VideoContext1_QueryAuthenticatedChannel(This,pChannel,InputSize,pInput,OutputSize,pOutput)	\
    ( (This)->lpVtbl -> QueryAuthenticatedChannel(This,pChannel,InputSize,pInput,OutputSize,pOutput) ) 

#define ID3D11VideoContext1_ConfigureAuthenticatedChannel(This,pChannel,InputSize,pInput,pOutput)	\
    ( (This)->lpVtbl -> ConfigureAuthenticatedChannel(This,pChannel,InputSize,pInput,pOutput) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamRotation(This,pVideoProcessor,StreamIndex,Enable,Rotation)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamRotation(This,pVideoProcessor,StreamIndex,Enable,Rotation) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamRotation(This,pVideoProcessor,StreamIndex,pEnable,pRotation)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamRotation(This,pVideoProcessor,StreamIndex,pEnable,pRotation) ) 


#define ID3D11VideoContext1_SubmitDecoderBuffers1(This,pDecoder,NumBuffers,pBufferDesc)	\
    ( (This)->lpVtbl -> SubmitDecoderBuffers1(This,pDecoder,NumBuffers,pBufferDesc) ) 

#define ID3D11VideoContext1_GetDataForNewHardwareKey(This,pCryptoSession,PrivateInputSize,pPrivatInputData,pPrivateOutputData)	\
    ( (This)->lpVtbl -> GetDataForNewHardwareKey(This,pCryptoSession,PrivateInputSize,pPrivatInputData,pPrivateOutputData) ) 

#define ID3D11VideoContext1_CheckCryptoSessionStatus(This,pCryptoSession,pStatus)	\
    ( (This)->lpVtbl -> CheckCryptoSessionStatus(This,pCryptoSession,pStatus) ) 

#define ID3D11VideoContext1_DecoderEnableDownsampling(This,pDecoder,InputColorSpace,pOutputDesc,ReferenceFrameCount)	\
    ( (This)->lpVtbl -> DecoderEnableDownsampling(This,pDecoder,InputColorSpace,pOutputDesc,ReferenceFrameCount) ) 

#define ID3D11VideoContext1_DecoderUpdateDownsampling(This,pDecoder,pOutputDesc)	\
    ( (This)->lpVtbl -> DecoderUpdateDownsampling(This,pDecoder,pOutputDesc) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputColorSpace1(This,pVideoProcessor,ColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputColorSpace1(This,pVideoProcessor,ColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorSetOutputShaderUsage(This,pVideoProcessor,ShaderUsage)	\
    ( (This)->lpVtbl -> VideoProcessorSetOutputShaderUsage(This,pVideoProcessor,ShaderUsage) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputColorSpace1(This,pVideoProcessor,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputColorSpace1(This,pVideoProcessor,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorGetOutputShaderUsage(This,pVideoProcessor,pShaderUsage)	\
    ( (This)->lpVtbl -> VideoProcessorGetOutputShaderUsage(This,pVideoProcessor,pShaderUsage) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamColorSpace1(This,pVideoProcessor,StreamIndex,ColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamColorSpace1(This,pVideoProcessor,StreamIndex,ColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorSetStreamMirror(This,pVideoProcessor,StreamIndex,Enable,FlipHorizontal,FlipVertical)	\
    ( (This)->lpVtbl -> VideoProcessorSetStreamMirror(This,pVideoProcessor,StreamIndex,Enable,FlipHorizontal,FlipVertical) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamColorSpace1(This,pVideoProcessor,StreamIndex,pColorSpace)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamColorSpace1(This,pVideoProcessor,StreamIndex,pColorSpace) ) 

#define ID3D11VideoContext1_VideoProcessorGetStreamMirror(This,pVideoProcessor,StreamIndex,pEnable,pFlipHorizontal,pFlipVertical)	\
    ( (This)->lpVtbl -> VideoProcessorGetStreamMirror(This,pVideoProcessor,StreamIndex,pEnable,pFlipHorizontal,pFlipVertical) ) 

#define ID3D11VideoContext1_VideoProcessorGetBehaviorHints(This,pVideoProcessor,OutputWidth,OutputHeight,OutputFormat,StreamCount,pStreams,pBehaviorHints)	\
    ( (This)->lpVtbl -> VideoProcessorGetBehaviorHints(This,pVideoProcessor,OutputWidth,OutputHeight,OutputFormat,StreamCount,pStreams,pBehaviorHints) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11VideoContext1_INTERFACE_DEFINED__ */


#ifndef __ID3D11VideoDevice1_INTERFACE_DEFINED__
#define __ID3D11VideoDevice1_INTERFACE_DEFINED__

/* interface ID3D11VideoDevice1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11VideoDevice1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29DA1D51-1321-4454-804B-F5FC9F861F0F")
    ID3D11VideoDevice1 : public ID3D11VideoDevice
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCryptoSessionPrivateDataSize( 
            /* [annotation] */ 
            _In_  const GUID *pCryptoType,
            /* [annotation] */ 
            _In_opt_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  const GUID *pKeyExchangeType,
            /* [annotation] */ 
            _Out_  UINT *pPrivateInputSize,
            /* [annotation] */ 
            _Out_  UINT *pPrivateOutputSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoDecoderCaps( 
            /* [annotation] */ 
            _In_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  UINT SampleWidth,
            /* [annotation] */ 
            _In_  UINT SampleHeight,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _In_  UINT BitRate,
            /* [annotation] */ 
            _In_opt_  const GUID *pCryptoType,
            /* [annotation] */ 
            _Out_  UINT *pDecoderCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckVideoDecoderDownsampling( 
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pInputDesc,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_CONFIG *pInputConfig,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc,
            /* [annotation] */ 
            _Out_  BOOL *pSupported,
            /* [annotation] */ 
            _Out_  BOOL *pRealTimeHint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RecommendVideoDecoderDownsampleParameters( 
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pInputDesc,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_CONFIG *pInputConfig,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_SAMPLE_DESC *pRecommendedOutputDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11VideoDevice1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11VideoDevice1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11VideoDevice1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11VideoDevice1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoDecoder )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pVideoDesc,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_CONFIG *pConfig,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11VideoDecoder **ppDecoder);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoProcessor )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessorEnumerator *pEnum,
            /* [annotation] */ 
            _In_  UINT RateConversionIndex,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11VideoProcessor **ppVideoProcessor);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAuthenticatedChannel )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  D3D11_AUTHENTICATED_CHANNEL_TYPE ChannelType,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11AuthenticatedChannel **ppAuthenticatedChannel);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCryptoSession )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const GUID *pCryptoType,
            /* [annotation] */ 
            _In_opt_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  const GUID *pKeyExchangeType,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11CryptoSession **ppCryptoSession);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoDecoderOutputView )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VideoDecoderOutputView **ppVDOVView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoProcessorInputView )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessorEnumerator *pEnum,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VideoProcessorInputView **ppVPIView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoProcessorOutputView )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  ID3D11VideoProcessorEnumerator *pEnum,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VideoProcessorOutputView **ppVPOView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoProcessorEnumerator )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_PROCESSOR_CONTENT_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11VideoProcessorEnumerator **ppEnum);
        
        UINT ( STDMETHODCALLTYPE *GetVideoDecoderProfileCount )( 
            ID3D11VideoDevice1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoDecoderProfile )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  UINT Index,
            /* [annotation] */ 
            _Out_  GUID *pDecoderProfile);
        
        HRESULT ( STDMETHODCALLTYPE *CheckVideoDecoderFormat )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  BOOL *pSupported);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoDecoderConfigCount )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pDesc,
            /* [annotation] */ 
            _Out_  UINT *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoDecoderConfig )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pDesc,
            /* [annotation] */ 
            _In_  UINT Index,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_DECODER_CONFIG *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentProtectionCaps )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_opt_  const GUID *pCryptoType,
            /* [annotation] */ 
            _In_opt_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_CONTENT_PROTECTION_CAPS *pCaps);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCryptoKeyExchange )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const GUID *pCryptoType,
            /* [annotation] */ 
            _In_opt_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  UINT Index,
            /* [annotation] */ 
            _Out_  GUID *pKeyExchangeType);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetCryptoSessionPrivateDataSize )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const GUID *pCryptoType,
            /* [annotation] */ 
            _In_opt_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  const GUID *pKeyExchangeType,
            /* [annotation] */ 
            _Out_  UINT *pPrivateInputSize,
            /* [annotation] */ 
            _Out_  UINT *pPrivateOutputSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoDecoderCaps )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const GUID *pDecoderProfile,
            /* [annotation] */ 
            _In_  UINT SampleWidth,
            /* [annotation] */ 
            _In_  UINT SampleHeight,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _In_  UINT BitRate,
            /* [annotation] */ 
            _In_opt_  const GUID *pCryptoType,
            /* [annotation] */ 
            _Out_  UINT *pDecoderCaps);
        
        HRESULT ( STDMETHODCALLTYPE *CheckVideoDecoderDownsampling )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pInputDesc,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_CONFIG *pInputConfig,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_SAMPLE_DESC *pOutputDesc,
            /* [annotation] */ 
            _Out_  BOOL *pSupported,
            /* [annotation] */ 
            _Out_  BOOL *pRealTimeHint);
        
        HRESULT ( STDMETHODCALLTYPE *RecommendVideoDecoderDownsampleParameters )( 
            ID3D11VideoDevice1 * This,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_DESC *pInputDesc,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  const D3D11_VIDEO_DECODER_CONFIG *pInputConfig,
            /* [annotation] */ 
            _In_  const DXGI_RATIONAL *pFrameRate,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_SAMPLE_DESC *pRecommendedOutputDesc);
        
        END_INTERFACE
    } ID3D11VideoDevice1Vtbl;

    interface ID3D11VideoDevice1
    {
        CONST_VTBL struct ID3D11VideoDevice1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11VideoDevice1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11VideoDevice1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11VideoDevice1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11VideoDevice1_CreateVideoDecoder(This,pVideoDesc,pConfig,ppDecoder)	\
    ( (This)->lpVtbl -> CreateVideoDecoder(This,pVideoDesc,pConfig,ppDecoder) ) 

#define ID3D11VideoDevice1_CreateVideoProcessor(This,pEnum,RateConversionIndex,ppVideoProcessor)	\
    ( (This)->lpVtbl -> CreateVideoProcessor(This,pEnum,RateConversionIndex,ppVideoProcessor) ) 

#define ID3D11VideoDevice1_CreateAuthenticatedChannel(This,ChannelType,ppAuthenticatedChannel)	\
    ( (This)->lpVtbl -> CreateAuthenticatedChannel(This,ChannelType,ppAuthenticatedChannel) ) 

#define ID3D11VideoDevice1_CreateCryptoSession(This,pCryptoType,pDecoderProfile,pKeyExchangeType,ppCryptoSession)	\
    ( (This)->lpVtbl -> CreateCryptoSession(This,pCryptoType,pDecoderProfile,pKeyExchangeType,ppCryptoSession) ) 

#define ID3D11VideoDevice1_CreateVideoDecoderOutputView(This,pResource,pDesc,ppVDOVView)	\
    ( (This)->lpVtbl -> CreateVideoDecoderOutputView(This,pResource,pDesc,ppVDOVView) ) 

#define ID3D11VideoDevice1_CreateVideoProcessorInputView(This,pResource,pEnum,pDesc,ppVPIView)	\
    ( (This)->lpVtbl -> CreateVideoProcessorInputView(This,pResource,pEnum,pDesc,ppVPIView) ) 

#define ID3D11VideoDevice1_CreateVideoProcessorOutputView(This,pResource,pEnum,pDesc,ppVPOView)	\
    ( (This)->lpVtbl -> CreateVideoProcessorOutputView(This,pResource,pEnum,pDesc,ppVPOView) ) 

#define ID3D11VideoDevice1_CreateVideoProcessorEnumerator(This,pDesc,ppEnum)	\
    ( (This)->lpVtbl -> CreateVideoProcessorEnumerator(This,pDesc,ppEnum) ) 

#define ID3D11VideoDevice1_GetVideoDecoderProfileCount(This)	\
    ( (This)->lpVtbl -> GetVideoDecoderProfileCount(This) ) 

#define ID3D11VideoDevice1_GetVideoDecoderProfile(This,Index,pDecoderProfile)	\
    ( (This)->lpVtbl -> GetVideoDecoderProfile(This,Index,pDecoderProfile) ) 

#define ID3D11VideoDevice1_CheckVideoDecoderFormat(This,pDecoderProfile,Format,pSupported)	\
    ( (This)->lpVtbl -> CheckVideoDecoderFormat(This,pDecoderProfile,Format,pSupported) ) 

#define ID3D11VideoDevice1_GetVideoDecoderConfigCount(This,pDesc,pCount)	\
    ( (This)->lpVtbl -> GetVideoDecoderConfigCount(This,pDesc,pCount) ) 

#define ID3D11VideoDevice1_GetVideoDecoderConfig(This,pDesc,Index,pConfig)	\
    ( (This)->lpVtbl -> GetVideoDecoderConfig(This,pDesc,Index,pConfig) ) 

#define ID3D11VideoDevice1_GetContentProtectionCaps(This,pCryptoType,pDecoderProfile,pCaps)	\
    ( (This)->lpVtbl -> GetContentProtectionCaps(This,pCryptoType,pDecoderProfile,pCaps) ) 

#define ID3D11VideoDevice1_CheckCryptoKeyExchange(This,pCryptoType,pDecoderProfile,Index,pKeyExchangeType)	\
    ( (This)->lpVtbl -> CheckCryptoKeyExchange(This,pCryptoType,pDecoderProfile,Index,pKeyExchangeType) ) 

#define ID3D11VideoDevice1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11VideoDevice1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11VideoDevice1_GetCryptoSessionPrivateDataSize(This,pCryptoType,pDecoderProfile,pKeyExchangeType,pPrivateInputSize,pPrivateOutputSize)	\
    ( (This)->lpVtbl -> GetCryptoSessionPrivateDataSize(This,pCryptoType,pDecoderProfile,pKeyExchangeType,pPrivateInputSize,pPrivateOutputSize) ) 

#define ID3D11VideoDevice1_GetVideoDecoderCaps(This,pDecoderProfile,SampleWidth,SampleHeight,pFrameRate,BitRate,pCryptoType,pDecoderCaps)	\
    ( (This)->lpVtbl -> GetVideoDecoderCaps(This,pDecoderProfile,SampleWidth,SampleHeight,pFrameRate,BitRate,pCryptoType,pDecoderCaps) ) 

#define ID3D11VideoDevice1_CheckVideoDecoderDownsampling(This,pInputDesc,InputColorSpace,pInputConfig,pFrameRate,pOutputDesc,pSupported,pRealTimeHint)	\
    ( (This)->lpVtbl -> CheckVideoDecoderDownsampling(This,pInputDesc,InputColorSpace,pInputConfig,pFrameRate,pOutputDesc,pSupported,pRealTimeHint) ) 

#define ID3D11VideoDevice1_RecommendVideoDecoderDownsampleParameters(This,pInputDesc,InputColorSpace,pInputConfig,pFrameRate,pRecommendedOutputDesc)	\
    ( (This)->lpVtbl -> RecommendVideoDecoderDownsampleParameters(This,pInputDesc,InputColorSpace,pInputConfig,pFrameRate,pRecommendedOutputDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11VideoDevice1_INTERFACE_DEFINED__ */


#ifndef __ID3D11VideoProcessorEnumerator1_INTERFACE_DEFINED__
#define __ID3D11VideoProcessorEnumerator1_INTERFACE_DEFINED__

/* interface ID3D11VideoProcessorEnumerator1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11VideoProcessorEnumerator1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("465217F2-5568-43CF-B5B9-F61D54531CA1")
    ID3D11VideoProcessorEnumerator1 : public ID3D11VideoProcessorEnumerator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckVideoProcessorFormatConversion( 
            /* [annotation] */ 
            _In_  DXGI_FORMAT InputFormat,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  DXGI_FORMAT OutputFormat,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE OutputColorSpace,
            /* [annotation] */ 
            _Out_  BOOL *pSupported) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11VideoProcessorEnumerator1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11VideoProcessorEnumerator1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11VideoProcessorEnumerator1 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoProcessorContentDesc )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_CONTENT_DESC *pContentDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CheckVideoProcessorFormat )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoProcessorCaps )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_CAPS *pCaps);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoProcessorRateConversionCaps )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  UINT TypeIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS *pCaps);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoProcessorCustomRate )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  UINT TypeIndex,
            /* [annotation] */ 
            _In_  UINT CustomRateIndex,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_CUSTOM_RATE *pRate);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoProcessorFilterRange )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  D3D11_VIDEO_PROCESSOR_FILTER Filter,
            /* [annotation] */ 
            _Out_  D3D11_VIDEO_PROCESSOR_FILTER_RANGE *pRange);
        
        HRESULT ( STDMETHODCALLTYPE *CheckVideoProcessorFormatConversion )( 
            ID3D11VideoProcessorEnumerator1 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT InputFormat,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE InputColorSpace,
            /* [annotation] */ 
            _In_  DXGI_FORMAT OutputFormat,
            /* [annotation] */ 
            _In_  DXGI_COLOR_SPACE_TYPE OutputColorSpace,
            /* [annotation] */ 
            _Out_  BOOL *pSupported);
        
        END_INTERFACE
    } ID3D11VideoProcessorEnumerator1Vtbl;

    interface ID3D11VideoProcessorEnumerator1
    {
        CONST_VTBL struct ID3D11VideoProcessorEnumerator1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11VideoProcessorEnumerator1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11VideoProcessorEnumerator1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11VideoProcessorEnumerator1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11VideoProcessorEnumerator1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11VideoProcessorEnumerator1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11VideoProcessorEnumerator1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11VideoProcessorEnumerator1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11VideoProcessorEnumerator1_GetVideoProcessorContentDesc(This,pContentDesc)	\
    ( (This)->lpVtbl -> GetVideoProcessorContentDesc(This,pContentDesc) ) 

#define ID3D11VideoProcessorEnumerator1_CheckVideoProcessorFormat(This,Format,pFlags)	\
    ( (This)->lpVtbl -> CheckVideoProcessorFormat(This,Format,pFlags) ) 

#define ID3D11VideoProcessorEnumerator1_GetVideoProcessorCaps(This,pCaps)	\
    ( (This)->lpVtbl -> GetVideoProcessorCaps(This,pCaps) ) 

#define ID3D11VideoProcessorEnumerator1_GetVideoProcessorRateConversionCaps(This,TypeIndex,pCaps)	\
    ( (This)->lpVtbl -> GetVideoProcessorRateConversionCaps(This,TypeIndex,pCaps) ) 

#define ID3D11VideoProcessorEnumerator1_GetVideoProcessorCustomRate(This,TypeIndex,CustomRateIndex,pRate)	\
    ( (This)->lpVtbl -> GetVideoProcessorCustomRate(This,TypeIndex,CustomRateIndex,pRate) ) 

#define ID3D11VideoProcessorEnumerator1_GetVideoProcessorFilterRange(This,Filter,pRange)	\
    ( (This)->lpVtbl -> GetVideoProcessorFilterRange(This,Filter,pRange) ) 


#define ID3D11VideoProcessorEnumerator1_CheckVideoProcessorFormatConversion(This,InputFormat,InputColorSpace,OutputFormat,OutputColorSpace,pSupported)	\
    ( (This)->lpVtbl -> CheckVideoProcessorFormatConversion(This,InputFormat,InputColorSpace,OutputFormat,OutputColorSpace,pSupported) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11VideoProcessorEnumerator1_INTERFACE_DEFINED__ */


#ifndef __ID3D11Device1_INTERFACE_DEFINED__
#define __ID3D11Device1_INTERFACE_DEFINED__

/* interface ID3D11Device1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Device1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a04bfb29-08ef-43d6-a49c-a9bdbdcbe686")
    ID3D11Device1 : public ID3D11Device
    {
    public:
        virtual void STDMETHODCALLTYPE GetImmediateContext1( 
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext1( 
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBlendState1( 
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState1( 
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeviceContextState( 
            UINT Flags,
            /* [annotation] */ 
            _In_reads_( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            REFIID EmulatedInterface,
            /* [annotation] */ 
            _Out_opt_  D3D_FEATURE_LEVEL *pChosenFeatureLevel,
            /* [annotation] */ 
            _Out_opt_  ID3DDeviceContextState **ppContextState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSharedResource1( 
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSharedResourceByName( 
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Device1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Device1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Device1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Device1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
            ID3D11Device1 * This,
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
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
            ID3D11Device1 * This,
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
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query **ppQuery);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Counter **ppCounter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
            ID3D11Device1 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
            ID3D11Device1 * This,
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
            ID3D11Device1 * This,
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
            ID3D11Device1 * This);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            ID3D11Device1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
            ID3D11Device1 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
            ID3D11Device1 * This,
            UINT RaiseFlags);
        
        UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
            ID3D11Device1 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
            ID3D11Device1 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
            ID3D11Device1 * This,
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
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
            ID3D11Device1 * This,
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        END_INTERFACE
    } ID3D11Device1Vtbl;

    interface ID3D11Device1
    {
        CONST_VTBL struct ID3D11Device1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Device1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device1_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device1_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device1_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device1_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device1_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device1_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device1_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device1_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device1_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device1_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device1_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device1_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device1_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device1_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device1_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device1_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device1_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device1_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device1_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device1_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device1_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device1_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device1_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device1_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device1_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device1_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device1_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device1_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device1_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device1_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device1_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device1_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device1_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device1_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device1_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device1_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device1_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device1_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device1_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device1_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device1_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device1_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device1_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device1_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device1_INTERFACE_DEFINED__ */


#ifndef __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__
#define __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__

/* interface ID3DUserDefinedAnnotation */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3DUserDefinedAnnotation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b2daad8b-03d4-4dbf-95eb-32ab4b63d0ab")
    ID3DUserDefinedAnnotation : public IUnknown
    {
    public:
        virtual INT STDMETHODCALLTYPE BeginEvent( 
            /* [annotation] */ 
            _In_  LPCWSTR Name) = 0;
        
        virtual INT STDMETHODCALLTYPE EndEvent( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetMarker( 
            /* [annotation] */ 
            _In_  LPCWSTR Name) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetStatus( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3DUserDefinedAnnotationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3DUserDefinedAnnotation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3DUserDefinedAnnotation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3DUserDefinedAnnotation * This);
        
        INT ( STDMETHODCALLTYPE *BeginEvent )( 
            ID3DUserDefinedAnnotation * This,
            /* [annotation] */ 
            _In_  LPCWSTR Name);
        
        INT ( STDMETHODCALLTYPE *EndEvent )( 
            ID3DUserDefinedAnnotation * This);
        
        void ( STDMETHODCALLTYPE *SetMarker )( 
            ID3DUserDefinedAnnotation * This,
            /* [annotation] */ 
            _In_  LPCWSTR Name);
        
        BOOL ( STDMETHODCALLTYPE *GetStatus )( 
            ID3DUserDefinedAnnotation * This);
        
        END_INTERFACE
    } ID3DUserDefinedAnnotationVtbl;

    interface ID3DUserDefinedAnnotation
    {
        CONST_VTBL struct ID3DUserDefinedAnnotationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3DUserDefinedAnnotation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3DUserDefinedAnnotation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3DUserDefinedAnnotation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3DUserDefinedAnnotation_BeginEvent(This,Name)	\
    ( (This)->lpVtbl -> BeginEvent(This,Name) ) 

#define ID3DUserDefinedAnnotation_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3DUserDefinedAnnotation_SetMarker(This,Name)	\
    ( (This)->lpVtbl -> SetMarker(This,Name) ) 

#define ID3DUserDefinedAnnotation_GetStatus(This)	\
    ( (This)->lpVtbl -> GetStatus(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_1_0000_0009 */
/* [local] */ 

DEFINE_GUID(IID_ID3D11BlendState1,0xcc86fabe,0xda55,0x401d,0x85,0xe7,0xe3,0xc9,0xde,0x28,0x77,0xe9);
DEFINE_GUID(IID_ID3D11RasterizerState1,0x1217d7a6,0x5039,0x418c,0xb0,0x42,0x9c,0xbe,0x25,0x6a,0xfd,0x6e);
DEFINE_GUID(IID_ID3DDeviceContextState,0x5c1e0d8a,0x7c23,0x48f9,0x8c,0x59,0xa9,0x29,0x58,0xce,0xff,0x11);
DEFINE_GUID(IID_ID3D11DeviceContext1,0xbb2c6faa,0xb5fb,0x4082,0x8e,0x6b,0x38,0x8b,0x8c,0xfa,0x90,0xe1);
DEFINE_GUID(IID_ID3D11VideoContext1,0xA7F026DA,0xA5F8,0x4487,0xA5,0x64,0x15,0xE3,0x43,0x57,0x65,0x1E);
DEFINE_GUID(IID_ID3D11VideoDevice1,0x29DA1D51,0x1321,0x4454,0x80,0x4B,0xF5,0xFC,0x9F,0x86,0x1F,0x0F);
DEFINE_GUID(IID_ID3D11VideoProcessorEnumerator1,0x465217F2,0x5568,0x43CF,0xB5,0xB9,0xF6,0x1D,0x54,0x53,0x1C,0xA1);
DEFINE_GUID(IID_ID3D11Device1,0xa04bfb29,0x08ef,0x43d6,0xa4,0x9c,0xa9,0xbd,0xbd,0xcb,0xe6,0x86);
DEFINE_GUID(IID_ID3DUserDefinedAnnotation,0xb2daad8b,0x03d4,0x4dbf,0x95,0xeb,0x32,0xab,0x4b,0x63,0xd0,0xab);


extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_1_0000_0009_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


