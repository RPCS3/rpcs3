 // no longer used  - but might get
 // our own facility in the future?
 // FacilityNames=(FACILITY_VFW=0x4)
 // To add a message:
 //
 // The MessageId is the number of the message.
 // Accepted severities are 'Success' and 'Warning'.
 //
 // Facility should be FACILITY_ITF (was FACILITY_VFW).
 //
 // The SymbolicName is the name used in the code to identify the message.
 // The text of a message starts the line after 'Language=' and
 // ends before a line with only a '.' in column one.
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: VFW_E_INVALIDMEDIATYPE
//
// MessageText:
//
//  An invalid media type was specified.%0
//
#define VFW_E_INVALIDMEDIATYPE           ((HRESULT)0x80040200L)

//
// MessageId: VFW_E_INVALIDSUBTYPE
//
// MessageText:
//
//  An invalid media subtype was specified.%0
//
#define VFW_E_INVALIDSUBTYPE             ((HRESULT)0x80040201L)

//
// MessageId: VFW_E_NEED_OWNER
//
// MessageText:
//
//  This object can only be created as an aggregated object.%0
//
#define VFW_E_NEED_OWNER                 ((HRESULT)0x80040202L)

//
// MessageId: VFW_E_ENUM_OUT_OF_SYNC
//
// MessageText:
//
//  The enumerator has become invalid.%0
//
#define VFW_E_ENUM_OUT_OF_SYNC           ((HRESULT)0x80040203L)

//
// MessageId: VFW_E_ALREADY_CONNECTED
//
// MessageText:
//
//  At least one of the pins involved in the operation is already connected.%0
//
#define VFW_E_ALREADY_CONNECTED          ((HRESULT)0x80040204L)

//
// MessageId: VFW_E_FILTER_ACTIVE
//
// MessageText:
//
//  This operation cannot be performed because the filter is active.%0
//
#define VFW_E_FILTER_ACTIVE              ((HRESULT)0x80040205L)

//
// MessageId: VFW_E_NO_TYPES
//
// MessageText:
//
//  One of the specified pins supports no media types.%0
//
#define VFW_E_NO_TYPES                   ((HRESULT)0x80040206L)

//
// MessageId: VFW_E_NO_ACCEPTABLE_TYPES
//
// MessageText:
//
//  There is no common media type between these pins.%0
//
#define VFW_E_NO_ACCEPTABLE_TYPES        ((HRESULT)0x80040207L)

//
// MessageId: VFW_E_INVALID_DIRECTION
//
// MessageText:
//
//  Two pins of the same direction cannot be connected together.%0
//
#define VFW_E_INVALID_DIRECTION          ((HRESULT)0x80040208L)

//
// MessageId: VFW_E_NOT_CONNECTED
//
// MessageText:
//
//  The operation cannot be performed because the pins are not connected.%0
//
#define VFW_E_NOT_CONNECTED              ((HRESULT)0x80040209L)

//
// MessageId: VFW_E_NO_ALLOCATOR
//
// MessageText:
//
//  No sample buffer allocator is available.%0
//
#define VFW_E_NO_ALLOCATOR               ((HRESULT)0x8004020AL)

//
// MessageId: VFW_E_RUNTIME_ERROR
//
// MessageText:
//
//  A run-time error occurred.%0
//
#define VFW_E_RUNTIME_ERROR              ((HRESULT)0x8004020BL)

//
// MessageId: VFW_E_BUFFER_NOTSET
//
// MessageText:
//
//  No buffer space has been set.%0
//
#define VFW_E_BUFFER_NOTSET              ((HRESULT)0x8004020CL)

//
// MessageId: VFW_E_BUFFER_OVERFLOW
//
// MessageText:
//
//  The buffer is not big enough.%0
//
#define VFW_E_BUFFER_OVERFLOW            ((HRESULT)0x8004020DL)

//
// MessageId: VFW_E_BADALIGN
//
// MessageText:
//
//  An invalid alignment was specified.%0
//
#define VFW_E_BADALIGN                   ((HRESULT)0x8004020EL)

//
// MessageId: VFW_E_ALREADY_COMMITTED
//
// MessageText:
//
//  Cannot change allocated memory while the filter is active.%0
//
#define VFW_E_ALREADY_COMMITTED          ((HRESULT)0x8004020FL)

//
// MessageId: VFW_E_BUFFERS_OUTSTANDING
//
// MessageText:
//
//  One or more buffers are still active.%0
//
#define VFW_E_BUFFERS_OUTSTANDING        ((HRESULT)0x80040210L)

//
// MessageId: VFW_E_NOT_COMMITTED
//
// MessageText:
//
//  Cannot allocate a sample when the allocator is not active.%0
//
#define VFW_E_NOT_COMMITTED              ((HRESULT)0x80040211L)

