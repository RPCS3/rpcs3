//------------------------------------------------------------------------------
// File: CtlUtil.h
//
// Desc: DirectShow base classes.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// Base classes implementing IDispatch parsing for the basic control dual
// interfaces. Derive from these and implement just the custom method and
// property methods. We also implement CPosPassThru that can be used by
// renderers and transforms to pass by IMediaPosition and IMediaSeeking

#ifndef __CTLUTIL__
#define __CTLUTIL__

// OLE Automation has different ideas of TRUE and FALSE

#define OATRUE (-1)
#define OAFALSE (0)


// It's possible that we could replace this class with CreateStdDispatch

class CBaseDispatch
{
    ITypeInfo * m_pti;

public:

    CBaseDispatch() : m_pti(NULL) {}
    ~CBaseDispatch();

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      REFIID riid,
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);
};


class AM_NOVTABLE CMediaControl :
    public IMediaControl,
    public CUnknown
{
    CBaseDispatch m_basedisp;

public:

    CMediaControl(const TCHAR *, LPUNKNOWN);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);
};


class AM_NOVTABLE CMediaEvent :
    public IMediaEventEx,
    public CUnknown
{
    CBaseDispatch m_basedisp;

public:

    CMediaEvent(const TCHAR *, LPUNKNOWN);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);
};


class AM_NOVTABLE CMediaPosition :
    public IMediaPosition,
    public CUnknown
{
    CBaseDispatch m_basedisp;


public:

    CMediaPosition(const TCHAR *, LPUNKNOWN);
    CMediaPosition(const TCHAR *, LPUNKNOWN, HRESULT *phr);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

};


// OA-compatibility means that we must use double as the RefTime value,
// and REFERENCE_TIME (essentially a LONGLONG) within filters.
// this class converts between the two

class COARefTime : public CRefTime {
public:

    COARefTime() {
    };

    COARefTime(CRefTime t)
        : CRefTime(t)
    {
    };

    COARefTime(REFERENCE_TIME t)
        : CRefTime(t)
    {
    };

    COARefTime(double d) {
        m_time = (LONGLONG) (d * 10000000);
    };

    operator double() {
        return double(m_time) / 10000000;
    };

    operator REFERENCE_TIME() {
        return m_time;
    };

    COARefTime& operator=(const double& rd)  {
        m_time = (LONGLONG) (rd * 10000000);
        return *this;
    }

    COARefTime& operator=(const REFERENCE_TIME& rt)  {
        m_time = rt;
        return *this;
    }

    inline BOOL operator==(const COARefTime& rt)
    {
        return m_time == rt.m_time;
    };

    inline BOOL operator!=(const COARefTime& rt)
    {
        return m_time != rt.m_time;
    };

    inline BOOL operator < (const COARefTime& rt)
    {
        return m_time < rt.m_time;
    };

    inline BOOL operator > (const COARefTime& rt)
    {
        return m_time > rt.m_time;
    };

    inline BOOL operator >= (const COARefTime& rt)
    {
        return m_time >= rt.m_time;
    };

    inline BOOL operator <= (const COARefTime& rt)
    {
        return m_time <= rt.m_time;
    };

    inline COARefTime operator+(const COARefTime& rt)
    {
        return COARefTime(m_time + rt.m_time);
    };

    inline COARefTime operator-(const COARefTime& rt)
    {
        return COARefTime(m_time - rt.m_time);
    };

    inline COARefTime operator*(LONG l)
    {
        return COARefTime(m_time * l);
    };

    inline COARefTime operator/(LONG l)
    {
        return COARefTime(m_time / l);
    };

private:
    //  Prevent bugs from constructing from LONG (which gets
    //  converted to double and then multiplied by 10000000
    COARefTime(LONG);
    int operator=(LONG);
};


