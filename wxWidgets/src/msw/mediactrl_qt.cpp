/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/mediactrl_qt.cpp
// Purpose:     QuickTime Media Backend for Windows
// Author:      Ryan Norton <wxprojects@comcast.net>
// Modified by: Robin Dunn (moved QT code from mediactrl.cpp)
//
// Created:     11/07/04
// RCS-ID:      $Id: mediactrl_qt.cpp 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


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

#if wxUSE_MEDIACTRL

#include "wx/mediactrl.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/dcclient.h"
    #include "wx/timer.h"
    #include "wx/math.h"        // log10 & pow
#endif

#include "wx/msw/private.h" // user info and wndproc setting/getting
#include "wx/dynlib.h"

//---------------------------------------------------------------------------
// Externals (somewhere in src/msw/app.cpp and src/msw/window.cpp)
//---------------------------------------------------------------------------
extern "C" WXDLLIMPEXP_BASE HINSTANCE wxGetInstance(void);
#ifdef __WXWINCE__
extern WXDLLIMPEXP_CORE       wxChar *wxCanvasClassName;
#else
extern WXDLLIMPEXP_CORE const wxChar *wxCanvasClassName;
#endif

LRESULT WXDLLIMPEXP_CORE APIENTRY _EXPORT wxWndProc(HWND hWnd, UINT message,
                                   WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------------
// Killed MSVC warnings
//---------------------------------------------------------------------------
//disable "cast truncates constant value" for VARIANT_BOOL values
//passed as parameters in VC5 and up
#ifdef _MSC_VER
#pragma warning (disable:4310)
#endif


//---------------------------------------------------------------------------
//  wxQTMediaBackend
//
// We don't include Quicktime headers here and define all the types
// ourselves because looking for the quicktime libaries etc. would
// be tricky to do and making this a dependency for the MSVC projects
// would be unrealistic.
//
// Thanks to Robert Roebling for the wxDL macro/library idea
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  QT Includes
//---------------------------------------------------------------------------
//#include <qtml.h>                   // Windoze QT include
//#include <QuickTimeComponents.h>    // Standard QT stuff
#include "wx/dynlib.h"

//---------------------------------------------------------------------------
//  QT Types
//---------------------------------------------------------------------------
typedef struct MovieRecord* Movie;
typedef wxInt16 OSErr;
typedef wxInt32 OSStatus;
#define noErr 0
#define fsRdPerm 1
typedef unsigned char                   Str255[256];
#define StringPtr unsigned char*
#define newMovieActive 1
#define newMovieAsyncOK (1 << 8)
#define Ptr char*
#define Handle Ptr*
#define Fixed long
#define OSType unsigned long
#define CGrafPtr struct GrafPort *
#define TimeScale long
#define TimeBase struct TimeBaseRecord *
typedef struct ComponentInstanceRecord * ComponentInstance;
#define kMovieLoadStatePlayable 10000
#define Boolean int
#define MovieController ComponentInstance

#ifndef URLDataHandlerSubType
#if defined(__WATCOMC__) || defined(__MINGW32__)
// use magic numbers for compilers which complain about multicharacter integers
const OSType URLDataHandlerSubType     = 1970433056;
const OSType VisualMediaCharacteristic = 1702454643;
#else
const OSType URLDataHandlerSubType     = 'url ';
const OSType VisualMediaCharacteristic = 'eyes';
#endif
#endif

struct FSSpec
{
    short      vRefNum;
    long       parID;
    Str255     name;  // Str63 on mac, Str255 on msw
};

struct Rect
{
    short      top;
    short      left;
    short      bottom;
    short      right;
};

struct wide
{
    wxInt32    hi;
    wxUint32   lo;
};

struct TimeRecord
{
    wide       value; // units
    TimeScale  scale; // units per second
    TimeBase   base;
};

struct Point
{
    short                           v;
    short                           h;
};

struct EventRecord
{
    wxUint16                       what;
    wxUint32                          message;
    wxUint32                          when;
    Point                           where;
    wxUint16                  modifiers;
};

enum
{
    mcTopLeftMovie              = 1,
    mcScaleMovieToFit           = 2,
    mcWithBadge                 = 4,
    mcNotVisible                = 8,
    mcWithFrame                 = 16
};

//---------------------------------------------------------------------------
//  QT Library
//---------------------------------------------------------------------------
#define wxDL_METHOD_DEFINE( rettype, name, args, shortargs, defret ) \
    typedef rettype (* name ## Type) args ; \
    name ## Type pfn_ ## name; \
    rettype name args \
    { if (m_ok) return pfn_ ## name shortargs ; return defret; }

#define wxDL_VOIDMETHOD_DEFINE( name, args, shortargs ) \
    typedef void (* name ## Type) args ; \
    name ## Type pfn_ ## name; \
    void name args \
    { if (m_ok) pfn_ ## name shortargs ; }

#define wxDL_METHOD_LOAD( lib, name, success ) \
    pfn_ ## name = (name ## Type) lib.GetSymbol( wxT(#name), &success ); \
    if (!success) return false


class WXDLLIMPEXP_MEDIA wxQuickTimeLibrary
{
public:
    ~wxQuickTimeLibrary()
    {
        if (m_dll.IsLoaded())
            m_dll.Unload();
    }

    bool Initialize();
    bool IsOk() const {return m_ok;}

protected:
    wxDynamicLibrary m_dll;
    bool m_ok;

public:
    wxDL_VOIDMETHOD_DEFINE( StartMovie, (Movie m), (m) )
    wxDL_VOIDMETHOD_DEFINE( StopMovie, (Movie m), (m) )
    wxDL_METHOD_DEFINE( bool, IsMovieDone, (Movie m), (m), false)
    wxDL_VOIDMETHOD_DEFINE( GoToBeginningOfMovie, (Movie m), (m) )
    wxDL_METHOD_DEFINE( OSErr, GetMoviesError, (), (), -1)
    wxDL_METHOD_DEFINE( OSErr, EnterMovies, (), (), -1)
    wxDL_VOIDMETHOD_DEFINE( ExitMovies, (), () )
    wxDL_METHOD_DEFINE( OSErr, InitializeQTML, (long flags), (flags), -1)
    wxDL_VOIDMETHOD_DEFINE( TerminateQTML, (), () )

    wxDL_METHOD_DEFINE( OSErr, NativePathNameToFSSpec,
                        (char* inName, FSSpec* outFile, long flags),
                        (inName, outFile, flags), -1)

    wxDL_METHOD_DEFINE( OSErr, OpenMovieFile,
                        (const FSSpec * fileSpec, short * resRefNum, wxInt8 permission),
                        (fileSpec, resRefNum, permission), -1 )

    wxDL_METHOD_DEFINE( OSErr, CloseMovieFile,
                        (short resRefNum), (resRefNum), -1)

    wxDL_METHOD_DEFINE( OSErr, NewMovieFromFile,
                            (Movie * theMovie, short resRefNum, short *  resId,
                             StringPtr resName, short newMovieFlags,
                             bool * dataRefWasChanged),
                             (theMovie, resRefNum, resId, resName, newMovieFlags,
                              dataRefWasChanged), -1)

    wxDL_VOIDMETHOD_DEFINE( SetMovieRate, (Movie m, Fixed rate), (m, rate) )
    wxDL_METHOD_DEFINE( Fixed, GetMovieRate, (Movie m), (m), 0)
    wxDL_VOIDMETHOD_DEFINE( MoviesTask, (Movie m, long maxms), (m, maxms) )
    wxDL_VOIDMETHOD_DEFINE( BlockMove,
        (const char* p1, const char* p2, long s), (p1,p2,s) )
    wxDL_METHOD_DEFINE( Handle, NewHandleClear, (long s), (s), NULL )

    wxDL_METHOD_DEFINE( OSErr, NewMovieFromDataRef,
                           (Movie * m, short flags, short * id,
                            Handle  dataRef, OSType  dataRefType),
                            (m,flags,id,dataRef,dataRefType), -1 )

    wxDL_VOIDMETHOD_DEFINE( DisposeHandle, (Handle h), (h) )
    wxDL_VOIDMETHOD_DEFINE( GetMovieNaturalBoundsRect, (Movie m, Rect* r), (m,r) )
    wxDL_METHOD_DEFINE( void*, GetMovieIndTrackType,
                        (Movie m, long index, OSType type, long flags),
                        (m,index,type,flags), NULL )
    wxDL_VOIDMETHOD_DEFINE( CreatePortAssociation,
            (void* hWnd, void* junk, long morejunk), (hWnd, junk, morejunk) )
    wxDL_METHOD_DEFINE(void*, GetNativeWindowPort, (void* hWnd), (hWnd), NULL)
    wxDL_VOIDMETHOD_DEFINE(SetMovieGWorld, (Movie m, CGrafPtr port, void* whatever),
                            (m, port, whatever) )
    wxDL_VOIDMETHOD_DEFINE(DisposeMovie, (Movie m), (m) )
    wxDL_VOIDMETHOD_DEFINE(SetMovieBox, (Movie m, Rect* r), (m,r))
    wxDL_VOIDMETHOD_DEFINE(SetMovieTimeScale, (Movie m, long s), (m,s))
    wxDL_METHOD_DEFINE(long, GetMovieDuration, (Movie m), (m), 0)
    wxDL_METHOD_DEFINE(TimeBase, GetMovieTimeBase, (Movie m), (m), 0)
    wxDL_METHOD_DEFINE(TimeScale, GetMovieTimeScale, (Movie m), (m), 0)
    wxDL_METHOD_DEFINE(long, GetMovieTime, (Movie m, void* cruft), (m,cruft), 0)
    wxDL_VOIDMETHOD_DEFINE(SetMovieTime, (Movie m, TimeRecord* tr), (m,tr) )
    wxDL_METHOD_DEFINE(short, GetMovieVolume, (Movie m), (m), 0)
    wxDL_VOIDMETHOD_DEFINE(SetMovieVolume, (Movie m, short sVolume), (m,sVolume) )
    wxDL_VOIDMETHOD_DEFINE(SetMovieTimeValue, (Movie m, long s), (m,s))
    wxDL_METHOD_DEFINE(ComponentInstance, NewMovieController, (Movie m, const Rect* mr, long fl), (m,mr,fl), 0)
    wxDL_VOIDMETHOD_DEFINE(DisposeMovieController, (ComponentInstance ci), (ci))
    wxDL_METHOD_DEFINE(int, MCSetVisible, (ComponentInstance m, int b), (m, b), 0)

    wxDL_VOIDMETHOD_DEFINE(PrePrerollMovie, (Movie m, long t, Fixed r, WXFARPROC p1, void* p2), (m,t,r,p1,p2) )
    wxDL_VOIDMETHOD_DEFINE(PrerollMovie, (Movie m, long t, Fixed r), (m,t,r) )
    wxDL_METHOD_DEFINE(Fixed, GetMoviePreferredRate, (Movie m), (m), 0)
    wxDL_METHOD_DEFINE(long, GetMovieLoadState, (Movie m), (m), 0)
    wxDL_METHOD_DEFINE(void*, NewRoutineDescriptor, (WXFARPROC f, int l, void* junk), (f, l, junk), 0)
    wxDL_VOIDMETHOD_DEFINE(DisposeRoutineDescriptor, (void* f), (f))
    wxDL_METHOD_DEFINE(void*, GetCurrentArchitecture, (), (), 0)
    wxDL_METHOD_DEFINE(int, MCDoAction, (ComponentInstance ci, long f, void* p), (ci,f,p), 0)
    wxDL_VOIDMETHOD_DEFINE(MCSetControllerBoundsRect, (ComponentInstance ci, Rect* r), (ci,r))
    wxDL_VOIDMETHOD_DEFINE(DestroyPortAssociation, (CGrafPtr g), (g))
    wxDL_VOIDMETHOD_DEFINE(NativeEventToMacEvent, (MSG* p1, EventRecord* p2), (p1,p2))
    wxDL_VOIDMETHOD_DEFINE(MCIsPlayerEvent, (ComponentInstance ci, EventRecord* p2), (ci, p2))
    wxDL_METHOD_DEFINE(int, MCSetMovie, (ComponentInstance ci, Movie m, void* p1, Point w),
                          (ci,m,p1,w),0)
    wxDL_VOIDMETHOD_DEFINE(MCPositionController,
        (ComponentInstance ci, Rect* r, void* junk, void* morejunk), (ci,r,junk,morejunk))
    wxDL_VOIDMETHOD_DEFINE(MCSetActionFilterWithRefCon,
        (ComponentInstance ci, WXFARPROC cb, void* ref), (ci,cb,ref))
    wxDL_VOIDMETHOD_DEFINE(MCGetControllerInfo, (MovieController mc, long* flags), (mc,flags))
    wxDL_VOIDMETHOD_DEFINE(BeginUpdate, (CGrafPtr port), (port))
    wxDL_VOIDMETHOD_DEFINE(UpdateMovie, (Movie m), (m))
    wxDL_VOIDMETHOD_DEFINE(EndUpdate, (CGrafPtr port), (port))
    wxDL_METHOD_DEFINE( OSErr, GetMoviesStickyError, (), (), -1)
};

bool wxQuickTimeLibrary::Initialize()
{
    m_ok = false;

    // Turn off the wxDynamicLibrary logging as we're prepared to handle the
    // errors
    wxLogNull nolog;

    if (!m_dll.Load(wxT("qtmlClient.dll")))
    {
        return false;
    }

    wxDL_METHOD_LOAD( m_dll, StartMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, StopMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, IsMovieDone, m_ok );
    wxDL_METHOD_LOAD( m_dll, GoToBeginningOfMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMoviesError, m_ok );
    wxDL_METHOD_LOAD( m_dll, EnterMovies, m_ok );
    wxDL_METHOD_LOAD( m_dll, ExitMovies, m_ok );
    wxDL_METHOD_LOAD( m_dll, InitializeQTML, m_ok );
    wxDL_METHOD_LOAD( m_dll, TerminateQTML, m_ok );
    wxDL_METHOD_LOAD( m_dll, NativePathNameToFSSpec, m_ok );
    wxDL_METHOD_LOAD( m_dll, OpenMovieFile, m_ok );
    wxDL_METHOD_LOAD( m_dll, CloseMovieFile, m_ok );
    wxDL_METHOD_LOAD( m_dll, NewMovieFromFile, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieRate, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieRate, m_ok );
    wxDL_METHOD_LOAD( m_dll, MoviesTask, m_ok );
    wxDL_METHOD_LOAD( m_dll, BlockMove, m_ok );
    wxDL_METHOD_LOAD( m_dll, NewHandleClear, m_ok );
    wxDL_METHOD_LOAD( m_dll, NewMovieFromDataRef, m_ok );
    wxDL_METHOD_LOAD( m_dll, DisposeHandle, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieNaturalBoundsRect, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieIndTrackType, m_ok );
    wxDL_METHOD_LOAD( m_dll, CreatePortAssociation, m_ok );
    wxDL_METHOD_LOAD( m_dll, DestroyPortAssociation, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetNativeWindowPort, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieGWorld, m_ok );
    wxDL_METHOD_LOAD( m_dll, DisposeMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieBox, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieTimeScale, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieDuration, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieTimeBase, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieTimeScale, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieTime, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieTime, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieVolume, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieVolume, m_ok );
    wxDL_METHOD_LOAD( m_dll, SetMovieTimeValue, m_ok );
    wxDL_METHOD_LOAD( m_dll, NewMovieController, m_ok );
    wxDL_METHOD_LOAD( m_dll, DisposeMovieController, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCSetVisible, m_ok );
    wxDL_METHOD_LOAD( m_dll, PrePrerollMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, PrerollMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMoviePreferredRate, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMovieLoadState, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCDoAction, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCSetControllerBoundsRect, m_ok );
    wxDL_METHOD_LOAD( m_dll, NativeEventToMacEvent, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCIsPlayerEvent, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCSetMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCSetActionFilterWithRefCon, m_ok );
    wxDL_METHOD_LOAD( m_dll, MCGetControllerInfo, m_ok );
    wxDL_METHOD_LOAD( m_dll, BeginUpdate, m_ok );
    wxDL_METHOD_LOAD( m_dll, UpdateMovie, m_ok );
    wxDL_METHOD_LOAD( m_dll, EndUpdate, m_ok );
    wxDL_METHOD_LOAD( m_dll, GetMoviesStickyError, m_ok );

    m_ok = true;

    return true;
}

class WXDLLIMPEXP_MEDIA wxQTMediaBackend : public wxMediaBackendCommonBase
{
public:
    wxQTMediaBackend();
    virtual ~wxQTMediaBackend();

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

    virtual bool Load(const wxURI& location,
                      const wxURI& proxy)
    { return wxMediaBackend::Load(location, proxy); }

    virtual bool Load(const wxString& fileName);
    virtual bool Load(const wxURI& location);

    virtual wxMediaState GetState();

    virtual bool SetPosition(wxLongLong where);
    virtual wxLongLong GetPosition();
    virtual wxLongLong GetDuration();

    virtual void Move(int x, int y, int w, int h);
    wxSize GetVideoSize() const;

    virtual double GetPlaybackRate();
    virtual bool SetPlaybackRate(double dRate);

    virtual double GetVolume();
    virtual bool SetVolume(double);

    void Cleanup();
    void FinishLoad();

    static void PPRMProc (Movie theMovie, OSErr theErr, void* theRefCon);

    // TODO: Last param actually long - does this work on 64bit machines?
    static Boolean MCFilterProc(MovieController theController,
        short action, void *params, LONG_PTR refCon);

    static LRESULT CALLBACK QTWndProc(HWND, UINT, WPARAM, LPARAM);

    virtual bool ShowPlayerControls(wxMediaCtrlPlayerControls flags);

    wxSize m_bestSize;              // Original movie size
    Movie m_movie;    // QT Movie handle/instance
    bool m_bVideo;                  // Whether or not we have video
    bool m_bPlaying;                // Whether or not movie is playing
    wxTimer* m_timer;               // Load or Play timer
    wxQuickTimeLibrary m_lib;       // DLL to load functions from
    ComponentInstance m_pMC;        // Movie Controller

    friend class wxQTMediaEvtHandler;

    DECLARE_DYNAMIC_CLASS(wxQTMediaBackend)
};

// helper to hijack background erasing for the QT window
class WXDLLIMPEXP_MEDIA wxQTMediaEvtHandler : public wxEvtHandler
{
public:
    wxQTMediaEvtHandler(wxQTMediaBackend *qtb, WXHWND hwnd)
    {
        m_qtb = qtb;
        m_hwnd = hwnd;

        m_qtb->m_ctrl->Connect(m_qtb->m_ctrl->GetId(),
            wxEVT_ERASE_BACKGROUND,
            wxEraseEventHandler(wxQTMediaEvtHandler::OnEraseBackground),
            NULL, this);
    }

    void OnEraseBackground(wxEraseEvent& event);

private:
    wxQTMediaBackend *m_qtb;
    WXHWND m_hwnd;

    DECLARE_NO_COPY_CLASS(wxQTMediaEvtHandler)
};


//===========================================================================
//  IMPLEMENTATION
//===========================================================================


//---------------------------------------------------------------------------
// wxQTMediaBackend
//
// TODO: Use a less kludgy way to pause/get state/set state
// FIXME: Greg Hazel reports that sometimes files that cannot be played
// with this backend are treated as playable anyway - not verified though.
//---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxQTMediaBackend, wxMediaBackend)

// Time between timer calls - this is the Apple recommendation to the TCL
// team I believe
#define MOVIE_DELAY 20

//---------------------------------------------------------------------------
// wxQTLoadTimer
//
//  QT, esp. QT for Windows is very picky about how you go about
//  async loading.  If you were to go through a Windows message loop
//  or a MoviesTask or both and then check the movie load state
//  it would still return 1000 (loading)... even (pre)prerolling doesn't
//  help.  However, making a load timer like this works
//---------------------------------------------------------------------------
class wxQTLoadTimer : public wxTimer
{
public:
    wxQTLoadTimer(Movie movie, wxQTMediaBackend* parent, wxQuickTimeLibrary* pLib) :
      m_movie(movie), m_parent(parent), m_pLib(pLib) {}

    void Notify()
    {
        m_pLib->MoviesTask(m_movie, 0);
        // kMovieLoadStatePlayable
        if (m_pLib->GetMovieLoadState(m_movie) >= 10000)
        {
            m_parent->FinishLoad();
            delete this;
        }
    }

protected:
    Movie m_movie;                  //Our movie instance
    wxQTMediaBackend* m_parent;     //Backend pointer
    wxQuickTimeLibrary* m_pLib;     //Interfaces
};


// --------------------------------------------------------------------------
// wxQTPlayTimer - Handle Asyncronous Playing
//
// 1) Checks to see if the movie is done, and if not continues
//    streaming the movie
// 2) Sends the wxEVT_MEDIA_STOP event if we have reached the end of
//    the movie.
// --------------------------------------------------------------------------
class wxQTPlayTimer : public wxTimer
{
public:
    wxQTPlayTimer(Movie movie, wxQTMediaBackend* parent,
                  wxQuickTimeLibrary* pLib) :
        m_movie(movie), m_parent(parent), m_pLib(pLib) {}

    void Notify()
    {
        //
        //  OK, a little explaining - basically originally
        //  we only called MoviesTask if the movie was actually
        //  playing (not paused or stopped)... this was before
        //  we realized MoviesTask actually handles repainting
        //  of the current frame - so if you were to resize
        //  or something it would previously not redraw that
        //  portion of the movie.
        //
        //  So now we call MoviesTask always so that it repaints
        //  correctly.
        //
        m_pLib->MoviesTask(m_movie, 0);

        //
        // Handle the stop event - if the movie has reached
        // the end, notify our handler
        //
        //  m_bPlaying == !(Stopped | Paused)
        //
        if (m_parent->m_bPlaying)
        {
            if (m_pLib->IsMovieDone(m_movie))
            {
                if ( m_parent->SendStopEvent() )
                {
                    m_parent->Stop();
                    wxASSERT(m_pLib->GetMoviesError() == noErr);

                    m_parent->QueueFinishEvent();
                }
            }
        }
    }

protected:
    Movie m_movie;                  // Our movie instance
    wxQTMediaBackend* m_parent;     //Backend pointer
    wxQuickTimeLibrary* m_pLib;         //Interfaces
};


//---------------------------------------------------------------------------
// wxQTMediaBackend::QTWndProc
//
// Forwards events to the Movie Controller so that it can
// redraw itself/process messages etc..
//---------------------------------------------------------------------------
LRESULT CALLBACK wxQTMediaBackend::QTWndProc(HWND hWnd, UINT nMsg,
                                             WPARAM wParam, LPARAM lParam)
{
    wxQTMediaBackend* pThis = (wxQTMediaBackend*)wxGetWindowUserData(hWnd);

    MSG msg;
    msg.hwnd = hWnd;
    msg.message = nMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = 0;
    msg.pt.x = 0;
    msg.pt.y = 0;
    EventRecord theEvent;
    pThis->m_lib.NativeEventToMacEvent(&msg, &theEvent);
    pThis->m_lib.MCIsPlayerEvent(pThis->m_pMC, &theEvent);

    return pThis->m_ctrl->MSWWindowProc(nMsg, wParam, lParam);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend Destructor
//
// Sets m_timer to NULL signifying we havn't loaded anything yet
//---------------------------------------------------------------------------
wxQTMediaBackend::wxQTMediaBackend()
: m_movie(NULL), m_bPlaying(false), m_timer(NULL), m_pMC(NULL)
{
}

//---------------------------------------------------------------------------
// wxQTMediaBackend Destructor
//
// 1) Cleans up the QuickTime movie instance
// 2) Decrements the QuickTime reference counter - if this reaches
//    0, QuickTime shuts down
// 3) Decrements the QuickTime Windows Media Layer reference counter -
//    if this reaches 0, QuickTime shuts down the Windows Media Layer
//---------------------------------------------------------------------------
wxQTMediaBackend::~wxQTMediaBackend()
{
    if (m_movie)
        Cleanup();

    if (m_lib.IsOk())
    {
        if (m_pMC)
        {
            m_lib.DisposeMovieController(m_pMC);
            // m_pMC = NULL;
        }

        // destroy wxQTMediaEvtHandler we pushed on it
        m_ctrl->PopEventHandler(true);

        m_lib.DestroyPortAssociation(
            (CGrafPtr)m_lib.GetNativeWindowPort(m_ctrl->GetHWND()));

        //Note that ExitMovies() is not necessary, but
        //the docs are fuzzy on whether or not TerminateQTML is
        m_lib.ExitMovies();
        m_lib.TerminateQTML();
    }
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::CreateControl
//
// 1) Intializes QuickTime
// 2) Creates the control window
//---------------------------------------------------------------------------
bool wxQTMediaBackend::CreateControl(wxControl* ctrl, wxWindow* parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     const wxValidator& validator,
                                     const wxString& name)
{
    if (!m_lib.Initialize())
        return false;

    int nError = m_lib.InitializeQTML(0);
    if (nError != noErr)    //-2093 no dll
    {
        wxFAIL_MSG(wxString::Format(wxT("Couldn't Initialize Quicktime-%i"), nError));
        return false;
    }

    m_lib.EnterMovies();

    // Create window
    // By default wxWindow(s) is created with a border -
    // so we need to get rid of those
    //
    // Since we don't have a child window like most other
    // backends, we don't need wxCLIP_CHILDREN
    if ( !ctrl->wxControl::Create(parent, id, pos, size,
                            (style & ~wxBORDER_MASK) | wxBORDER_NONE,
                            validator, name) )
    {
        return false;
    }

    m_ctrl = wxStaticCast(ctrl, wxMediaCtrl);

    // Create a port association for our window so we
    // can use it as a WindowRef
    m_lib.CreatePortAssociation(m_ctrl->GetHWND(), NULL, 0L);

    // Part of a suggestion from Greg Hazel
    // to repaint movie when idle
    m_ctrl->PushEventHandler(new wxQTMediaEvtHandler(this, m_ctrl->GetHWND()));

    // done
    return true;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Load (file version)
//
// 1) Get an FSSpec from the Windows path name
// 2) Open the movie
// 3) Obtain the movie instance from the movie resource
// 4) Close the movie resource
// 5) Finish loading
//---------------------------------------------------------------------------
bool wxQTMediaBackend::Load(const wxString& fileName)
{
    if (m_movie)
        Cleanup();

    bool result = true;
    OSErr err = noErr;
    short movieResFile = 0; //= 0 because of annoying VC6 warning
    FSSpec sfFile;

    err = m_lib.NativePathNameToFSSpec(
        (char*) (const char*) fileName.mb_str(),
        &sfFile, 0);
    result = (err == noErr);

    if (result)
    {
        err = m_lib.OpenMovieFile(&sfFile, &movieResFile, fsRdPerm);
        result = (err == noErr);
    }

    if (result)
    {
        short movieResID = 0;
        Str255 movieName;

        err = m_lib.NewMovieFromFile(
                   &m_movie,
                   movieResFile,
                   &movieResID,
                   movieName,
                   newMovieActive,
                   NULL ); // wasChanged
        result = (err == noErr /*&& m_lib.GetMoviesStickyError() == noErr*/);

        // check m_lib.GetMoviesStickyError() because it may not find the
        // proper codec and play black video and other strange effects,
        // not to mention mess up the dynamic backend loading scheme
        // of wxMediaCtrl - so it just does what the QuickTime player does
        if (result)
        {
            m_lib.CloseMovieFile(movieResFile);
            FinishLoad();
        }
    }

    return result;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::PPRMProc (static)
//
// Called when done PrePrerolling the movie.
// Note that in 99% of the cases this does nothing...
// Anyway we set up the loading timer here to tell us when the movie is done
//---------------------------------------------------------------------------
void wxQTMediaBackend::PPRMProc (Movie theMovie,
                                 OSErr WXUNUSED_UNLESS_DEBUG(theErr),
                                 void* theRefCon)
{
    wxASSERT( theMovie );
    wxASSERT( theRefCon );
    wxASSERT( theErr == noErr );

    wxQTMediaBackend* pBE = (wxQTMediaBackend*) theRefCon;

    long lTime = pBE->m_lib.GetMovieTime(theMovie,NULL);
    Fixed rate = pBE->m_lib.GetMoviePreferredRate(theMovie);
    pBE->m_lib.PrerollMovie(theMovie, lTime, rate);
    pBE->m_timer = new wxQTLoadTimer(pBE->m_movie, pBE, &pBE->m_lib);
    pBE->m_timer->Start(MOVIE_DELAY);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Load (URL Version)
//
// 1) Build an escaped URI from location
// 2) Create a handle to store the URI string
// 3) Put the URI string inside the handle
// 4) Make a QuickTime URL data ref from the handle with the URI in it
// 5) Clean up the URI string handle
// 6) Do some prerolling
// 7) Finish Loading
//---------------------------------------------------------------------------
bool wxQTMediaBackend::Load(const wxURI& location)
{
    if (m_movie)
        Cleanup();

    wxString theURI = location.BuildURI();

    Handle theHandle = m_lib.NewHandleClear(theURI.length() + 1);
    wxASSERT(theHandle);

    m_lib.BlockMove(theURI.mb_str(), *theHandle, theURI.length() + 1);

    // create the movie from the handle that refers to the URI
    OSErr err = m_lib.NewMovieFromDataRef(&m_movie, newMovieActive |
                                                    newMovieAsyncOK
                                                    /* | newMovieIdleImportOK */,
                                NULL, theHandle,
                                URLDataHandlerSubType);

    m_lib.DisposeHandle(theHandle);

    if (err == noErr)
    {
        long timeNow;
        Fixed playRate;

        timeNow = m_lib.GetMovieTime(m_movie, NULL);
        wxASSERT(m_lib.GetMoviesError() == noErr);

        playRate = m_lib.GetMoviePreferredRate(m_movie);
        wxASSERT(m_lib.GetMoviesError() == noErr);

        // Note that the callback here is optional,
        // but without it PrePrerollMovie can be buggy
        // (see Apple ml).  Also, some may wonder
        // why we need this at all - this is because
        // Apple docs say QuickTime streamed movies
        // require it if you don't use a Movie Controller,
        // which we don't by default.
        //
        m_lib.PrePrerollMovie(m_movie, timeNow, playRate,
                              (WXFARPROC)wxQTMediaBackend::PPRMProc,
                              (void*)this);

        return true;
    }
    else
        return false;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::FinishLoad
//
// 1) Create the movie timer
// 2) Get real size of movie for GetBestSize/sizers
// 3) Set the movie time scale to something usable so that seeking
//    etc.  will work correctly
// 4) Set our Movie Controller to display the movie if it exists,
//    otherwise set the bounds of the Movie
// 5) Refresh parent window
//---------------------------------------------------------------------------
void wxQTMediaBackend::FinishLoad()
{
    // Create the playing/streaming timer
    m_timer = new wxQTPlayTimer(m_movie, (wxQTMediaBackend*) this, &m_lib);
    wxASSERT(m_timer);

    m_timer->Start(MOVIE_DELAY, wxTIMER_CONTINUOUS);

    // get the real size of the movie
    Rect outRect;
    memset(&outRect, 0, sizeof(Rect)); // suppress annoying VC6 warning
    m_lib.GetMovieNaturalBoundsRect (m_movie, &outRect);
    wxASSERT(m_lib.GetMoviesError() == noErr);

    m_bestSize.x = outRect.right - outRect.left;
    m_bestSize.y = outRect.bottom - outRect.top;

    // Handle the movie GWorld
    if (m_pMC)
    {
        Point thePoint;
        thePoint.h = thePoint.v = 0;
        m_lib.MCSetMovie(m_pMC, m_movie,
               m_lib.GetNativeWindowPort(m_ctrl->GetHandle()),
               thePoint);
        m_lib.MCSetVisible(m_pMC, true);
        m_bestSize.y += 16;
    }
    else
    {
        m_lib.SetMovieGWorld(m_movie,
                       (CGrafPtr) m_lib.GetNativeWindowPort(m_ctrl->GetHWND()),
                       NULL);
    }

    // Set the movie to millisecond precision
    m_lib.SetMovieTimeScale(m_movie, 1000);
    wxASSERT(m_lib.GetMoviesError() == noErr);

    NotifyMovieLoaded();
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Play
//
// 1) Start the QT movie
// 2) Start the movie loading timer
//
// NOTE:  This will still return success even when
// the movie is still loading, and as mentioned in wxQTLoadTimer
// I don't know of a way to force this to be sync - so if its
// still loading the function will return true but the movie will
// still be in the stopped state
//---------------------------------------------------------------------------
bool wxQTMediaBackend::Play()
{
    m_lib.StartMovie(m_movie);
    m_bPlaying = true;

    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Pause
//
// 1) Stop the movie
// 2) Stop the movie timer
//---------------------------------------------------------------------------
bool wxQTMediaBackend::Pause()
{
    m_bPlaying = false;
    m_lib.StopMovie(m_movie);

    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Stop
//
// 1) Stop the movie
// 2) Stop the movie timer
// 3) Seek to the beginning of the movie
//---------------------------------------------------------------------------
bool wxQTMediaBackend::Stop()
{
    m_bPlaying = false;

    m_lib.StopMovie(m_movie);
    if (m_lib.GetMoviesError() == noErr)
        m_lib.GoToBeginningOfMovie(m_movie);

    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetPlaybackRate
//
// Get the movie playback rate from ::GetMovieRate
//---------------------------------------------------------------------------
double wxQTMediaBackend::GetPlaybackRate()
{
    return ( ((double)m_lib.GetMovieRate(m_movie)) / 0x10000);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::SetPlaybackRate
//
// Convert dRate to Fixed and Set the movie rate through SetMovieRate
//---------------------------------------------------------------------------
bool wxQTMediaBackend::SetPlaybackRate(double dRate)
{
    m_lib.SetMovieRate(m_movie, (Fixed) (dRate * 0x10000));

    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::SetPosition
//
// 1) Create a time record struct (TimeRecord) with appropriate values
// 2) Pass struct to SetMovieTime
//---------------------------------------------------------------------------
bool wxQTMediaBackend::SetPosition(wxLongLong where)
{
    // NB:  For some reason SetMovieTime does not work
    // correctly with the Quicktime Windows SDK (6)
    // From Muskelkatermann at the wxForum
    // http://www.solidsteel.nl/users/wxwidgets/viewtopic.php?t=2957
    // RN - note that I have not verified this but there
    // is no harm in calling SetMovieTimeValue instead
#if 0
    TimeRecord theTimeRecord;
    memset(&theTimeRecord, 0, sizeof(TimeRecord));
    theTimeRecord.value.lo = where.GetLo();
    theTimeRecord.scale = m_lib.GetMovieTimeScale(m_movie);
    theTimeRecord.base = m_lib.GetMovieTimeBase(m_movie);
    m_lib.SetMovieTime(m_movie, &theTimeRecord);
#else
    m_lib.SetMovieTimeValue(m_movie, where.GetLo());
#endif

    return (m_lib.GetMoviesError() == noErr);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetPosition
//
// 1) Calls GetMovieTime to get the position we are in in the movie
// in milliseconds (we called
//---------------------------------------------------------------------------
wxLongLong wxQTMediaBackend::GetPosition()
{
    return m_lib.GetMovieTime(m_movie, NULL);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetVolume
//
// Gets the volume through GetMovieVolume - which returns a 16 bit short -
//
// +--------+--------+
// +   (1)  +   (2)  +
// +--------+--------+
//
// (1) first 8 bits are value before decimal
// (2) second 8 bits are value after decimal
//
// Volume ranges from -1.0 (gain but no sound), 0 (no sound and no gain) to
// 1 (full gain and sound)
//---------------------------------------------------------------------------
double wxQTMediaBackend::GetVolume()
{
    short sVolume = m_lib.GetMovieVolume(m_movie);
    wxASSERT(m_lib.GetMoviesError() == noErr);

    if (sVolume & (128 << 8)) //negative - no sound
        return 0.0;

    return sVolume / 256.0;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::SetVolume
//
// Sets the volume through SetMovieVolume - which takes a 16 bit short -
//
// +--------+--------+
// +   (1)  +   (2)  +
// +--------+--------+
//
// (1) first 8 bits are value before decimal
// (2) second 8 bits are value after decimal
//
// Volume ranges from -1.0 (gain but no sound), 0 (no sound and no gain) to
// 1 (full gain and sound)
//---------------------------------------------------------------------------
bool wxQTMediaBackend::SetVolume(double dVolume)
{
    m_lib.SetMovieVolume(m_movie, (short) (dVolume * 256));
    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetDuration
//
// Calls GetMovieDuration
//---------------------------------------------------------------------------
wxLongLong wxQTMediaBackend::GetDuration()
{
    return m_lib.GetMovieDuration(m_movie);
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetState
//
// Determines the current state:
// if we are at the beginning, then we are stopped
//---------------------------------------------------------------------------
wxMediaState wxQTMediaBackend::GetState()
{
    if (m_bPlaying)
        return wxMEDIASTATE_PLAYING;
    else if ( !m_movie || wxQTMediaBackend::GetPosition() == 0 )
        return wxMEDIASTATE_STOPPED;
    else
        return wxMEDIASTATE_PAUSED;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Cleanup
//
// Diposes of the movie timer, Disassociates the Movie Controller with
// movie and hides it if it exists, and stops and disposes
// of the QT movie
//---------------------------------------------------------------------------
void wxQTMediaBackend::Cleanup()
{
    m_bPlaying = false;

    if (m_timer)
    {
        delete m_timer;
        m_timer = NULL;
    }

    m_lib.StopMovie(m_movie);

    if (m_pMC)
    {
        Point thePoint;
        thePoint.h = thePoint.v = 0;
        m_lib.MCSetVisible(m_pMC, false);
        m_lib.MCSetMovie(m_pMC, NULL, NULL, thePoint);
    }

    m_lib.DisposeMovie(m_movie);
    m_movie = NULL;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::ShowPlayerControls
//
// Creates a movie controller for the Movie if the user wants it
//---------------------------------------------------------------------------
bool wxQTMediaBackend::ShowPlayerControls(wxMediaCtrlPlayerControls flags)
{
    if (m_pMC)
    {
        // restore old wndproc
        wxSetWindowProc((HWND)m_ctrl->GetHWND(), wxWndProc);
        m_lib.DisposeMovieController(m_pMC);
        m_pMC = NULL;

        // movie controller height
        m_bestSize.y -= 16;
    }

    if (flags && m_movie)
    {
        Rect rect;
        wxRect wxrect = m_ctrl->GetClientRect();

        // make room for controller
        if (wxrect.width < 320)
            wxrect.width = 320;

        rect.top = (short)wxrect.y;
        rect.left = (short)wxrect.x;
        rect.right = (short)(rect.left + wxrect.width);
        rect.bottom = (short)(rect.top + wxrect.height);

        if (!m_pMC)
        {
            m_pMC = m_lib.NewMovieController(m_movie, &rect, mcTopLeftMovie |
                                //                        mcScaleMovieToFit |
                                //                        mcWithBadge |
                                                        mcWithFrame);
            m_lib.MCDoAction(m_pMC, 32, (void*)true); // mcActionSetKeysEnabled
            m_lib.MCSetActionFilterWithRefCon(m_pMC,
                (WXFARPROC)wxQTMediaBackend::MCFilterProc, (void*)this);
            m_bestSize.y += 16; // movie controller height

            // By default the movie controller uses its own colour palette
            // for the movie which can be bad on some files, so turn it off.
            // Also turn off its frame / border for the movie
            // Also take care of a couple of the interface flags here
            long mcFlags = 0;
            m_lib.MCDoAction(m_pMC, 39/*mcActionGetFlags*/, (void*)&mcFlags);

            mcFlags |=
                // (1<< 0) /*mcFlagSuppressMovieFrame*/ |
                (1<< 3) /*mcFlagsUseWindowPalette*/
                | ((flags & wxMEDIACTRLPLAYERCONTROLS_STEP)
                      ? 0 : (1<< 1) /*mcFlagSuppressStepButtons*/)
                | ((flags & wxMEDIACTRLPLAYERCONTROLS_VOLUME)
                      ? 0 : (1<< 2) /*mcFlagSuppressSpeakerButton*/)
//              | (1<< 4) /*mcFlagDontInvalidate*/ // if we take care of repainting ourselves
                      ;

            m_lib.MCDoAction(m_pMC, 38/*mcActionSetFlags*/, (void*)mcFlags);

            // intercept the wndproc of our control window
            wxSetWindowProc((HWND)m_ctrl->GetHWND(), wxQTMediaBackend::QTWndProc);

            // set the user data of our window
            wxSetWindowUserData((HWND)m_ctrl->GetHWND(), this);
        }
    }

    NotifyMovieSizeChanged();

    return m_lib.GetMoviesError() == noErr;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::MCFilterProc (static)
//
// Callback for when the movie controller recieves a message
//---------------------------------------------------------------------------
Boolean wxQTMediaBackend::MCFilterProc(MovieController WXUNUSED(theController),
                               short action,
                               void * WXUNUSED(params),
                               LONG_PTR refCon)
{
// NB: potential optimisation
//    if (action == 1)
//        return 0;

    wxQTMediaBackend* pThis = (wxQTMediaBackend*)refCon;

    switch (action)
    {
    case 1:
        // don't process idle events
        break;

    case 8:
        // play button triggered - MC will set movie to opposite state
        // of current - playing ? paused : playing
        if (pThis)
            pThis->m_bPlaying = !(pThis->m_bPlaying);

        // NB: Sometimes it doesn't redraw properly -
        // if you click on the button but don't move the mouse
        // the button will not change its state until you move
        // mcActionDraw and Refresh/Update combo do nothing
        // to help this unfortunately
        break;

    default:
        break;
    }

    return 0;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::GetVideoSize
//
// Returns the actual size of the QT movie
//---------------------------------------------------------------------------
wxSize wxQTMediaBackend::GetVideoSize() const
{
    return m_bestSize;
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::Move
//
// Sets the bounds of either the Movie or Movie Controller
//---------------------------------------------------------------------------
void wxQTMediaBackend::Move(int WXUNUSED(x), int WXUNUSED(y), int w, int h)
{
    if (m_movie)
    {
        // make room for controller
        if (m_pMC)
        {
            if (w < 320)
                w = 320;

            Rect theRect = {0, 0, (short)h, (short)w};
            m_lib.MCSetControllerBoundsRect(m_pMC, &theRect);
        }
        else
        {
            Rect theRect = {0, 0, (short)h, (short)w};
            m_lib.SetMovieBox(m_movie, &theRect);
        }

        wxASSERT(m_lib.GetMoviesError() == noErr);
    }
}

//---------------------------------------------------------------------------
// wxQTMediaBackend::OnEraseBackground
//
// Suggestion from Greg Hazel to repaint the movie when idle
// (on pause also)
//
// TODO: We may be repainting too much here - under what exact circumstances
// do we need this? I think Move also repaints correctly for the Movie
// Controller, so in that instance we don't need this either
//---------------------------------------------------------------------------
void wxQTMediaEvtHandler::OnEraseBackground(wxEraseEvent& evt)
{
    wxQuickTimeLibrary& m_pLib = m_qtb->m_lib;

    if ( m_qtb->m_pMC )
    {
        // repaint movie controller
        m_pLib.MCDoAction(m_qtb->m_pMC, 2 /*mcActionDraw*/,
                            m_pLib.GetNativeWindowPort(m_hwnd));
    }
    else if ( m_qtb->m_movie )
    {
        // no movie controller
        CGrafPtr port = (CGrafPtr)m_pLib.GetNativeWindowPort(m_hwnd);

        m_pLib.BeginUpdate(port);
        m_pLib.UpdateMovie(m_qtb->m_movie);
        wxASSERT(m_pLib.GetMoviesError() == noErr);
        m_pLib.EndUpdate(port);
    }
    else
    {
        // no movie
        // let the system repaint the window
        evt.Skip();
    }
}

//---------------------------------------------------------------------------
//  End QT Backend
//---------------------------------------------------------------------------

// in source file that contains stuff you don't directly use
#include "wx/html/forcelnk.h"
FORCE_LINK_ME(wxmediabackend_qt)

#endif // wxUSE_MEDIACTRL && wxUSE_ACTIVEX
