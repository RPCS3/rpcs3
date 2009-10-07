/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __PLUGINCALLBACKS_H__
#define __PLUGINCALLBACKS_H__

// --------------------------------------------------------------------------------------
//  <<< Important Notes to Plugin Authors >>>
// --------------------------------------------------------------------------------------
//  * C++ only: Exceptions thrown by plugins may not be handled correctly if allowed to
//    escape the scope of the plugin, and could result in unexpected behavior or odd crashes.
//    For C++ plugins this means ensuring that any code that uses 'new' or STL containers
//    (string, list, vector, etc) are contained within a try{} block, since the STL can
//    throw std::bad_alloc.
//
//  * Many callbacks are optional, and have been marked as such. Any optional callback can be
//    left NULL.  Any callback not marked optional and left NULL will cause the emulator to
//    invalidate the plugin on either enumeration or initialization.

// --------------------------------------------------------------------------------------
//  <<< Important Notes to All Developers >>>
// --------------------------------------------------------------------------------------
//  * Callback APIs cannot involve LIB-C or STL objects (such as FILE or std::string).  The
//    internal layout of these structures can vary between versions of GLIB-C / MSVCRT.
//
//  * Callback APIs cannot alloc/free memory across dynamic library boundaries.  An object
//    allocated by a plugin must be freed by that plugin.
//
//  * C++ exception handling cannot be used by either plugin callbacks or emulator callbacks.
//    This includes the emulator's Console callbacks, for example, since the nature of C++
//    ID-based RTTI could cause a C++ plugin with its own catch handlers to catch exceptions
//    of mismatched types from the emulator.
//


#ifndef BOOL
	typedef int BOOL;
#endif

// --------------------------------------------------------------------------------------
//  PS2E_HWND  -  OS-independent window handle
// --------------------------------------------------------------------------------------
// HWND is our only operating system dependent type.  For it to be defined as accurately
// as possible, this header file needs to be included after whatever window/GUI platform
// headers you need (wxWidgets, Windows.h, GTK, etc).
//
// We could be lazy with this typedef, because window handles are always a (void*) on all
// platforms that matter to us (windows, gtk, OSX).  But Windows has some type strictness
// on its HWND define that could be useful, and well it's probably good practice to use
// platform available defines when they exist.
//
#if defined( _WX_DEFS_H_ )
	typedef WXWidget PS2E_HWND;
#elif defined( _WINDEF_ )
	// For Windows let's use HWND, since it has some type strictness applied to it.
	typedef HWND PS2E_HWND;
#else
	// Unsupported platform... use void* as a best guess.  Should work fine for almost
	// any GUI platform, and certainly works for any currently supported one.
	typedef void* PS2E_HWND;
#endif

// --------------------------------------------------------------------------------------
//  PS2E_THISPTR - (ps2 component scope 'this' object pointer type)
// --------------------------------------------------------------------------------------
// This macro provides C++ plugin authors with a reasonably friendly way to automatically
// typecast the session objects for your plugin to your internal type.  Just #define
// PS2E_THISPTR to your internal structure type prior to loading this header file, and all
// APIs will assume your struct type in their signature.  Since all pointer types are inter-
// changeable, plugin APIs will retain full compatibility as long as PS2E_THISPTR is a
// pointer type.
//
#ifndef PS2E_THISPTR
#	define PS2E_THISPTR struct _PS2E_ComponentAPI*
#else
	// Ensure the user's defined PS2E_THISPTR retains the correct signature for our
	// plugin API.
	C_ASSERT( sizeof(PS2E_THISPTR) == sizeof(void*) );
#endif

// PS2E_LIB_THISPTR - (library scope version of PS2E_THISPTR)
#ifndef PS2E_LIB_THISPTR
#	define PS2E_LIB_THISPTR void*
#else
	// Ensure the user's defined PS2E_THISPTR retains the correct signature for our
	// plugin API.
	C_ASSERT( sizeof(PS2E_LIB_THISPTR) == sizeof(void*) );
#endif

// Use fastcall by default, since under most circumstances the object-model approach of the
// API will benefit considerably from it.
#define PS2E_CALLBACK __fastcall


#ifndef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------------------------------
//  Plugin Type / Version Enumerations
// ------------------------------------------------------------------------------------

enum PS2E_ComponentTypes
{
	PS2E_TYPE_GS = 0,
	PS2E_TYPE_PAD,
	PS2E_TYPE_SPU2,
	PS2E_TYPE_CDVD,
	PS2E_TYPE_DEV9,
	PS2E_TYPE_USB,
	PS2E_TYPE_FW,
	PS2E_TYPE_SIO,
	PS2E_TYPE_Mcd,
};

enum PluginLibVersion
{
	PS2E_VER_GS		= 0x1000,
	PS2E_VER_PAD	= 0x1000,
	PS2E_VER_SPU2	= 0x1000,
	PS2E_VER_CDVD	= 0x1000,
	PS2E_VER_DEV9	= 0x1000,
	PS2E_VER_USB	= 0x1000,
	PS2E_VER_FW		= 0x1000,
	PS2E_VER_SIO	= 0x1000
};

enum OSDIconTypes
{
	OSD_Icon_None = 0,
	OSD_Icon_Error,
	OSD_Icon_Notice,		// An exclamation point maybe?

	// [TODO] -- dunno.  What else?

	// Emulators implementing their own custom non-standard icon extensions should do so
	// somewhere after OSD_Icon_ReserveEnd.  All values below this are
	OSD_Icon_ReserveEnd = 0x1000
};

