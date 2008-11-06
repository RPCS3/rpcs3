//------------------------------------------------------------------------------
// File: AMVideo.h
//
// Desc: Video related definitions and interfaces for ActiveMovie.
//
// Copyright (c) 1992 - 2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __AMVIDEO__
#define __AMVIDEO__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <ddraw.h>


// This is an interface on the video renderer that provides information about
// DirectDraw with respect to its use by the renderer. For example it allows
// an application to get details of the surface and any hardware capabilities
// that are available. It also allows someone to adjust the surfaces that the
// renderer should use and furthermore even set the DirectDraw instance. We
// allow someone to set the DirectDraw instance because DirectDraw can only
// be opened once per process so it helps resolve conflicts. There is some
// duplication in this interface as the hardware/emulated/FOURCCs available
// can all be found through the IDirectDraw interface, this interface allows
// simple access to that information without calling the DirectDraw provider
// itself. The AMDDS prefix is ActiveMovie DirectDraw Switches abbreviated.

#define AMDDS_NONE 0x00             // No use for DCI/DirectDraw
#define AMDDS_DCIPS 0x01            // Use DCI primary surface
#define AMDDS_PS 0x02               // Use DirectDraw primary
#define AMDDS_RGBOVR 0x04           // RGB overlay surfaces
#define AMDDS_YUVOVR 0x08           // YUV overlay surfaces
#define AMDDS_RGBOFF 0x10           // RGB offscreen surfaces
#define AMDDS_YUVOFF 0x20           // YUV offscreen surfaces
#define AMDDS_RGBFLP 0x40           // RGB flipping surfaces
#define AMDDS_YUVFLP 0x80           // YUV flipping surfaces
#define AMDDS_ALL 0xFF              // ALL the previous flags
#define AMDDS_DEFAULT AMDDS_ALL     // Use all available surfaces

#define AMDDS_YUV (AMDDS_YUVOFF | AMDDS_YUVOVR | AMDDS_YUVFLP)
#define AMDDS_RGB (AMDDS_RGBOFF | AMDDS_RGBOVR | AMDDS_RGBFLP)
#define AMDDS_PRIMARY (AMDDS_DCIPS | AMDDS_PS)

// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IDirectDrawVideo

DECLARE_INTERFACE_(IDirectDrawVideo, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IDirectDrawVideo methods

    STDMETHOD(GetSwitches)(THIS_ DWORD *pSwitches) PURE;
    STDMETHOD(SetSwitches)(THIS_ DWORD Switches) PURE;
    STDMETHOD(GetCaps)(THIS_ DDCAPS *pCaps) PURE;
    STDMETHOD(GetEmulatedCaps)(THIS_ DDCAPS *pCaps) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ DDSURFACEDESC *pSurfaceDesc) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_ DWORD *pCount,DWORD *pCodes) PURE;
    STDMETHOD(SetDirectDraw)(THIS_ LPDIRECTDRAW pDirectDraw) PURE;
    STDMETHOD(GetDirectDraw)(THIS_ LPDIRECTDRAW *ppDirectDraw) PURE;
    STDMETHOD(GetSurfaceType)(THIS_ DWORD *pSurfaceType) PURE;
    STDMETHOD(SetDefault)(THIS) PURE;
    STDMETHOD(UseScanLine)(THIS_ long UseScanLine) PURE;
    STDMETHOD(CanUseScanLine)(THIS_ long *UseScanLine) PURE;
    STDMETHOD(UseOverlayStretch)(THIS_ long UseOverlayStretch) PURE;
    STDMETHOD(CanUseOverlayStretch)(THIS_ long *UseOverlayStretch) PURE;
    STDMETHOD(UseWhenFullScreen)(THIS_ long UseWhenFullScreen) PURE;
    STDMETHOD(WillUseFullScreen)(THIS_ long *UseWhenFullScreen) PURE;
};


// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IQualProp