// A utility class that handles IMediaPosition and IMediaSeeking on behalf
// of single-input pin renderers, or transform filters.
//
// Renderers will expose this from the filter; transform filters will
// expose it from the output pin and not the renderer.
//
// Create one of these, giving it your IPin* for your input pin, and delegate
// all IMediaPosition methods to it. It will query the input pin for
// IMediaPosition and respond appropriately.
//
// Call ForceRefresh if the pin connection changes.
//
// This class no longer caches the upstream IMediaPosition or IMediaSeeking
// it acquires it on each method call. This means ForceRefresh is not needed.
// The method is kept for source compatibility and to minimise the changes
// if we need to put it back later for performance reasons.

class CPosPassThru : public IMediaSeeking, public CMediaPosition
{
    IPin *m_pPin;

    HRESULT GetPeer(IMediaPosition **ppMP);
    HRESULT GetPeerSeeking(IMediaSeeking **ppMS);

public:

    CPosPassThru(const TCHAR *, LPUNKNOWN, HRESULT*, IPin *);
    DECLARE_IUNKNOWN

    HRESULT ForceRefresh() {
        return S_OK;
    };

    // override to return an accurate current position
    virtual HRESULT GetMediaTime(LONGLONG *pStartTime,LONGLONG *pEndTime) {
        return E_FAIL;
    }

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // IMediaSeeking methods
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsFormatSupported( const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat( GUID *pFormat);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                   LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetCurrentPosition( LONGLONG * pCurrent );
    STDMETHODIMP GetStopPosition( LONGLONG * pStop );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetDuration( LONGLONG *pDuration);
    STDMETHODIMP GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest );
    STDMETHODIMP GetPreroll( LONGLONG *pllPreroll );

    // IMediaPosition properties
    STDMETHODIMP get_Duration(REFTIME * plength);
    STDMETHODIMP put_CurrentPosition(REFTIME llTime);
    STDMETHODIMP get_StopTime(REFTIME * pllTime);
    STDMETHODIMP put_StopTime(REFTIME llTime);
    STDMETHODIMP get_PrerollTime(REFTIME * pllTime);
    STDMETHODIMP put_PrerollTime(REFTIME llTime);
    STDMETHODIMP get_Rate(double * pdRate);
    STDMETHODIMP put_Rate(double dRate);
    STDMETHODIMP get_CurrentPosition(REFTIME * pllTime);
    STDMETHODIMP CanSeekForward(LONG *pCanSeekForward);
    STDMETHODIMP CanSeekBackward(LONG *pCanSeekBackward);

private:
    HRESULT GetSeekingLongLong( HRESULT (__stdcall IMediaSeeking::*pMethod)( LONGLONG * ),
                                LONGLONG * pll );
};


// Adds the ability to return a current position

class CRendererPosPassThru : public CPosPassThru
{
    CCritSec m_PositionLock;    // Locks access to our position
    LONGLONG m_StartMedia;      // Start media time last seen
    LONGLONG m_EndMedia;        // And likewise the end media
    BOOL m_bReset;              // Have media times been set

public:

    // Used to help with passing media times through graph

    CRendererPosPassThru(const TCHAR *, LPUNKNOWN, HRESULT*, IPin *);
    HRESULT RegisterMediaTime(IMediaSample *pMediaSample);
    HRESULT RegisterMediaTime(LONGLONG StartTime,LONGLONG EndTime);
    HRESULT GetMediaTime(LONGLONG *pStartTime,LONGLONG *pEndTime);
    HRESULT ResetMediaTime();
    HRESULT EOS();
};

STDAPI CreatePosPassThru(
    LPUNKNOWN pAgg,
    BOOL bRenderer,
    IPin *pPin,
    IUnknown **ppPassThru
);

// A class that handles the IDispatch part of IBasicAudio and leaves the
// properties and methods themselves pure virtual.

class AM_NOVTABLE CBasicAudio : public IBasicAudio, public CUnknown
{
    CBaseDispatch m_basedisp;

public:

    CBasicAudio(const TCHAR *, LPUNKNOWN);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);
};


// A class that handles the IDispatch part of IBasicVideo and leaves the
// properties and methods themselves pure virtual.

class AM_NOVTABLE CBaseBasicVideo : public IBasicVideo2, public CUnknown
{
    CBaseDispatch m_basedisp;

public:

