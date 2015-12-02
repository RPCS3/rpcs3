

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

#ifndef __dxgi1_2_h__
#define __dxgi1_2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIDisplayControl_FWD_DEFINED__
#define __IDXGIDisplayControl_FWD_DEFINED__
typedef interface IDXGIDisplayControl IDXGIDisplayControl;

#endif 	/* __IDXGIDisplayControl_FWD_DEFINED__ */


#ifndef __IDXGIOutputDuplication_FWD_DEFINED__
#define __IDXGIOutputDuplication_FWD_DEFINED__
typedef interface IDXGIOutputDuplication IDXGIOutputDuplication;

#endif 	/* __IDXGIOutputDuplication_FWD_DEFINED__ */


#ifndef __IDXGISurface2_FWD_DEFINED__
#define __IDXGISurface2_FWD_DEFINED__
typedef interface IDXGISurface2 IDXGISurface2;

#endif 	/* __IDXGISurface2_FWD_DEFINED__ */


#ifndef __IDXGIResource1_FWD_DEFINED__
#define __IDXGIResource1_FWD_DEFINED__
typedef interface IDXGIResource1 IDXGIResource1;

#endif 	/* __IDXGIResource1_FWD_DEFINED__ */


#ifndef __IDXGIDevice2_FWD_DEFINED__
#define __IDXGIDevice2_FWD_DEFINED__
typedef interface IDXGIDevice2 IDXGIDevice2;

#endif 	/* __IDXGIDevice2_FWD_DEFINED__ */


#ifndef __IDXGISwapChain1_FWD_DEFINED__
#define __IDXGISwapChain1_FWD_DEFINED__
typedef interface IDXGISwapChain1 IDXGISwapChain1;

#endif 	/* __IDXGISwapChain1_FWD_DEFINED__ */


#ifndef __IDXGIFactory2_FWD_DEFINED__
#define __IDXGIFactory2_FWD_DEFINED__
typedef interface IDXGIFactory2 IDXGIFactory2;

#endif 	/* __IDXGIFactory2_FWD_DEFINED__ */


#ifndef __IDXGIAdapter2_FWD_DEFINED__
#define __IDXGIAdapter2_FWD_DEFINED__
typedef interface IDXGIAdapter2 IDXGIAdapter2;

#endif 	/* __IDXGIAdapter2_FWD_DEFINED__ */


#ifndef __IDXGIOutput1_FWD_DEFINED__
#define __IDXGIOutput1_FWD_DEFINED__
typedef interface IDXGIOutput1 IDXGIOutput1;

#endif 	/* __IDXGIOutput1_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_2_0000_0000 */
/* [local] */ 

#include <winapifamily.h>
#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIDisplayControl_INTERFACE_DEFINED__
#define __IDXGIDisplayControl_INTERFACE_DEFINED__

/* interface IDXGIDisplayControl */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDisplayControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ea9dbf1a-c88e-4486-854a-98aa0138f30c")
    IDXGIDisplayControl : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsStereoEnabled( void) = 0;
        
        virtual void STDMETHODCALLTYPE SetStereoEnabled( 
            BOOL enabled) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDisplayControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDisplayControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDisplayControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDisplayControl * This);
        
        BOOL ( STDMETHODCALLTYPE *IsStereoEnabled )( 
            IDXGIDisplayControl * This);
        
        void ( STDMETHODCALLTYPE *SetStereoEnabled )( 
            IDXGIDisplayControl * This,
            BOOL enabled);
        
        END_INTERFACE
    } IDXGIDisplayControlVtbl;

    interface IDXGIDisplayControl
    {
        CONST_VTBL struct IDXGIDisplayControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDisplayControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDisplayControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDisplayControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDisplayControl_IsStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsStereoEnabled(This) ) 