DECLARE_INTERFACE_(IQualProp, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // Compare these with the functions in class CGargle in gargle.h

    STDMETHOD(get_FramesDroppedInRenderer)(THIS_ int *pcFrames) PURE;  // Out
    STDMETHOD(get_FramesDrawn)(THIS_ int *pcFramesDrawn) PURE;         // Out
    STDMETHOD(get_AvgFrameRate)(THIS_ int *piAvgFrameRate) PURE;       // Out
    STDMETHOD(get_Jitter)(THIS_ int *iJitter) PURE;                    // Out
    STDMETHOD(get_AvgSyncOffset)(THIS_ int *piAvg) PURE;               // Out
    STDMETHOD(get_DevSyncOffset)(THIS_ int *piDev) PURE;               // Out
};


// This interface allows an application or plug in distributor to control a
// full screen renderer. The Modex renderer supports this interface. When
// connected a renderer should load the display modes it has available
// The number of modes available can be obtained through CountModes. Then
// information on each individual mode is available by calling GetModeInfo
// and IsModeAvailable. An application may enable and disable any modes
// by calling the SetEnabled flag with OATRUE or OAFALSE (not C/C++ TRUE
// and FALSE values) - the current value may be queried by IsModeEnabled

// A more generic way of setting the modes enabled that is easier to use
// when writing applications is the clip loss factor. This defines the
// amount of video that can be lost when deciding which display mode to
// use. Assuming the decoder cannot compress the video then playing an
// MPEG file (say 352x288) into a 320x200 display will lose about 25% of
// the image. The clip loss factor specifies the upper range permissible.
// To allow typical MPEG video to be played in 320x200 it defaults to 25%

// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IFullScreenVideo

DECLARE_INTERFACE_(IFullScreenVideo, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IFullScreenVideo methods

    STDMETHOD(CountModes)(THIS_ long *pModes) PURE;
    STDMETHOD(GetModeInfo)(THIS_ long Mode,long *pWidth,long *pHeight,long *pDepth) PURE;
    STDMETHOD(GetCurrentMode)(THIS_ long *pMode) PURE;
    STDMETHOD(IsModeAvailable)(THIS_ long Mode) PURE;
    STDMETHOD(IsModeEnabled)(THIS_ long Mode) PURE;
    STDMETHOD(SetEnabled)(THIS_ long Mode,long bEnabled) PURE;
    STDMETHOD(GetClipFactor)(THIS_ long *pClipFactor) PURE;
    STDMETHOD(SetClipFactor)(THIS_ long ClipFactor) PURE;
    STDMETHOD(SetMessageDrain)(THIS_ HWND hwnd) PURE;
    STDMETHOD(GetMessageDrain)(THIS_ HWND *hwnd) PURE;
    STDMETHOD(SetMonitor)(THIS_ long Monitor) PURE;
    STDMETHOD(GetMonitor)(THIS_ long *Monitor) PURE;
    STDMETHOD(HideOnDeactivate)(THIS_ long Hide) PURE;
    STDMETHOD(IsHideOnDeactivate)(THIS) PURE;
    STDMETHOD(SetCaption)(THIS_ BSTR strCaption) PURE;
    STDMETHOD(GetCaption)(THIS_ BSTR *pstrCaption) PURE;
    STDMETHOD(SetDefault)(THIS) PURE;
};


// This adds the accelerator table capabilities in fullscreen. This is being
// added between the original runtime release and the full SDK release. We
// cannot just add the method to IFullScreenVideo as we don't want to force
// applications to have to ship the ActiveMovie support DLLs - this is very
// important to applications that plan on being downloaded over the Internet

// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IFullScreenVideoEx