//
// MessageId: VFW_E_SIZENOTSET
//
// MessageText:
//
//  Cannot allocate memory because no size has been set.%0
//
#define VFW_E_SIZENOTSET                 ((HRESULT)0x80040212L)

//
// MessageId: VFW_E_NO_CLOCK
//
// MessageText:
//
//  Cannot lock for synchronization because no clock has been defined.%0
//
#define VFW_E_NO_CLOCK                   ((HRESULT)0x80040213L)

//
// MessageId: VFW_E_NO_SINK
//
// MessageText:
//
//  Quality messages could not be sent because no quality sink has been defined.%0
//
#define VFW_E_NO_SINK                    ((HRESULT)0x80040214L)

//
// MessageId: VFW_E_NO_INTERFACE
//
// MessageText:
//
//  A required interface has not been implemented.%0
//
#define VFW_E_NO_INTERFACE               ((HRESULT)0x80040215L)

//
// MessageId: VFW_E_NOT_FOUND
//
// MessageText:
//
//  An object or name was not found.%0
//
#define VFW_E_NOT_FOUND                  ((HRESULT)0x80040216L)

//
// MessageId: VFW_E_CANNOT_CONNECT
//
// MessageText:
//
//  No combination of intermediate filters could be found to make the connection.%0
//
#define VFW_E_CANNOT_CONNECT             ((HRESULT)0x80040217L)

//
// MessageId: VFW_E_CANNOT_RENDER
//
// MessageText:
//
//  No combination of filters could be found to render the stream.%0
//
#define VFW_E_CANNOT_RENDER              ((HRESULT)0x80040218L)

//
// MessageId: VFW_E_CHANGING_FORMAT
//
// MessageText:
//
//  Could not change formats dynamically.%0
//
#define VFW_E_CHANGING_FORMAT            ((HRESULT)0x80040219L)

//
// MessageId: VFW_E_NO_COLOR_KEY_SET
//
// MessageText:
//
//  No color key has been set.%0
//
#define VFW_E_NO_COLOR_KEY_SET           ((HRESULT)0x8004021AL)

//
// MessageId: VFW_E_NOT_OVERLAY_CONNECTION
//
// MessageText:
//
//  Current pin connection is not using the IOverlay transport.%0
//
#define VFW_E_NOT_OVERLAY_CONNECTION     ((HRESULT)0x8004021BL)

//
// MessageId: VFW_E_NOT_SAMPLE_CONNECTION
//
// MessageText:
//
//  Current pin connection is not using the IMemInputPin transport.%0
//
#define VFW_E_NOT_SAMPLE_CONNECTION      ((HRESULT)0x8004021CL)

//
// MessageId: VFW_E_PALETTE_SET
//
// MessageText:
//
//  Setting a color key would conflict with the palette already set.%0
//
#define VFW_E_PALETTE_SET                ((HRESULT)0x8004021DL)

//
// MessageId: VFW_E_COLOR_KEY_SET
//
// MessageText:
//
//  Setting a palette would conflict with the color key already set.%0
//
#define VFW_E_COLOR_KEY_SET              ((HRESULT)0x8004021EL)

//
// MessageId: VFW_E_NO_COLOR_KEY_FOUND
//
// MessageText:
//
//  No matching color key is available.%0
//
#define VFW_E_NO_COLOR_KEY_FOUND         ((HRESULT)0x8004021FL)

//
// MessageId: VFW_E_NO_PALETTE_AVAILABLE
//
// MessageText:
//
//  No palette is available.%0
//
#define VFW_E_NO_PALETTE_AVAILABLE       ((HRESULT)0x80040220L)

//
// MessageId: VFW_E_NO_DISPLAY_PALETTE
//
// MessageText:
//
//  Display does not use a palette.%0
//
#define VFW_E_NO_DISPLAY_PALETTE         ((HRESULT)0x80040221L)

//
// MessageId: VFW_E_TOO_MANY_COLORS
//
// MessageText:
//
//  Too many colors for the current display settings.%0
//
#define VFW_E_TOO_MANY_COLORS            ((HRESULT)0x80040222L)

//
// MessageId: VFW_E_STATE_CHANGED
//
// MessageText:
//
//  The state changed while waiting to process the sample.%0
//
#define VFW_E_STATE_CHANGED              ((HRESULT)0x80040223L)

//
// MessageId: VFW_E_NOT_STOPPED
//
// MessageText:
//
//  The operation could not be performed because the filter is not stopped.%0
//
#define VFW_E_NOT_STOPPED                ((HRESULT)0x80040224L)

//
// MessageId: VFW_E_NOT_PAUSED
//
// MessageText:
//
//  The operation could not be performed because the filter is not paused.%0
//
#define VFW_E_NOT_PAUSED                 ((HRESULT)0x80040225L)

