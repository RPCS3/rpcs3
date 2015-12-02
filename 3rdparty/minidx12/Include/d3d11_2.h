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

#ifndef __d3d11_2_h__
#define __d3d11_2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11DeviceContext2_FWD_DEFINED__
#define __ID3D11DeviceContext2_FWD_DEFINED__
typedef interface ID3D11DeviceContext2 ID3D11DeviceContext2;

#endif 	/* __ID3D11DeviceContext2_FWD_DEFINED__ */


#ifndef __ID3D11Device2_FWD_DEFINED__
#define __ID3D11Device2_FWD_DEFINED__
typedef interface ID3D11Device2 ID3D11Device2;

#endif 	/* __ID3D11Device2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgi1_3.h"
#include "d3dcommon.h"
#include "d3d11_1.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d11_2_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d3d11_1.h" //
#ifdef __cplusplus
extern "C"{
#endif
typedef struct D3D11_TILED_RESOURCE_COORDINATE
    {
    UINT X;
    UINT Y;
    UINT Z;
    UINT Subresource;
    } 	D3D11_TILED_RESOURCE_COORDINATE;

typedef struct D3D11_TILE_REGION_SIZE
    {
    UINT NumTiles;
    BOOL bUseBox;
    UINT Width;
    UINT16 Height;
    UINT16 Depth;
    } 	D3D11_TILE_REGION_SIZE;

typedef 
enum D3D11_TILE_MAPPING_FLAG
    {
        D3D11_TILE_MAPPING_NO_OVERWRITE	= 0x1
    } 	D3D11_TILE_MAPPING_FLAG;

typedef 
enum D3D11_TILE_RANGE_FLAG
    {
        D3D11_TILE_RANGE_NULL	= 0x1,
        D3D11_TILE_RANGE_SKIP	= 0x2,
        D3D11_TILE_RANGE_REUSE_SINGLE_TILE	= 0x4
    } 	D3D11_TILE_RANGE_FLAG;

typedef struct D3D11_SUBRESOURCE_TILING
    {
    UINT WidthInTiles;
    UINT16 HeightInTiles;
    UINT16 DepthInTiles;
    UINT StartTileIndexInOverallResource;
    } 	D3D11_SUBRESOURCE_TILING;

#define	D3D11_PACKED_TILE	( 0xffffffff )

typedef struct D3D11_TILE_SHAPE
    {
    UINT WidthInTexels;
    UINT HeightInTexels;
    UINT DepthInTexels;
    } 	D3D11_TILE_SHAPE;

typedef struct D3D11_PACKED_MIP_DESC
    {
    UINT8 NumStandardMips;
    UINT8 NumPackedMips;
    UINT NumTilesForPackedMips;
    UINT StartTileIndexInOverallResource;
    } 	D3D11_PACKED_MIP_DESC;

typedef 
enum D3D11_CHECK_MULTISAMPLE_QUALITY_LEVELS_FLAG
    {
        D3D11_CHECK_MULTISAMPLE_QUALITY_LEVELS_TILED_RESOURCE	= 0x1
    } 	D3D11_CHECK_MULTISAMPLE_QUALITY_LEVELS_FLAG;

typedef 
enum D3D11_TILE_COPY_FLAG
    {
        D3D11_TILE_COPY_NO_OVERWRITE	= 0x1,
        D3D11_TILE_COPY_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE	= 0x2,
        D3D11_TILE_COPY_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER	= 0x4
    } 	D3D11_TILE_COPY_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_d3d11_2_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_2_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11DeviceContext2_INTERFACE_DEFINED__
#define __ID3D11DeviceContext2_INTERFACE_DEFINED__

/* interface ID3D11DeviceContext2 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11DeviceContext2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("420d5b32-b90c-4da4-bef0-359f6a24a83a")
    ID3D11DeviceContext2 : public ID3D11DeviceContext1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateTileMappings( 
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
            _In_  UINT Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyTileMappings( 
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
            _In_  UINT Flags) = 0;
        
        virtual void STDMETHODCALLTYPE CopyTiles( 
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
            _In_  UINT Flags) = 0;
        
        virtual void STDMETHODCALLTYPE UpdateTiles( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pDestTiledResource,
            /* [annotation] */ 
            _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
            /* [annotation] */ 
            _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
            /* [annotation] */ 
            _In_  const void *pSourceTileData,
            /* [annotation] */ 
            _In_  UINT Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeTilePool( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT64 NewSizeInBytes) = 0;
        
        virtual void STDMETHODCALLTYPE TiledResourceBarrier( 
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsAnnotationEnabled( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetMarkerInt( 
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data) = 0;
        
        virtual void STDMETHODCALLTYPE BeginEventInt( 
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data) = 0;
        
        virtual void STDMETHODCALLTYPE EndEvent( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11DeviceContext2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11DeviceContext2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11DeviceContext2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11DeviceContext2 * This);
        
        void ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11Device **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11PixelShader *pPixelShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11VertexShader *pVertexShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DrawIndexed )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  UINT IndexCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation);
        
        void ( STDMETHODCALLTYPE *Draw )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  UINT VertexCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation);
        
        HRESULT ( STDMETHODCALLTYPE *Map )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource);
        
        void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IASetInputLayout )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11InputLayout *pInputLayout);
        
        void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  UINT VertexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation);
        
        void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);
        
        void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *Begin )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        void ( STDMETHODCALLTYPE *End )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags);
        
        void ( STDMETHODCALLTYPE *SetPredication )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue);
        
        void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11BlendState *pBlendState,
            /* [annotation] */ 
            _In_opt_  const FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _In_  UINT SampleMask);
        
        void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
            /* [annotation] */ 
            _In_  UINT StencilRef);
        
        void ( STDMETHODCALLTYPE *SOSetTargets )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets);
        
        void ( STDMETHODCALLTYPE *DrawAuto )( 
            ID3D11DeviceContext2 * This);
        
        void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *Dispatch )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountX,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountY,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountZ);
        
        void ( STDMETHODCALLTYPE *DispatchIndirect )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs);
        
        void ( STDMETHODCALLTYPE *RSSetState )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11RasterizerState *pRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSSetViewports )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            /* [annotation] */ 
            _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource);
        
        void ( STDMETHODCALLTYPE *UpdateSubresource )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pDstBuffer,
            /* [annotation] */ 
            _In_  UINT DstAlignedByteOffset,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pSrcView);
        
        void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11RenderTargetView *pRenderTargetView,
            /* [annotation] */ 
            _In_  const FLOAT ColorRGBA[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const UINT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const FLOAT Values[ 4 ]);
        
        void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_  UINT ClearFlags,
            /* [annotation] */ 
            _In_  FLOAT Depth,
            /* [annotation] */ 
            _In_  UINT8 Stencil);
        
        void ( STDMETHODCALLTYPE *GenerateMips )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11ShaderResourceView *pShaderResourceView);
        
        void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            FLOAT MinLOD);
        
        FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *ResolveSubresource )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11CommandList *pCommandList,
            BOOL RestoreContextState);
        
        void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11HullShader *pHullShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DomainShader *pDomainShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);
        
        void ( STDMETHODCALLTYPE *CSSetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11ComputeShader *pComputeShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSSetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *PSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *VSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);
        
        void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
            /* [annotation] */ 
            _Out_opt_  DXGI_FORMAT *Format,
            /* [annotation] */ 
            _Out_opt_  UINT *Offset);
        
        void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *GSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);
        
        void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *VSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *GetPredication )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
            /* [annotation] */ 
            _Out_opt_  BOOL *pPredicateValue);
        
        void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *GSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);
        
        void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
            /* [annotation] */ 
            _Out_opt_  FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _Out_opt_  UINT *pSampleMask);
        
        void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef);
        
        void ( STDMETHODCALLTYPE *SOGetTargets )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);
        
        void ( STDMETHODCALLTYPE *RSGetState )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);
        
        void ( STDMETHODCALLTYPE *RSGetViewports )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);
        
        void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);
        
        void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *HSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *HSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *DSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *DSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);
        
        void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
        void ( STDMETHODCALLTYPE *CSGetShader )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances);
        
        void ( STDMETHODCALLTYPE *CSGetSamplers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);
        
        void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);
        
        void ( STDMETHODCALLTYPE *ClearState )( 
            ID3D11DeviceContext2 * This);
        
        void ( STDMETHODCALLTYPE *Flush )( 
            ID3D11DeviceContext2 * This);
        
        D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
            ID3D11DeviceContext2 * This);
        
        UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
            ID3D11DeviceContext2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
            ID3D11DeviceContext2 * This,
            BOOL RestoreDeferredContextState,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);
        
        void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource);
        
        void ( STDMETHODCALLTYPE *DiscardView )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView);
        
        void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3DDeviceContextState *pState,
            /* [annotation] */ 
            _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);
        
        void ( STDMETHODCALLTYPE *ClearView )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pView,
            /* [annotation] */ 
            _In_  const FLOAT Color[ 4 ],
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
            UINT NumRects);
        
        void ( STDMETHODCALLTYPE *DiscardView1 )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11View *pResourceView,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
            UINT NumRects);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateTileMappings )( 
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
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
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  ID3D11Buffer *pTilePool,
            /* [annotation] */ 
            _In_  UINT64 NewSizeInBytes);
        
        void ( STDMETHODCALLTYPE *TiledResourceBarrier )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
            /* [annotation] */ 
            _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier);
        
        BOOL ( STDMETHODCALLTYPE *IsAnnotationEnabled )( 
            ID3D11DeviceContext2 * This);
        
        void ( STDMETHODCALLTYPE *SetMarkerInt )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *BeginEventInt )( 
            ID3D11DeviceContext2 * This,
            /* [annotation] */ 
            _In_  LPCWSTR pLabel,
            INT Data);
        
        void ( STDMETHODCALLTYPE *EndEvent )( 
            ID3D11DeviceContext2 * This);
        
        END_INTERFACE
    } ID3D11DeviceContext2Vtbl;

    interface ID3D11DeviceContext2
    {
        CONST_VTBL struct ID3D11DeviceContext2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11DeviceContext2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext2_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext2_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext2_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext2_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext2_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext2_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext2_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext2_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext2_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext2_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext2_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext2_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext2_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext2_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext2_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext2_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext2_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext2_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext2_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext2_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext2_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext2_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext2_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext2_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext2_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext2_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext2_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext2_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext2_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext2_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext2_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext2_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext2_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext2_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext2_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext2_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext2_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext2_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext2_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext2_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext2_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext2_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext2_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext2_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext2_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext2_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext2_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext2_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext2_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext2_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext2_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext2_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext2_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext2_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext2_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext2_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext2_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext2_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext2_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext2_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext2_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext2_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext2_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext2_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext2_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext2_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext2_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext2_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext2_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext2_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext2_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext2_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext2_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext2_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 


#define ID3D11DeviceContext2_UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D11DeviceContext2_CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags) ) 

#define ID3D11DeviceContext2_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D11DeviceContext2_UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags)	\
    ( (This)->lpVtbl -> UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags) ) 