//-
// --------------------------------------------------------------------------------------
//  PS2E_VersionInfo
// --------------------------------------------------------------------------------------
// This structure is populated by the plugin via the PS2E_PluginLibAPI::GetVersion()
// callback.  Specify -1 for any Version or Revision to disable/ignore it.  The emulator
// will not factor that info into the display name of the plugin.
//
typedef struct _PS2E_VersionInfo
{
	// Low/Mid/High versions combine to form a number in the format of: 2.3.1
	// ... where 2 is the high version, 3 mid, and 1 low.
	s16 VersionHigh;
	s16 VersionMid;
	s16 VersionLow;

	// Revision typically refers a revision control system (such as SVN).  When displayed
	// by the emulator it will have an 'r' prefixed before it.
	s32 Revision;

} PS2E_VersionInfo;

// --------------------------------------------------------------------------------------
//  PS2E_SessionInfo
// --------------------------------------------------------------------------------------
// This struct is populated by the emulator prior to starting emulation, and is passed to
// each plugin via a call to PS2E_PluginLibAPI::EmuStart().
//
typedef struct _PS2E_SessionInfo
{
	PS2E_HWND window;

	u32* CycleEE;		// current EE cycle count
	u32* CycleIOP;		// current IOP cycle count

	u32  ElfCRC;		// CRC of the ELF header for this app/game

	// Sony's assigned serial number, valid only for CD/CDVD games (ASCII-Z string).
	//   Ex: SLUS-2932 (if the running app is not a sony-registered game, the serial
	// will be a zero length string).
	char Serial[16];

} PS2E_SessionInfo;

// --------------------------------------------------------------------------------------
//  PS2E_EmulatorInfo
// --------------------------------------------------------------------------------------
// This struct is populated by the emulator when the application is started, and is passed
// to plugins via PS2E_InitAPI.  Plugins may optionally use this information to determine
// compatibility or to select which special CPU-oriented builds and/or functions to bind
// to callbacks.
//
// The GetInt/GetBoolean/GetString/etc methods Provide an interface through which a plugin
// can retrieve special emulator-specific settings and information, or fetch options straight
// from the emulator's ini file itself.  These are intended for advanced plugin/emu binding
// only, and should generally be accompanied with the appropriate version/name checks.
//
// Emulators may implement this as a direct match to the emu's ini file/registry contents
// (recommended), or may provide additional and/or alternative custom strings.  Direct
// ini.registry relationships are preferred since those are easy for a plugin author to
// reference without documentation.
//
typedef struct _PS2E_EmulatorInfo
{
	// brief name of the emulator (ex: "PCSX2") [required]
	// Depending on the design of the emulator, this string may optionally include version
	// information, however that is not recommended since it can inhibit backward support.
	const char*	EmuName;

	// Version information.  All fields besides the emulator's name are optional.
	struct _PS2E_VersionInfo	EmuVersion;

	// Number of Physical Cores, as detected by the emulator.
	// This should always match the real # of cores supported by hardware.
	int		PhysicalCores;

	// Number of Logical Cores, as detected and/or managed by the emulator.
	// This is not necessarily a reflection of real hardware capabilities.  The emu reserves
	// the right to report this value as it deems appropriate, in management of threading
	// resources.
	int		LogicalCores;

	// GetInt
	// Self-explanatory.
	//
	// Returns:
	//   0 - Value was retrieved successfully.
	//   1 - Unknown value.  Contents of dest are unchanged.
	BOOL (PS2E_CALLBACK* GetInt)( const char* name, int* dest );

	// GetBoolean
	// Assigns *dest either 1 (true) or 0 (false).  Note to Emulators: Returning any non-
	// zero value for true probably "works" but is not recommended, since C/C++ standard
	// specifically defines the result of bool->int conversions as a 0 or 1 result.
	//
	// Returns:
	//   0 - Value was retrieved successfully.
	//   1 - Unknown value.  Contents of dest are unchanged.
	BOOL (PS2E_CALLBACK* GetBoolean)( const char* name, BOOL* result );

	// GetString
	// Copies an ASCII-Z string into the dest pointer, to max length allowed.  The result
	// is always safely zero-terminated (none of that snprintf crap where you have to
	// zero-terminate yourself >_<).
	//
	// Returns:
	//   0 - Value was retrieved successfully.
	//   1 - Unknown value.  Contents of dest are unchanged.
	BOOL (PS2E_CALLBACK* GetString)( const char* name, char* dest, int maxlen );

	// GetStringAlloc
	// Provides an alternative to GetString, that can retrieve strings of arbitrary length.
	// The plugin must provide a valid allocator callback, which takes a size parameter and
	// returns a pointer to an allocation large enough to hold the size.
	//
	// It is then the responsibility of the plugin to free the allocated pointer when it
	// is done with it.
	//
	char* (PS2E_CALLBACK* GetStringAlloc)( const char* name, void* (PS2E_CALLBACK *allocator)(int size) );

	// OSD_WriteLn
	// This function allows the plugin to post messages to the emulator's On-Screen Display.
	// The OSD message will be displayed with the specified icon (optional) for the duration
	// of a few seconds, or until other messages have scrolled it out of view.
	// Implementation of the OSD is emulator specific, and there is no guarantee that the
	// OSD will be honored at all.  If the emulator does not support OSD then this function
	// call is treated as a NOP.
	//
	// Typically a plugin author should only use the OSD for infrequent notices that are
	// potentially useful to users playing games (particularly at fullscreen).  Trouble-
	// shooting and debug information is best dumped to console or to disk log.
	//
	// Parameters:
	//  icon  - an icon identifier, typically from the PS2E_OSDIconTypes enumeration.  Specific
	//     versions of emulators may provide their own icon extensions.  The emulator will
	//     silently ignore unrecognized icon identifiers, thus retaining cross-compat.
	//
	//  msg   - string message displayed to the user.
	//
	void (PS2E_CALLBACK* OSD_WriteLn)( int icon, const char* msg );

	// TODO : Create the ConsoleLogger class.
	// Provides a set of basic console functions for writing text to the emulator's
	// console.  Some emulators may not support a console, in which case these functions
	// will be NOPs.   For plain and simple to-disk logging, plugins should create and use
	// their own logging facilities.
	//ConsoleLogger  Console;

} PS2E_EmulatorInfo;


