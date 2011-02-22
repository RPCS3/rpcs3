/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#if SDL_VIDEO_RENDER_D3D && !SDL_RENDER_DISABLED


#include "../../core/windows/SDL_windows.h"

#include "SDL_loadso.h"
#include "SDL_syswm.h"
#include "../SDL_sysrender.h"

#if SDL_VIDEO_RENDER_D3D
#define D3D_DEBUG_INFO
#include <d3d9.h>
#endif

#ifdef ASSEMBLE_SHADER
///////////////////////////////////////////////////////////////////////////
// ID3DXBuffer:
// ------------
// The buffer object is used by D3DX to return arbitrary size data.
//
// GetBufferPointer -
//    Returns a pointer to the beginning of the buffer.
//
// GetBufferSize -
//    Returns the size of the buffer, in bytes.
///////////////////////////////////////////////////////////////////////////

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

typedef interface ID3DXBuffer {
    const struct ID3DXBufferVtbl FAR* lpVtbl;
} ID3DXBuffer;
typedef const struct ID3DXBufferVtbl ID3DXBufferVtbl;
const struct ID3DXBufferVtbl
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
};

HRESULT WINAPI
    D3DXAssembleShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST LPVOID*                   pDefines,
        LPVOID                          pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

#endif /* ASSEMBLE_SHADER */


/* Direct3D renderer implementation */

static SDL_Renderer *D3D_CreateRenderer(SDL_Window * window, Uint32 flags);
static void D3D_WindowEvent(SDL_Renderer * renderer,
                            const SDL_WindowEvent *event);
static int D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, void **pixels, int *pitch);
static void D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D_UpdateViewport(SDL_Renderer * renderer);
static int D3D_RenderClear(SDL_Renderer * renderer);
static int D3D_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int D3D_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int D3D_RenderFillRects(SDL_Renderer * renderer,
                               const SDL_Rect * rects, int count);
