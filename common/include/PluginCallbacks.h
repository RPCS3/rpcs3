/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef __PLUGINCALLBACKS_H__
#define __PLUGINCALLBACKS_H__

#include "x86caps.h"		// fixme: x86caps.h needs to be implemented from the pcsx2 emitter cpucaps structs

//////////////////////////////////////////////////////////////////////////////////////////
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


//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_THISPTR - (ps2 component scope 'this' object pointer type)
//
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

//////////////////////////////////////////////////////////////////////////////////////////
// Plugin Type / Version Enumerations
//
enum PS2E_ComponentTypes
{
	PS2E_TYPE_GS=0,
	PS2E_TYPE_PAD,
	PS2E_TYPE_SPU2,
	PS2E_TYPE_CDVD,
	PS2E_TYPE_DEV9,
	PS2E_TYPE_USB,
	PS2E_TYPE_FW,
	PS2E_TYPE_SIO,

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

/////////////////////////////////////////////////////////////////////////////////////////
// PS2E_VersionInfo
// 
// This structure is populated by the plugin via the PS2E_PluginLibAPI::GetVersion()
// callback.  Specify -1 for any Version or Revision to disable/ignore it.  The emulator
// will not factor that info into the display name of the plugin.
//
typedef struct PS2E_VersionInfo
{
	// Low/Mid/High versions combine to form a number in the format of: 2.3.1
	// ... where 2 is the high version, 3 mid, and 1 low.
	s16 VersionLow;
	s16 VersionMid;
	s16 VersionHigh;

	// Revision typically refers a revision control system (such as SVN).  When displayed
	// by the emulator it will have an 'r' prefixed before it.
	s32 Revision;

} PS2E_VersionInfo;

//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_MachineInfo
// 
// This struct is populated by the emulator when the application is started, and is passed
// to plugins via PS2E_PluginLibAPI::Init().  Plugins may optionally use this information
// to determine compatibility or to select which special CPU-oriented builds and/or functions
// to bind to callbacks.
//
// Fields marked as Optional can either be NULL or -1, denoting the field is unused/ignored.
//
typedef struct _PS2E_MachineInfo
{
	const x86CPU_INFO*	x86caps;

	// brief name of the emulator (ex: "PCSX2") [required]
	// Depending on the design of the emulator, this string may optionally include version
	// information, however that is not recommended since it can inhibit backward support.
	const char*	EmuName;

	s16			EmuVersionLow;			// [optional]
	s16			EmuVersionMid;			// [optional]
	s16			EmuVersionHigh;			// [optional]
	s32			EmuRevision;			// emulator's revision number. [optional]

} PS2E_MachineInfo;

//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_SessionInfo
// 
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
	const char Serial[16];

} PS2E_SessionInfo;

/////////////////////////////////////////////////////////////////////////////////////////
// PS2E_EmulatorAPI
//
// These functions are provided to the PS2 plugins by the emulator.  Plugins may call these
// functions to perform operations or retrieve data.
//
typedef struct _PS2E_EmulatorAPI
{
	// TODO : Create the ConsoleLogger class.
	// Provides a set of basic console functions for writing text to the emulator's
	// console.  Some emulators may not support a console, in which case these functions
	// will be NOPs.   For plain and simple to-disk logging, plugins should create and use
	// their own logging facilities.
	ConsoleLogger*  Console;

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
	void (PS2E_CALLBACK* OSD_WriteLn)( int icon, const char* msg );
	
} PS2E_EmulatorAPI;


//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_FreezeData
//
// Structure used to pass savestate info between emulator and plugin.
//
typedef struct _PS2E_FreezeData
{
	u32   Size;			// size of the data being frozen or thawed.  This value is allowed to be changed by Freeze().
	void* Data;			// pointer to the data target (freeze) or source (thaw)

} PS2E_FreezeData;


//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_ComponentAPI
//
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

	// Reserved area at the end of the structure, for future API expansion.
	void* reserved[16];

} PS2E_ComponentAPI;