#define IDXGIDisplayControl_SetStereoEnabled(This,enabled)	\
    ( (This)->lpVtbl -> SetStereoEnabled(This,enabled) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDisplayControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0001 */
/* [local] */ 

typedef struct DXGI_OUTDUPL_MOVE_RECT
    {
    POINT SourcePoint;
    RECT DestinationRect;
    } 	DXGI_OUTDUPL_MOVE_RECT;

typedef struct DXGI_OUTDUPL_DESC
    {
    DXGI_MODE_DESC ModeDesc;
    DXGI_MODE_ROTATION Rotation;
    BOOL DesktopImageInSystemMemory;
    } 	DXGI_OUTDUPL_DESC;

typedef struct DXGI_OUTDUPL_POINTER_POSITION
    {
    POINT Position;
    BOOL Visible;
    } 	DXGI_OUTDUPL_POINTER_POSITION;

typedef 
enum DXGI_OUTDUPL_POINTER_SHAPE_TYPE
    {
        DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME	= 0x1,
        DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR	= 0x2,
        DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR	= 0x4
    } 	DXGI_OUTDUPL_POINTER_SHAPE_TYPE;

typedef struct DXGI_OUTDUPL_POINTER_SHAPE_INFO
    {
    UINT Type;
    UINT Width;
    UINT Height;
    UINT Pitch;
    POINT HotSpot;
    } 	DXGI_OUTDUPL_POINTER_SHAPE_INFO;

typedef struct DXGI_OUTDUPL_FRAME_INFO
    {
    LARGE_INTEGER LastPresentTime;
    LARGE_INTEGER LastMouseUpdateTime;
    UINT AccumulatedFrames;
    BOOL RectsCoalesced;
    BOOL ProtectedContentMaskedOut;
    DXGI_OUTDUPL_POINTER_POSITION PointerPosition;
    UINT TotalMetadataBufferSize;
    UINT PointerShapeBufferSize;
    } 	DXGI_OUTDUPL_FRAME_INFO;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutputDuplication_INTERFACE_DEFINED__
#define __IDXGIOutputDuplication_INTERFACE_DEFINED__

/* interface IDXGIOutputDuplication */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutputDuplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("191cfac3-a341-470d-b26e-a864f428319c")
    IDXGIOutputDuplication : public IDXGIObject
    {
    public:
        virtual void STDMETHODCALLTYPE GetDesc( 
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_DESC *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AcquireNextFrame( 
            /* [annotation][in] */ 
            _In_  UINT TimeoutInMilliseconds,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_FRAME_INFO *pFrameInfo,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIResource **ppDesktopResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameDirtyRects( 
            /* [annotation][in] */ 
            _In_  UINT DirtyRectsBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(DirtyRectsBufferSize, *pDirtyRectsBufferSizeRequired)  RECT *pDirtyRectsBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pDirtyRectsBufferSizeRequired) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameMoveRects( 
            /* [annotation][in] */ 
            _In_  UINT MoveRectsBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(MoveRectsBufferSize, *pMoveRectsBufferSizeRequired)  DXGI_OUTDUPL_MOVE_RECT *pMoveRectBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pMoveRectsBufferSizeRequired) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFramePointerShape( 
            /* [annotation][in] */ 
            _In_  UINT PointerShapeBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(PointerShapeBufferSize, *pPointerShapeBufferSizeRequired)  void *pPointerShapeBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pPointerShapeBufferSizeRequired,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_POINTER_SHAPE_INFO *pPointerShapeInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapDesktopSurface( 
            /* [annotation][out] */ 
            _Out_  DXGI_MAPPED_RECT *pLockedRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnMapDesktopSurface( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseFrame( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutputDuplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutputDuplication * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutputDuplication * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutputDuplication * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        void ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutputDuplication * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *AcquireNextFrame )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  UINT TimeoutInMilliseconds,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_FRAME_INFO *pFrameInfo,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIResource **ppDesktopResource);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameDirtyRects )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  UINT DirtyRectsBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(DirtyRectsBufferSize, *pDirtyRectsBufferSizeRequired)  RECT *pDirtyRectsBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pDirtyRectsBufferSizeRequired);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameMoveRects )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  UINT MoveRectsBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(MoveRectsBufferSize, *pMoveRectsBufferSizeRequired)  DXGI_OUTDUPL_MOVE_RECT *pMoveRectBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pMoveRectsBufferSizeRequired);
        
        HRESULT ( STDMETHODCALLTYPE *GetFramePointerShape )( 
            IDXGIOutputDuplication * This,
            /* [annotation][in] */ 
            _In_  UINT PointerShapeBufferSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_to_(PointerShapeBufferSize, *pPointerShapeBufferSizeRequired)  void *pPointerShapeBuffer,
            /* [annotation][out] */ 
            _Out_  UINT *pPointerShapeBufferSizeRequired,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTDUPL_POINTER_SHAPE_INFO *pPointerShapeInfo);
        
        HRESULT ( STDMETHODCALLTYPE *MapDesktopSurface )( 
            IDXGIOutputDuplication * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MAPPED_RECT *pLockedRect);
        
        HRESULT ( STDMETHODCALLTYPE *UnMapDesktopSurface )( 
            IDXGIOutputDuplication * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseFrame )( 
            IDXGIOutputDuplication * This);
        
        END_INTERFACE
    } IDXGIOutputDuplicationVtbl;

    interface IDXGIOutputDuplication
    {
        CONST_VTBL struct IDXGIOutputDuplicationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutputDuplication_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutputDuplication_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutputDuplication_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutputDuplication_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutputDuplication_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutputDuplication_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutputDuplication_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutputDuplication_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutputDuplication_AcquireNextFrame(This,TimeoutInMilliseconds,pFrameInfo,ppDesktopResource)	\
    ( (This)->lpVtbl -> AcquireNextFrame(This,TimeoutInMilliseconds,pFrameInfo,ppDesktopResource) ) 

#define IDXGIOutputDuplication_GetFrameDirtyRects(This,DirtyRectsBufferSize,pDirtyRectsBuffer,pDirtyRectsBufferSizeRequired)	\
    ( (This)->lpVtbl -> GetFrameDirtyRects(This,DirtyRectsBufferSize,pDirtyRectsBuffer,pDirtyRectsBufferSizeRequired) ) 

#define IDXGIOutputDuplication_GetFrameMoveRects(This,MoveRectsBufferSize,pMoveRectBuffer,pMoveRectsBufferSizeRequired)	\
    ( (This)->lpVtbl -> GetFrameMoveRects(This,MoveRectsBufferSize,pMoveRectBuffer,pMoveRectsBufferSizeRequired) ) 

#define IDXGIOutputDuplication_GetFramePointerShape(This,PointerShapeBufferSize,pPointerShapeBuffer,pPointerShapeBufferSizeRequired,pPointerShapeInfo)	\
    ( (This)->lpVtbl -> GetFramePointerShape(This,PointerShapeBufferSize,pPointerShapeBuffer,pPointerShapeBufferSizeRequired,pPointerShapeInfo) ) 

#define IDXGIOutputDuplication_MapDesktopSurface(This,pLockedRect)	\
    ( (This)->lpVtbl -> MapDesktopSurface(This,pLockedRect) ) 

#define IDXGIOutputDuplication_UnMapDesktopSurface(This)	\
    ( (This)->lpVtbl -> UnMapDesktopSurface(This) ) 

#define IDXGIOutputDuplication_ReleaseFrame(This)	\
    ( (This)->lpVtbl -> ReleaseFrame(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutputDuplication_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0002 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
typedef 
enum DXGI_ALPHA_MODE
    {
        DXGI_ALPHA_MODE_UNSPECIFIED	= 0,
        DXGI_ALPHA_MODE_PREMULTIPLIED	= 1,
        DXGI_ALPHA_MODE_STRAIGHT	= 2,
        DXGI_ALPHA_MODE_IGNORE	= 3,
        DXGI_ALPHA_MODE_FORCE_DWORD	= 0xffffffff
    } 	DXGI_ALPHA_MODE;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0002_v0_0_s_ifspec;

#ifndef __IDXGISurface2_INTERFACE_DEFINED__
#define __IDXGISurface2_INTERFACE_DEFINED__

/* interface IDXGISurface2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISurface2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aba496dd-b617-4cb8-a866-bc44d7eb1fa2")
    IDXGISurface2 : public IDXGISurface1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetResource( 
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppParentResource,
            /* [annotation][out] */ 
            _Out_  UINT *pSubresourceIndex) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISurface2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISurface2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISurface2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISurface2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISurface2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SURFACE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *Map )( 
            IDXGISurface2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MAPPED_RECT *pLockedRect,
            /* [in] */ UINT MapFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Unmap )( 
            IDXGISurface2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDC )( 
            IDXGISurface2 * This,
            /* [in] */ BOOL Discard,
            /* [annotation][out] */ 
            _Out_  HDC *phdc);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseDC )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_opt_  RECT *pDirtyRect);
        
        HRESULT ( STDMETHODCALLTYPE *GetResource )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppParentResource,
            /* [annotation][out] */ 
            _Out_  UINT *pSubresourceIndex);
        
        END_INTERFACE
    } IDXGISurface2Vtbl;

    interface IDXGISurface2
    {
        CONST_VTBL struct IDXGISurface2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISurface2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISurface2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISurface2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISurface2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISurface2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISurface2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISurface2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISurface2_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISurface2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISurface2_Map(This,pLockedRect,MapFlags)	\
    ( (This)->lpVtbl -> Map(This,pLockedRect,MapFlags) ) 

#define IDXGISurface2_Unmap(This)	\
    ( (This)->lpVtbl -> Unmap(This) ) 


#define IDXGISurface2_GetDC(This,Discard,phdc)	\
    ( (This)->lpVtbl -> GetDC(This,Discard,phdc) ) 

#define IDXGISurface2_ReleaseDC(This,pDirtyRect)	\
    ( (This)->lpVtbl -> ReleaseDC(This,pDirtyRect) ) 


#define IDXGISurface2_GetResource(This,riid,ppParentResource,pSubresourceIndex)	\
    ( (This)->lpVtbl -> GetResource(This,riid,ppParentResource,pSubresourceIndex) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISurface2_INTERFACE_DEFINED__ */


#ifndef __IDXGIResource1_INTERFACE_DEFINED__
#define __IDXGIResource1_INTERFACE_DEFINED__

/* interface IDXGIResource1 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIResource1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30961379-4609-4a41-998e-54fe567ee0c1")
    IDXGIResource1 : public IDXGIResource
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSubresourceSurface( 
            UINT index,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface2 **ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle( 
            /* [annotation][in] */ 
            _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
            /* [annotation][in] */ 
            _In_  DWORD dwAccess,
            /* [annotation][in] */ 
            _In_opt_  LPCWSTR lpName,
            /* [annotation][out] */ 
            _Out_  HANDLE *pHandle) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIResource1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIResource1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIResource1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIResource1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetSharedHandle )( 
            IDXGIResource1 * This,
            /* [annotation][out] */ 
            _Out_  HANDLE *pSharedHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetUsage )( 
            IDXGIResource1 * This,
            /* [out] */ DXGI_USAGE *pUsage);
        
        HRESULT ( STDMETHODCALLTYPE *SetEvictionPriority )( 
            IDXGIResource1 * This,
            /* [in] */ UINT EvictionPriority);
        
        HRESULT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
            IDXGIResource1 * This,
            /* [annotation][retval][out] */ 
            _Out_  UINT *pEvictionPriority);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSubresourceSurface )( 
            IDXGIResource1 * This,
            UINT index,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface2 **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
            /* [annotation][in] */ 
            _In_  DWORD dwAccess,
            /* [annotation][in] */ 
            _In_opt_  LPCWSTR lpName,
            /* [annotation][out] */ 
            _Out_  HANDLE *pHandle);
        
        END_INTERFACE
    } IDXGIResource1Vtbl;

    interface IDXGIResource1
    {
        CONST_VTBL struct IDXGIResource1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIResource1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIResource1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIResource1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIResource1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIResource1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIResource1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIResource1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIResource1_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGIResource1_GetSharedHandle(This,pSharedHandle)	\
    ( (This)->lpVtbl -> GetSharedHandle(This,pSharedHandle) ) 

#define IDXGIResource1_GetUsage(This,pUsage)	\
    ( (This)->lpVtbl -> GetUsage(This,pUsage) ) 

#define IDXGIResource1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define IDXGIResource1_GetEvictionPriority(This,pEvictionPriority)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This,pEvictionPriority) ) 


#define IDXGIResource1_CreateSubresourceSurface(This,index,ppSurface)	\
    ( (This)->lpVtbl -> CreateSubresourceSurface(This,index,ppSurface) ) 

#define IDXGIResource1_CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIResource1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0004 */
/* [local] */ 

typedef 
enum _DXGI_OFFER_RESOURCE_PRIORITY
    {
        DXGI_OFFER_RESOURCE_PRIORITY_LOW	= 1,
        DXGI_OFFER_RESOURCE_PRIORITY_NORMAL	= ( DXGI_OFFER_RESOURCE_PRIORITY_LOW + 1 ) ,
        DXGI_OFFER_RESOURCE_PRIORITY_HIGH	= ( DXGI_OFFER_RESOURCE_PRIORITY_NORMAL + 1 ) 
    } 	DXGI_OFFER_RESOURCE_PRIORITY;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0004_v0_0_s_ifspec;

#ifndef __IDXGIDevice2_INTERFACE_DEFINED__
#define __IDXGIDevice2_INTERFACE_DEFINED__

/* interface IDXGIDevice2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDevice2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05008617-fbfd-4051-a790-144884b4f6a9")
    IDXGIDevice2 : public IDXGIDevice1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OfferResources( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReclaimResources( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnqueueSetEvent( 
            /* [annotation][in] */ 
            _In_  HANDLE hEvent) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDevice2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDevice2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDevice2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDevice2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice2 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
            IDXGIDevice2 * This,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IUnknown *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
            /* [in] */ UINT NumResources);
        
        HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
            IDXGIDevice2 * This,
            /* [in] */ INT Priority);
        
        HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
            IDXGIDevice2 * This,
            /* [annotation][retval][out] */ 
            _Out_  INT *pPriority);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGIDevice2 * This,
            /* [in] */ UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGIDevice2 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);
        
        HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent);
        
        END_INTERFACE
    } IDXGIDevice2Vtbl;

    interface IDXGIDevice2
    {
        CONST_VTBL struct IDXGIDevice2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDevice2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice2_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice2_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice2_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice2_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice2_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice2_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice2_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice2_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice2_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice2_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0005 */
/* [local] */ 

#define	DXGI_ENUM_MODES_STEREO	( 4UL )

#define	DXGI_ENUM_MODES_DISABLED_STEREO	( 8UL )

#define	DXGI_SHARED_RESOURCE_READ	( 0x80000000L )

#define	DXGI_SHARED_RESOURCE_WRITE	( 1 )

typedef struct DXGI_MODE_DESC1
    {
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
    BOOL Stereo;
    } 	DXGI_MODE_DESC1;

typedef 
enum DXGI_SCALING
    {
        DXGI_SCALING_STRETCH	= 0,
        DXGI_SCALING_NONE	= 1,
        DXGI_SCALING_ASPECT_RATIO_STRETCH	= 2
    } 	DXGI_SCALING;

typedef struct DXGI_SWAP_CHAIN_DESC1
    {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    BOOL Stereo;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    DXGI_SCALING Scaling;
    DXGI_SWAP_EFFECT SwapEffect;
    DXGI_ALPHA_MODE AlphaMode;
    UINT Flags;
    } 	DXGI_SWAP_CHAIN_DESC1;

typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC
    {
    DXGI_RATIONAL RefreshRate;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
    BOOL Windowed;
    } 	DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

typedef struct DXGI_PRESENT_PARAMETERS
    {
    UINT DirtyRectsCount;
    /* [annotation] */ 
    _Field_size_full_opt_(DirtyRectsCount)  RECT *pDirtyRects;
    RECT *pScrollRect;
    POINT *pScrollOffset;
    } 	DXGI_PRESENT_PARAMETERS;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0005_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0005_v0_0_s_ifspec;

#ifndef __IDXGISwapChain1_INTERFACE_DEFINED__
#define __IDXGISwapChain1_INTERFACE_DEFINED__

/* interface IDXGISwapChain1 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("790a45f7-0d42-4876-983a-0a55cfe6f4aa")
    IDXGISwapChain1 : public IDXGISwapChain
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc1( 
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc( 
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHwnd( 
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCoreWindow( 
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Present1( 
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput( 
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor( 
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor( 
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRotation( 
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRotation( 
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain1 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain1 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain1 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain1 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain1 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        END_INTERFACE
    } IDXGISwapChain1Vtbl;

    interface IDXGISwapChain1
    {
        CONST_VTBL struct IDXGISwapChain1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain1_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain1_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain1_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain1_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain1_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain1_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain1_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain1_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain1_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain1_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain1_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain1_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain1_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain1_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain1_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain1_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain1_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain1_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain1_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain1_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain1_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactory2_INTERFACE_DEFINED__
#define __IDXGIFactory2_INTERFACE_DEFINED__

/* interface IDXGIFactory2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIFactory2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
    IDXGIFactory2 : public IDXGIFactory1
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd( 
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  IUnknown *pWindow,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid( 
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _Out_  LUID *pLuid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow( 
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent( 
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual void STDMETHODCALLTYPE UnregisterStereoStatus( 
            /* [annotation][in] */ 
            _In_  DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow( 
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent( 
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie) = 0;
        
        virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus( 
            /* [annotation][in] */ 
            _In_  DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIFactory2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIFactory2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIFactory2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIFactory2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory2 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
            IDXGIFactory2 * This,
            HWND WindowHandle,
            UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
            IDXGIFactory2 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pWindowHandle);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory2 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory2 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
        BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
            IDXGIFactory2 * This);
        
        BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
            IDXGIFactory2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
            IDXGIFactory2 * This,
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
            IDXGIFactory2 * This,
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
            IDXGIFactory2 * This,
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _Out_  LUID *pLuid);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  HWND WindowHandle,
            /* [annotation][in] */ 
            _In_  UINT wMsg,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent,
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCookie);
        
        void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
            IDXGIFactory2 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        END_INTERFACE
    } IDXGIFactory2Vtbl;

    interface IDXGIFactory2
    {
        CONST_VTBL struct IDXGIFactory2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIFactory2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory2_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory2_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory2_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory2_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory2_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory2_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory2_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory2_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory2_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0007 */
/* [local] */ 

typedef 
enum DXGI_GRAPHICS_PREEMPTION_GRANULARITY
    {
        DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY	= 0,
        DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY	= 1,
        DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY	= 2,
        DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY	= 3,
        DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY	= 4
    } 	DXGI_GRAPHICS_PREEMPTION_GRANULARITY;

typedef 
enum DXGI_COMPUTE_PREEMPTION_GRANULARITY
    {
        DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY	= 0,
        DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY	= 1,
        DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY	= 2,
        DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY	= 3,
        DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY	= 4
    } 	DXGI_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct DXGI_ADAPTER_DESC2
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
    UINT Flags;
    DXGI_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    DXGI_COMPUTE_PREEMPTION_GRANULARITY ComputePreemptionGranularity;
    } 	DXGI_ADAPTER_DESC2;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0007_v0_0_s_ifspec;