// --------------------------------------------------------------------------------------
//  PS2E_FreezeData
// --------------------------------------------------------------------------------------
// Structure used to pass savestate info between emulator and plugin.
//
typedef struct _PS2E_FreezeData
{
	u32   Size;			// size of the data being frozen or thawed.  This value is allowed to be changed by Freeze().
	void* Data;			// pointer to the data target (freeze) or source (thaw)

} PS2E_FreezeData;


// --------------------------------------------------------------------------------------
//  PS2E_ComponentAPI
// --------------------------------------------------------------------------------------
// The PluginTypeAPI is provided for every PS2 component plugin (see PS2E_ComponentTypes
// enumeration).  For typical dlls which only provide one  plugin type of functionality,
// the plugin only needs one instance of this struct.  For multi-type plugins, for example
// a plugin that supports both DEV9 and FW together, an interface must be provided for
// each component supported.
//
// These are functions provided to the PS2 emulator from the plugin.  The emulator will
// call these functions and expect the plugin to perform defined tasks.
//
typedef struct _PS2E_ComponentAPI
{
	// EmuStart
	// This function is called by the emulator when an emulation session is started.  The
	// plugin should take this opportunity to bind itself to the given window handle, open
	// necessary audio/video/input devices, etc.
	//
	// Parameters:
	//   session - provides relevant emulation session information.  Provided pointer is
	//      valid until after the subsequent call to EmuClose()
	//
	// Threading: EmuStart is called from the GUI thread. All other emulation threads are
	//   guaranteed to be suspended or closed at the time of this call (no locks required).
	//
	void (PS2E_CALLBACK* EmuStart)( PS2E_THISPTR thisptr, const PS2E_SessionInfo *session );

	// EmuClose
	// This function is called by the emulator prior to stopping emulation.  The window
	// handle specified in EmuStart is guaranteed to be valid at the time EmuClose is called,
	// and the plugin should unload/unbind all window dependencies at this time.
	//
	// Threading: EmuClose is called from the GUI thread.  All other emulation threads are
	//   guaranteed to be suspended or closed at the time of this call (no locks required).
	//
	void (PS2E_CALLBACK* EmuClose)( PS2E_THISPTR thisptr );

	// CalcFreezeSize
	// This function should calculate and return the amount of memory needed for the plugin
	// to freeze its complete emulation state.  The value can be larger than the required
	// amount of space, but cannot be smaller.
	//
	// The emulation state when this function is called is guaranteed to be the same as
	// the following call to Freeze.
	//
	// Thread Safety:
	//   May be called from any thread (GUI, Emu, GS, Unknown, etc).
	//   All Emulation threads are halted at a PS2 logical vsync-end event.
	//   No locking is necessary.
	u32  (PS2E_CALLBACK* CalcFreezeSize)( PS2E_THISPTR thisptr );

	// Freeze
	// This function should make a complete copy of the plugin's emulation state into the
	// provided dest->Data pointer.  The plugin is allowed to reduce the dest->Size value
	// but is not allowed to make it larger.  The plugin will only receive calls to Freeze
	// and Thaw while a plugin is in an EmuStart() state.
	//
	// Parameters:
	//   dest - a pointer to the Data/Size destination buffer (never NULL).
	//
	// Thread Safety:
	//   May be called from any thread (GUI, Emu, GS, Unknown, etc).
	//   All Emulation threads are halted at a PS2 logical vsync-end event.
	//   No locking is necessary.
	void (PS2E_CALLBACK* Freeze)( PS2E_THISPTR thisptr, PS2E_FreezeData* dest );

	// Thaw
	// Plugin should restore a complete emulation state from the given FreezeData.  The
	// plugin will only receive calls to Freeze and Thaw while a plugin is in an EmuStart()
	// state.
	//
	// Thread Safety:
	//   May be called from any thread (GUI, Emu, GS, Unknown, etc).
	//   All Emulation threads are halted at a PS2 logical vsync-end event.
	//   No locking is necessary.
	void (PS2E_CALLBACK* Thaw)( PS2E_THISPTR thisptr, const PS2E_FreezeData* src );

	// Configure
	// The plugin should open a modal dialog box with plugin-specific settings and prop-
	// erties.  This function can be NULL, in which case the user's Configure option for
	// this plugin is grayed out.
	//
	// All emulation is suspended and the plugin's state is saved to memory prior to this
	// function being called.  Configure is only called outside the context of EmuStart()
	// (after a call to EmuClose()).
	//
	// Plugin authors should ensure to re-read and re-apply all settings on EmuStart(),
	// which will ensure that any user changes will be applied immediately.  For changes
	// that can be applied without emulation suspension, see/use the GUI extensions for
	// menu and toolbar shortcuts.
	//
	// Thread Safety:
	//   Always called from the GUI thread, with emulation in a halted state (no locks
	//   needed).
	void (PS2E_CALLBACK* Configure)( PS2E_THISPTR thisptr );

	// GetLastError
	// This is an optional method with allows the emulator to retrieve extended formatted
	// error information about a recent failed plugin call.  If implemented by the plugin,
	// it should store message information in it's PS2E_THISPTR allocation, and free the
	// string buffers when the plugin's instance is deleted.
	//
	// The plugin is allowed to return NULL for either msg_diag or msg_user (or both).
	// Returned pointers should be static global arrays, and must be NULL terminated.  If
	// only one message is provided, it will be used for both console log and popup.
	//
	// Parameters:
	//   msg_diag - diagnostic message, which is english only and typically intended for
	//      console or disk logging.
	//
	//   msg_user - optional translated user message, which is displayed as a popup to explain
	//      to the user why the plugin failed to initialize.
	//
	// Thread safety:
	//   * Thread Affinity: none.  May be called from any thread.
	//   * Interlocking: Instance.  All calls from the emu are fully interlocked against
	//     the plugin instance.
	//
	void (PS2E_CALLBACK* GetLastError)( PS2E_THISPTR thisptr, char* const* msg_diag, wchar_t* const* msg_user );

	// Reserved area at the end of the structure, for future API expansion.
	void* reserved[8];

} PS2E_ComponentAPI;


