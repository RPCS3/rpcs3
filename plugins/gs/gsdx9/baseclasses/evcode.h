//------------------------------------------------------------------------------
// File: EvCode.h
//
// Desc: List of standard Quartz event codes and the expected params.
//
// Copyright (c) 1992 - 2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// Event codes are broken into two groups
//   -- system event codes
//   -- extension event codes
// All system event codes are below EC_USER

#define EC_SYSTEMBASE                       0x00
#define EC_USER                             0x8000

// System-defined event codes
// ==========================
//
// There are three types of system-defined event codes:
//
// 1.  Those which are always passed through to the application
//     (To be collected by calls to GetEvent or within WaitForCompletion.)
//     (e.g. EC_ERRORABORT, EC_USERABORT.)
//
// 2.  Those which are pure internal and will never be passed to
//     the application.  (e.g. EC_SHUTDOWN)
//
// 3.  Those which have default handling.  Default handing implies that
//     the event is not passed to the application.  However, default
//     handling may be canceled by calling
//     IMediaEvent::CancelDefaultHandling.  If the default handling is
//     cancelled in this way, then the message will be delivered to the
//     application and the application must action it appropriately.
//     Default handling can be restored by calling RestoreDefaultHandling.
//
// We will refer to these events as application, internal and defaulted
// events respectively.
//
// System-defined events may have interface pointers, BSTR's, etc passed
// as parameters.  It is therefore essential that, for any message
// retrieved using GetEvent, a matching call to FreeEventParams is made
// to ensure that relevant interfaces are released and storage freed.
// Failure to call FreeEventParams will result in memory leaks, if not
// worse.
//
// Filters sending these messages to the filter graph should not AddRef()
// any interfaces that they may pass as parameters.  The filter graph
// manager will AddRef them if required.  E.g. if the event is to be queued
// for the application or queued to a worker thread.

// Each event listed below is immediately followed by a parameter list
// detailing the types of the parameters associated with the message,
// and an indication of whether the message is an application, internal
// or defaulted message.  This is then followed by a short description.
// The use of "void" in the parameter list implies that the parameter is not
// used.  Such parameters should be zero.

// Other defined EC_ regions:
// DVD event codes              0x0100 - 0x0150 (dvdevcod.h)
// audio device event codes     0x0200 - 0x0250 (audevcod.h)
// WindowsMedia SDK-originated events 0x0251 - 0x0300 (see below)
// MSVIDCTL                     0x0301 - 0x0325 (msvidctl.idl)
// stream buffer engine (PVR)   0x0326 - 0x0350 (sbe.idl)

#define EC_COMPLETE                         0x01
// ( HRESULT, void ) : defaulted (special)
// Signals the completed playback of a stream within the graph.  This message
// is sent by renderers when they receive end-of-stream.  The default handling
// of this message results in a _SINGLE_ EC_COMPLETE being sent to the
// application when ALL of the individual renderers have signaled EC_COMPLETE
// to the filter graph.  If the default handing is canceled, the application
// will see all of the individual EC_COMPLETEs.


#define EC_USERABORT                        0x02
// ( void, void ) : application
// In some sense, the user has requested that playback be terminated.
// This message is typically sent by renderers that render into a
// window if the user closes the window into which it was rendering.
// It is up to the application to decide if playback should actually
// be stopped.


#define EC_ERRORABORT                       0x03
// ( HRESULT, void ) : application
// Operation aborted because of error


#define EC_TIME                             0x04
// ( DWORD, DWORD ) : application
// The requested reference time occurred.  (This event is currently not used).
// lParam1 is low dword of ref time, lParam2 is high dword of reftime.


#define EC_REPAINT                          0x05
// ( IPin * (could be NULL), void ) : defaulted
// A repaint is required - lParam1 contains the (IPin *) that needs the data
// to be sent again. Default handling is: if the output pin which the IPin is
// attached  to supports the IMediaEventSink interface then it will be called
// with the EC_REPAINT first.  If that fails then normal repaint processing is
// done by the filter graph.