static int D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int D3D_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                                Uint32 format, void * pixels, int pitch);
static void D3D_RenderPresent(SDL_Renderer * renderer);
static void D3D_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void D3D_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver D3D_RenderDriver = {
    D3D_CreateRenderer,
    {
     "direct3d",
     (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
     1,
     {SDL_PIXELFORMAT_ARGB8888},
     0,
     0}
};

typedef struct
{
    void* d3dDLL;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    UINT adapter;
    D3DPRESENT_PARAMETERS pparams;
    SDL_bool updateSize;
    SDL_bool beginScene;
} D3D_RenderData;

typedef struct
{
    IDirect3DTexture9 *texture;
} D3D_TextureData;

typedef struct
{
    float x, y, z;
    DWORD color;
    float u, v;
} Vertex;

static void
D3D_SetError(const char *prefix, HRESULT result)
{
    const char *error;

    switch (result) {
    case D3DERR_WRONGTEXTUREFORMAT:
        error = "WRONGTEXTUREFORMAT";
        break;
    case D3DERR_UNSUPPORTEDCOLOROPERATION:
        error = "UNSUPPORTEDCOLOROPERATION";
        break;
    case D3DERR_UNSUPPORTEDCOLORARG:
        error = "UNSUPPORTEDCOLORARG";
        break;
    case D3DERR_UNSUPPORTEDALPHAOPERATION:
        error = "UNSUPPORTEDALPHAOPERATION";
        break;
    case D3DERR_UNSUPPORTEDALPHAARG:
        error = "UNSUPPORTEDALPHAARG";
        break;
    case D3DERR_TOOMANYOPERATIONS:
        error = "TOOMANYOPERATIONS";
        break;
    case D3DERR_CONFLICTINGTEXTUREFILTER:
        error = "CONFLICTINGTEXTUREFILTER";
        break;
    case D3DERR_UNSUPPORTEDFACTORVALUE:
        error = "UNSUPPORTEDFACTORVALUE";
        break;
    case D3DERR_CONFLICTINGRENDERSTATE:
        error = "CONFLICTINGRENDERSTATE";
        break;
    case D3DERR_UNSUPPORTEDTEXTUREFILTER:
        error = "UNSUPPORTEDTEXTUREFILTER";
        break;
    case D3DERR_CONFLICTINGTEXTUREPALETTE:
        error = "CONFLICTINGTEXTUREPALETTE";
        break;
    case D3DERR_DRIVERINTERNALERROR:
        error = "DRIVERINTERNALERROR";
        break;
    case D3DERR_NOTFOUND:
        error = "NOTFOUND";
        break;
    case D3DERR_MOREDATA:
        error = "MOREDATA";
        break;
    case D3DERR_DEVICELOST:
        error = "DEVICELOST";
        break;
    case D3DERR_DEVICENOTRESET:
        error = "DEVICENOTRESET";
        break;
    case D3DERR_NOTAVAILABLE:
        error = "NOTAVAILABLE";
        break;
    case D3DERR_OUTOFVIDEOMEMORY:
        error = "OUTOFVIDEOMEMORY";
        break;
    case D3DERR_INVALIDDEVICE:
        error = "INVALIDDEVICE";
        break;
    case D3DERR_INVALIDCALL:
        error = "INVALIDCALL";
        break;
    case D3DERR_DRIVERINVALIDCALL:
        error = "DRIVERINVALIDCALL";
        break;
    case D3DERR_WASSTILLDRAWING:
        error = "WASSTILLDRAWING";
        break;
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static D3DFORMAT
PixelFormatToD3DFMT(Uint32 format)
{
    switch (format) {
    case SDL_PIXELFORMAT_RGB565:
        return D3DFMT_R5G6B5;
    case SDL_PIXELFORMAT_RGB888:
        return D3DFMT_X8R8G8B8;
    case SDL_PIXELFORMAT_ARGB8888:
        return D3DFMT_A8R8G8B8;
    default:
        return D3DFMT_UNKNOWN;
    }
}

static Uint32
D3DFMTToPixelFormat(D3DFORMAT format)
{
    switch (format) {
    case D3DFMT_R5G6B5:
        return SDL_PIXELFORMAT_RGB565;
    case D3DFMT_X8R8G8B8:
        return SDL_PIXELFORMAT_RGB888;
    case D3DFMT_A8R8G8B8:
        return SDL_PIXELFORMAT_ARGB8888;
    default:
        return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static int
D3D_Reset(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    result = IDirect3DDevice9_Reset(data->device, &data->pparams);
    if (FAILED(result)) {
        if (result == D3DERR_DEVICELOST) {
            /* Don't worry about it, we'll reset later... */
            return 0;
        } else {
            D3D_SetError("Reset()", result);
            return -1;
        }
    }
    IDirect3DDevice9_SetVertexShader(data->device, NULL);
    IDirect3DDevice9_SetFVF(data->device,
                            D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
                                    D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);
    return 0;
}

static int
D3D_ActivateRenderer(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    if (data->updateSize) {
        SDL_Window *window = renderer->window;
        int w, h;

        SDL_GetWindowSize(window, &w, &h);
        data->pparams.BackBufferWidth = w;
        data->pparams.BackBufferHeight = h;
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
            data->pparams.BackBufferFormat =
                PixelFormatToD3DFMT(SDL_GetWindowPixelFormat(window));
        } else {
            data->pparams.BackBufferFormat = D3DFMT_UNKNOWN;
        }
        if (D3D_Reset(renderer) < 0) {
            return -1;
        }
        D3D_UpdateViewport(renderer);

        data->updateSize = SDL_FALSE;
    }
    if (data->beginScene) {
        result = IDirect3DDevice9_BeginScene(data->device);
        if (result == D3DERR_DEVICELOST) {
            if (D3D_Reset(renderer) < 0) {
                return -1;
            }
            result = IDirect3DDevice9_BeginScene(data->device);
        }
        if (FAILED(result)) {
            D3D_SetError("BeginScene()", result);
            return -1;
        }
        data->beginScene = SDL_FALSE;
    }
    return 0;
}

SDL_Renderer *
D3D_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    D3D_RenderData *data;
    SDL_SysWMinfo windowinfo;
    HRESULT result;
    D3DPRESENT_PARAMETERS pparams;
    IDirect3DSwapChain9 *chain;
    D3DCAPS9 caps;
    Uint32 window_flags;
    int w, h;
    SDL_DisplayMode fullscreen_mode;
    D3DMATRIX matrix;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (D3D_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_free(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    data->d3dDLL = SDL_LoadObject("D3D9.DLL");
    if (data->d3dDLL) {
        IDirect3D9 *(WINAPI * D3DCreate) (UINT SDKVersion);

        D3DCreate =
            (IDirect3D9 * (WINAPI *) (UINT)) SDL_LoadFunction(data->d3dDLL,
                                                            "Direct3DCreate9");
        if (D3DCreate) {
            data->d3d = D3DCreate(D3D_SDK_VERSION);
        }
        if (!data->d3d) {
            SDL_UnloadObject(data->d3dDLL);
            data->d3dDLL = NULL;
        }
    }
    if (!data->d3d) {
        SDL_free(renderer);
        SDL_free(data);
        SDL_SetError("Unable to create Direct3D interface");
        return NULL;
    }

    renderer->WindowEvent = D3D_WindowEvent;
    renderer->CreateTexture = D3D_CreateTexture;
    renderer->UpdateTexture = D3D_UpdateTexture;
    renderer->LockTexture = D3D_LockTexture;
    renderer->UnlockTexture = D3D_UnlockTexture;
    renderer->UpdateViewport = D3D_UpdateViewport;
    renderer->RenderClear = D3D_RenderClear;
    renderer->RenderDrawPoints = D3D_RenderDrawPoints;
    renderer->RenderDrawLines = D3D_RenderDrawLines;
    renderer->RenderFillRects = D3D_RenderFillRects;
    renderer->RenderCopy = D3D_RenderCopy;
    renderer->RenderReadPixels = D3D_RenderReadPixels;
    renderer->RenderPresent = D3D_RenderPresent;
    renderer->DestroyTexture = D3D_DestroyTexture;
    renderer->DestroyRenderer = D3D_DestroyRenderer;
    renderer->info = D3D_RenderDriver.info;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    SDL_VERSION(&windowinfo.version);
    SDL_GetWindowWMInfo(window, &windowinfo);

    window_flags = SDL_GetWindowFlags(window);
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowDisplayMode(window, &fullscreen_mode);

    SDL_zero(pparams);
    pparams.hDeviceWindow = windowinfo.info.win.window;
    pparams.BackBufferWidth = w;
    pparams.BackBufferHeight = h;
    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        pparams.BackBufferFormat =
            PixelFormatToD3DFMT(fullscreen_mode.format);
    } else {
        pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    }
    pparams.BackBufferCount = 1;
    pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;

    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        pparams.Windowed = FALSE;
        pparams.FullScreen_RefreshRateInHz =
            fullscreen_mode.refresh_rate;
    } else {
        pparams.Windowed = TRUE;
        pparams.FullScreen_RefreshRateInHz = 0;
    }
    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    } else {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    /* FIXME: Which adapter? */
    data->adapter = D3DADAPTER_DEFAULT;
    IDirect3D9_GetDeviceCaps(data->d3d, data->adapter, D3DDEVTYPE_HAL, &caps);

    result = IDirect3D9_CreateDevice(data->d3d, data->adapter,
                                     D3DDEVTYPE_HAL,
                                     pparams.hDeviceWindow,
                                     (caps.
                                      DevCaps &
                                      D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING :
                                     D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                     &pparams, &data->device);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("CreateDevice()", result);
        return NULL;
    }
    data->beginScene = SDL_TRUE;

    /* Get presentation parameters to fill info */
    result = IDirect3DDevice9_GetSwapChain(data->device, 0, &chain);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetSwapChain()", result);
        return NULL;
    }
    result = IDirect3DSwapChain9_GetPresentParameters(chain, &pparams);
    if (FAILED(result)) {
        IDirect3DSwapChain9_Release(chain);
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetPresentParameters()", result);
        return NULL;
    }
    IDirect3DSwapChain9_Release(chain);
    if (pparams.PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    data->pparams = pparams;

    IDirect3DDevice9_GetDeviceCaps(data->device, &caps);
    renderer->info.max_texture_width = caps.MaxTextureWidth;
    renderer->info.max_texture_height = caps.MaxTextureHeight;

    /* Set up parameters for rendering */
    IDirect3DDevice9_SetVertexShader(data->device, NULL);
    IDirect3DDevice9_SetFVF(data->device,
                            D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_ZENABLE, D3DZB_FALSE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
                                    D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);
    /* Enable color modulation by diffuse color */
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLOROP,
                                          D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG1,
                                          D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG2,
                                          D3DTA_DIFFUSE);
    /* Enable alpha modulation by diffuse alpha */
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAOP,
                                          D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG1,
                                          D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG2,
                                          D3DTA_DIFFUSE);
    /* Disable second texture stage, since we're done */
    IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_COLOROP,
                                          D3DTOP_DISABLE);
    IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_ALPHAOP,
                                          D3DTOP_DISABLE);

    /* Set an identity world and view matrix */
    matrix.m[0][0] = 1.0f;
    matrix.m[0][1] = 0.0f;
    matrix.m[0][2] = 0.0f;
    matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = 0.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[1][2] = 0.0f;
    matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = 0.0f;
    matrix.m[2][1] = 0.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = 0.0f;
    matrix.m[3][1] = 0.0f;
    matrix.m[3][2] = 0.0f;
    matrix.m[3][3] = 1.0f;
    IDirect3DDevice9_SetTransform(data->device, D3DTS_WORLD, &matrix);
    IDirect3DDevice9_SetTransform(data->device, D3DTS_VIEW, &matrix);

    return renderer;
}