// --------------------------------------------------------------------------------------
//  PS2E_LibraryAPI
// --------------------------------------------------------------------------------------
// The LibraryAPI is an overall library-scope set of functions that perform basic Init,
// Shutdown, and global configuration operations.
//
// These are functions provided to the PS2 emulator from the plugin.  The emulator will
// call these functions and expect the plugin to perform defined tasks.
//
// Threading:
//   - get* callbacks in this struct are not bound to any particular thread. Implementations
//     should not assume any specific thread affinity.
//
//   - set* callbacks in this struct are bound to the GUI thread of the emulator, and will
//     always be invoked from that thread.
//
typedef struct _PS2E_LibraryAPI
{
	// GetName
	// Returns an ASCII-Z (zero-terminated) string name of the plugin.  The name should
	// *not* include version or build information.  That info is returned separately
	// via GetVersion.  The return value cannot be NULL (if it is NULL, the emulator
	// will assume the DLL is invalid and ignore it).
	//
	// The pointer should reference a static/global scope char array, or an allocated
	// heap pointer (not recommended).
	//
	// This function may be called multiple times by the emulator, so it should accommodate
	// for such if it performs heap allocations or other initialization procedures.
	const char* (PS2E_CALLBACK* GetName)();

	// GetVersion
	// This function returns name and version information for the requested PS2 component.
	// If the plugin does not support the requested component, it should return NULL.
	// The returned pointer, if non-NULL, must be either a static value [recommended] or a
	// heap-allocated value, and valid for the lifetime of the plugin.
	//
	// This function may be called multiple times by the emulator, so it should accommodate
	// for such if it performs heap allocations or other initialization procedures.
	//
	// Typically a plugin will return the same version for all supported components.  The
	// component parameter is mostly provided to allow this function to serve a dual purpose
	// of both component versioning and as a component enumerator.
	//
	// See PS2E_VersionInfo for more details.
	//
	// Parameters:
	//   component - indicates the ps2 component plugin to be versioned.  If the plugin
	//       does not support the requested component, the function should return NULL.
	//
	const PS2E_VersionInfo* (PS2E_CALLBACK* GetVersion)( u32 component );

	// Test
	// Called by the plugin enumerator to check the hardware availability of the specified
	// component.  The function should return 1 if the plugin appears to be supported, or
	// 0 if the test failed.
	//
	// While a plugin is welcome to use its own CPU capabilities information, it is recommended
	// that you use the emulator provided CPU capabilities structure instead.  The emulator may
	// have provisions in its interface to allow for the forced disabling of extended CPU cap-
	// abilities, for testing purposes.
	//
	BOOL (PS2E_CALLBACK* Test)( u32 component, const PS2E_EmulatorInfo* xinfo );

	// NewComponentInstance
	// The emulator calls this function to fetch the API for the requested component.
	// The plugin is expected to perform an "availability test" (the same test as performed
	// by Test()) and return NULL if the plugin does not support the host machine's hardware
	// or software installations.
	//
	// This function is only called for components which the plugin returned a non-NULL
	// version information struct for in GetVersion().
	//
	// Plugin Allocation Strategy:
	//   The Component API has been designed with function invocation efficiency in mind.
	//   To allocate your plugin's component instance you should create a structure that
	//   contains PS2E_ComponentAPI_* as the first member (where * refers to the PS2
	//   component type), and plugin-specific instance data is stored as any number of
	//   subsequent members in the struct.
	//
	// Parameters:
	//   component - indicates the ps2 component API to return.
	//   dest      - structure to fill with plugin function implementations.  Dest should
	//      be manually typecast by the plugin to match the requested component.
	//
	// OnError:
	//   Plugins may optionally prepare more detailed information on why the plugin failed
	//   it's availability test which the emu can request via GetLastError.
	//
	PS2E_THISPTR (PS2E_CALLBACK* NewComponentInstance)( u32 component );

	// DeleteComponentInstance
	// Called by the emulator when the plugin component is to be shutdown.  The component API
	// instance pointer can be safely deleted here.
	void (PS2E_CALLBACK* DeleteComponentInstance)( PS2E_THISPTR instance );

	// SetSettingsFolder
	// Callback is passed an ASCII-Z string representing the folder where the emulator's
	// settings files are stored (may either be under the user's documents folder, or a
	// location relative to the CWD of the emu application).
	//
	// Typically this callback is only issued once per plugin session, aand prior to the
	// opening of any PS2 components.  It is the responsibility of the emu to save the
	// emulation state, shutdown plugins, and restart everything anew from the new settings
	// in such an event as a dynamic change of the settings folder.
	//
	void (PS2E_CALLBACK* SetSettingsFolder)( const char* folder );

	// SetLogFolder
	// This callback may be issued at any time.  It is the responsibility of the plugin
	// to do the necessary actions to close existing disk logging facilities and re-open
	// new facilities.
	//
	// Thread Safety:
	//   This function is always called from the GUI thread.  All emulation threads are
	//   suspended during the call, so no locking is required.
	//
	void (PS2E_CALLBACK* SetLogFolder)( const char* folder );

	// Reserved area at the end of the structure, for future API expansion.  This area
	// should always be zeroed out, so that future versions of emulators that may have
	// defined functions here will recognize the functions as not supported.
	void* reserved[12];

} PS2E_LibraryAPI;