// Stream error notifications
#define EC_STREAM_ERROR_STOPPED             0x06
#define EC_STREAM_ERROR_STILLPLAYING        0x07
// ( HRESULT, DWORD ) : application
// lParam 1 is major code, lParam2 is minor code, either may be zero.


#define EC_ERROR_STILLPLAYING               0x08
// ( HRESULT, void ) : application
// The filter graph manager may issue Run's to the graph asynchronously.
// If such a Run fails, EC_ERROR_STILLPLAYING is issued to notify the
// application of the failure.  The state of the underlying filters
// at such a time will be indeterminate - they will all have been asked
// to run, but some are almost certainly not.


#define EC_PALETTE_CHANGED                  0x09
// ( void, void ) : application
// notify application that the video palette has changed


#define EC_VIDEO_SIZE_CHANGED               0x0A
// ( DWORD, void ) : application
// Sent by video renderers.
// Notifies the application that the native video size has changed.
// LOWORD of the DWORD is the new width, HIWORD is the new height.


#define EC_QUALITY_CHANGE                   0x0B
// ( void, void ) : application
// Notify application that playback degradation has occurred


#define EC_SHUTTING_DOWN                    0x0C
// ( void, void ) : internal
// This message is sent by the filter graph manager to any plug-in
// distributors which support IMediaEventSink to notify them that
// the filter graph is starting to shutdown.


#define EC_CLOCK_CHANGED                    0x0D
// ( void, void ) : application
// Notify application that the clock has changed.
// (i.e. SetSyncSource has been called on the filter graph and has been
// distributed successfully to the filters in the graph.)


#define EC_PAUSED                           0x0E
// ( HRESULT, void ) : application
// Notify application the previous pause request has completed


#define EC_OPENING_FILE                     0x10
#define EC_BUFFERING_DATA                   0x11
// ( BOOL, void ) : application
// lParam1 == 1   --> starting to open file or buffer data
// lParam1 == 0   --> not opening or buffering any more
// (This event does not appear to be used by ActiveMovie.)


#define EC_FULLSCREEN_LOST                  0x12
// ( void, IBaseFilter * ) : application
// Sent by full screen renderers when switched away from full screen.
// IBaseFilter may be NULL.


#define EC_ACTIVATE                         0x13
// ( BOOL, IBaseFilter * ) : internal
// Sent by video renderers when they lose or gain activation.
// lParam1 is set to 1 if gained or 0 if lost
// lParam2 is the IBaseFilter* for the filter that is sending the message
// Used for sound follows focus and full-screen switching


#define EC_NEED_RESTART                     0x14
// ( void, void ) : defaulted
// Sent by renderers when they regain a resource (e.g. audio renderer).
// Causes a restart by Pause/put_Current/Run (if running).


#define EC_WINDOW_DESTROYED                 0x15
// ( IBaseFilter *, void ) : internal
// Sent by video renderers when the window has been destroyed. Handled
// by the filter graph / distributor telling the resource manager.
// lParam1 is the IBaseFilter* of the filter whose window is being destroyed


#define EC_DISPLAY_CHANGED                  0x16
// ( IPin *, void ) : internal
// Sent by renderers when they detect a display change. the filter graph
// will arrange for the graph to be stopped and the pin send in lParam1
// to be reconnected. by being reconnected it allows a renderer to reset
// and connect with a more appropriate format for the new display mode
// lParam1 contains an (IPin *) that should be reconnected by the graph


#define EC_STARVATION                       0x17
// ( void, void ) : defaulted
// Sent by a filter when it detects starvation. Default handling (only when
// running) is for the graph to be paused until all filters enter the
// paused state and then run. Normally this would be sent by a parser or source
// filter when too little data is arriving.


#define EC_OLE_EVENT                        0x18
// ( BSTR, BSTR ) : application
// Sent by a filter to pass a text string to the application.
// Conventionally, the first string is a type, and the second a parameter.


#define EC_NOTIFY_WINDOW                    0x19
// ( HWND, void ) : internal
// Pass the window handle around during pin connection.