//
// MessageId: VFW_E_NOT_RUNNING
//
// MessageText:
//
//  The operation could not be performed because the filter is not running.%0
//
#define VFW_E_NOT_RUNNING                ((HRESULT)0x80040226L)

//
// MessageId: VFW_E_WRONG_STATE
//
// MessageText:
//
//  The operation could not be performed because the filter is in the wrong state.%0
//
#define VFW_E_WRONG_STATE                ((HRESULT)0x80040227L)

//
// MessageId: VFW_E_START_TIME_AFTER_END
//
// MessageText:
//
//  The sample start time is after the sample end time.%0
//
#define VFW_E_START_TIME_AFTER_END       ((HRESULT)0x80040228L)

//
// MessageId: VFW_E_INVALID_RECT
//
// MessageText:
//
//  The supplied rectangle is invalid.%0
//
#define VFW_E_INVALID_RECT               ((HRESULT)0x80040229L)

//
// MessageId: VFW_E_TYPE_NOT_ACCEPTED
//
// MessageText:
//
//  This pin cannot use the supplied media type.%0
//
#define VFW_E_TYPE_NOT_ACCEPTED          ((HRESULT)0x8004022AL)

//
// MessageId: VFW_E_SAMPLE_REJECTED
//
// MessageText:
//
//  This sample cannot be rendered.%0
//
#define VFW_E_SAMPLE_REJECTED            ((HRESULT)0x8004022BL)

//
// MessageId: VFW_E_SAMPLE_REJECTED_EOS
//
// MessageText:
//
//  This sample cannot be rendered because the end of the stream has been reached.%0
//
#define VFW_E_SAMPLE_REJECTED_EOS        ((HRESULT)0x8004022CL)

//
// MessageId: VFW_E_DUPLICATE_NAME
//
// MessageText:
//
//  An attempt to add a filter with a duplicate name failed.%0
//
#define VFW_E_DUPLICATE_NAME             ((HRESULT)0x8004022DL)

//
// MessageId: VFW_S_DUPLICATE_NAME
//
// MessageText:
//
//  An attempt to add a filter with a duplicate name succeeded with a modified name.%0
//
#define VFW_S_DUPLICATE_NAME             ((HRESULT)0x0004022DL)

//
// MessageId: VFW_E_TIMEOUT
//
// MessageText:
//
//  A time-out has expired.%0
//
#define VFW_E_TIMEOUT                    ((HRESULT)0x8004022EL)

//
// MessageId: VFW_E_INVALID_FILE_FORMAT
//
// MessageText:
//
//  The file format is invalid.%0
//
#define VFW_E_INVALID_FILE_FORMAT        ((HRESULT)0x8004022FL)

//
// MessageId: VFW_E_ENUM_OUT_OF_RANGE
//
// MessageText:
//
//  The list has already been exhausted.%0
//
#define VFW_E_ENUM_OUT_OF_RANGE          ((HRESULT)0x80040230L)

//
// MessageId: VFW_E_CIRCULAR_GRAPH
//
// MessageText:
//
//  The filter graph is circular.%0
//
#define VFW_E_CIRCULAR_GRAPH             ((HRESULT)0x80040231L)

//
// MessageId: VFW_E_NOT_ALLOWED_TO_SAVE
//
// MessageText:
//
//  Updates are not allowed in this state.%0
//
#define VFW_E_NOT_ALLOWED_TO_SAVE        ((HRESULT)0x80040232L)

//
// MessageId: VFW_E_TIME_ALREADY_PASSED
//
// MessageText:
//
//  An attempt was made to queue a command for a time in the past.%0
//
#define VFW_E_TIME_ALREADY_PASSED        ((HRESULT)0x80040233L)

//
// MessageId: VFW_E_ALREADY_CANCELLED
//
// MessageText:
//
//  The queued command has already been canceled.%0
//
#define VFW_E_ALREADY_CANCELLED          ((HRESULT)0x80040234L)

//
// MessageId: VFW_E_CORRUPT_GRAPH_FILE
//
// MessageText:
//
//  Cannot render the file because it is corrupt.%0
//
#define VFW_E_CORRUPT_GRAPH_FILE         ((HRESULT)0x80040235L)

//
// MessageId: VFW_E_ADVISE_ALREADY_SET
//
// MessageText:
//
//  An overlay advise link already exists.%0
//
#define VFW_E_ADVISE_ALREADY_SET         ((HRESULT)0x80040236L)

//
// MessageId: VFW_S_STATE_INTERMEDIATE
//
// MessageText:
//
//  The state transition has not completed.%0
//
#define VFW_S_STATE_INTERMEDIATE         ((HRESULT)0x00040237L)

//
// MessageId: VFW_E_NO_MODEX_AVAILABLE
//
// MessageText:
//
//  No full-screen modes are available.%0
//
#define VFW_E_NO_MODEX_AVAILABLE         ((HRESULT)0x80040238L)