    CBaseBasicVideo(const TCHAR *, LPUNKNOWN);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);

    STDMETHODIMP GetPreferredAspectRatio(
      long *plAspectX,
      long *plAspectY)
    {
        return E_NOTIMPL;
    }
};


// A class that handles the IDispatch part of IVideoWindow and leaves the
// properties and methods themselves pure virtual.

class AM_NOVTABLE CBaseVideoWindow : public IVideoWindow, public CUnknown
{
    CBaseDispatch m_basedisp;

public:

    CBaseVideoWindow(const TCHAR *, LPUNKNOWN);

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IDispatch methods */
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo);

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid);

    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr);
};


// abstract class to help source filters with their implementation
// of IMediaPosition. Derive from this and set the duration (and stop
// position). Also override NotifyChange to do something when the properties
// change.

class AM_NOVTABLE CSourcePosition : public CMediaPosition
{

public:
    CSourcePosition(const TCHAR *, LPUNKNOWN, HRESULT*, CCritSec *);

    // IMediaPosition methods
    STDMETHODIMP get_Duration(REFTIME * plength);
    STDMETHODIMP put_CurrentPosition(REFTIME llTime);
    STDMETHODIMP get_StopTime(REFTIME * pllTime);
    STDMETHODIMP put_StopTime(REFTIME llTime);
    STDMETHODIMP get_PrerollTime(REFTIME * pllTime);
    STDMETHODIMP put_PrerollTime(REFTIME llTime);
    STDMETHODIMP get_Rate(double * pdRate);
    STDMETHODIMP put_Rate(double dRate);
    STDMETHODIMP CanSeekForward(LONG *pCanSeekForward);
    STDMETHODIMP CanSeekBackward(LONG *pCanSeekBackward);

    // override if you can return the data you are actually working on
    STDMETHODIMP get_CurrentPosition(REFTIME * pllTime) {
        return E_NOTIMPL;
    };

protected:

    // we call this to notify changes. Override to handle them
    virtual HRESULT ChangeStart() PURE;
    virtual HRESULT ChangeStop() PURE;
    virtual HRESULT ChangeRate() PURE;

    COARefTime m_Duration;
    COARefTime m_Start;
    COARefTime m_Stop;
    double m_Rate;

    CCritSec * m_pLock;
};

class AM_NOVTABLE CSourceSeeking :
    public IMediaSeeking,
    public CUnknown
{

public:

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IMediaSeeking methods

    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                                    LONGLONG    Source, const GUID * pSourceFormat );

    STDMETHODIMP SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
			     , LONGLONG * pStop,  DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );

    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll);


protected:

    // ctor
    CSourceSeeking(const TCHAR *, LPUNKNOWN, HRESULT*, CCritSec *);

    // we call this to notify changes. Override to handle them
    virtual HRESULT ChangeStart() PURE;
    virtual HRESULT ChangeStop() PURE;
    virtual HRESULT ChangeRate() PURE;

    CRefTime m_rtDuration;      // length of stream
    CRefTime m_rtStart;         // source will start here
    CRefTime m_rtStop;          // source will stop here
    double m_dRateSeeking;

    // seeking capabilities
    DWORD m_dwSeekingCaps;

    CCritSec * m_pLock;
};


// Base classes supporting Deferred commands.

// Deferred commands are queued by calls to methods on the IQueueCommand
// interface, exposed by the filtergraph and by some filters. A successful
// call to one of these methods will return an IDeferredCommand interface
// representing the queued command.
//
// A CDeferredCommand object represents a single deferred command, and exposes
// the IDeferredCommand interface as well as other methods permitting time
// checks and actual execution. It contains a reference to the CCommandQueue
// object on which it is queued.
//
// CCommandQueue is a base class providing a queue of CDeferredCommand
// objects, and methods to add, remove, check status and invoke the queued
// commands. A CCommandQueue object would be part of an object that
// implemented IQueueCommand.

class CCmdQueue;

// take a copy of the params and store them. Release any allocated
// memory in destructor