/////////////////////////////////////////////////////////////////////////////////////////
// PS2E_LibraryAPI
//
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
	// Exceptions:
	//   C++ Plugins may alternately use exception handling to return more detailed
	//   information on why the plugin failed it's availability test.  [TODO]
	//
	s32 (PS2E_CALLBACK* Test)( u32 component, const PS2E_MachineInfo* xinfo );

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
	// Exceptions:
	//   C++ Plugins may alternately use exception handling to return more detailed
	//   information on why the plugin failed it's availability test.  [TODO]
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

//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_Image
//
// Simple RGBA image data container, for passing surface textures to the GS plugin, and
// for receiving snapshots from the GS plugin.
//
// fixme - this might be more ideal as BGRA or ABGR format on Windows platforms?
//
typedef struct _PS2E_Image
{
	u32 width
	u32 height;
	u8* data;		// RGBA data.  top to bottom.

} PS2E_Image;

//////////////////////////////////////////////////////////////////////////////////////////
// PS2E_ComponentAPI_GS
//
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
	void (PS2E_CALLBACK* TakeSnapshot)( PS2E_THISPTR thisptr, PS2E_Image* dest );

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

	void (PS2E_CALLBACK* OSD_SetPosition)( PS2E_THISPTR thisptr, int xpos, int ypos );

	void (PS2E_CALLBACK* GSvsync)(int field);
	
	//
	//
	void (PS2E_CALLBACK* GSreadFIFO)(u128 *pMem, int qwc);

	// GStransferTag
	// Sends a set of GIFtags.  Note that SIGNAL, FINISH, and LABEL tags are handled
	// internally by the emulator in a thread-safe manner -- the GS plugin can safely
	// ignore the tags (and there is no guarantee the emulator will even bother to
	// pass the tags onto the GS).
	void (PS2E_CALLBACK* GStransferTags)(u128 *pMem, int tagcnt);

	// GStransferPackedTag
	// Sends a set of packed GIFtags. Note that SIGNAL, FINISH, and LABEL tags are handled
	// internally by the emulator in a thread-safe manner -- the GS plugin can safely
	// ignore the tags (and there is no guarantee the emulator will even bother to
	// pass the tags onto the GS).
	void (PS2E_CALLBACK* GStransferPackedTags)(u128 *pMem, int tagcnt);

	// GStransferImage
	// Uploads GIFtag image data.
	void (PS2E_CALLBACK* GStransferImage)(u128 *pMem, u32 len_qwc);
	
	void* reserved[8];

} PS2E_ComponentAPI_GS;

// PS2E_InitAPI
// Called by the emulator when the plugin is loaded into memory.  The emulator uses the
// presence of this function to detect PS2E-v2 plugin API, and will direct all subsequent
// calls through the returned LibraryAPI.  The function is allowed to return NULL if the
// emulator's version information or machine capabilities are insufficient for the
// plugin's needs.
//
// This function is called *once* for the duration of a loaded plugin.
// 
// Parameters:
//   xinfo - Machine info and capabilities, usable for cpu detection.  This pointer is 
//     valid for the duration of the plugin's tenure in memory.
//
// Returns:
//   A pointer to a static structure that contains the API for this plugin, or NULL if
//   the plugin explicitly does not support the emulator version.
//
// Exceptions:
//   C++ Plugins can use exceptions instead of NULL to return additional information on
//   why the plugin failed to init the API.  [TODO]
//
typedef const PS2E_LibraryAPI* (PS2E_CALLBACK* _PS2E_InitAPI)( const PS2E_MachineInfo* xinfo );


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// Begin old legacy API here (present for reference purposes only, until all plugin API
// specifics have been accounted for)
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


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

// GS
extern _GSinit            GSinit;
extern _GSopen            GSopen;
extern _GSclose           GSclose;
extern _GSshutdown        GSshutdown;
extern _GSvsync           GSvsync;
extern _GSgifTransfer1    GSgifTransfer1;
extern _GSgifTransfer2    GSgifTransfer2;
extern _GSgifTransfer3    GSgifTransfer3;
extern _GSgetLastTag      GSgetLastTag;
extern _GSgifSoftReset    GSgifSoftReset;
extern _GSreadFIFO        GSreadFIFO;
extern _GSreadFIFO2       GSreadFIFO2;

