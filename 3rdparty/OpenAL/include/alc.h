#ifndef AL_ALC_H
#define AL_ALC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALC_API
 #if defined(AL_LIBTYPE_STATIC)
  #define ALC_API
 #elif defined(_WIN32)
  #define ALC_API __declspec(dllimport)
 #else
  #define ALC_API extern
 #endif
#endif

#ifdef _WIN32
 #define ALC_APIENTRY __cdecl
#else
 #define ALC_APIENTRY
#endif


/* Deprecated macros. */
#define ALCAPI                                   ALC_API
#define ALCAPIENTRY                              ALC_APIENTRY
#define ALC_INVALID                              0

/** Supported ALC version? */
#define ALC_VERSION_0_1                          1

/** Opaque device handle */
typedef struct ALCdevice ALCdevice;
/** Opaque context handle */
typedef struct ALCcontext ALCcontext;

/** 8-bit boolean */
typedef char ALCboolean;

/** character */
typedef char ALCchar;

/** signed 8-bit integer */
typedef signed char ALCbyte;

/** unsigned 8-bit integer */
typedef unsigned char ALCubyte;

/** signed 16-bit integer */
typedef short ALCshort;

/** unsigned 16-bit integer */
typedef unsigned short ALCushort;

/** signed 32-bit integer */
typedef int ALCint;

/** unsigned 32-bit integer */
typedef unsigned int ALCuint;

/** non-negative 32-bit integer size */
typedef int ALCsizei;

/** 32-bit enumeration value */
typedef int ALCenum;

/** 32-bit IEEE-754 floating-point */
typedef float ALCfloat;

/** 64-bit IEEE-754 floating-point */
typedef double ALCdouble;

/** void type (for opaque pointers only) */
typedef void ALCvoid;


/* Enumeration values begin at column 50. Do not use tabs. */

/** Boolean False. */
#define ALC_FALSE                                0

/** Boolean True. */
#define ALC_TRUE                                 1

/** Context attribute: <int> Hz. */
#define ALC_FREQUENCY                            0x1007

/** Context attribute: <int> Hz. */
#define ALC_REFRESH                              0x1008

/** Context attribute: AL_TRUE or AL_FALSE synchronous context? */
#define ALC_SYNC                                 0x1009

/** Context attribute: <int> requested Mono (3D) Sources. */
#define ALC_MONO_SOURCES                         0x1010

/** Context attribute: <int> requested Stereo Sources. */
#define ALC_STEREO_SOURCES                       0x1011

/** No error. */
#define ALC_NO_ERROR                             0

/** Invalid device handle. */
#define ALC_INVALID_DEVICE                       0xA001

/** Invalid context handle. */
#define ALC_INVALID_CONTEXT                      0xA002

/** Invalid enumeration passed to an ALC call. */
#define ALC_INVALID_ENUM                         0xA003

/** Invalid value passed to an ALC call. */
#define ALC_INVALID_VALUE                        0xA004

/** Out of memory. */
#define ALC_OUT_OF_MEMORY                        0xA005


/** Runtime ALC major version. */
#define ALC_MAJOR_VERSION                        0x1000
/** Runtime ALC minor version. */
#define ALC_MINOR_VERSION                        0x1001

/** Context attribute list size. */
#define ALC_ATTRIBUTES_SIZE                      0x1002
/** Context attribute list properties. */
#define ALC_ALL_ATTRIBUTES                       0x1003

/** String for the default device specifier. */
#define ALC_DEFAULT_DEVICE_SPECIFIER             0x1004
/**
 * Device specifier string.
 *
 * If device handle is NULL, it is instead a null-character separated list of
 * strings of known device specifiers (list ends with an empty string).
 */
#define ALC_DEVICE_SPECIFIER                     0x1005
/** String for space-separated list of ALC extensions. */
#define ALC_EXTENSIONS                           0x1006


/** Capture extension */
#define ALC_EXT_CAPTURE 1
/**
 * Capture device specifier string.
 *
 * If device handle is NULL, it is instead a null-character separated list of
 * strings of known capture device specifiers (list ends with an empty string).
 */
#define ALC_CAPTURE_DEVICE_SPECIFIER             0x310
/** String for the default capture device specifier. */
#define ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER     0x311
/** Number of sample frames available for capture. */
#define ALC_CAPTURE_SAMPLES                      0x312


/** Enumerate All extension */
#define ALC_ENUMERATE_ALL_EXT 1
/** String for the default extended device specifier. */
#define ALC_DEFAULT_ALL_DEVICES_SPECIFIER        0x1012
/**
 * Device's extended specifier string.
 *
 * If device handle is NULL, it is instead a null-character separated list of
 * strings of known extended device specifiers (list ends with an empty string).
 */
#define ALC_ALL_DEVICES_SPECIFIER                0x1013


#ifndef ALC_NO_PROTOTYPES
/* Context management. */

/** Create and attach a context to the given device. */
ALC_API ALCcontext* ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrlist);
/**
 * Makes the given context the active process-wide context. Passing NULL clears
 * the active context.
 */