static void
D3D_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;

    if (event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        data->updateSize = SDL_TRUE;
    }
}

static int
D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    D3DFORMAT display_format = renderdata->pparams.BackBufferFormat;
    D3D_TextureData *data;
    D3DPOOL pool;
    DWORD usage;
    HRESULT result;

    data = (D3D_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    texture->driverdata = data;

#ifdef USE_DYNAMIC_TEXTURE
    if (texture->access == SDL_TEXTUREACCESS_STREAMING) {
        pool = D3DPOOL_DEFAULT;
        usage = D3DUSAGE_DYNAMIC;
    } else
#endif
    {
        pool = D3DPOOL_MANAGED;
        usage = 0;
    }

    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, usage,
                                       PixelFormatToD3DFMT(texture->format),
                                       pool, &data->texture, NULL);
    if (FAILED(result)) {
        D3D_SetError("CreateTexture()", result);
        return -1;
    }

    return 0;
}

static int
D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    const Uint8 *src;
    Uint8 *dst;
    int row, length;
    HRESULT result;

#ifdef USE_DYNAMIC_TEXTURE
    if (texture->access == SDL_TEXTUREACCESS_STREAMING &&
        rect->x == 0 && rect->y == 0 &&
        rect->w == texture->w && rect->h == texture->h) {
        result = IDirect3DTexture9_LockRect(data->texture, 0, &locked, NULL, D3DLOCK_DISCARD);
    } else