extern _GSkeyEvent        GSkeyEvent;
extern _GSchangeSaveState GSchangeSaveState;
extern _GSmakeSnapshot	   GSmakeSnapshot;
extern _GSmakeSnapshot2   GSmakeSnapshot2;
extern _GSirqCallback 	   GSirqCallback;
extern _GSprintf      	   GSprintf;
extern _GSsetBaseMem 	   GSsetBaseMem;
extern _GSsetGameCRC		GSsetGameCRC;
extern _GSsetFrameSkip	   GSsetFrameSkip;
extern _GSsetupRecording GSsetupRecording;
extern _GSreset		   GSreset;
extern _GSwriteCSR		   GSwriteCSR;
extern _GSgetDriverInfo   GSgetDriverInfo;
#ifdef _WINDOWS_
extern _GSsetWindowInfo   GSsetWindowInfo;
#endif
extern _GSfreeze          GSfreeze;
extern _GSconfigure       GSconfigure;
extern _GStest            GStest;
extern _GSabout           GSabout;

// PAD1
extern _PADinit           PAD1init;
extern _PADopen           PAD1open;
extern _PADclose          PAD1close;
extern _PADshutdown       PAD1shutdown;
extern _PADkeyEvent       PAD1keyEvent;
extern _PADstartPoll      PAD1startPoll;
extern _PADpoll           PAD1poll;
extern _PADquery          PAD1query;
extern _PADupdate         PAD1update;

extern _PADfreeze         PAD1freeze;
extern _PADgsDriverInfo   PAD1gsDriverInfo;
extern _PADconfigure      PAD1configure;
extern _PADtest           PAD1test;
extern _PADabout          PAD1about;

// PAD2
extern _PADinit           PAD2init;
extern _PADopen           PAD2open;
extern _PADclose          PAD2close;
extern _PADshutdown       PAD2shutdown;
extern _PADkeyEvent       PAD2keyEvent;
extern _PADstartPoll      PAD2startPoll;
extern _PADpoll           PAD2poll;
extern _PADquery          PAD2query;
extern _PADupdate         PAD2update;

extern _PADfreeze         PAD2freeze;
extern _PADgsDriverInfo   PAD2gsDriverInfo;
extern _PADconfigure      PAD2configure;
extern _PADtest           PAD2test;
extern _PADabout          PAD2about;

// SIO[2]
extern _SIOinit           SIOinit[2][9];
extern _SIOopen           SIOopen[2][9];
extern _SIOclose          SIOclose[2][9];
extern _SIOshutdown       SIOshutdown[2][9];
extern _SIOstartPoll      SIOstartPoll[2][9];
extern _SIOpoll           SIOpoll[2][9];
extern _SIOquery          SIOquery[2][9];
extern _SIOkeyEvent       SIOkeyEvent;

extern _SIOfreeze         SIOfreeze[2][9];
extern _SIOconfigure      SIOconfigure[2][9];
extern _SIOtest           SIOtest[2][9];
extern _SIOabout          SIOabout[2][9];

// SPU2
extern _SPU2init          SPU2init;
extern _SPU2open          SPU2open;
extern _SPU2close         SPU2close;
extern _SPU2shutdown      SPU2shutdown;
extern _SPU2write         SPU2write;
extern _SPU2read          SPU2read;
extern _SPU2readDMA4Mem   SPU2readDMA4Mem;
extern _SPU2writeDMA4Mem  SPU2writeDMA4Mem;
extern _SPU2interruptDMA4 SPU2interruptDMA4;
extern _SPU2readDMA7Mem   SPU2readDMA7Mem;
extern _SPU2writeDMA7Mem  SPU2writeDMA7Mem;
extern _SPU2setDMABaseAddr SPU2setDMABaseAddr;
extern _SPU2interruptDMA7 SPU2interruptDMA7;
extern _SPU2ReadMemAddr   SPU2ReadMemAddr;
extern _SPU2setupRecording SPU2setupRecording;
extern _SPU2WriteMemAddr   SPU2WriteMemAddr;
extern _SPU2irqCallback   SPU2irqCallback;