DECLARE_INTERFACE_(IFullScreenVideoEx, IFullScreenVideo)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IFullScreenVideo methods

    STDMETHOD(CountModes)(THIS_ long *pModes) PURE;
    STDMETHOD(GetModeInfo)(THIS_ long Mode,long *pWidth,long *pHeight,long *pDepth) PURE;
    STDMETHOD(GetCurrentMode)(THIS_ long *pMode) PURE;
    STDMETHOD(IsModeAvailable)(THIS_ long Mode) PURE;
    STDMETHOD(IsModeEnabled)(THIS_ long Mode) PURE;
    STDMETHOD(SetEnabled)(THIS_ long Mode,long bEnabled) PURE;
    STDMETHOD(GetClipFactor)(THIS_ long *pClipFactor) PURE;
    STDMETHOD(SetClipFactor)(THIS_ long ClipFactor) PURE;
    STDMETHOD(SetMessageDrain)(THIS_ HWND hwnd) PURE;
    STDMETHOD(GetMessageDrain)(THIS_ HWND *hwnd) PURE;
    STDMETHOD(SetMonitor)(THIS_ long Monitor) PURE;
    STDMETHOD(GetMonitor)(THIS_ long *Monitor) PURE;
    STDMETHOD(HideOnDeactivate)(THIS_ long Hide) PURE;
    STDMETHOD(IsHideOnDeactivate)(THIS) PURE;
    STDMETHOD(SetCaption)(THIS_ BSTR strCaption) PURE;
    STDMETHOD(GetCaption)(THIS_ BSTR *pstrCaption) PURE;
    STDMETHOD(SetDefault)(THIS) PURE;

    // IFullScreenVideoEx

    STDMETHOD(SetAcceleratorTable)(THIS_ HWND hwnd,HACCEL hAccel) PURE;
    STDMETHOD(GetAcceleratorTable)(THIS_ HWND *phwnd,HACCEL *phAccel) PURE;
    STDMETHOD(KeepPixelAspectRatio)(THIS_ long KeepAspect) PURE;
    STDMETHOD(IsKeepPixelAspectRatio)(THIS_ long *pKeepAspect) PURE;
};


// The SDK base classes contain a base video mixer class. Video mixing in a
// software environment is tricky because we typically have multiple streams
// each sending data at unpredictable times. To work with this we defined a
// pin that is the lead pin, when data arrives on this pin we do a mix. As
// an alternative we may not want to have a lead pin but output samples at
// predefined spaces, like one every 1/15 of a second, this interfaces also
// supports that mode of operations (there is a working video mixer sample)

// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IBaseVideoMixer

DECLARE_INTERFACE_(IBaseVideoMixer, IUnknown)
{
    STDMETHOD(SetLeadPin)(THIS_ int iPin) PURE;
    STDMETHOD(GetLeadPin)(THIS_ int *piPin) PURE;
    STDMETHOD(GetInputPinCount)(THIS_ int *piPinCount) PURE;
    STDMETHOD(IsUsingClock)(THIS_ int *pbValue) PURE;
    STDMETHOD(SetUsingClock)(THIS_ int bValue) PURE;
    STDMETHOD(GetClockPeriod)(THIS_ int *pbValue) PURE;
    STDMETHOD(SetClockPeriod)(THIS_ int bValue) PURE;
};

#define iPALETTE_COLORS 256     // Maximum colours in palette
#define iEGA_COLORS 16          // Number colours in EGA palette
#define iMASK_COLORS 3          // Maximum three components
#define iTRUECOLOR 16           // Minimum true colour device
#define iRED 0                  // Index position for RED mask
#define iGREEN 1                // Index position for GREEN mask
#define iBLUE 2                 // Index position for BLUE mask
#define iPALETTE 8              // Maximum colour depth using a palette
#define iMAXBITS 8              // Maximum bits per colour component


// Used for true colour images that also have a palette

typedef struct tag_TRUECOLORINFO {
    DWORD   dwBitMasks[iMASK_COLORS];
    RGBQUAD bmiColors[iPALETTE_COLORS];
} TRUECOLORINFO;


// The BITMAPINFOHEADER contains all the details about the video stream such
// as the actual image dimensions and their pixel depth. A source filter may
// also request that the sink take only a section of the video by providing a
// clipping rectangle in rcSource. In the worst case where the sink filter
// forgets to check this on connection it will simply render the whole thing
// which isn't a disaster. Ideally a sink filter will check the rcSource and
// if it doesn't support image extraction and the rectangle is not empty then
// it will reject the connection. A filter should use SetRectEmpty to reset a
// rectangle to all zeroes (and IsRectEmpty to later check the rectangle).
// The rcTarget specifies the destination rectangle for the video, for most
// source filters they will set this to all zeroes, a downstream filter may
// request that the video be placed in a particular area of the buffers it
// supplies in which case it will call QueryAccept with a non empty target

typedef struct tagVIDEOINFOHEADER {

    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

    BITMAPINFOHEADER bmiHeader;

} VIDEOINFOHEADER;