#endif
    {
        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;
        result = IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect, 0);
    }

    if (FAILED(result)) {
        D3D_SetError("LockRect()", result);
        return -1;
    }

    src = pixels;
    dst = locked.pBits;
    length = rect->w * SDL_BYTESPERPIXEL(texture->format);
    if (length == pitch && length == locked.Pitch) {
        SDL_memcpy(dst, src, length*rect->h);
    } else {
        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += pitch;
            dst += locked.Pitch;
        }
    }
    IDirect3DTexture9_UnlockRect(data->texture, 0);

    return 0;
}

static int
D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, void **pixels, int *pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    HRESULT result;

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result = IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect, 0);
    if (FAILED(result)) {
        D3D_SetError("LockRect()", result);
        return -1;
    }
    *pixels = locked.pBits;
    *pitch = locked.Pitch;
    return 0;
}

static void
D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    IDirect3DTexture9_UnlockRect(data->texture, 0);
}

static int
D3D_UpdateViewport(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3DVIEWPORT9 viewport;
    D3DMATRIX matrix;

    /* Set the viewport */
    viewport.X = renderer->viewport.x;
    viewport.Y = renderer->viewport.y;
    viewport.Width = renderer->viewport.w;
    viewport.Height = renderer->viewport.h;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    IDirect3DDevice9_SetViewport(data->device, &viewport);

    /* Set an orthographic projection matrix */
    matrix.m[0][0] = 2.0f / renderer->viewport.w;
    matrix.m[0][1] = 0.0f;
    matrix.m[0][2] = 0.0f;
    matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = 0.0f;
    matrix.m[1][1] = -2.0f / renderer->viewport.h;
    matrix.m[1][2] = 0.0f;
    matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = 0.0f;
    matrix.m[2][1] = 0.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = -1.0f;
    matrix.m[3][1] = 1.0f;
    matrix.m[3][2] = 0.0f;
    matrix.m[3][3] = 1.0f;
    IDirect3DDevice9_SetTransform(data->device, D3DTS_PROJECTION, &matrix);

    return 0;
}