#ifndef __IDXGIAdapter2_INTERFACE_DEFINED__
#define __IDXGIAdapter2_INTERFACE_DEFINED__

/* interface IDXGIAdapter2 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIAdapter2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0AA1AE0A-FA0E-4B84-8644-E05FF8E5ACB5")
    IDXGIAdapter2 : public IDXGIAdapter1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc2( 
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC2 *pDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIAdapter2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIAdapter2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIAdapter2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIAdapter2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIAdapter2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIAdapter2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIAdapter2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIAdapter2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
            IDXGIAdapter2 * This,
            /* [in] */ UINT Output,
            /* [annotation][out][in] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIAdapter2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
            IDXGIAdapter2 * This,
            /* [annotation][in] */ 
            _In_  REFGUID InterfaceName,
            /* [annotation][out] */ 
            _Out_  LARGE_INTEGER *pUMDVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGIAdapter2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc2 )( 
            IDXGIAdapter2 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC2 *pDesc);
        
        END_INTERFACE
    } IDXGIAdapter2Vtbl;

    interface IDXGIAdapter2
    {
        CONST_VTBL struct IDXGIAdapter2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIAdapter2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter2_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter2_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 


#define IDXGIAdapter2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define IDXGIAdapter2_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter2_INTERFACE_DEFINED__ */


#ifndef __IDXGIOutput1_INTERFACE_DEFINED__
#define __IDXGIOutput1_INTERFACE_DEFINED__

/* interface IDXGIOutput1 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00cddea8-939b-4b83-a340-a685226666cc")
    IDXGIOutput1 : public IDXGIOutput
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList1( 
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode1( 
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData1( 
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DuplicateOutput( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput1 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput1 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput1 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput1 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        END_INTERFACE
    } IDXGIOutput1Vtbl;

    interface IDXGIOutput1
    {
        CONST_VTBL struct IDXGIOutput1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput1_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput1_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput1_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput1_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput1_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput1_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput1_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput1_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput1_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput1_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput1_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput1_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput1_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput1_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput1_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput1_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_2_0000_0009 */
/* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
DEFINE_GUID(IID_IDXGIDisplayControl,0xea9dbf1a,0xc88e,0x4486,0x85,0x4a,0x98,0xaa,0x01,0x38,0xf3,0x0c);
DEFINE_GUID(IID_IDXGIOutputDuplication,0x191cfac3,0xa341,0x470d,0xb2,0x6e,0xa8,0x64,0xf4,0x28,0x31,0x9c);
DEFINE_GUID(IID_IDXGISurface2,0xaba496dd,0xb617,0x4cb8,0xa8,0x66,0xbc,0x44,0xd7,0xeb,0x1f,0xa2);
DEFINE_GUID(IID_IDXGIResource1,0x30961379,0x4609,0x4a41,0x99,0x8e,0x54,0xfe,0x56,0x7e,0xe0,0xc1);
DEFINE_GUID(IID_IDXGIDevice2,0x05008617,0xfbfd,0x4051,0xa7,0x90,0x14,0x48,0x84,0xb4,0xf6,0xa9);
DEFINE_GUID(IID_IDXGISwapChain1,0x790a45f7,0x0d42,0x4876,0x98,0x3a,0x0a,0x55,0xcf,0xe6,0xf4,0xaa);
DEFINE_GUID(IID_IDXGIFactory2,0x50c83a1c,0xe072,0x4c48,0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0);
DEFINE_GUID(IID_IDXGIAdapter2,0x0AA1AE0A,0xFA0E,0x4B84,0x86,0x44,0xE0,0x5F,0xF8,0xE5,0xAC,0xB5);
DEFINE_GUID(IID_IDXGIOutput1,0x00cddea8,0x939b,0x4b83,0xa3,0x40,0xa6,0x85,0x22,0x66,0x66,0xcc);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0009_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


