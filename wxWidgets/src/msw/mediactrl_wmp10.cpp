/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/mediactrl_wmp10.cpp
// Purpose:     Windows Media Player 9/10 Media Backend for Windows
// Author:      Ryan Norton <wxprojects@comcast.net>
// Modified by:
// Created:     11/07/04
// RCS-ID:      $Id: mediactrl_wmp10.cpp 67183 2011-03-14 10:27:12Z JS $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

//-----------------Introduction----------------------------------------------
// This backend is for Desktops with either WMP 9 or 10 and Windows
// mobile (5.0, some 2003 SmartPhones etc.) WMP 10 only (9 has no automation
// interface but you can hack it with message hacks to play through
// a currently running instance of media player).
//
// There are quite a few WMP 10 interfaces and unlike other media
// backends we actually set it to automatically play files as it has
// as huge caveat that you cannot (technically) obtain media information
// from IWMPMedia including duration and video size until a media file
// has literally started to play. There is a hack (and indeed we have
// it within this file) to enable duration getting from a non-playing
// file, but there is no hack I (RN) know of to get the video size from
// a file that isn't playing.
//
// The workaround for this is to send the wxEVT_MEDIA_LOADED when the file
// is about to be played - and if the user didn't change the state of the
// media (m_bWasStateChanged), when set it back to the stop state.
//
// The ActiveX control itself is particularily stubborn, calling
// IOleInPlaceSite::OnPosRectChange every file change trying to set itself
// to something different then what we told it to before.
//
// The docs are at
// http://msdn.microsoft.com/library/en-us/wmplay10/mmp_sdk/windowsmediaplayer10sdk.asp

//===========================================================================
//  DECLARATIONS
//===========================================================================

//---------------------------------------------------------------------------
// Pre-compiled header stuff
//---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_MEDIACTRL && wxUSE_ACTIVEX

#include "wx/mediactrl.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
#endif

#include "wx/msw/private.h" // user info and wndproc setting/getting
#include "wx/msw/ole/activex.h" // wxActiveXContainer - COM-specific stuff

//---------------------------------------------------------------------------
// ATL Includes - define WXTEST_ATL if you
// want to use CAxWindow instead of wxActiveXContainer (note that
// this is mainly for testing as the activex events arn't implemented here)
//---------------------------------------------------------------------------
#if 0
    #define WXTEST_ATL
#endif

#ifdef WXTEST_ATL
    #include <atlbase.h>
    CComModule _Module;
    #define min(x,y) (x < y ? x : y)
    #include <atlcom.h>
    #include <atlhost.h>
    #include <atlctl.h>
#endif

//---------------------------------------------------------------------------
// Other defines
//---------------------------------------------------------------------------

// disable "cast truncates constant value" for VARIANT_BOOL values
// passed as parameters in VC6
#ifdef _MSC_VER
#pragma warning (disable:4310)
#endif

// error logger for HRESULTS (nothing really now)
#define wxWMP10LOG(x)

//---------------------------------------------------------------------------
// Various definitions dumped from wmp.IDL
//---------------------------------------------------------------------------

// CLSID_WMP10ALT is on CE and in some MS docs - on others it is the plain ver
const CLSID CLSID_WMP10              = {0x6BF52A50,0x394A,0x11D3,{0xB1,0x53,0x00,0xC0,0x4F,0x79,0xFA,0xA6}};
const CLSID CLSID_WMP10ALT           = {0x6BF52A52,0x394A,0x11D3,{0xB1,0x53,0x00,0xC0,0x4F,0x79,0xFA,0xA6}};

const IID IID_IWMPSettings = {0x9104D1AB,0x80C9,0x4FED,{0xAB,0xF0,0x2E,0x64,0x17,0xA6,0xDF,0x14}};
const IID IID_IWMPCore = {0xD84CCA99,0xCCE2,0x11D2,{0x9E,0xCC,0x00,0x00,0xF8,0x08,0x59,0x81}};
#ifndef WXTEST_ATL
    const IID IID_IWMPPlayer = {0x6BF52A4F,0x394A,0x11D3,{0xB1,0x53,0x00,0xC0,0x4F,0x79,0xFA,0xA6}};
#endif

const IID IID_IWMPMedia = {0x94D55E95,0x3FAC,0x11D3,{0xB1,0x55,0x00,0xC0,0x4F,0x79,0xFA,0xA6}};
const IID IID_IWMPControls = {0x74C09E02,0xF828,0x11D2,{0xA7,0x4B,0x00,0xA0,0xC9,0x05,0xF3,0x6E}};
const IID IID_IWMPPlayer2 = {0x0E6B01D1,0xD407,0x4C85,{0xBF,0x5F,0x1C,0x01,0xF6,0x15,0x02,0x80}};
const IID IID_IWMPCore2 = {0xBC17E5B7,0x7561,0x4C18,{0xBB,0x90,0x17,0xD4,0x85,0x77,0x56,0x59}};
const IID IID_IWMPCore3 = {0x7587C667,0x628F,0x499F,{0x88,0xE7,0x6A,0x6F,0x4E,0x88,0x84,0x64}};
const IID IID_IWMPNetwork = {0xEC21B779,0xEDEF,0x462D,{0xBB,0xA4,0xAD,0x9D,0xDE,0x2B,0x29,0xA7}};