// make sure the pbmi is initialized before using these macros
#define TRUECOLOR(pbmi)  ((TRUECOLORINFO *)(((LPBYTE)&((pbmi)->bmiHeader)) \
					+ (pbmi)->bmiHeader.biSize))
#define COLORS(pbmi)	((RGBQUAD *)(((LPBYTE)&((pbmi)->bmiHeader)) 	\
					+ (pbmi)->bmiHeader.biSize))
#define BITMASKS(pbmi)	((DWORD *)(((LPBYTE)&((pbmi)->bmiHeader)) 	\
					+ (pbmi)->bmiHeader.biSize))

// All the image based filters use this to communicate their media types. It's
// centred principally around the BITMAPINFO. This structure always contains a
// BITMAPINFOHEADER followed by a number of other fields depending on what the
// BITMAPINFOHEADER contains. If it contains details of a palettised format it
// will be followed by one or more RGBQUADs defining the palette. If it holds
// details of a true colour format then it may be followed by a set of three
// DWORD bit masks that specify where the RGB data can be found in the image
// (For more information regarding BITMAPINFOs see the Win32 documentation)

// The rcSource and rcTarget fields are not for use by filters supplying the
// data. The destination (target) rectangle should be set to all zeroes. The
// source may also be zero filled or set with the dimensions of the video. So
// if the video is 352x288 pixels then set it to (0,0,352,288). These fields
// are mainly used by downstream filters that want to ask the source filter
// to place the image in a different position in an output buffer. So when
// using for example the primary surface the video renderer may ask a filter
// to place the video images in a destination position of (100,100,452,388)
// on the display since that's where the window is positioned on the display

// !!! WARNING !!!
// DO NOT use this structure unless you are sure that the BITMAPINFOHEADER
// has a normal biSize == sizeof(BITMAPINFOHEADER) !
// !!! WARNING !!!

typedef struct tagVIDEOINFO {

    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

    BITMAPINFOHEADER bmiHeader;

    union {
        RGBQUAD         bmiColors[iPALETTE_COLORS];     // Colour palette
        DWORD           dwBitMasks[iMASK_COLORS];       // True colour masks
        TRUECOLORINFO   TrueColorInfo;                  // Both of the above
    };

} VIDEOINFO;

// These macros define some standard bitmap format sizes

#define SIZE_EGA_PALETTE (iEGA_COLORS * sizeof(RGBQUAD))
#define SIZE_PALETTE (iPALETTE_COLORS * sizeof(RGBQUAD))
#define SIZE_MASKS (iMASK_COLORS * sizeof(DWORD))
#define SIZE_PREHEADER (FIELD_OFFSET(VIDEOINFOHEADER,bmiHeader))
#define SIZE_VIDEOHEADER (sizeof(BITMAPINFOHEADER) + SIZE_PREHEADER)
// !!! for abnormal biSizes
// #define SIZE_VIDEOHEADER(pbmi) ((pbmi)->bmiHeader.biSize + SIZE_PREHEADER)

// DIBSIZE calculates the number of bytes required by an image

#define WIDTHBYTES(bits) ((DWORD)(((bits)+31) & (~31)) / 8)
#define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((DWORD)(bi).biWidth * (DWORD)(bi).biBitCount)
#define _DIBSIZE(bi) (DIBWIDTHBYTES(bi) * (DWORD)(bi).biHeight)
#define DIBSIZE(bi) ((bi).biHeight < 0 ? (-1)*(_DIBSIZE(bi)) : _DIBSIZE(bi))

// This compares the bit masks between two VIDEOINFOHEADERs

#define BIT_MASKS_MATCH(pbmi1,pbmi2)                                \
    (((pbmi1)->dwBitMasks[iRED] == (pbmi2)->dwBitMasks[iRED]) &&        \
     ((pbmi1)->dwBitMasks[iGREEN] == (pbmi2)->dwBitMasks[iGREEN]) &&    \
     ((pbmi1)->dwBitMasks[iBLUE] == (pbmi2)->dwBitMasks[iBLUE]))

// These zero fill different parts of the VIDEOINFOHEADER structure