//
// MessageId: VFW_E_NO_ADVISE_SET
//
// MessageText:
//
//  This Advise cannot be canceled because it was not successfully set.%0
//
#define VFW_E_NO_ADVISE_SET              ((HRESULT)0x80040239L)

//
// MessageId: VFW_E_NO_FULLSCREEN
//
// MessageText:
//
//  A full-screen mode is not available.%0
//
#define VFW_E_NO_FULLSCREEN              ((HRESULT)0x8004023AL)

//
// MessageId: VFW_E_IN_FULLSCREEN_MODE
//
// MessageText:
//
//  Cannot call IVideoWindow methods while in full-screen mode.%0
//
#define VFW_E_IN_FULLSCREEN_MODE         ((HRESULT)0x8004023BL)

//
// MessageId: VFW_E_UNKNOWN_FILE_TYPE
//
// MessageText:
//
//  The media type of this file is not recognized.%0
//
#define VFW_E_UNKNOWN_FILE_TYPE          ((HRESULT)0x80040240L)

//
// MessageId: VFW_E_CANNOT_LOAD_SOURCE_FILTER
//
// MessageText:
//
//  The source filter for this file could not be loaded.%0
//
#define VFW_E_CANNOT_LOAD_SOURCE_FILTER  ((HRESULT)0x80040241L)

//
// MessageId: VFW_S_PARTIAL_RENDER
//
// MessageText:
//
//  Some of the streams in this movie are in an unsupported format.%0
//
#define VFW_S_PARTIAL_RENDER             ((HRESULT)0x00040242L)

//
// MessageId: VFW_E_FILE_TOO_SHORT
//
// MessageText:
//
//  A file appeared to be incomplete.%0
//
#define VFW_E_FILE_TOO_SHORT             ((HRESULT)0x80040243L)

//
// MessageId: VFW_E_INVALID_FILE_VERSION
//
// MessageText:
//
//  The version number of the file is invalid.%0
//
#define VFW_E_INVALID_FILE_VERSION       ((HRESULT)0x80040244L)

//
// MessageId: VFW_S_SOME_DATA_IGNORED
//
// MessageText:
//
//  The file contained some property settings that were not used.%0
//
#define VFW_S_SOME_DATA_IGNORED          ((HRESULT)0x00040245L)

//
// MessageId: VFW_S_CONNECTIONS_DEFERRED
//
// MessageText:
//
//  Some connections have failed and have been deferred.%0
//
#define VFW_S_CONNECTIONS_DEFERRED       ((HRESULT)0x00040246L)

//
// MessageId: VFW_E_INVALID_CLSID
//
// MessageText:
//
//  This file is corrupt: it contains an invalid class identifier.%0
//
#define VFW_E_INVALID_CLSID              ((HRESULT)0x80040247L)

//
// MessageId: VFW_E_INVALID_MEDIA_TYPE
//
// MessageText:
//
//  This file is corrupt: it contains an invalid media type.%0
//
#define VFW_E_INVALID_MEDIA_TYPE         ((HRESULT)0x80040248L)

 // Message id from WINWarning.H
//
// MessageId: VFW_E_BAD_KEY
//
// MessageText:
//
//  A registry entry is corrupt.%0
//
#define VFW_E_BAD_KEY                    ((HRESULT)0x800403F2L)

 // Message id from WINWarning.H
//
// MessageId: VFW_S_NO_MORE_ITEMS
//
// MessageText:
//
//  The end of the list has been reached.%0
//
#define VFW_S_NO_MORE_ITEMS              ((HRESULT)0x00040103L)

//
// MessageId: VFW_E_SAMPLE_TIME_NOT_SET
//
// MessageText:
//
//  No time stamp has been set for this sample.%0
//
#define VFW_E_SAMPLE_TIME_NOT_SET        ((HRESULT)0x80040249L)

//
// MessageId: VFW_S_RESOURCE_NOT_NEEDED
//
// MessageText:
//
//  The resource specified is no longer needed.%0
//
#define VFW_S_RESOURCE_NOT_NEEDED        ((HRESULT)0x00040250L)

//
// MessageId: VFW_E_MEDIA_TIME_NOT_SET
//
// MessageText:
//
//  No media time stamp has been set for this sample.%0
//
#define VFW_E_MEDIA_TIME_NOT_SET         ((HRESULT)0x80040251L)

//
// MessageId: VFW_E_NO_TIME_FORMAT_SET
//
// MessageText:
//
//  No media time format has been selected.%0
//
#define VFW_E_NO_TIME_FORMAT_SET         ((HRESULT)0x80040252L)

//
// MessageId: VFW_E_MONO_AUDIO_HW
//
// MessageText:
//
//  Cannot change balance because audio device is mono only.%0
//
#define VFW_E_MONO_AUDIO_HW              ((HRESULT)0x80040253L)