enum WMPOpenState
{
    wmposUndefined  = 0,
    wmposPlaylistChanging   = 1,
    wmposPlaylistLocating   = 2,
    wmposPlaylistConnecting = 3,
    wmposPlaylistLoading    = 4,
    wmposPlaylistOpening    = 5,
    wmposPlaylistOpenNoMedia    = 6,
    wmposPlaylistChanged    = 7,
    wmposMediaChanging  = 8,
    wmposMediaLocating  = 9,
    wmposMediaConnecting    = 10,
    wmposMediaLoading   = 11,
    wmposMediaOpening   = 12,
    wmposMediaOpen  = 13,
    wmposBeginCodecAcquisition  = 14,
    wmposEndCodecAcquisition    = 15,
    wmposBeginLicenseAcquisition    = 16,
    wmposEndLicenseAcquisition  = 17,
    wmposBeginIndividualization = 18,
    wmposEndIndividualization   = 19,
    wmposMediaWaiting   = 20,
    wmposOpeningUnknownURL  = 21
};

enum WMPPlayState
{
    wmppsUndefined  = 0,
    wmppsStopped    = 1,
    wmppsPaused = 2,
    wmppsPlaying    = 3,
    wmppsScanForward    = 4,
    wmppsScanReverse    = 5,
    wmppsBuffering  = 6,
    wmppsWaiting    = 7,
    wmppsMediaEnded = 8,
    wmppsTransitioning  = 9,
    wmppsReady  = 10,
    wmppsReconnecting   = 11,
    wmppsLast   = 12
};


struct IWMPMedia : public IDispatch
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isIdentical(
        /* [in] */ IWMPMedia __RPC_FAR *pIWMPMedia,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvbool) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_sourceURL(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrSourceURL) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_name(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrName) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_name(
        /* [in] */ BSTR pbstrName) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_imageSourceWidth(
        /* [retval][out] */ long __RPC_FAR *pWidth) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_imageSourceHeight(
        /* [retval][out] */ long __RPC_FAR *pHeight) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_markerCount(
        /* [retval][out] */ long __RPC_FAR *pMarkerCount) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getMarkerTime(
        /* [in] */ long MarkerNum,
        /* [retval][out] */ double __RPC_FAR *pMarkerTime) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getMarkerName(
        /* [in] */ long MarkerNum,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrMarkerName) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_duration(
        /* [retval][out] */ double __RPC_FAR *pDuration) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_durationString(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrDuration) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_attributeCount(
        /* [retval][out] */ long __RPC_FAR *plCount) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getAttributeName(
        /* [in] */ long lIndex,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrItemName) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getItemInfo(
        /* [in] */ BSTR bstrItemName,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrVal) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setItemInfo(
        /* [in] */ BSTR bstrItemName,
        /* [in] */ BSTR bstrVal) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getItemInfoByAtom(
        /* [in] */ long lAtom,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrVal) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE isMemberOf(
        /* [in] */ IUnknown __RPC_FAR *pPlaylist,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarfIsMemberOf) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE isReadOnlyItem(
        /* [in] */ BSTR bstrItemName,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarfIsReadOnly) = 0;

};

struct IWMPControls : public IDispatch
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isAvailable(
        /* [in] */ BSTR bstrItem,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsAvailable) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE play( void) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE stop( void) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE pause( void) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE fastForward( void) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE fastReverse( void) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentPosition(
        /* [retval][out] */ double __RPC_FAR *pdCurrentPosition) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_currentPosition(
        /* [in] */ double pdCurrentPosition) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentPositionString(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrCurrentPosition) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE next( void) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE previous( void) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentItem(
        /* [retval][out] */ IWMPMedia __RPC_FAR *__RPC_FAR *ppIWMPMedia) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_currentItem(
        /* [in] */ IWMPMedia __RPC_FAR *ppIWMPMedia) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentMarker(
        /* [retval][out] */ long __RPC_FAR *plMarker) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_currentMarker(
        /* [in] */ long plMarker) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE playItem(
        /* [in] */ IWMPMedia __RPC_FAR *pIWMPMedia) = 0;

};


struct IWMPSettings : public IDispatch
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isAvailable(
        /* [in] */ BSTR bstrItem,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsAvailable) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_autoStart(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAutoStart) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_autoStart(
        /* [in] */ VARIANT_BOOL pfAutoStart) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_baseURL(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrBaseURL) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_baseURL(
        /* [in] */ BSTR pbstrBaseURL) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_defaultFrame(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrDefaultFrame) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_defaultFrame(
        /* [in] */ BSTR pbstrDefaultFrame) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_invokeURLs(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfInvokeURLs) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_invokeURLs(
        /* [in] */ VARIANT_BOOL pfInvokeURLs) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_mute(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfMute) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_mute(
        /* [in] */ VARIANT_BOOL pfMute) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_playCount(
        /* [retval][out] */ long __RPC_FAR *plCount) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_playCount(
        /* [in] */ long plCount) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_rate(
        /* [retval][out] */ double __RPC_FAR *pdRate) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_rate(
        /* [in] */ double pdRate) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_balance(
        /* [retval][out] */ long __RPC_FAR *plBalance) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_balance(
        /* [in] */ long plBalance) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_volume(
        /* [retval][out] */ long __RPC_FAR *plVolume) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_volume(
        /* [in] */ long plVolume) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getMode(
        /* [in] */ BSTR bstrMode,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pvarfMode) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setMode(
        /* [in] */ BSTR bstrMode,
        /* [in] */ VARIANT_BOOL varfMode) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_enableErrorDialogs(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfEnableErrorDialogs) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_enableErrorDialogs(
        /* [in] */ VARIANT_BOOL pfEnableErrorDialogs) = 0;

};