// --------------------------------------------------------------------------------------
//  PS2E_Image
// --------------------------------------------------------------------------------------
// Simple RGBA image data container, for passing surface textures to the GS plugin, and
// for receiving snapshots from the GS plugin.
//
// fixme - this might be more ideal as BGRA or ABGR format on Windows platforms?
//
typedef struct _PS2E_Image
{
	u32 width;
	u32 height;
	u8* data;		// RGBA data.  top to bottom.

} PS2E_Image;

// --------------------------------------------------------------------------------------
//  PS2E_ComponentAPI_GS
// --------------------------------------------------------------------------------------
// Thread Safety:
//   All GS callbacks are issued from the GS thread only, and are always called synchronously
//   with all other component API functions.  No locks are needed, and DirectX-based GS
//   plugins can safely disable DX multithreading support for speedup (unless the plugin
//   utilizes multiple threads of its own internally).
//
typedef struct _PS2E_ComponentAPI_GS
{
	// Base Component API (inherited structure)
	struct _PS2E_ComponentAPI Base;

	// SetSnapshotsFolder
	// Callback is passed an ASCII-Z string representing the folder where the emulator's
	// snapshots are to be saved (typically located under user documents, but may be CWD
	// or any user-specified location).
	//
	// Thread Safety:
	//   This function is only called from the GUI thread, however other threads are not
	//   suspended.
	//
	void (PS2E_CALLBACK* SetSnapshotsFolder)( PS2E_THISPTR thisptr, const char* folder );

	// TakeSnapshot
	// The GS plugin is to save the current frame into the given target image.  This
	// function is always called immediately after a GSvsync(), ensuring that the current
	// framebuffer is safely intact for capture.
	//
	// Returns TRUE if the snapshot succeeded, or FALSE if it failed (contents of dest
	// are considered indeterminate and will be ignored by the emu).
	BOOL (PS2E_CALLBACK* TakeSnapshot)( PS2E_THISPTR thisptr, PS2E_Image* dest );

	// OSD_SetTexture
	// Uploads a new OSD texture to the GS.  Display of the OSD should be performed at
	// the next soonest possible vsync.
	void (PS2E_CALLBACK* OSD_SetTexture)( PS2E_THISPTR thisptr, PS2E_Image* src );

	// OSD_SetAlpha
	//
	// Parameters:
	//  alphOverall - Specifies the 'full' opacity of the OSD. The alphaFade setting
	//     effectively slides from alphaOverall to 0.0.
	//
	//  alphaFade   - Specifies the fadeout status of the OSD.  This value can be loosely
	//     interpreted by the GS plugin.  The only requirement is that the GS plugin
	//     honor the fade value of 0.0 (OSD is not displayed).
	void (PS2E_CALLBACK* OSD_SetAlpha)( PS2E_THISPTR thisptr, float alphaOverall, float alphaFade );

	// OSD_SetPosition
	// Self-explanatory.
	void (PS2E_CALLBACK* OSD_SetPosition)( PS2E_THISPTR thisptr, int xpos, int ypos );

	// GSvsync
	//
	// Returns FALSE if the plugin encountered a critical error while updating the display;
	// indicating a device or emulation failure that should terminate the current emulation.
	// (if any critical errors accumulated during GStransferTags or GStransferImage, they
	//  should also be handled here by returning FALSE)
	//
	BOOL (PS2E_CALLBACK* GSvsync)(int field);

	//
	//
	void (PS2E_CALLBACK* GSreadFIFO)(u128 *pMem, int qwc);

	// GStransferTag
	// Sends a set of GIFtags.  Note that SIGNAL, FINISH, and LABEL tags are handled
	// internally by the emulator in a thread-safe manner -- the GS plugin can safely
	// ignore the tags (and there is no guarantee the emulator will even bother to
	// pass the tags onto the GS).
	//
	// Returns FALSE if the plugin encountered a critical error while setting texture;
	// indicating a device failure.
	void (PS2E_CALLBACK* GStransferTags)(u128 *pMem, int tagcnt);

	// GStransferPackedTag
	// Sends a set of packed GIFtags. Note that SIGNAL, FINISH, and LABEL tags are handled
	// internally by the emulator in a thread-safe manner -- the GS plugin can safely
	// ignore the tags (and there is no guarantee the emulator will even bother to
	// pass the tags onto the GS).
	void (PS2E_CALLBACK* GStransferPackedTags)(u128 *pMem, int tagcnt);

	// GStransferImage
	// Uploads GIFtag image data.
	//
	// fixme: Make sure this is designed sufficiently to account for emulator-side texture
	// caching.
	void (PS2E_CALLBACK* GStransferImage)(u128 *pMem, u32 len_qwc);

	void* reserved[8];

} PS2E_ComponentAPI_GS;