//
// MessageId: VFW_S_MEDIA_TYPE_IGNORED
//
// MessageText:
//
//  A connection could not be made with the media type in the persistent graph,%0
//  but has been made with a negotiated media type.%0
//
#define VFW_S_MEDIA_TYPE_IGNORED         ((HRESULT)0x00040254L)

//
// MessageId: VFW_E_NO_DECOMPRESSOR
//
// MessageText:
//
//  Cannot play back the video stream: no suitable decompressor could be found.%0
//
#define VFW_E_NO_DECOMPRESSOR            ((HRESULT)0x80040255L)

//
// MessageId: VFW_E_NO_AUDIO_HARDWARE
//
// MessageText:
//
//  Cannot play back the audio stream: no audio hardware is available, or the hardware is not responding.%0
//
#define VFW_E_NO_AUDIO_HARDWARE          ((HRESULT)0x80040256L)

//
// MessageId: VFW_S_VIDEO_NOT_RENDERED
//
// MessageText:
//
//  Cannot play back the video stream: no suitable decompressor could be found.%0
//
#define VFW_S_VIDEO_NOT_RENDERED         ((HRESULT)0x00040257L)

//
// MessageId: VFW_S_AUDIO_NOT_RENDERED
//
// MessageText:
//
//  Cannot play back the audio stream: no audio hardware is available.%0
//
#define VFW_S_AUDIO_NOT_RENDERED         ((HRESULT)0x00040258L)

//
// MessageId: VFW_E_RPZA
//
// MessageText:
//
//  Cannot play back the video stream: format 'RPZA' is not supported.%0
//
#define VFW_E_RPZA                       ((HRESULT)0x80040259L)

//
// MessageId: VFW_S_RPZA
//
// MessageText:
//
//  Cannot play back the video stream: format 'RPZA' is not supported.%0
//
#define VFW_S_RPZA                       ((HRESULT)0x0004025AL)

//
// MessageId: VFW_E_PROCESSOR_NOT_SUITABLE
//
// MessageText:
//
//  ActiveMovie cannot play MPEG movies on this processor.%0
//
#define VFW_E_PROCESSOR_NOT_SUITABLE     ((HRESULT)0x8004025BL)

//
// MessageId: VFW_E_UNSUPPORTED_AUDIO
//
// MessageText:
//
//  Cannot play back the audio stream: the audio format is not supported.%0
//
#define VFW_E_UNSUPPORTED_AUDIO          ((HRESULT)0x8004025CL)

//
// MessageId: VFW_E_UNSUPPORTED_VIDEO
//
// MessageText:
//
//  Cannot play back the video stream: the video format is not supported.%0
//
#define VFW_E_UNSUPPORTED_VIDEO          ((HRESULT)0x8004025DL)

//
// MessageId: VFW_E_MPEG_NOT_CONSTRAINED
//
// MessageText:
//
//  ActiveMovie cannot play this video stream because it falls outside the constrained standard.%0
//
#define VFW_E_MPEG_NOT_CONSTRAINED       ((HRESULT)0x8004025EL)

//
// MessageId: VFW_E_NOT_IN_GRAPH
//
// MessageText:
//
//  Cannot perform the requested function on an object that is not in the filter graph.%0
//
#define VFW_E_NOT_IN_GRAPH               ((HRESULT)0x8004025FL)

//
// MessageId: VFW_S_ESTIMATED
//
// MessageText:
//
//  The value returned had to be estimated.  It's accuracy can not be guaranteed.%0
//
#define VFW_S_ESTIMATED                  ((HRESULT)0x00040260L)

//
// MessageId: VFW_E_NO_TIME_FORMAT
//
// MessageText:
//
//  Cannot get or set time related information on an object that is using a time format of TIME_FORMAT_NONE.%0
//
#define VFW_E_NO_TIME_FORMAT             ((HRESULT)0x80040261L)

//
// MessageId: VFW_E_READ_ONLY
//
// MessageText:
//
//  The connection cannot be made because the stream is read only and the filter alters the data.%0
//
#define VFW_E_READ_ONLY                  ((HRESULT)0x80040262L)

//
// MessageId: VFW_S_RESERVED
//
// MessageText:
//
//  This success code is reserved for internal purposes within ActiveMovie.%0
//
#define VFW_S_RESERVED                   ((HRESULT)0x00040263L)

//
// MessageId: VFW_E_BUFFER_UNDERFLOW
//
// MessageText:
//
//  The buffer is not full enough.%0
//
#define VFW_E_BUFFER_UNDERFLOW           ((HRESULT)0x80040264L)

//
// MessageId: VFW_E_UNSUPPORTED_STREAM
//
// MessageText:
//
//  Cannot play back the file.  The format is not supported.%0
//
#define VFW_E_UNSUPPORTED_STREAM         ((HRESULT)0x80040265L)