struct IWMPNetwork : public IDispatch
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_bandWidth(
        /* [retval][out] */ long __RPC_FAR *plBandwidth) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_recoveredPackets(
        /* [retval][out] */ long __RPC_FAR *plRecoveredPackets) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_sourceProtocol(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrSourceProtocol) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_receivedPackets(
        /* [retval][out] */ long __RPC_FAR *plReceivedPackets) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_lostPackets(
        /* [retval][out] */ long __RPC_FAR *plLostPackets) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_receptionQuality(
        /* [retval][out] */ long __RPC_FAR *plReceptionQuality) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_bufferingCount(
        /* [retval][out] */ long __RPC_FAR *plBufferingCount) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_bufferingProgress(
        /* [retval][out] */ long __RPC_FAR *plBufferingProgress) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_bufferingTime(
        /* [retval][out] */ long __RPC_FAR *plBufferingTime) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_bufferingTime(
        /* [in] */ long plBufferingTime) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_frameRate(
        /* [retval][out] */ long __RPC_FAR *plFrameRate) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_maxBitRate(
        /* [retval][out] */ long __RPC_FAR *plBitRate) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_bitRate(
        /* [retval][out] */ long __RPC_FAR *plBitRate) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getProxySettings(
        /* [in] */ BSTR bstrProtocol,
        /* [retval][out] */ long __RPC_FAR *plProxySetting) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setProxySettings(
        /* [in] */ BSTR bstrProtocol,
        /* [in] */ long lProxySetting) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getProxyName(
        /* [in] */ BSTR bstrProtocol,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrProxyName) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setProxyName(
        /* [in] */ BSTR bstrProtocol,
        /* [in] */ BSTR bstrProxyName) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getProxyPort(
        /* [in] */ BSTR bstrProtocol,
        /* [retval][out] */ long __RPC_FAR *lProxyPort) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setProxyPort(
        /* [in] */ BSTR bstrProtocol,
        /* [in] */ long lProxyPort) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getProxyExceptionList(
        /* [in] */ BSTR bstrProtocol,
        /* [retval][out] */ BSTR __RPC_FAR *pbstrExceptionList) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setProxyExceptionList(
        /* [in] */ BSTR bstrProtocol,
        /* [in] */ BSTR pbstrExceptionList) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getProxyBypassForLocal(
        /* [in] */ BSTR bstrProtocol,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfBypassForLocal) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setProxyBypassForLocal(
        /* [in] */ BSTR bstrProtocol,
        /* [in] */ VARIANT_BOOL fBypassForLocal) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_maxBandwidth(
        /* [retval][out] */ long __RPC_FAR *lMaxBandwidth) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_maxBandwidth(
        /* [in] */ long lMaxBandwidth) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_downloadProgress(
        /* [retval][out] */ long __RPC_FAR *plDownloadProgress) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_encodedFrameRate(
        /* [retval][out] */ long __RPC_FAR *plFrameRate) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_framesSkipped(
        /* [retval][out] */ long __RPC_FAR *plFrames) = 0;

};

struct IWMPCore : public IDispatch
{
public:
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE close( void) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_URL(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrURL) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_URL(
        /* [in] */ BSTR pbstrURL) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_openState(
        /* [retval][out] */ WMPOpenState __RPC_FAR *pwmpos) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_playState(
        /* [retval][out] */ WMPPlayState __RPC_FAR *pwmpps) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_controls(
        /* [retval][out] */ IWMPControls __RPC_FAR *__RPC_FAR *ppControl) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_settings(
        /* [retval][out] */ IWMPSettings __RPC_FAR *__RPC_FAR *ppSettings) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentMedia(
        /* [retval][out] */ IWMPMedia __RPC_FAR *__RPC_FAR *ppMedia) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_currentMedia(
        /* [in] */ IUnknown __RPC_FAR *ppMedia) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_mediaCollection(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppMediaCollection) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_playlistCollection(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppPlaylistCollection) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_versionInfo(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrVersionInfo) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE launchURL(
        /* [in] */ BSTR bstrURL) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_network(
        /* [retval][out] */ IWMPNetwork __RPC_FAR *__RPC_FAR *ppQNI) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_currentPlaylist(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppPL) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_currentPlaylist(
        /* [in] */ IUnknown __RPC_FAR *ppPL) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_cdromCollection(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppCdromCollection) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_closedCaption(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppClosedCaption) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_isOnline(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfOnline) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Error(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppError) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_status(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrStatus) = 0;

};

struct IWMPCore2 : public IWMPCore
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_dvd(
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDVD) = 0;

};