// --------------------------------------------------------------------------------------
//  PS2E_ComponentAPI_Mcd
// --------------------------------------------------------------------------------------
// Thread Safety:
//  * Thread affinity is not guaranteed.  Calls may be made from either the main emu thread
//    or an IOP child thread (if the emulator uses one).
//
//  * No locking required: All calls to the memory cards are interlocked by the emulator.
//
typedef struct _PS2E_ComponentAPI_Mcd
{
	// Base Component API (inherited structure)
	struct _PS2E_ComponentAPI Base;

	// McdIsPresent
	// Called by the emulator to detect the availability of a memory card.  This function
	// will be called frequently -- essentially whenever the SIO port for the memory card
	// has its status polled - so its overhead should be minimal when possible.
	//
	// Returns:
	//   0 if the card is not available, or 1 if it is available.
	//
	BOOL (PS2E_CALLBACK* McdIsPresent)( PS2E_THISPTR thisptr, uint port, uint slot );

	// McdRead
	// Requests that a block of data be loaded from the memorycard into the specified dest
	// buffer (which is allocated by the caller).  Bytes read should match the requested
	// size.  Reads *must* be performed synchronously (function cannot return until the
	// read op has finished).
	//
	// Returns:
	//   0 on failure, and 1 on success.  Emulator may use GetLastError to retrieve additional
	//   information for logging or displaying to the user.
	//
	BOOL (PS2E_CALLBACK* McdRead)( PS2E_THISPTR thisptr, uint port, uint slot, u8 *dest, u32 adr, int size );

	// McdSave
	// Saves the provided block of data to the memorycard at the specified seek address.
	// Writes *must* be performed synchronously (function cannot return until the write op
	// has finished).  Write cache flushing is optional.
	//
	// Returns:
	//   0 on failure, and 1 on success.  Emulator may use GetLastError to retrieve additional
	//   information for logging or displaying to the user.
	//
	BOOL (PS2E_CALLBACK* McdSave)( PS2E_THISPTR thisptr, uint port, uint slot, const u8 *src, u32 adr, int size );

	// McdErase
	// Saves "cleared" data to the memorycard at the specified seek address.  Cleared data
	// is a series of 0xff values (all bits set to 1).
	// Writes *must* be performed synchronously (function cannot return until the write op
	// has finished).  Write cache flushing is optional.
	//
	// Returns:
	//   0 on failure, and 1 on success.  Emulator may use GetLastError to retrieve additional
	//   information for logging or displaying to the user.
	//
	BOOL (PS2E_CALLBACK* McdEraseBlock)( PS2E_THISPTR thisptr, uint port, uint slot, u32 adr );

	u64 (PS2E_CALLBACK* McdGetCRC)( PS2E_THISPTR thisptr, uint port, uint slot );

	void* reserved[8];

} PS2E_ComponentAPI_Mcd;




// ------------------------------------------------------------------------------------
//  KeyEvent type enumerations
// ------------------------------------------------------------------------------------

enum PS2E_KeyEventTypes
{
	PS2E_KEY_UP = 0,
	PS2E_KEY_DOWN
};

#define PS2E_SHIFT		1
#define PS2E_CONTROL	2
#define PS2E_ALT		4

// --------------------------------------------------------------------------------------
//  PS2E_KeyEvent
// --------------------------------------------------------------------------------------
// Structure used to key event data from pad plugin to emulator.
//
typedef struct _PS2E_KeyEvent
{
	PS2E_KeyEventTypes event;
	// Value of the key being pressed or released
	uint value;
	// Combination of PS2E_SHIFT, PS2E_CONTROL, and/or PS2E_ALT, indicating which
	// modifier keys were also down when the key was pressed.
	uint flags;
} PS2E_KeyEvent;


// --------------------------------------------------------------------------------------
//  PS2E_ComponentAPI_Pad
// --------------------------------------------------------------------------------------
// Thread Safety:
//  * Thread affinity is not guaranteed, except for PadKeyEvent, which is called in the
//    GUI thread.  Other calls may be made from either the main emu thread or an
//    IOP child thread (if the emulator uses one).
//
typedef struct _PS2E_ComponentAPI_Pad
{
	// Base Component API (inherited structure)
	struct _PS2E_ComponentAPI Base;

	// PadIsPresent
	// Called by the emulator to detect the availability of a pad.  This function
	// will be called frequently -- essentially whenever the SIO port for the pad
	// has its status polled - so its overhead should be minimal when possible.
	//
	// A plugin should behave reasonably when a pad that's not plugged in is polled.
	//
	// Returns:
	//   0 if the card is not available, or 1 if it is available.
	//
	BOOL (PS2E_CALLBACK* PadIsPresent)( PS2E_THISPTR thisptr, uint port, uint slot );

	// PadStartPoll
	// Called by the emulator to start polling the specified pad.
	//
	// Returns:
	//   First byte in response to the poll (Typically 0xff).
	//
	u8 (PS2E_CALLBACK* PadStartPoll)( PS2E_THISPTR thisptr, uint port, uint slot );

	// PadPoll
	// Continues polling the specified pad, sending the given value.
	//
	// Returns:
	//   Next byte in response to the poll.
	//
	u8 (PS2E_CALLBACK* PadPoll)( PS2E_THISPTR thisptr, u8 value );

	// PadKeyEvent
	// Called by the emulator in the gui thread to check for keys being pressed or released.
	//
	// Returns:
	//   PS2E_KeyEvent:  Key being pressed or released.  Should stay valid until next call to
	//                   PadKeyEvent or plugin is closed with EmuClose.
	//
	typedef PS2E_KeyEvent* (CALLBACK* PadKeyEvent)();

	void* reserved[8];

} PS2E_ComponentAPI_Pad;