class CDispParams : public DISPPARAMS
{
public:
    CDispParams(UINT nArgs, VARIANT* pArgs, HRESULT *phr = NULL);
    ~CDispParams();
};


// CDeferredCommand lifetime is controlled by refcounts. Caller of
// InvokeAt.. gets a refcounted interface pointer, and the CCmdQueue
// object also holds a refcount on us. Calling Cancel or Invoke takes
// us off the CCmdQueue and thus reduces the refcount by 1. Once taken
// off the queue we cannot be put back on the queue.

class CDeferredCommand
    : public CUnknown,
      public IDeferredCommand
{
public:

    CDeferredCommand(
        CCmdQueue * pQ,
        LPUNKNOWN   pUnk,               // aggregation outer unk
        HRESULT *   phr,
        LPUNKNOWN   pUnkExecutor,       // object that will execute this cmd
        REFTIME     time,
        GUID*       iid,
        long        dispidMethod,
        short       wFlags,
        long        cArgs,
        VARIANT*    pDispParams,
        VARIANT*    pvarResult,
        short*      puArgErr,
        BOOL        bStream
        );

    DECLARE_IUNKNOWN

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IDeferredCommand methods
    STDMETHODIMP Cancel();
    STDMETHODIMP Confidence(
                    LONG* pConfidence);
    STDMETHODIMP Postpone(
                    REFTIME newtime);
    STDMETHODIMP GetHResult(
                    HRESULT* phrResult);

    // other public methods

    HRESULT Invoke();

    // access methods

    // returns TRUE if streamtime, FALSE if presentation time
    BOOL IsStreamTime() {
       return m_bStream;
    };

    CRefTime GetTime() {
        return m_time;
    };

    REFIID GetIID() {
        return *m_iid;
    };

    long GetMethod() {
        return m_dispidMethod;
    };

    short GetFlags() {
        return m_wFlags;
    };

    DISPPARAMS* GetParams() {
        return &m_DispParams;
    };

    VARIANT* GetResult() {
        return m_pvarResult;
    };

protected:

    CCmdQueue* m_pQueue;

    // pUnk for the interface that we will execute the command on
    LPUNKNOWN   m_pUnk;

    // stored command data
    REFERENCE_TIME     m_time;
    GUID*       m_iid;
    long        m_dispidMethod;
    short       m_wFlags;
    VARIANT*    m_pvarResult;
    BOOL        m_bStream;
    CDispParams m_DispParams;
    DISPID      m_DispId;         //  For get and put

    // we use this for ITypeInfo access
    CBaseDispatch   m_Dispatch;

    // save retval here
    HRESULT     m_hrResult;
};


// a list of CDeferredCommand objects. this is a base class providing
// the basics of access to the list. If you want to use CDeferredCommand
// objects then your queue needs to be derived from this class.

class AM_NOVTABLE CCmdQueue
{
public:
    CCmdQueue();
    virtual ~CCmdQueue();

    // returns a new CDeferredCommand object that will be initialised with
    // the parameters and will be added to the queue during construction.
    // returns S_OK if successfully created otherwise an error and
    // no object has been queued.
    virtual HRESULT  New(
        CDeferredCommand **ppCmd,
        LPUNKNOWN   pUnk,
        REFTIME     time,
        GUID*       iid,
        long        dispidMethod,
        short       wFlags,
        long        cArgs,
        VARIANT*    pDispParams,
        VARIANT*    pvarResult,
        short*      puArgErr,
        BOOL        bStream
    );

    // called by the CDeferredCommand object to add and remove itself
    // from the queue
    virtual HRESULT Insert(CDeferredCommand* pCmd);
    virtual HRESULT Remove(CDeferredCommand* pCmd);