//
// MessageId: VFW_E_NO_TRANSPORT
//
// MessageText:
//
//  Pins cannot connect due to not supporting the same transport.%0
//
#define VFW_E_NO_TRANSPORT               ((HRESULT)0x80040266L)

//
// MessageId: VFW_S_STREAM_OFF
//
// MessageText:
//
//  The stream has been turned off.%0
//
#define VFW_S_STREAM_OFF                 ((HRESULT)0x00040267L)

//
// MessageId: VFW_S_CANT_CUE
//
// MessageText:
//
//  The graph can't be cued because of lack of or corrupt data.%0
//
#define VFW_S_CANT_CUE                   ((HRESULT)0x00040268L)

//
// MessageId: VFW_E_BAD_VIDEOCD
//
// MessageText:
//
//  The Video CD can't be read correctly by the device or is the data is corrupt.%0
//
#define VFW_E_BAD_VIDEOCD                ((HRESULT)0x80040269L)

//
// MessageId: VFW_S_NO_STOP_TIME
//
// MessageText:
//
//  The stop time for the sample was not set.%0
//
#define VFW_S_NO_STOP_TIME               ((HRESULT)0x00040270L)

//
// MessageId: VFW_E_OUT_OF_VIDEO_MEMORY
//
// MessageText:
//
//  There is not enough Video Memory at this display resolution and number of colors. Reducing resolution might help.%0
//
#define VFW_E_OUT_OF_VIDEO_MEMORY        ((HRESULT)0x80040271L)

//
// MessageId: VFW_E_VP_NEGOTIATION_FAILED
//
// MessageText:
//
//  The VideoPort connection negotiation process has failed.%0
//
#define VFW_E_VP_NEGOTIATION_FAILED      ((HRESULT)0x80040272L)

//
// MessageId: VFW_E_DDRAW_CAPS_NOT_SUITABLE
//
// MessageText:
//
//  Either DirectDraw has not been installed or the Video Card capabilities are not suitable. Make sure the display is not in 16 color mode.%0
//
#define VFW_E_DDRAW_CAPS_NOT_SUITABLE    ((HRESULT)0x80040273L)

//
// MessageId: VFW_E_NO_VP_HARDWARE
//
// MessageText:
//
//  No VideoPort hardware is available, or the hardware is not responding.%0
//
#define VFW_E_NO_VP_HARDWARE             ((HRESULT)0x80040274L)

//
// MessageId: VFW_E_NO_CAPTURE_HARDWARE
//
// MessageText:
//
//  No Capture hardware is available, or the hardware is not responding.%0
//
#define VFW_E_NO_CAPTURE_HARDWARE        ((HRESULT)0x80040275L)

//
// MessageId: VFW_E_DVD_OPERATION_INHIBITED
//
// MessageText:
//
//  This User Operation is inhibited by DVD Content at this time.%0
//
#define VFW_E_DVD_OPERATION_INHIBITED    ((HRESULT)0x80040276L)

//
// MessageId: VFW_E_DVD_INVALIDDOMAIN
//
// MessageText:
//
//  This Operation is not permitted in the current domain.%0
//
#define VFW_E_DVD_INVALIDDOMAIN          ((HRESULT)0x80040277L)

//
// MessageId: VFW_E_DVD_NO_BUTTON
//
// MessageText:
//
//  The specified button is invalid or is not present at the current time, or there is no button present at the specified location.%0
//
#define VFW_E_DVD_NO_BUTTON              ((HRESULT)0x80040278L)

//
// MessageId: VFW_E_DVD_GRAPHNOTREADY
//
// MessageText:
//
//  DVD-Video playback graph has not been built yet.%0
//
#define VFW_E_DVD_GRAPHNOTREADY          ((HRESULT)0x80040279L)

//
// MessageId: VFW_E_DVD_RENDERFAIL
//
// MessageText:
//
//  DVD-Video playback graph building failed.%0
//
#define VFW_E_DVD_RENDERFAIL             ((HRESULT)0x8004027AL)

//
// MessageId: VFW_E_DVD_DECNOTENOUGH
//
// MessageText:
//
//  DVD-Video playback graph could not be built due to insufficient decoders.%0
//
#define VFW_E_DVD_DECNOTENOUGH           ((HRESULT)0x8004027BL)

//
// MessageId: VFW_E_DDRAW_VERSION_NOT_SUITABLE
//
// MessageText:
//
//  Version number of DirectDraw not suitable. Make sure to install dx5 or higher version.%0
//
#define VFW_E_DDRAW_VERSION_NOT_SUITABLE ((HRESULT)0x8004027CL)