#define ID3D11DeviceContext2_ResizeTilePool(This,pTilePool,NewSizeInBytes)	\
    ( (This)->lpVtbl -> ResizeTilePool(This,pTilePool,NewSizeInBytes) ) 

#define ID3D11DeviceContext2_TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier)	\
    ( (This)->lpVtbl -> TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier) ) 

#define ID3D11DeviceContext2_IsAnnotationEnabled(This)	\
    ( (This)->lpVtbl -> IsAnnotationEnabled(This) ) 

#define ID3D11DeviceContext2_SetMarkerInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> SetMarkerInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext2_BeginEventInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> BeginEventInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext2_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext2_INTERFACE_DEFINED__ */


#ifndef __ID3D11Device2_INTERFACE_DEFINED__
#define __ID3D11Device2_INTERFACE_DEFINED__

/* interface ID3D11Device2 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11Device2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9d06dffa-d1e5-4d07-83a8-1bb123f2f841")
    ID3D11Device2 : public ID3D11Device1
    {
    public:
        virtual void STDMETHODCALLTYPE GetImmediateContext2( 
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext2 **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext2( 
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext) = 0;
        
        virtual void STDMETHODCALLTYPE GetResourceTiling( 
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
            _Out_writes_(*pNumSubresourceTilings)  D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels1( 
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11Device2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11Device2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11Device2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11Device2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);
        
        HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
            ID3D11Device2 * This,
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
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
            ID3D11Device2 * This,
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
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Query **ppQuery);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11Counter **ppCounter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
            ID3D11Device2 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _COM_Outptr_opt_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
            ID3D11Device2 * This,
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
            ID3D11Device2 * This,
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
            ID3D11Device2 * This);
        
        UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
            ID3D11Device2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
            ID3D11Device2 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
            ID3D11Device2 * This,
            UINT RaiseFlags);
        
        UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
            ID3D11Device2 * This);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
            ID3D11Device2 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
            ID3D11Device2 * This,
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
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _COM_Outptr_  void **ppResource);
        
        void ( STDMETHODCALLTYPE *GetImmediateContext2 )( 
            ID3D11Device2 * This,
            /* [annotation] */ 
            _Outptr_  ID3D11DeviceContext2 **ppImmediateContext);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext2 )( 
            ID3D11Device2 * This,
            UINT ContextFlags,
            /* [annotation] */ 
            _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext);
        
        void ( STDMETHODCALLTYPE *GetResourceTiling )( 
            ID3D11Device2 * This,
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
            ID3D11Device2 * This,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels);
        
        END_INTERFACE
    } ID3D11Device2Vtbl;

    interface ID3D11Device2
    {
        CONST_VTBL struct ID3D11Device2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11Device2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device2_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device2_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device2_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device2_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device2_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device2_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device2_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device2_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device2_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device2_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device2_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device2_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device2_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device2_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device2_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device2_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device2_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device2_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device2_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device2_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device2_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device2_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device2_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device2_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device2_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device2_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device2_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device2_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device2_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device2_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device2_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device2_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device2_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device2_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device2_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device2_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device2_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device2_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device2_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device2_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device2_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device2_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device2_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device2_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 


#define ID3D11Device2_GetImmediateContext2(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext2(This,ppImmediateContext) ) 

#define ID3D11Device2_CreateDeferredContext2(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext2(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device2_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D11Device2_CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11_2_0000_0002 */
/* [local] */ 

DEFINE_GUID(IID_ID3D11DeviceContext2,0x420d5b32,0xb90c,0x4da4,0xbe,0xf0,0x35,0x9f,0x6a,0x24,0xa8,0x3a);
DEFINE_GUID(IID_ID3D11Device2,0x9d06dffa,0xd1e5,0x4d07,0x83,0xa8,0x1b,0xb1,0x23,0xf2,0xf8,0x41);


extern RPC_IF_HANDLE __MIDL_itf_d3d11_2_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11_2_0000_0002_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