extern _SPU2setClockPtr   SPU2setClockPtr;
extern _SPU2setTimeStretcher SPU2setTimeStretcher;

extern _SPU2keyEvent        SPU2keyEvent;
extern _SPU2async         SPU2async;
extern _SPU2freeze        SPU2freeze;
extern _SPU2configure     SPU2configure;
extern _SPU2test          SPU2test;
extern _SPU2about         SPU2about;

// CDVD
extern _CDVDinit          CDVDinit;
extern _CDVDopen          CDVDopen;
extern _CDVDclose         CDVDclose;
extern _CDVDshutdown      CDVDshutdown;
extern _CDVDreadTrack     CDVDreadTrack;
extern _CDVDgetBuffer     CDVDgetBuffer;
extern _CDVDreadSubQ      CDVDreadSubQ;
extern _CDVDgetTN         CDVDgetTN;
extern _CDVDgetTD         CDVDgetTD;
extern _CDVDgetTOC        CDVDgetTOC;
extern _CDVDgetDiskType   CDVDgetDiskType;
extern _CDVDgetTrayStatus CDVDgetTrayStatus;
extern _CDVDctrlTrayOpen  CDVDctrlTrayOpen;
extern _CDVDctrlTrayClose CDVDctrlTrayClose;

extern _CDVDkeyEvent        CDVDkeyEvent;
extern _CDVDfreeze          CDVDfreeze;
extern _CDVDconfigure     CDVDconfigure;
extern _CDVDtest          CDVDtest;
extern _CDVDabout         CDVDabout;
extern _CDVDnewDiskCB     CDVDnewDiskCB;

// DEV9
extern _DEV9init          DEV9init;
extern _DEV9open          DEV9open;
extern _DEV9close         DEV9close;
extern _DEV9shutdown      DEV9shutdown;
extern _DEV9read8         DEV9read8;
extern _DEV9read16        DEV9read16;
extern _DEV9read32        DEV9read32;
extern _DEV9write8        DEV9write8;
extern _DEV9write16       DEV9write16;
extern _DEV9write32       DEV9write32;
extern _DEV9readDMA8Mem   DEV9readDMA8Mem;
extern _DEV9writeDMA8Mem  DEV9writeDMA8Mem;
extern _DEV9irqCallback   DEV9irqCallback;
extern _DEV9irqHandler    DEV9irqHandler;

extern _DEV9keyEvent        DEV9keyEvent;
extern _DEV9configure     DEV9configure;
extern _DEV9freeze        DEV9freeze;
extern _DEV9test          DEV9test;
extern _DEV9about         DEV9about;

// USB
extern _USBinit           USBinit;
extern _USBopen           USBopen;
extern _USBclose          USBclose;
extern _USBshutdown       USBshutdown;
extern _USBread8          USBread8;
extern _USBread16         USBread16;
extern _USBread32         USBread32;
extern _USBwrite8         USBwrite8;
extern _USBwrite16        USBwrite16;
extern _USBwrite32        USBwrite32;
extern _USBasync          USBasync;

extern _USBirqCallback    USBirqCallback;
extern _USBirqHandler     USBirqHandler;
extern _USBsetRAM         USBsetRAM;

extern _USBkeyEvent       USBkeyEvent;
extern _USBconfigure      USBconfigure;
extern _USBfreeze         USBfreeze;
extern _USBtest           USBtest;
extern _USBabout          USBabout;

// FW
extern _FWinit            FWinit;
extern _FWopen            FWopen;
extern _FWclose           FWclose;
extern _FWshutdown        FWshutdown;
extern _FWread32          FWread32;
extern _FWwrite32         FWwrite32;
extern _FWirqCallback     FWirqCallback;

extern _FWkeyEvent        FWkeyEvent;
extern _FWconfigure       FWconfigure;
extern _FWfreeze          FWfreeze;
extern _FWtest            FWtest;
extern _FWabout           FWabout;

#ifndef __cplusplus
}
#endif

#endif // __PLUGINCALLBACKS_H__