// Only use these macros for pbmi's with a normal BITMAPINFOHEADER biSize
#define RESET_MASKS(pbmi) (ZeroMemory((PVOID)(pbmi)->dwBitFields,SIZE_MASKS))
#define RESET_HEADER(pbmi) (ZeroMemory((PVOID)(pbmi),SIZE_VIDEOHEADER))
#define RESET_PALETTE(pbmi) (ZeroMemory((PVOID)(pbmi)->bmiColors,SIZE_PALETTE));

#if 0
// !!! This is the right way to do it, but may break existing code
#define RESET_MASKS(pbmi) (ZeroMemory((PVOID)(((LPBYTE)(pbmi)->bmiHeader) + \
			(pbmi)->bmiHeader.biSize,SIZE_MASKS)))
#define RESET_HEADER(pbmi) (ZeroMemory((PVOID)(pbmi), SIZE_PREHEADER +	    \
			sizeof(BITMAPINFOHEADER)))
#define RESET_PALETTE(pbmi) (ZeroMemory((PVOID)(((LPBYTE)(pbmi)->bmiHeader) + \
			(pbmi)->bmiHeader.biSize,SIZE_PALETTE))
#endif

// Other (hopefully) useful bits and bobs

#define PALETTISED(pbmi) ((pbmi)->bmiHeader.biBitCount <= iPALETTE)
#define PALETTE_ENTRIES(pbmi) ((DWORD) 1 << (pbmi)->bmiHeader.biBitCount)

// Returns the address of the BITMAPINFOHEADER from the VIDEOINFOHEADER
#define HEADER(pVideoInfo) (&(((VIDEOINFOHEADER *) (pVideoInfo))->bmiHeader))


// MPEG variant - includes a DWORD length followed by the
// video sequence header after the video header.
//
// The sequence header includes the sequence header start code and the
// quantization matrices associated with the first sequence header in the
// stream so is a maximum of 140 bytes long.

typedef struct tagMPEG1VIDEOINFO {

    VIDEOINFOHEADER hdr;                    // Compatible with VIDEOINFO
    DWORD           dwStartTimeCode;        // 25-bit Group of pictures time code
                                            // at start of data
    DWORD           cbSequenceHeader;       // Length in bytes of bSequenceHeader
    BYTE            bSequenceHeader[1];     // Sequence header including
                                            // quantization matrices if any
} MPEG1VIDEOINFO;

#define MAX_SIZE_MPEG1_SEQUENCE_INFO 140
#define SIZE_MPEG1VIDEOINFO(pv) (FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader[0]) + (pv)->cbSequenceHeader)
#define MPEG1_SEQUENCE_INFO(pv) ((const BYTE *)(pv)->bSequenceHeader)


// Analog video variant - Use this when the format is FORMAT_AnalogVideo
//
// rcSource defines the portion of the active video signal to use
// rcTarget defines the destination rectangle
//    both of the above are relative to the dwActiveWidth and dwActiveHeight fields
// dwActiveWidth is currently set to 720 for all formats (but could change for HDTV)
// dwActiveHeight is 483 for NTSC and 575 for PAL/SECAM  (but could change for HDTV)

typedef struct tagAnalogVideoInfo {
    RECT            rcSource;           // Width max is 720, height varies w/ TransmissionStd
    RECT            rcTarget;           // Where the video should go
    DWORD           dwActiveWidth;      // Always 720 (CCIR-601 active samples per line)
    DWORD           dwActiveHeight;     // 483 for NTSC, 575 for PAL/SECAM
    REFERENCE_TIME  AvgTimePerFrame;    // Normal ActiveMovie units (100 nS)
} ANALOGVIDEOINFO;

//
// AM_KSPROPSETID_FrameStep property set definitions
//
typedef enum {
        //  Step
	AM_PROPERTY_FRAMESTEP_STEP   = 0x01,
	AM_PROPERTY_FRAMESTEP_CANCEL = 0x02,

        //  S_OK for these 2 means we can - S_FALSE if we can't
        AM_PROPERTY_FRAMESTEP_CANSTEP = 0x03,
        AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE = 0x04
} AM_PROPERTY_FRAMESTEP;

typedef struct _AM_FRAMESTEP_STEP
{
    //  1 means step 1 frame forward
    //  0 is invalid
    //  n (n > 1) means skip n - 1 frames and show the nth
    DWORD dwFramesToStep;
} AM_FRAMESTEP_STEP;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __AMVIDEO__