//
// MessageId: VFW_E_COPYPROT_FAILED
//
// MessageText:
//
//  Copy protection cannot be enabled. Please make sure any other copy protected content is not being shown now.%0
//
#define VFW_E_COPYPROT_FAILED            ((HRESULT)0x8004027DL)

//
// MessageId: VFW_S_NOPREVIEWPIN
//
// MessageText:
//
//  There was no preview pin available, so the capture pin output is being split to provide both capture and preview.%0
//
#define VFW_S_NOPREVIEWPIN               ((HRESULT)0x0004027EL)

//
// MessageId: VFW_E_TIME_EXPIRED
//
// MessageText:
//
//  This object cannot be used anymore as its time has expired.%0
//
#define VFW_E_TIME_EXPIRED               ((HRESULT)0x8004027FL)

//
// MessageId: VFW_S_DVD_NON_ONE_SEQUENTIAL
//
// MessageText:
//
//  The current title was not a sequential set of chapters (PGC), and the returned timing information might not be continuous.%0
//
#define VFW_S_DVD_NON_ONE_SEQUENTIAL     ((HRESULT)0x00040280L)

//
// MessageId: VFW_E_DVD_WRONG_SPEED
//
// MessageText:
//
//  The operation cannot be performed at the current playback speed.%0
//
#define VFW_E_DVD_WRONG_SPEED            ((HRESULT)0x80040281L)

//
// MessageId: VFW_E_DVD_MENU_DOES_NOT_EXIST
//
// MessageText:
//
//  The specified menu doesn't exist.%0
//
#define VFW_E_DVD_MENU_DOES_NOT_EXIST    ((HRESULT)0x80040282L)

//
// MessageId: VFW_E_DVD_CMD_CANCELLED
//
// MessageText:
//
//  The specified command was either cancelled or no longer exists.%0
//
#define VFW_E_DVD_CMD_CANCELLED          ((HRESULT)0x80040283L)

//
// MessageId: VFW_E_DVD_STATE_WRONG_VERSION
//
// MessageText:
//
//  The data did not contain a recognized version.%0
//
#define VFW_E_DVD_STATE_WRONG_VERSION    ((HRESULT)0x80040284L)

//
// MessageId: VFW_E_DVD_STATE_CORRUPT
//
// MessageText:
//
//  The state data was corrupt.%0
//
#define VFW_E_DVD_STATE_CORRUPT          ((HRESULT)0x80040285L)

//
// MessageId: VFW_E_DVD_STATE_WRONG_DISC
//
// MessageText:
//
//  The state data is from a different disc.%0
//
#define VFW_E_DVD_STATE_WRONG_DISC       ((HRESULT)0x80040286L)

//
// MessageId: VFW_E_DVD_INCOMPATIBLE_REGION
//
// MessageText:
//
//  The region was not compatible with the current drive.%0
//
#define VFW_E_DVD_INCOMPATIBLE_REGION    ((HRESULT)0x80040287L)

//
// MessageId: VFW_E_DVD_NO_ATTRIBUTES
//
// MessageText:
//
//  The requested DVD stream attribute does not exist.%0
//
#define VFW_E_DVD_NO_ATTRIBUTES          ((HRESULT)0x80040288L)

//
// MessageId: VFW_E_DVD_NO_GOUP_PGC
//
// MessageText:
//
//  Currently there is no GoUp (Annex J user function) program chain (PGC).%0
//
#define VFW_E_DVD_NO_GOUP_PGC            ((HRESULT)0x80040289L)

//
// MessageId: VFW_E_DVD_LOW_PARENTAL_LEVEL
//
// MessageText:
//
//  The current parental level was too low.%0
//
#define VFW_E_DVD_LOW_PARENTAL_LEVEL     ((HRESULT)0x8004028AL)

//
// MessageId: VFW_E_DVD_NOT_IN_KARAOKE_MODE
//
// MessageText:
//
//  The current audio is not karaoke content.%0
//
#define VFW_E_DVD_NOT_IN_KARAOKE_MODE    ((HRESULT)0x8004028BL)

//
// MessageId: VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE
//
// MessageText:
//
//  The audio stream did not contain sufficient information to determine the contents of each channel.%0
//
#define VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE ((HRESULT)0x0004028CL)

//
// MessageId: VFW_S_DVD_NOT_ACCURATE
//
// MessageText:
//
//  The seek into the movie was not frame accurate.%0
//
#define VFW_S_DVD_NOT_ACCURATE           ((HRESULT)0x0004028DL)

//
// MessageId: VFW_E_FRAME_STEP_UNSUPPORTED
//
// MessageText:
//
//  Frame step is not supported on this configuration.%0
//
#define VFW_E_FRAME_STEP_UNSUPPORTED     ((HRESULT)0x8004028EL)

//
// MessageId: VFW_E_DVD_STREAM_DISABLED
//
// MessageText:
//
//  The specified stream is disabled and cannot be selected.%0
//
#define VFW_E_DVD_STREAM_DISABLED        ((HRESULT)0x8004028FL)