#define EC_STREAM_CONTROL_STOPPED           0x1A
// ( IPin * pSender, DWORD dwCookie )
// Notification that an earlier call to IAMStreamControl::StopAt
// has now take effect.  Calls to the method can be marked
// with a cookie which is passed back in the second parameter,
// allowing applications to easily tie together request
// and completion notifications.
//
// NB: IPin will point to the pin that actioned the Stop.  This
// may not be the pin that the StopAt was sent to.

#define EC_STREAM_CONTROL_STARTED           0x1B
// ( IPin * pSender, DWORD dwCookie )
// Notification that an earlier call to IAMStreamControl::StartAt
// has now take effect.  Calls to the method can be marked
// with a cookie which is passed back in the second parameter,
// allowing applications to easily tie together request
// and completion notifications.
//
// NB: IPin will point to the pin that actioned the Start.  This
// may not be the pin that the StartAt was sent to.

#define EC_END_OF_SEGMENT                   0x1C
//
// ( const REFERENCE_TIME *pStreamTimeAtEndOfSegment, DWORD dwSegmentNumber )
//
// pStreamTimeAtEndOfSegment
//     pointer to the accumulated stream clock
//     time since the start of the segment - this is directly computable
//     as the sum of the previous and current segment durations (Stop - Start)
//     and the rate applied to each segment
//     The source add this time to the time within each segment to get
//     a total elapsed time
//
// dwSegmentNumber
//     Segment number - starts at 0
//
// Notifies that a segment end has been reached when the
// AM_SEEKING_Segment flags was set for IMediaSeeking::SetPositions
// Passes in an IMediaSeeking interface to allow the next segment
// to be defined by the application

#define EC_SEGMENT_STARTED                  0x1D
//
// ( const REFERENCE_TIME *pStreamTimeAtStartOfSegment, DWORD dwSegmentNumber)
//
// pStreamTimeAtStartOfSegment
//     pointer to the accumulated stream clock
//     time since the start of the segment - this is directly computable
//     as the sum of the previous segment durations (Stop - Start)
//     and the rate applied to each segment
//
// dwSegmentNumber
//     Segment number - starts at 0
//
// Notifies that a new segment has been started.
// This is sent synchronously by any entity that will issue
// EC_END_OF_SEGMENT when a new segment is started
// (See IMediaSeeking::SetPositions - AM_SEEKING_Segment flag)
// It is used to compute how many EC_END_OF_SEGMENT notifications
// to expect at the end of a segment and as a consitency check


#define EC_LENGTH_CHANGED                  0x1E
// (void, void)
// sent to indicate that the length of the "file" has changed

#define EC_DEVICE_LOST                     0x1f
// (IUnknown, 0)
//
// request window notification when the device is available again
// (through WM_DEVICECHANGED messages registered with
// RegisterDeviceNotification; see IAMDeviceRemoval interface)

#define EC_STEP_COMPLETE                      0x24
// (BOOL bCacelled, void)
// Step request complete
// if bCancelled is TRUE the step was cancelled.  This can happen
// if the application issued some control request or because there
// was a mode change etc etc

// Event code 25 is reserved for future use.

#define EC_TIMECODE_AVAILABLE           0x30
// Sent by filter supporting timecode
// Param1 has a pointer to the sending object
// Param2 has the device ID of the sending object

#define EC_EXTDEVICE_MODE_CHANGE        0x31
// Sent by filter supporting IAMExtDevice
// Param1 has the new mode
// Param2 has the device ID of the sending object

#define EC_STATE_CHANGE                    0x32
// ( FILTER_STATE, BOOL bInternal)
// Used to notify the application of any state changes in the filter graph.
// lParam1  is of type enum FILTER_STATE (defined in strmif.h) and indicates
//          the state of the filter graph.
//
// lParam2 == 0 indicates that the previous state change request has completed
//              & a change in application state.
// lParam2 == 1 reserved for future use to indicate internal state changes.


#define EC_GRAPH_CHANGED                        0x50
// Sent by filter to notify interesting graph changes

#define EC_CLOCK_UNSET                      0x51
// ( void, void ) : application
// Used to notify the filter graph to unset the current graph clock.
// Has the affect of forcing the filter graph to reestablish the graph clock
// on the next Pause/Run (note that this is only used by ksproxy, when the pin
// of a clock providing filter is disconnected)