static int
D3D_RenderClear(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    HRESULT result;

    if (D3D_ActivateRenderer(renderer) < 0) {
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    /* Don't reset the viewport if we don't have to! */
    if (!renderer->viewport.x && !renderer->viewport.y &&
        renderer->viewport.w == data->pparams.BackBufferWidth &&
        renderer->viewport.h == data->pparams.BackBufferHeight) {
        result = IDirect3DDevice9_Clear(data->device, 0, NULL, D3DCLEAR_TARGET, color, 0.0f, 0);
    } else {
        D3DVIEWPORT9 viewport;

        /* Clear is defined to clear the entire render target */
        viewport.X = 0;
        viewport.Y = 0;
        viewport.Width = data->pparams.BackBufferWidth;
        viewport.Height = data->pparams.BackBufferHeight;
        viewport.MinZ = 0.0f;
        viewport.MaxZ = 1.0f;
        IDirect3DDevice9_SetViewport(data->device, &viewport);

        result = IDirect3DDevice9_Clear(data->device, 0, NULL, D3DCLEAR_TARGET, color, 0.0f, 0);

        /* Reset the viewport */
        viewport.X = renderer->viewport.x;
        viewport.Y = renderer->viewport.y;
        viewport.Width = renderer->viewport.w;
        viewport.Height = renderer->viewport.h;
        viewport.MinZ = 0.0f;
        viewport.MaxZ = 1.0f;
        IDirect3DDevice9_SetViewport(data->device, &viewport);
    }

    if (FAILED(result)) {
        D3D_SetError("Clear()", result);
        return -1;
    }
    return 0;
}

static void
D3D_SetBlendMode(D3D_RenderData * data, int blendMode)
{
    switch (blendMode) {
    case SDL_BLENDMODE_NONE:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        FALSE);
        break;
    case SDL_BLENDMODE_BLEND:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_INVSRCALPHA);
        break;
    case SDL_BLENDMODE_ADD:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_ONE);
        break;
    case SDL_BLENDMODE_MOD:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_ZERO);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_SRCCOLOR);
        break;
    }
}

static int
D3D_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    Vertex *vertices;
    int i;
    HRESULT result;

    if (D3D_ActivateRenderer(renderer) < 0) {
        return -1;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    vertices = SDL_stack_alloc(Vertex, count);
    for (i = 0; i < count; ++i) {
        vertices[i].x = (float) points[i].x;
        vertices[i].y = (float) points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_POINTLIST, count,
                                         vertices, sizeof(*vertices));
    SDL_stack_free(vertices);
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    return 0;
}