//
// MessageId: VFW_E_DVD_TITLE_UNKNOWN
//
// MessageText:
//
//  The operation depends on the current title number, however the navigator has not yet entered the VTSM or the title domains,
//  so the 'current' title index is unknown.%0
//
#define VFW_E_DVD_TITLE_UNKNOWN          ((HRESULT)0x80040290L)

//
// MessageId: VFW_E_DVD_INVALID_DISC
//
// MessageText:
//
//  The specified path does not point to a valid DVD disc.%0
//
#define VFW_E_DVD_INVALID_DISC           ((HRESULT)0x80040291L)

//
// MessageId: VFW_E_DVD_NO_RESUME_INFORMATION
//
// MessageText:
//
//  There is currently no resume information.%0
//
#define VFW_E_DVD_NO_RESUME_INFORMATION  ((HRESULT)0x80040292L)

//
// MessageId: VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD
//
// MessageText:
//
//  This thread has already blocked this output pin.  There is no need to call IPinFlowControl::Block() again.%0
//
#define VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD ((HRESULT)0x80040293L)

//
// MessageId: VFW_E_PIN_ALREADY_BLOCKED
//
// MessageText:
//
//  IPinFlowControl::Block() has been called on another thread.  The current thread cannot make any assumptions about this pin's block state.%0
//
#define VFW_E_PIN_ALREADY_BLOCKED        ((HRESULT)0x80040294L)

//
// MessageId: VFW_E_CERTIFICATION_FAILURE
//
// MessageText:
//
//  An operation failed due to a certification failure.%0
//
#define VFW_E_CERTIFICATION_FAILURE      ((HRESULT)0x80040295L)

//
// MessageId: VFW_E_VMR_NOT_IN_MIXER_MODE
//
// MessageText:
//
//  The VMR has not yet created a mixing component.  That is, IVMRFilterConfig::SetNumberofStreams has not yet been called.%0
//
#define VFW_E_VMR_NOT_IN_MIXER_MODE      ((HRESULT)0x80040296L)

//
// MessageId: VFW_E_VMR_NO_AP_SUPPLIED
//
// MessageText:
//
//  The application has not yet provided the VMR filter with a valid allocator-presenter object.%0
//
#define VFW_E_VMR_NO_AP_SUPPLIED         ((HRESULT)0x80040297L)
//
// MessageId: VFW_E_VMR_NO_DEINTERLACE_HW
//
// MessageText:
//
//  The VMR could not find any de-interlacing hardware on the current display device.%0
//
#define VFW_E_VMR_NO_DEINTERLACE_HW      ((HRESULT)0x80040298L)
//
// MessageId: VFW_E_VMR_NO_PROCAMP_HW
//
// MessageText:
//
//  The VMR could not find any ProcAmp hardware on the current display device.%0
//
#define VFW_E_VMR_NO_PROCAMP_HW          ((HRESULT)0x80040299L)
//
// MessageId: VFW_E_DVD_VMR9_INCOMPATIBLEDEC
//
// MessageText:
//
//  VMR9 does not work with VPE-based hardware decoders.%0
//
#define VFW_E_DVD_VMR9_INCOMPATIBLEDEC   ((HRESULT)0x8004029AL)
//
// MessageId: VFW_E_NO_COPP_HW
//
// MessageText:
//
//  The current display device does not support Content Output Protection Protocol (COPP) H/W.%0
//
#define VFW_E_NO_COPP_HW   ((HRESULT)0x8004029BL)
//
//
// E_PROP_SET_UNSUPPORTED and E_PROP_ID_UNSUPPORTED are added here using
// HRESULT_FROM_WIN32() because VC5 doesn't have WinNT's new error codes
// from winerror.h, and because it is more convienent to have them already
// formed as HRESULTs.  These should correspond to:
//     HRESULT_FROM_WIN32(ERROR_NOT_FOUND)     == E_PROP_ID_UNSUPPORTED
//     HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND) == E_PROP_SET_UNSUPPORTED
#if !defined(E_PROP_SET_UNSUPPORTED)
//
// MessageId: E_PROP_SET_UNSUPPORTED
//
// MessageText:
//
//  The Specified property set is not supported.%0
//
#define E_PROP_SET_UNSUPPORTED           ((HRESULT)0x80070492L)

#endif //!defined(E_PROP_SET_UNSUPPORTED)
#if !defined(E_PROP_ID_UNSUPPORTED)
//
// MessageId: E_PROP_ID_UNSUPPORTED
//
// MessageText:
//
//  The specified property ID is not supported for the specified property set.%0
//
#define E_PROP_ID_UNSUPPORTED            ((HRESULT)0x80070490L)

#endif //!defined(E_PROP_ID_UNSUPPORTED)