#define EC_VMR_RENDERDEVICE_SET                 0x53
// (Render_Device type, void)
// Identifies the type of rendering mechanism the VMR
// is using to display video.  Types used include:
#define VMR_RENDER_DEVICE_OVERLAY       0x01
#define VMR_RENDER_DEVICE_VIDMEM        0x02
#define VMR_RENDER_DEVICE_SYSMEM        0x04


#define EC_VMR_SURFACE_FLIPPED          0x54
// (hr - Flip return code, void)
// Identifies the VMR's allocator-presenter has called the DDraw flip api on
// the surface being presented.   This allows the VMR to keep its DX-VA table
// of DDraw surfaces in sync with DDraws flipping chain.

#define EC_VMR_RECONNECTION_FAILED      0x55
// (hr - ReceiveConnection return code, void)
// Identifies that an upstream decoder tried to perform a dynamic format
// change and the VMR was unable to accept the new format.


#define EC_PREPROCESS_COMPLETE          0x56
// Sent by the WM ASF writer filter (WMSDK V9 version) to signal the completion 
// of a pre-process run when running in multipass encode mode. 
// Param1 = 0, Param2 = IBaseFilter ptr of sending filter

#define EC_CODECAPI_EVENT               0x57
// Sent by the Codec API when an event is encountered.  Both the Data
// must be freed by the recipient using CoTaskMemFree
// Param1 = UserDataPointer, Param2 = VOID* Data

//------------------------------------------
//
//  BDA events:
//
//      Event code 0x80 through 0x8f are reserved for BDA
//


//------------------------------------------
//
// WindowsMedia SDK filter-specific events:
//
//
// Note that for EC_WMT_EVENT events the wmsdk-based filters use the following structure for
// passing event parameters to the app:
#ifndef AM_WMT_EVENT_DATA_DEFINED
#define AM_WMT_EVENT_DATA_DEFINED
typedef struct {
    HRESULT hrStatus;        // status code
    void * pData;            // event data
} AM_WMT_EVENT_DATA;
#endif
//
#define EC_WMT_EVENT_BASE                  0x0251
//
#define EC_WMT_INDEX_EVENT                  EC_WMT_EVENT_BASE
// WindowsMedia SDK-originated file indexing status, sent by WMSDK-based filters
//
// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
// lParam2 is specific to the lParam event
//
//     the following WMT_STATUS messages are sent for this event:
//         WMT_STARTED        - lParam2 is 0
//         WMT_CLOSED         - lParam2 is 0
//         WMT_INDEX_PROGRESS - lParam2 is a DWORD containing the progress percent complete
//
#define EC_WMT_EVENT                        EC_WMT_EVENT_BASE+1
// WindowsMedia SDK-originated event, sent by WMSDK-based filters
//
// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
// lParam2 is a pointer an AM_WMT_EVENT_DATA structure where,
//                          hrStatus is the status code sent by the wmsdk
//                          pData is specific to the lParam1 event
//
//     the following WMT_STATUS messages are sent by the WMSDK Reader filter for this event:
//         WMT_NO_RIGHTS        - pData is a pointer to a WCHAR string containing a challenge URL
//         WMT_ACQUIRE_LICENSE  - lParam2 is a pointer to a WM_GET_LICENSE_DATA struct
//         WMT_NO_RIGHTS_EX     - lParam2 is a pointer to a WM_GET_LICENSE_DATA struct
//         WMT_NEEDS_INDIVIDUALIZATION - lParam2 is NULL
//         WMT_INDIVIDUALIZE    - lParam2 is a pointer to a WM_INDIVIDUALIZE_STATUS struct
//     the WMSDK (V9) ASF Writer filter will send this event in response to a wmsdk-signaled error during file
//         writing, along with the wmsdk WMT_STATUS error as the lParam1 and hrStatus embedded in the 
//         AM_WMT_EVENT_DATA struct pointed to by the lParam2 pointer.
//
// end WMSDK-originated events
//-----------------------------------------


#define EC_BUILT                            0x300
// Sent to notify transition from unbuilt to built state


#define EC_UNBUILT                          0x301
// Sent to notify transtion from built to unbuilt state