static int
D3D_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    Vertex *vertices;
    int i;
    HRESULT result;

    if (D3D_ActivateRenderer(renderer) < 0) {
        return -1;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    vertices = SDL_stack_alloc(Vertex, count);
    for (i = 0; i < count; ++i) {
        vertices[i].x = (float) points[i].x;
        vertices[i].y = (float) points[i].y;
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_LINESTRIP, count-1,
                                         vertices, sizeof(*vertices));

    /* DirectX 9 has the same line rasterization semantics as GDI,
       so we need to close the endpoint of the line */
    if (points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        vertices[0].x = (float) points[count-1].x;
        vertices[0].y = (float) points[count-1].y;
        result = IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_POINTLIST, 1, vertices, sizeof(*vertices));
    }

    SDL_stack_free(vertices);
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    return 0;
}

static int
D3D_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects,
                    int count)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    DWORD color;
    int i;
    float minx, miny, maxx, maxy;
    Vertex vertices[4];
    HRESULT result;

    if (D3D_ActivateRenderer(renderer) < 0) {
        return -1;
    }

    D3D_SetBlendMode(data, renderer->blendMode);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) 0);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }

    color = D3DCOLOR_ARGB(renderer->a, renderer->r, renderer->g, renderer->b);

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = &rects[i];

        minx = (float) rect->x;
        miny = (float) rect->y;
        maxx = (float) rect->x + rect->w;
        maxy = (float) rect->y + rect->h;

        vertices[0].x = minx;
        vertices[0].y = miny;
        vertices[0].z = 0.0f;
        vertices[0].color = color;
        vertices[0].u = 0.0f;
        vertices[0].v = 0.0f;

        vertices[1].x = maxx;
        vertices[1].y = miny;
        vertices[1].z = 0.0f;
        vertices[1].color = color;
        vertices[1].u = 0.0f;
        vertices[1].v = 0.0f;

        vertices[2].x = maxx;
        vertices[2].y = maxy;
        vertices[2].z = 0.0f;
        vertices[2].color = color;
        vertices[2].u = 0.0f;
        vertices[2].v = 0.0f;

        vertices[3].x = minx;
        vertices[3].y = maxy;
        vertices[3].z = 0.0f;
        vertices[3].color = color;
        vertices[3].u = 0.0f;
        vertices[3].v = 0.0f;

        result =
            IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_TRIANGLEFAN,
                                             2, vertices, sizeof(*vertices));
        if (FAILED(result)) {
            D3D_SetError("DrawPrimitiveUP()", result);
            return -1;
        }
    }
    return 0;
}