struct IWMPCore3 : public IWMPCore2
{
public:
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE newPlaylist(
        /* [in] */ BSTR bstrName,
        /* [in] */ BSTR bstrURL,
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppPlaylist) = 0;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE newMedia(
        /* [in] */ BSTR bstrURL,
        /* [retval][out] */ IWMPMedia __RPC_FAR *__RPC_FAR *ppMedia) = 0;

};

#ifdef WXTEST_ATL
    MIDL_INTERFACE("6BF52A4F-394A-11D3-B153-00C04F79FAA6")
#else
    struct
#endif
IWMPPlayer : public IWMPCore
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_enabled(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnabled) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_enabled(
        /* [in] */ VARIANT_BOOL pbEnabled) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_fullScreen(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbFullScreen) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_fullScreen(
        VARIANT_BOOL pbFullScreen) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_enableContextMenu(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnableContextMenu) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_enableContextMenu(
        VARIANT_BOOL pbEnableContextMenu) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_uiMode(
        /* [in] */ BSTR pbstrMode) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_uiMode(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrMode) = 0;
};

struct IWMPPlayer2 : public IWMPCore
{
public:
    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_enabled(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnabled) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_enabled(
        /* [in] */ VARIANT_BOOL pbEnabled) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_fullScreen(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbFullScreen) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_fullScreen(
        VARIANT_BOOL pbFullScreen) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_enableContextMenu(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnableContextMenu) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_enableContextMenu(
        VARIANT_BOOL pbEnableContextMenu) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_uiMode(
        /* [in] */ BSTR pbstrMode) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_uiMode(
        /* [retval][out] */ BSTR __RPC_FAR *pbstrMode) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_stretchToFit(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnabled) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_stretchToFit(
        /* [in] */ VARIANT_BOOL pbEnabled) = 0;

    virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_windowlessVideo(
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbEnabled) = 0;

    virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_windowlessVideo(
        /* [in] */ VARIANT_BOOL pbEnabled) = 0;

};

//---------------------------------------------------------------------------
//
//  wxWMP10MediaBackend
//
//---------------------------------------------------------------------------

class WXDLLIMPEXP_MEDIA wxWMP10MediaBackend : public wxMediaBackendCommonBase
{
public:
    wxWMP10MediaBackend();
    virtual ~wxWMP10MediaBackend();

    virtual bool CreateControl(wxControl* ctrl, wxWindow* parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     const wxValidator& validator,
                                     const wxString& name);

    virtual bool Play();
    virtual bool Pause();
    virtual bool Stop();

    virtual bool Load(const wxString& fileName);
    virtual bool Load(const wxURI& location);
    virtual bool Load(const wxURI& location, const wxURI& proxy);

    bool DoLoad(const wxString& location);
    void FinishLoad();

    virtual wxMediaState GetState();

    virtual bool SetPosition(wxLongLong where);
    virtual wxLongLong GetPosition();
    virtual wxLongLong GetDuration();

    virtual void Move(int x, int y, int w, int h);
    wxSize GetVideoSize() const;

    virtual double GetPlaybackRate();
    virtual bool SetPlaybackRate(double);

    virtual double GetVolume();
    virtual bool SetVolume(double);

    virtual bool ShowPlayerControls(wxMediaCtrlPlayerControls flags);

    virtual wxLongLong GetDownloadProgress();
    virtual wxLongLong GetDownloadTotal();


#ifdef WXTEST_ATL
        CAxWindow  m_wndView;
#else
        wxActiveXContainer* m_pAX;
#endif
    IWMPPlayer* m_pWMPPlayer;       // Main activex interface
    IWMPSettings* m_pWMPSettings;   // Settings such as volume
    IWMPControls* m_pWMPControls;   // Control interface (play etc.)
    wxSize m_bestSize;              // Actual movie size

    bool m_bWasStateChanged;        // See the "introduction"

    friend class wxWMP10MediaEvtHandler;
    DECLARE_DYNAMIC_CLASS(wxWMP10MediaBackend)
};

#ifndef WXTEST_ATL
class WXDLLIMPEXP_MEDIA wxWMP10MediaEvtHandler : public wxEvtHandler
{
public:
    wxWMP10MediaEvtHandler(wxWMP10MediaBackend *amb) :
       m_amb(amb)
    {
        m_amb->m_pAX->Connect(m_amb->m_pAX->GetId(),
            wxEVT_ACTIVEX,
            wxActiveXEventHandler(wxWMP10MediaEvtHandler::OnActiveX),
            NULL, this
                              );
    }

    void OnActiveX(wxActiveXEvent& event);

private:
    wxWMP10MediaBackend *m_amb;

    DECLARE_NO_COPY_CLASS(wxWMP10MediaEvtHandler)
};
#endif

//===========================================================================
//  IMPLEMENTATION
//===========================================================================

//---------------------------------------------------------------------------
//
// wxWMP10MediaBackend
//
//---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxWMP10MediaBackend, wxMediaBackend)

//---------------------------------------------------------------------------
// wxWMP10MediaBackend Constructor
//---------------------------------------------------------------------------
wxWMP10MediaBackend::wxWMP10MediaBackend()
                 :
#ifndef WXTEST_ATL
                m_pAX(NULL),