// --------------------------------------------------------------------------------------
//  PS2E_InitAPI
// --------------------------------------------------------------------------------------
// Called by the emulator when the plugin is loaded into memory.  The emulator uses the
// presence of this function to detect PS2E-v2 plugin API, and will direct all subsequent
// calls through the returned LibraryAPI.  The function is allowed to return NULL if the
// emulator's version information or machine capabilities are insufficient for the
// plugin's needs.
//
// Note: It is recommended that plugins query machine capabilities from the emulator rather
// than the operating system or CPUID directly, since it allows the emulator the option of
// overriding the reported capabilities, for diagnostic purposes.  (such behavior is not
// required, however)
//
// This function is called *once* for the duration of a loaded plugin.
//
// Returns:
//   A pointer to a static structure that contains the API for this plugin, or NULL if
//   the plugin explicitly does not support the emulator version or machine specs.
//
// OnError:
//   Plugins may optionally prepare more detailed information on why the plugin failed
//   it's availability test which the emu can request via PS2E_GetLastError.
//
// Thread Safety:
//  * Affinity: Called only from the Main/GUI thread.
//  * Interlocking: Full interlocking garaunteed.
//
typedef const PS2E_LibraryAPI* (PS2E_CALLBACK* _PS2E_InitAPI)( const PS2E_EmulatorInfo* emuinfo );

// --------------------------------------------------------------------------------------
//  PS2E_GetLastError
// --------------------------------------------------------------------------------------
// Optional method which may be called by the emulator if the plugin returned NULL on
// PS2E_InitAPI.  Plugins may return NULL for either/both msg_diag and msg_user.  Returned
// pointers should be static global arrays, and must be NULL terminated.  If only one
// message is provided, it will be used for both console log and popup.
//
// Parameters:
//   msg_diag - diagnostic message, which is english only and typically intended for console
//      or disk logging.
//
//   msg_user - optional translated user message, which is displayed as a popup to explain
//      to the user why the plugin failed to initialize.
//
typedef void (PS2E_CALLBACK* _PS2E_GetLastError)( char* const* msg_diag, wchar_t* const* msg_user  );

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// Begin old legacy API here (present for reference purposes only, until all plugin API
// specifics have been accounted for)
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#if 0
// PAD
typedef s32  (CALLBACK* _PADinit)(char *configpath, u32 flags);
typedef s32  (CALLBACK* _PADopen)(void *pDisplay);
typedef void (CALLBACK* _PADclose)();
typedef void (CALLBACK* _PADshutdown)();
typedef keyEvent* (CALLBACK* _PADkeyEvent)();
typedef u8   (CALLBACK* _PADstartPoll)(u8 pad);
typedef u8   (CALLBACK* _PADpoll)(u8 value);
typedef u32  (CALLBACK* _PADquery)();
typedef void (CALLBACK* _PADupdate)(u8 pad);

typedef void (CALLBACK* _PADgsDriverInfo)(GSdriverInfo *info);
typedef s32  (CALLBACK* _PADfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _PADconfigure)();
typedef s32  (CALLBACK* _PADtest)();
typedef void (CALLBACK* _PADabout)();
typedef s32  (CALLBACK* _PADsetSlot)(u8 port, u8 slot);
typedef s32  (CALLBACK* _PADqueryMtap)(u8 port);

// SIO
typedef s32  (CALLBACK* _SIOinit)(int types, SIOchangeSlotCB f);
typedef s32  (CALLBACK* _SIOopen)(void *pDisplay);
typedef void (CALLBACK* _SIOclose)();
typedef void (CALLBACK* _SIOshutdown)();
typedef s32   (CALLBACK* _SIOstartPoll)(u8 deviceType, u32 port, u32 slot, u8 *returnValue);
typedef s32   (CALLBACK* _SIOpoll)(u8 value, u8 *returnValue);
typedef u32  (CALLBACK* _SIOquery)();

typedef void (CALLBACK* _SIOkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _SIOfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _SIOconfigure)();
typedef s32  (CALLBACK* _SIOtest)();
typedef void (CALLBACK* _SIOabout)();

// SPU2
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _SPU2init)(char *configpath);
typedef s32  (CALLBACK* _SPU2open)(void *pDisplay);
typedef void (CALLBACK* _SPU2close)();
typedef void (CALLBACK* _SPU2shutdown)();
typedef void (CALLBACK* _SPU2write)(u32 mem, u16 value);
typedef u16  (CALLBACK* _SPU2read)(u32 mem);

typedef void (CALLBACK* _SPU2readDMA4Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2writeDMA4Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2interruptDMA4)();
typedef void (CALLBACK* _SPU2readDMA7Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2writeDMA7Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2interruptDMA7)();

typedef void (CALLBACK* _SPU2readDMAMem)(u16 *pMem, u32 size, u8 core);
typedef void (CALLBACK* _SPU2writeDMAMem)(u16 *pMem, u32 size, u8 core);
typedef void (CALLBACK* _SPU2interruptDMA)(u8 core);
typedef void (CALLBACK* _SPU2setDMABaseAddr)(uptr baseaddr);
typedef void (CALLBACK* _SPU2irqCallback)(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());
typedef bool (CALLBACK* _SPU2setupRecording)(bool start);