static int
D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3D_TextureData *texturedata = (D3D_TextureData *) texture->driverdata;
    LPDIRECT3DPIXELSHADER9 shader = NULL;
    float minx, miny, maxx, maxy;
    float minu, maxu, minv, maxv;
    DWORD color;
    Vertex vertices[4];
    HRESULT result;

    if (D3D_ActivateRenderer(renderer) < 0) {
        return -1;
    }

    minx = (float) dstrect->x - 0.5f;
    miny = (float) dstrect->y - 0.5f;
    maxx = (float) dstrect->x + dstrect->w - 0.5f;
    maxy = (float) dstrect->y + dstrect->h - 0.5f;

    minu = (float) srcrect->x / texture->w;
    maxu = (float) (srcrect->x + srcrect->w) / texture->w;
    minv = (float) srcrect->y / texture->h;
    maxv = (float) (srcrect->y + srcrect->h) / texture->h;

    color = D3DCOLOR_ARGB(texture->a, texture->r, texture->g, texture->b);

    vertices[0].x = minx;
    vertices[0].y = miny;
    vertices[0].z = 0.0f;
    vertices[0].color = color;
    vertices[0].u = minu;
    vertices[0].v = minv;

    vertices[1].x = maxx;
    vertices[1].y = miny;
    vertices[1].z = 0.0f;
    vertices[1].color = color;
    vertices[1].u = maxu;
    vertices[1].v = minv;

    vertices[2].x = maxx;
    vertices[2].y = maxy;
    vertices[2].z = 0.0f;
    vertices[2].color = color;
    vertices[2].u = maxu;
    vertices[2].v = maxv;

    vertices[3].x = minx;
    vertices[3].y = maxy;
    vertices[3].z = 0.0f;
    vertices[3].color = color;
    vertices[3].u = minu;
    vertices[3].v = maxv;

    D3D_SetBlendMode(data, texture->blendMode);

    IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MINFILTER,
                                     D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(data->device, 0, D3DSAMP_MAGFILTER,
                                     D3DTEXF_LINEAR);

    result =
        IDirect3DDevice9_SetTexture(data->device, 0, (IDirect3DBaseTexture9 *)
                                    texturedata->texture);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }
    if (shader) {
        result = IDirect3DDevice9_SetPixelShader(data->device, shader);
        if (FAILED(result)) {
            D3D_SetError("SetShader()", result);
            return -1;
        }
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_TRIANGLEFAN, 2,
                                         vertices, sizeof(*vertices));
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    if (shader) {
        result = IDirect3DDevice9_SetPixelShader(data->device, NULL);
        if (FAILED(result)) {
            D3D_SetError("SetShader()", result);
            return -1;
        }
    }
    return 0;
}

static int
D3D_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3DSURFACE_DESC desc;
    LPDIRECT3DSURFACE9 backBuffer;
    LPDIRECT3DSURFACE9 surface;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    HRESULT result;

    result = IDirect3DDevice9_GetBackBuffer(data->device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    if (FAILED(result)) {
        D3D_SetError("GetBackBuffer()", result);
        return -1;
    }

    result = IDirect3DSurface9_GetDesc(backBuffer, &desc);
    if (FAILED(result)) {
        D3D_SetError("GetDesc()", result);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    result = IDirect3DDevice9_CreateOffscreenPlainSurface(data->device, desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surface, NULL);
    if (FAILED(result)) {
        D3D_SetError("CreateOffscreenPlainSurface()", result);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    result = IDirect3DDevice9_GetRenderTargetData(data->device, backBuffer, surface);
    if (FAILED(result)) {
        D3D_SetError("GetRenderTargetData()", result);
        IDirect3DSurface9_Release(surface);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result = IDirect3DSurface9_LockRect(surface, &locked, &d3drect, D3DLOCK_READONLY);
    if (FAILED(result)) {
        D3D_SetError("LockRect()", result);
        IDirect3DSurface9_Release(surface);
        IDirect3DSurface9_Release(backBuffer);
        return -1;
    }

    SDL_ConvertPixels(rect->w, rect->h,
                      D3DFMTToPixelFormat(desc.Format), locked.pBits, locked.Pitch,
                      format, pixels, pitch);

    IDirect3DSurface9_UnlockRect(surface);

    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(backBuffer);

    return 0;
}

static void
D3D_RenderPresent(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    if (!data->beginScene) {
        IDirect3DDevice9_EndScene(data->device);
        data->beginScene = SDL_TRUE;
    }

    result = IDirect3DDevice9_TestCooperativeLevel(data->device);
    if (result == D3DERR_DEVICELOST) {
        /* We'll reset later */
        return;
    }
    if (result == D3DERR_DEVICENOTRESET) {
        D3D_Reset(renderer);
    }
    result = IDirect3DDevice9_Present(data->device, NULL, NULL, NULL, NULL);
    if (FAILED(result)) {
        D3D_SetError("Present()", result);
    }
}

static void
D3D_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->texture) {
        IDirect3DTexture9_Release(data->texture);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
D3D_DestroyRenderer(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;

    if (data) {
        if (data->device) {
            IDirect3DDevice9_Release(data->device);
        }
        if (data->d3d) {
            IDirect3D9_Release(data->d3d);
            SDL_UnloadObject(data->d3dDLL);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_D3D && !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