#endif
                m_pWMPPlayer(NULL),
                m_pWMPSettings(NULL),
                m_pWMPControls(NULL)

{
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend Destructor
//---------------------------------------------------------------------------
wxWMP10MediaBackend::~wxWMP10MediaBackend()
{
    if(m_pWMPPlayer)
    {
#ifndef WXTEST_ATL
        m_pAX->DissociateHandle();
        delete m_pAX;

        m_ctrl->PopEventHandler(true);
#else
        AtlAxWinTerm();
        _Module.Term();
#endif

        m_pWMPPlayer->Release();
        if (m_pWMPSettings)
            m_pWMPSettings->Release();
        if (m_pWMPControls)
            m_pWMPControls->Release();
    }
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::CreateControl
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::CreateControl(wxControl* ctrl, wxWindow* parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     const wxValidator& validator,
                                     const wxString& name)
{
#ifndef WXTEST_ATL
    if( ::CoCreateInstance(CLSID_WMP10, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWMPPlayer, (void**)&m_pWMPPlayer) != 0 )
    {
        if( ::CoCreateInstance(CLSID_WMP10ALT, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWMPPlayer, (void**)&m_pWMPPlayer) != 0 )
            return false;

        if( m_pWMPPlayer->get_settings(&m_pWMPSettings) != 0)
        {
            m_pWMPPlayer->Release();
            wxLogSysError(wxT("Could not obtain settings from WMP10!"));
            return false;
        }

        if( m_pWMPPlayer->get_controls(&m_pWMPControls) != 0)
        {
            m_pWMPSettings->Release();
            m_pWMPPlayer->Release();
            wxLogSysError(wxT("Could not obtain controls from WMP10!"));
            return false;
        }
    }
#endif

    //
    // Create window
    // By default wxWindow(s) is created with a border -
    // so we need to get rid of those
    //
    // Since we don't have a child window like most other
    // backends, we don't need wxCLIP_CHILDREN
    //
    if ( !ctrl->wxControl::Create(parent, id, pos, size,
                            (style & ~wxBORDER_MASK) | wxBORDER_NONE,
                            validator, name) )
        return false;

    //
    // Now create the ActiveX container along with the media player
    // interface and query them
    //
    m_ctrl = wxStaticCast(ctrl, wxMediaCtrl);

#ifndef WXTEST_ATL
    m_pAX = new wxActiveXContainer(ctrl, IID_IWMPPlayer, m_pWMPPlayer);

    // Connect for events
    m_ctrl->PushEventHandler(new wxWMP10MediaEvtHandler(this));
#else
    _Module.Init(NULL, ::GetModuleHandle(NULL));
    AtlAxWinInit();
    CComPtr<IAxWinHostWindow>  spHost;

    HRESULT hr;
    RECT rcClient;
    ::GetClientRect((HWND)ctrl->GetHandle(), &rcClient);
    m_wndView.Create((HWND)ctrl->GetHandle(), rcClient, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
    hr = m_wndView.QueryHost(&spHost);
    hr = spHost->CreateControl(CComBSTR(_T("{6BF52A52-394A-11d3-B153-00C04F79FAA6}")), m_wndView, 0);
    hr = m_wndView.QueryControl(&m_pWMPPlayer);

    if( m_pWMPPlayer->get_settings(&m_pWMPSettings) != 0)
    {
        m_pWMPPlayer->Release();
        wxLogSysError(wxT("Could not obtain settings from WMP10!"));
        return false;
    }

    if( m_pWMPPlayer->get_controls(&m_pWMPControls) != 0)
    {
        m_pWMPSettings->Release();
        m_pWMPPlayer->Release();
        wxLogSysError(wxT("Could not obtain controls from WMP10!"));
        return false;
    }
#endif

    //
    //  Here we set up wx-specific stuff for the default
    //  settings wxMediaCtrl says it will stay to
    //

    IWMPPlayer2* pWMPPlayer2; // Only 2 has windowless video and stretchtofit
    if(m_pWMPPlayer->QueryInterface(IID_IWMPPlayer2, (void**)&pWMPPlayer2) == 0)
    {
        // We don't check errors here as these arn't particularily important
        // and may not be implemented (i.e. stretchToFit on CE)
        pWMPPlayer2->put_windowlessVideo(VARIANT_TRUE);
        pWMPPlayer2->put_stretchToFit(VARIANT_TRUE);
        pWMPPlayer2->Release();
    }

    // by default true (see the "introduction")
    m_pWMPSettings->put_autoStart(VARIANT_TRUE);
    // by default enabled
    wxWMP10MediaBackend::ShowPlayerControls(wxMEDIACTRLPLAYERCONTROLS_NONE);
    // by default with AM only 0.5
    wxWMP10MediaBackend::SetVolume(1.0);

    // don't erase the background of our control window so that resizing is a
    // bit smoother
    m_ctrl->SetBackgroundStyle(wxBG_STYLE_CUSTOM);

    // success
    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Load (file version)
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Load(const wxString& fileName)
{
    return DoLoad(fileName);
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Load (URL Version)
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Load(const wxURI& location)
{
    return DoLoad(location.BuildURI());
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Load (URL Version with Proxy)
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Load(const wxURI& location,
                               const wxURI& proxy)
{
    bool bOK = false;

    IWMPNetwork* pWMPNetwork;
    if( m_pWMPPlayer->get_network(&pWMPNetwork) == 0 )
    {
        long lOldSetting;
        if( pWMPNetwork->getProxySettings(
                    wxBasicString(location.GetScheme()).Get(), &lOldSetting
                                        ) == 0 &&

            pWMPNetwork->setProxySettings(
                    wxBasicString(location.GetScheme()).Get(), // protocol
                                2) == 0) // 2 == manually specify
        {
            BSTR bsOldName = NULL;
            long lOldPort = 0;

            pWMPNetwork->getProxyName(
                        wxBasicString(location.GetScheme()).Get(),
                        &bsOldName);
            pWMPNetwork->getProxyPort(
                        wxBasicString(location.GetScheme()).Get(),
                        &lOldPort);

            long lPort;
            wxString server;
            if(proxy.IsReference())
            {
                server = proxy.GetScheme();
                lPort = wxAtoi(proxy.GetPath());
            }
            else
            {
                server = proxy.GetServer();
                lPort = wxAtoi(proxy.GetPort());
            }

            if( pWMPNetwork->setProxyName(
                        wxBasicString(location.GetScheme()).Get(), // proto
                        wxBasicString(server).Get() ) == 0  &&

                pWMPNetwork->setProxyPort(
                        wxBasicString(location.GetScheme()).Get(), // proto
                        lPort
                                         ) == 0
              )
            {
                bOK = DoLoad(location.BuildURI());

                pWMPNetwork->setProxySettings(
                    wxBasicString(location.GetScheme()).Get(), // protocol
                                lOldSetting);
                if(bsOldName)
                    pWMPNetwork->setProxyName(
                        wxBasicString(location.GetScheme()).Get(), // protocol
                                    bsOldName);

                if(lOldPort)
                    pWMPNetwork->setProxyPort(
                        wxBasicString(location.GetScheme()).Get(), // protocol
                                lOldPort);

                pWMPNetwork->Release();
            }
            else
                pWMPNetwork->Release();

        }
        else
            pWMPNetwork->Release();

    }

    return bOK;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::DoLoad
//
// Called by all functions - this actually renders
// the file and sets up the filter graph
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::DoLoad(const wxString& location)
{
    HRESULT hr;

#if 0 // See the "introduction" - this is the duration hack
    // ------------------ BLATENT HACK ALERT -------------------------
    // Normally we can only get the duration of things already in an
    // existing playlist or playing - however this clever "workaround"
    // enables us to get the duration even when stopped :)
    // http://weblogs.asp.net/rweigelt/archive/2003/07/02/9613.aspx

    IWMPCore3* pWMPCore3;
    double outDuration;
    if(m_pWMPPlayer->QueryInterface(IID_IWMPCore3, (void**) &pWMPCore3) == 0)
    {
        IWMPMedia* pWMPMedia;

        if( (hr = pWMPCore3->newMedia(wxBasicString(location).Get(),
                               &pWMPMedia)) == 0)
        {
            // this (get_duration) will actually FAIL, but it will work.
            pWMPMedia->get_duration(&outDuration);
            pWMPCore3->put_currentMedia(pWMPMedia);
            pWMPMedia->Release();
        }

        pWMPCore3->Release();
    }
    else
#endif
    {
        // just load it the "normal" way
        hr = m_pWMPPlayer->put_URL( wxBasicString(location).Get() );
    }

    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::FinishLoad
//
// Called when our media is about to play (a.k.a. wmposMediaOpen)
//---------------------------------------------------------------------------
void wxWMP10MediaBackend::FinishLoad()
{
    // Get the original video size
    // THIS WILL NOT WORK UNLESS THE MEDIA IS ABOUT TO PLAY
    // See the "introduction" - also get_currentMedia will return
    // "1" which is a VALID HRESULT value
    // and a NULL pWMPMedia if the media isn't the "current" one
    // which is rather unintuitive in the sense that it uses it
    // (i.e. basically not currently playing)...
    IWMPMedia* pWMPMedia;
    if(m_pWMPPlayer->get_currentMedia(&pWMPMedia) == 0)
    {
        pWMPMedia->get_imageSourceWidth((long*)&m_bestSize.x);
        pWMPMedia->get_imageSourceHeight((long*)&m_bestSize.y);
        pWMPMedia->Release();
    }
    else
    {
        wxLogDebug(wxT("Could not get media"));
    }

    NotifyMovieLoaded();
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::ShowPlayerControls
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::ShowPlayerControls(wxMediaCtrlPlayerControls flags)
{
    if(!flags)
    {
        m_pWMPPlayer->put_enabled(VARIANT_FALSE);
        m_pWMPPlayer->put_uiMode(wxBasicString(wxT("none")).Get());
    }
    else
    {
        // TODO: use "custom"? (note that CE only supports none/full)
        m_pWMPPlayer->put_uiMode(wxBasicString(wxT("full")).Get());
        m_pWMPPlayer->put_enabled(VARIANT_TRUE);
    }

    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Play
//
// Plays the stream.  If it is non-seekable, it will restart it (implicit).
//
// TODO: We use SUCCEEDED due to pickiness on IMediaPlayer are doing it here
// but do we need to?
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Play()
{
    // Actually try to play the movie (will fail if not loaded completely)
    HRESULT hr = m_pWMPControls->play();
    if(SUCCEEDED(hr))
    {
       m_bWasStateChanged = true;
       return true;
    }
    wxWMP10LOG(hr);
    return false;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Pause
//
// Pauses the stream.
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Pause()
{
    HRESULT hr = m_pWMPControls->pause();
    if(SUCCEEDED(hr))
    {
        m_bWasStateChanged = true;
        return true;
    }
    wxWMP10LOG(hr);
    return false;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Stop
//
// Stops the stream.
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::Stop()
{
    HRESULT hr = m_pWMPControls->stop();
    if(SUCCEEDED(hr))
    {
        // Seek to beginning
        wxWMP10MediaBackend::SetPosition(0);
        m_bWasStateChanged = true;
        return true;
    }
    wxWMP10LOG(hr);
    return false;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::SetPosition
//
// WMP10 position values are a double in seconds - we just translate
// to our base here
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::SetPosition(wxLongLong where)
{
    HRESULT hr = m_pWMPControls->put_currentPosition(
                        ((LONGLONG)where.GetValue()) / 1000.0
                                     );
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetPosition
//
// WMP10 position values are a double in seconds - we just translate
// to our base here
//---------------------------------------------------------------------------
wxLongLong wxWMP10MediaBackend::GetPosition()
{
    double outCur;
    HRESULT hr = m_pWMPControls->get_currentPosition(&outCur);
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return 0;
    }

    // h,m,s,milli - outCur is in 1 second (double)
    outCur *= 1000;
    wxLongLong ll;
    ll.Assign(outCur);

    return ll;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetVolume
//
// Volume 0-100
//---------------------------------------------------------------------------
double wxWMP10MediaBackend::GetVolume()
{
    long lVolume;
    HRESULT hr = m_pWMPSettings->get_volume(&lVolume);
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return 0.0;
    }

    return (double)lVolume / 100.0;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::SetVolume
//
// Volume 0-100
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::SetVolume(double dVolume)
{
    HRESULT hr = m_pWMPSettings->put_volume( (long) (dVolume * 100.0) );
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetDuration
//
// Obtains the duration of the media.
//
// See the "introduction"
//
// The good news is that this doesn't appear to have the XING header
// parser problem that WMP6 SDK/IActiveMovie/IMediaPlayer/IWMP has
//---------------------------------------------------------------------------
wxLongLong wxWMP10MediaBackend::GetDuration()
{
    double outDuration = 0.0;

    IWMPMedia* pWMPMedia;
    if(m_pWMPPlayer->get_currentMedia(&pWMPMedia) == 0)
    {
        if(pWMPMedia->get_duration(&outDuration) != 0)
        {
            wxLogDebug(wxT("get_duration failed"));
        }
        pWMPMedia->Release();
    }


    // h,m,s,milli - outDuration is in 1 second (double)
    outDuration *= 1000;
    wxLongLong ll;
    ll.Assign(outDuration);

    return ll;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetState
//
// Returns the current state
//---------------------------------------------------------------------------
wxMediaState wxWMP10MediaBackend::GetState()
{
    WMPPlayState nState;
    HRESULT hr = m_pWMPPlayer->get_playState(&nState);
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return wxMEDIASTATE_STOPPED;
    }

    switch(nState)
    {
    case wmppsPaused:
        return wxMEDIASTATE_PAUSED;
    case wmppsPlaying:
        return wxMEDIASTATE_PLAYING;
    default:
        return wxMEDIASTATE_STOPPED;
    }
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetPlaybackRate
//
// Just get the rate from WMP10
//---------------------------------------------------------------------------
double wxWMP10MediaBackend::GetPlaybackRate()
{
    double dRate;
    HRESULT hr = m_pWMPSettings->get_rate(&dRate);
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return 0.0;
    }
    return dRate;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::SetPlaybackRate
//
// Sets the playback rate of the media - DirectShow is pretty good
// about this, actually
//---------------------------------------------------------------------------
bool wxWMP10MediaBackend::SetPlaybackRate(double dRate)
{
    HRESULT hr = m_pWMPSettings->put_rate(dRate);
    if(FAILED(hr))
    {
        wxWMP10LOG(hr);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetVideoSize
//
// Obtains the cached original video size
//---------------------------------------------------------------------------
wxSize wxWMP10MediaBackend::GetVideoSize() const
{
    return m_bestSize;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::Move
//
// We take care of this in our redrawing
//---------------------------------------------------------------------------
void wxWMP10MediaBackend::Move(int WXUNUSED(x), int WXUNUSED(y),
#ifdef WXTEST_ATL
                            int w, int h
#else
                            int WXUNUSED(w), int WXUNUSED(h)
#endif
                            )
{
#ifdef WXTEST_ATL
    m_wndView.MoveWindow(0,0,w,h);
#endif
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetDownloadProgress()
//---------------------------------------------------------------------------
wxLongLong wxWMP10MediaBackend::GetDownloadProgress()
{
    IWMPNetwork* pWMPNetwork;
    if( m_pWMPPlayer->get_network(&pWMPNetwork) == 0 )
    {
        long lPercentProg;
        if(pWMPNetwork->get_downloadProgress(&lPercentProg) == 0)
        {
            pWMPNetwork->Release();
            return (GetDownloadTotal() * lPercentProg) / 100;
        }
        pWMPNetwork->Release();
    }
    return 0;
}

//---------------------------------------------------------------------------
// wxWMP10MediaBackend::GetDownloadTotal()
//---------------------------------------------------------------------------
wxLongLong wxWMP10MediaBackend::GetDownloadTotal()
{
    IWMPMedia* pWMPMedia;
    if(m_pWMPPlayer->get_currentMedia(&pWMPMedia) == 0)
    {
        BSTR bsOut;
        pWMPMedia->getItemInfo(wxBasicString(wxT("FileSize")).Get(),
                               &bsOut);

        wxString sFileSize = wxConvertStringFromOle(bsOut);
        long lFS;
        sFileSize.ToLong(&lFS);
        pWMPMedia->Release();
        return lFS;
    }

    return 0;
}


//---------------------------------------------------------------------------
// wxWMP10MediaBackend::OnActiveX
//
// Handle events sent from our activex control (_WMPOCXEvents actually).
//
// The weird numbers in the switch statement here are "dispatch ids"
// (the numbers in the id field like ( id(xxx) ) ) from amcompat.idl
// and wmp.IDL.
//---------------------------------------------------------------------------
#ifndef WXTEST_ATL
void wxWMP10MediaEvtHandler::OnActiveX(wxActiveXEvent& event)
{
    switch(event.GetDispatchId())
    {
    case 0x000013ed: // playstatechange
        if(event.ParamCount() >= 1)
        {
            switch (event[0].GetInteger())
            {
            case wmppsMediaEnded: // media ended
                if ( m_amb->SendStopEvent() )
                {
                    // NB: If we do Stop() or similar here the media
                    // actually starts over and plays a bit before
                    // stopping. It stops by default, however, so
                    // there is no real need to do anything here...

                    // send the event to our child
                    m_amb->QueueFinishEvent();
                }
                break;

            case wmppsStopped: // stopping
                m_amb->QueueStopEvent();
                break;
            case wmppsPaused: // pause
                m_amb->QueuePauseEvent();
                break;
            case wmppsPlaying: // play
                m_amb->QueuePlayEvent();
                break;
            default:
                break;
            }
        }
        else
            event.Skip();
        break;

    case 0x00001389: // openstatechange
        if(event.ParamCount() >= 1)
        {
            int nState = event[0].GetInteger();
            if(nState == wmposMediaOpen)
            {
                // See the "introduction"
                m_amb->m_bWasStateChanged = false;
                m_amb->FinishLoad();
                if(!m_amb->m_bWasStateChanged)
                    m_amb->Stop();
            }
        }
        else
            event.Skip();
        break;

    case 0x0000196e: // mousedown
        m_amb->m_ctrl->SetFocus();
        break;

    default:
        event.Skip();
        return;
    }
}

#endif
// in source file that contains stuff you don't directly use
#include "wx/html/forcelnk.h"
FORCE_LINK_ME(wxmediabackend_wmp10)

#if 0 // Windows Media Player Mobile 9 hacks

//------------------WMP Mobile 9 hacks-----------------------------------
// It was mentioned in the introduction that while there was no official
// programming interface on WMP mobile 9
// (SmartPhone/Pocket PC 2003 emulator etc.)
// there were some windows message hacks that are able to get
// you playing a file through WMP.
//
// Here are those hacks. They do indeed "work" as expected - just call
// SendMessage with one of those myterious values layed out in
// Peter Foot's Friday, May 21, 2004 Blog Post on the issue.
// (He says they are in a registery section entitled "Pendant Bus")
//
// How do you play a certain file? Simply calling "start [file]" or
// wxWinCEExecute([file]) should do the trick

bool wxWinCEExecute(const wxString& path, int nShowStyle = SW_SHOWNORMAL)
{
    WinStruct<SHELLEXECUTEINFO> sei;
    sei.lpFile = path.c_str();
    sei.lpVerb = _T("open");
    sei.nShow = nShowStyle;

    ::ShellExecuteEx(&sei);

    return ((int) sei.hInstApp) > 32;
}

bool MyApp::OnInit()
{
    HWND hwnd = ::FindWindow(TEXT("WMP for Mobile Devices"), TEXT("Windows Media"));
    if(!hwnd)
    {
        if( wxWinCEExecute(wxT("\\Windows\\wmplayer.exe"), SW_MINIMIZE) )
        {
            hwnd = ::FindWindow(TEXT("WMP for Mobile Devices"), TEXT("Windows Media"));
        }
    }

    if(hwnd)
    {
        // hide wmp window
        ::SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                       SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);

        // Stop         == 32970
        // Prev Track   == 32971
        // Next Track   == 32972
        // Shuffle      == 32973
        // Repeat       == 32974
        // Vol Up       == 32975
        // Vol Down     == 32976
        // Play         == 32978
        ::SendMessage(hwnd, 32978, NULL, 0);
    }
}

#endif // WMP mobile 9 hacks

#endif // wxUSE_MEDIACTRL && wxUSE_ACTIVEX