typedef void (CALLBACK* _SPU2setClockPtr)(u32*ptr);
typedef void (CALLBACK* _SPU2setTimeStretcher)(short int enable);

typedef u32 (CALLBACK* _SPU2ReadMemAddr)(u8 core);
typedef void (CALLBACK* _SPU2WriteMemAddr)(u8 core,u32 value);
typedef void (CALLBACK* _SPU2async)(u32 cycles);
typedef s32  (CALLBACK* _SPU2freeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _SPU2keyEvent)(keyEvent* ev);
typedef void (CALLBACK* _SPU2configure)();
typedef s32  (CALLBACK* _SPU2test)();
typedef void (CALLBACK* _SPU2about)();

// CDVD
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _CDVDinit)(char *configpath);
typedef s32  (CALLBACK* _CDVDopen)(const char* pTitleFilename);
typedef void (CALLBACK* _CDVDclose)();
typedef void (CALLBACK* _CDVDshutdown)();
typedef s32  (CALLBACK* _CDVDreadTrack)(u32 lsn, int mode);
typedef u8*  (CALLBACK* _CDVDgetBuffer)();
typedef s32  (CALLBACK* _CDVDreadSubQ)(u32 lsn, cdvdSubQ* subq);
typedef s32  (CALLBACK* _CDVDgetTN)(cdvdTN *Buffer);
typedef s32  (CALLBACK* _CDVDgetTD)(u8 Track, cdvdTD *Buffer);
typedef s32  (CALLBACK* _CDVDgetTOC)(void* toc);
typedef s32  (CALLBACK* _CDVDgetDiskType)();
typedef s32  (CALLBACK* _CDVDgetTrayStatus)();
typedef s32  (CALLBACK* _CDVDctrlTrayOpen)();
typedef s32  (CALLBACK* _CDVDctrlTrayClose)();

typedef void (CALLBACK* _CDVDkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _CDVDfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _CDVDconfigure)();
typedef s32  (CALLBACK* _CDVDtest)();
typedef void (CALLBACK* _CDVDabout)();
typedef void (CALLBACK* _CDVDnewDiskCB)(void (*callback)());

// DEV9
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _DEV9init)(char *configpath);
typedef s32  (CALLBACK* _DEV9open)(void *pDisplay);
typedef void (CALLBACK* _DEV9close)();
typedef void (CALLBACK* _DEV9shutdown)();
typedef u8   (CALLBACK* _DEV9read8)(u32 mem);
typedef u16  (CALLBACK* _DEV9read16)(u32 mem);
typedef u32  (CALLBACK* _DEV9read32)(u32 mem);
typedef void (CALLBACK* _DEV9write8)(u32 mem, u8 value);
typedef void (CALLBACK* _DEV9write16)(u32 mem, u16 value);
typedef void (CALLBACK* _DEV9write32)(u32 mem, u32 value);
typedef void (CALLBACK* _DEV9readDMA8Mem)(u32 *pMem, int size);
typedef void (CALLBACK* _DEV9writeDMA8Mem)(u32 *pMem, int size);
typedef void (CALLBACK* _DEV9irqCallback)(DEV9callback callback);
typedef DEV9handler (CALLBACK* _DEV9irqHandler)(void);

typedef void (CALLBACK* _DEV9keyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _DEV9freeze)(int mode, freezeData *data);
typedef void (CALLBACK* _DEV9configure)();
typedef s32  (CALLBACK* _DEV9test)();
typedef void (CALLBACK* _DEV9about)();

// USB
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _USBinit)(char *configpath);
typedef s32  (CALLBACK* _USBopen)(void *pDisplay);
typedef void (CALLBACK* _USBclose)();
typedef void (CALLBACK* _USBshutdown)();
typedef u8   (CALLBACK* _USBread8)(u32 mem);
typedef u16  (CALLBACK* _USBread16)(u32 mem);
typedef u32  (CALLBACK* _USBread32)(u32 mem);
typedef void (CALLBACK* _USBwrite8)(u32 mem, u8 value);
typedef void (CALLBACK* _USBwrite16)(u32 mem, u16 value);
typedef void (CALLBACK* _USBwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _USBasync)(u32 cycles);


typedef void (CALLBACK* _USBirqCallback)(USBcallback callback);
typedef USBhandler (CALLBACK* _USBirqHandler)(void);
typedef void (CALLBACK* _USBsetRAM)(void *mem);

typedef void (CALLBACK* _USBkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _USBfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _USBconfigure)();
typedef s32  (CALLBACK* _USBtest)();
typedef void (CALLBACK* _USBabout)();

//FW
typedef s32  (CALLBACK* _FWinit)(char *configpath);
typedef s32  (CALLBACK* _FWopen)(void *pDisplay);
typedef void (CALLBACK* _FWclose)();
typedef void (CALLBACK* _FWshutdown)();
typedef u32  (CALLBACK* _FWread32)(u32 mem);
typedef void (CALLBACK* _FWwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _FWirqCallback)(void (*callback)());

typedef void (CALLBACK* _FWkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _FWfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _FWconfigure)();
typedef s32  (CALLBACK* _FWtest)();
typedef void (CALLBACK* _FWabout)();

// General
extern _PS2EgetLibType PS2EgetLibType;
extern _PS2EgetLibVersion2 PS2EgetLibVersion2;
extern _PS2EgetLibName PS2EgetLibName;
extern _PS2EpassConfig PS2EpassConfig;

#endif

#ifndef __cplusplus
}
#endif

#endif // __PLUGINCALLBACKS_H__