ALC_API ALCboolean  ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context);
/** Resumes processing updates for the given context. */
ALC_API void        ALC_APIENTRY alcProcessContext(ALCcontext *context);
/** Suspends updates for the given context. */
ALC_API void        ALC_APIENTRY alcSuspendContext(ALCcontext *context);
/** Remove a context from its device and destroys it. */
ALC_API void        ALC_APIENTRY alcDestroyContext(ALCcontext *context);
/** Returns the currently active context. */
ALC_API ALCcontext* ALC_APIENTRY alcGetCurrentContext(void);
/** Returns the device that a particular context is attached to. */
ALC_API ALCdevice*  ALC_APIENTRY alcGetContextsDevice(ALCcontext *context);

/* Device management. */

/** Opens the named playback device. */
ALC_API ALCdevice* ALC_APIENTRY alcOpenDevice(const ALCchar *devicename);
/** Closes the given playback device. */
ALC_API ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device);

/* Error support. */

/** Obtain the most recent Device error. */
ALC_API ALCenum ALC_APIENTRY alcGetError(ALCdevice *device);

/* Extension support. */

/**
 * Query for the presence of an extension on the device. Pass a NULL device to
 * query a device-inspecific extension.
 */
ALC_API ALCboolean ALC_APIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname);
/**
 * Retrieve the address of a function. Given a non-NULL device, the returned
 * function may be device-specific.
 */
ALC_API ALCvoid*   ALC_APIENTRY alcGetProcAddress(ALCdevice *device, const ALCchar *funcname);
/**
 * Retrieve the value of an enum. Given a non-NULL device, the returned value
 * may be device-specific.
 */
ALC_API ALCenum    ALC_APIENTRY alcGetEnumValue(ALCdevice *device, const ALCchar *enumname);

/* Query functions. */

/** Returns information about the device, and error strings. */
ALC_API const ALCchar* ALC_APIENTRY alcGetString(ALCdevice *device, ALCenum param);
/** Returns information about the device and the version of OpenAL. */
ALC_API void           ALC_APIENTRY alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values);

/* Capture functions. */

/**
 * Opens the named capture device with the given frequency, format, and buffer
 * size.
 */
ALC_API ALCdevice* ALC_APIENTRY alcCaptureOpenDevice(const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize);
/** Closes the given capture device. */
ALC_API ALCboolean ALC_APIENTRY alcCaptureCloseDevice(ALCdevice *device);
/** Starts capturing samples into the device buffer. */
ALC_API void       ALC_APIENTRY alcCaptureStart(ALCdevice *device);
/** Stops capturing samples. Samples in the device buffer remain available. */
ALC_API void       ALC_APIENTRY alcCaptureStop(ALCdevice *device);
/** Reads samples from the device buffer. */
ALC_API void       ALC_APIENTRY alcCaptureSamples(ALCdevice *device, ALCvoid *buffer, ALCsizei samples);
#endif /* ALC_NO_PROTOTYPES */

/* Pointer-to-function types, useful for storing dynamically loaded ALC entry
 * points.
 */
typedef ALCcontext*    (ALC_APIENTRY *LPALCCREATECONTEXT)(ALCdevice *device, const ALCint *attrlist);
typedef ALCboolean     (ALC_APIENTRY *LPALCMAKECONTEXTCURRENT)(ALCcontext *context);
typedef void           (ALC_APIENTRY *LPALCPROCESSCONTEXT)(ALCcontext *context);
typedef void           (ALC_APIENTRY *LPALCSUSPENDCONTEXT)(ALCcontext *context);
typedef void           (ALC_APIENTRY *LPALCDESTROYCONTEXT)(ALCcontext *context);
typedef ALCcontext*    (ALC_APIENTRY *LPALCGETCURRENTCONTEXT)(void);
typedef ALCdevice*     (ALC_APIENTRY *LPALCGETCONTEXTSDEVICE)(ALCcontext *context);
typedef ALCdevice*     (ALC_APIENTRY *LPALCOPENDEVICE)(const ALCchar *devicename);
typedef ALCboolean     (ALC_APIENTRY *LPALCCLOSEDEVICE)(ALCdevice *device);
typedef ALCenum        (ALC_APIENTRY *LPALCGETERROR)(ALCdevice *device);
typedef ALCboolean     (ALC_APIENTRY *LPALCISEXTENSIONPRESENT)(ALCdevice *device, const ALCchar *extname);
typedef ALCvoid*       (ALC_APIENTRY *LPALCGETPROCADDRESS)(ALCdevice *device, const ALCchar *funcname);
typedef ALCenum        (ALC_APIENTRY *LPALCGETENUMVALUE)(ALCdevice *device, const ALCchar *enumname);
typedef const ALCchar* (ALC_APIENTRY *LPALCGETSTRING)(ALCdevice *device, ALCenum param);
typedef void           (ALC_APIENTRY *LPALCGETINTEGERV)(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values);
typedef ALCdevice*     (ALC_APIENTRY *LPALCCAPTUREOPENDEVICE)(const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize);
typedef ALCboolean     (ALC_APIENTRY *LPALCCAPTURECLOSEDEVICE)(ALCdevice *device);
typedef void           (ALC_APIENTRY *LPALCCAPTURESTART)(ALCdevice *device);
typedef void           (ALC_APIENTRY *LPALCCAPTURESTOP)(ALCdevice *device);
typedef void           (ALC_APIENTRY *LPALCCAPTURESAMPLES)(ALCdevice *device, ALCvoid *buffer, ALCsizei samples);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AL_ALC_H */