    // Command-Due Checking
    //
    // There are two schemes of synchronisation: coarse and accurate. In
    // coarse mode, you wait till the time arrives and then execute the cmd.
    // In accurate mode, you wait until you are processing the sample that
    // will appear at the time, and then execute the command. It's up to the
    // filter which one it will implement. The filtergraph will always
    // implement coarse mode for commands queued at the filtergraph.
    //
    // If you want coarse sync, you probably want to wait until there is a
    // command due, and then execute it. You can do this by calling
    // GetDueCommand. If you have several things to wait for, get the
    // event handle from GetDueHandle() and when this is signalled then call
    // GetDueCommand. Stream time will only advance between calls to Run and
    // EndRun. Note that to avoid an extra thread there is no guarantee that
    // if the handle is set there will be a command ready. Each time the
    // event is signalled, call GetDueCommand (probably with a 0 timeout);
    // This may return E_ABORT.
    //
    // If you want accurate sync, you must call GetCommandDueFor, passing
    // as a parameter the stream time of the samples you are about to process.
    // This will return:
    //   -- a stream-time command due at or before that stream time
    //   -- a presentation-time command due at or before the
    //      time that stream time will be presented (only between Run
    //      and EndRun calls, since outside of this, the mapping from
    //      stream time to presentation time is not known.
    //   -- any presentation-time command due now.
    // This means that if you want accurate synchronisation on samples that
    // might be processed during Paused mode, you need to use
    // stream-time commands.
    //
    // In all cases, commands remain queued until Invoked or Cancelled. The
    // setting and resetting of the event handle is managed entirely by this
    // queue object.

    // set the clock used for timing
    virtual HRESULT SetSyncSource(IReferenceClock*);

    // switch to run mode. Streamtime to Presentation time mapping known.
    virtual HRESULT Run(REFERENCE_TIME tStreamTimeOffset);

    // switch to Stopped or Paused mode. Time mapping not known.
    virtual HRESULT EndRun();

    // return a pointer to the next due command. Blocks for msTimeout
    // milliseconds until there is a due command.
    // Stream-time commands will only become due between Run and Endrun calls.
    // The command remains queued until invoked or cancelled.
    // Returns E_ABORT if timeout occurs, otherwise S_OK (or other error).
    // Returns an AddRef-ed object
    virtual HRESULT GetDueCommand(CDeferredCommand ** ppCmd, long msTimeout);

    // return the event handle that will be signalled whenever
    // there are deferred commands due for execution (when GetDueCommand
    // will not block).
    HANDLE GetDueHandle() {
        return HANDLE(m_evDue);
    };

    // return a pointer to a command that will be due for a given time.
    // Pass in a stream time here. The stream time offset will be passed
    // in via the Run method.
    // Commands remain queued until invoked or cancelled.
    // This method will not block. It will report VFW_E_NOT_FOUND if there
    // are no commands due yet.
    // Returns an AddRef-ed object
    virtual HRESULT GetCommandDueFor(REFERENCE_TIME tStream, CDeferredCommand**ppCmd);

    // check if a given time is due (TRUE if it is due yet)
    BOOL CheckTime(CRefTime time, BOOL bStream) {

        // if no clock, nothing is due!
        if (!m_pClock) {
            return FALSE;
        }

        // stream time
        if (bStream) {

            // not valid if not running
            if (!m_bRunning) {
                return FALSE;
            }
            // add on known stream time offset to get presentation time
            time += m_StreamTimeOffset;
        }

        CRefTime Now;
        m_pClock->GetTime((REFERENCE_TIME*)&Now);
        return (time <= Now);
    };

protected:

    // protect access to lists etc
    CCritSec m_Lock;

    // commands queued in presentation time are stored here
    CGenericList<CDeferredCommand> m_listPresentation;

    // commands queued in stream time are stored here
    CGenericList<CDeferredCommand> m_listStream;

    // set when any commands are due
    CAMEvent m_evDue;

    // creates an advise for the earliest time required, if any
    void SetTimeAdvise(void);

    // advise id from reference clock (0 if no outstanding advise)
    DWORD_PTR m_dwAdvise;

    // advise time is for this presentation time
    CRefTime m_tCurrentAdvise;

    // the reference clock we are using (addrefed)
    IReferenceClock* m_pClock;

    // true when running
    BOOL m_bRunning;

    // contains stream time offset when m_bRunning is true
    CRefTime m_StreamTimeOffset;
};

#endif // __CTLUTIL__
