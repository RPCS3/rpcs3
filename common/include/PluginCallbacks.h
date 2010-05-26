/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
//  PS2E  -  Version 2.xx!
// --------------------------------------------------------------------------------------
// This header file defines the new PS2E interface, which is laid out to be a little more
// efficient and easy to use, and boasts significantly improved APIs over the original
// PS2E v1.xx, which was mostly a series of hacked additions on top of PS1E.  In summary:
// this API is designed from the ground up to suit PS2 emulation, instead of being built
// on top of a PS1 API.
//
// Design Philosophies:
//
//   1. Core APIs are established using a pair of DLL/library bindings (one for plugin callbacks
//      and one for emulator callbacks), which pass structures of function pointers.
//
//   2. Plugin instance data should be attached to the end of the plugin's callback api
//      data structure (see PS2E_ComponentAPI), and the PS2E_ComponentAPI struct is
//      passed along with every callback defined in the structure.
//
//   3. All plugin callbacks use __fastcall calling convention (which passes the first
//      two parameters int he ECX and EDX registers).  Most compilers support this, and
//      register parameter passing is actually the standard convention on x86/64.
//
// Rationale: This design improves code generation efficiency, especially when using
// points 2 and 3 together (typically reduces 2 or 3 dereferences to 1 dereference).
// The drawback is that not all compilers support x86/32 __fastcall, and such compilers
// will be unable to create PS2Ev2 plugins.  GCC, MSVC, Intel, and Borland (as
// __msfastcall) do support it, and Watcom as well using #pragma aux.  Anything else
// we just don't care about.  Sorry. ;)
//
//   4. Emulation is restricted to a single instance per-process, which means that use of
//      static/global instance variables by plugins is perfectly valid (however discouraged
//      due to efficiency reasons, see 2 and 3).
//
// Rationale: Due to complexities in implementing an optimized PS2 emulator (dynamic
// recompilation, memory protection, console pipe management, thread management, etc.)
// it's really just better this way.  The drawback is that every geeks' dream of having
// 6 different games emulating at once, with each output texture mapped to the side of
// a rotating cube, will probably never come true.  Tsk.
//

// --------------------------------------------------------------------------------------
//  <<< Important Notes to Plugin Authors >>>
// --------------------------------------------------------------------------------------
//  * C++ only: C++ Exceptions must be confined to your plugin!  Exceptions thrown by plugins
//    may not be handled correctly if allowed to escape the scope of the plugin, and could
//    result in unexpected behavior or odd crashes (this especially true on non-Windows
//    operating systems).  For C++ plugins this means ensuring that any code that uses
//    'new' or STL containers (string, list, vector, etc) are contained within a try{}
//    block, since the STL can throw std::bad_alloc.
//
//  * C++ on MSW only: SEH Exceptions must not be swallowed "blindly."  In simple terms this
//    means do not use these without proper re-throws, as the emulator may rely on SEH
//    for either PS2 VM cache validation or thread cancellation:
//      - catch(...)
//      - __except(EXCEPTION_EXECUTE_HANDLER)
//
//  * Many callbacks are optional, and have been marked as such. Any optional callback can be
//    left NULL.  Any callback not marked optional and left NULL will cause the emulator to
//    invalidate the plugin on either enumeration or initialization.
//

// --------------------------------------------------------------------------------------
//  <<< Important Notes to All Developers >>>
// --------------------------------------------------------------------------------------
//  * Callback APIs cannot involve LIB-C or STL objects (such as FILE or std::string).  The
//    internal layout of these structures can vary between versions of GLIB-C / MSVCRT, and
//    in the case of STL and other C++ objects, can vary based on seemingly mundane compiler
//    switches.
//
//  * Callback APIs cannot alloc/free memory across dynamic library boundaries.  An object
//    allocated by a plugin must be freed by that plugin, and cannot be freed by the emu.
//
//  * C++ exception handling cannot be used by either plugin callbacks or emulator callbacks.
//    This includes the emulator's Console callbacks, for example, since the nature of C++
//    ID-based RTTI could cause a C++ plugin with its own catch handlers to catch exceptions
//    of mismatched types from the emulator.
//
//  * Addendum to Exception Handling: On most OS's, pthreads relies on C++ exceptions to
//    cancel threads.  On Windows, this uses SEH so it works safely even across plugin stack
//    frames.  On all other non-MSW platforms pthreads cancelation *must* be disabled in
//    both emulator and plugin except for a single explicit cancelation point (usually
//    provided during vsync).
//
//  * If following all these rules, then it shouldn't matter if you mix debug/release builds
//    of plugins, SEH options (however it's recommended to ALWAYS have SEH enabled, as it
//    hardly has any impact on performance on modern CPUs), compile with different versions
//    of your MSVC/GCC compiler, or use different versions of LibC or MSVCRT. :)


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
// API will benefit considerably from it.  (Yes, this means that compilers that do not
// support fastcall are basically unusable for plugin creation.  Too bad. :p
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
	// somewhere after OSD_Icon_ReserveEnd.  All values below this are reserved.
	// .
	// .
	// .
	OSD_Icon_ReserveEnd = 0x1000
};

enum PS2E_MenuItemStyle
{
	MenuType_Normal = 0,
	MenuType_Checked,
	MenuType_Radio,
	MenuType_Separator
};

typedef void* PS2E_MenuHandle;
typedef void* PS2E_MenuItemHandle;
typedef void PS2E_CALLBACK PS2E_OnMenuItemClicked( PS2E_THISPTR* thisptr, void* userptr );

// --------------------------------------------------------------------------------------
//  PS2E_ConsoleWriterAPI
// --------------------------------------------------------------------------------------
// APIs for writing text to the console.  Typically the emulator will write the text to
// both a console window and to a disk file, however actual implementation is up to the
// emulator.  All text must be either 7-bit ASCII or UTF8 encoded.  Other codepages or
// MBCS encodings will not be displayed properly.
//
// Standard streams STDIO and STDERR can be used instead, however there is no guarantee
// that they will be handled in a convenient fashion.  Different platforms and operating
// systems have different methods of standard stream pipes, which is why the ConsoleWriter
// API has been exposed to plugins.
//
// Development Notes:
//   'char' was chosen over 'wchar_t' because 'wchar_t' is a very loosely defined type that
//   can vary greatly between compilers (it can be as small as 8 bits and as large as a
//   compiler wants it to be).  Because of this we can't even make assumptions about its
//   size within the context of a single operating system; so it just won't do.  Just make
//   sure everything going into the function is UTF8 encoded and all is find.
//
typedef struct _PS2E_ConsoleWriterAPI
{
	// Writes text to console; no newline is appended.
	void (PS2E_CALLBACK* Write)( const char* fmt, ... );

	// Appends an automatic newline to the specified formatted output.
	void (PS2E_CALLBACK* WriteLn)( const char* fmt, ... );

	// This function always appends a newline.
	void (PS2E_CALLBACK* Error)( const char* fmt, ... );

	// This function always appends a newline.
	void (PS2E_CALLBACK* Warning)( const char* fmt, ... );

	void* reserved[4];

} PS2E_ConsoleWriterAPI;

// --------------------------------------------------------------------------------------
//  PS2E_ConsoleWriterWideAPI
// --------------------------------------------------------------------------------------
// This is the wide character version of the ConsoleWriter APi.  Please see the description
// of PS2E_ConsoleWriterAPI for details.
//
// Important Usage Note to Plugin Authors:
//   Before using the functions in this structure, you *must* confirm that the emulator's
//   size of wchar_t matches the size provided by your own compiler.  The size of wchar_t
//   is provided in the PS2E_EmulatorInfo struct.
//
typedef struct _PS2E_ConsoleWriterWideAPI
{
	// Writes text to console; no newline is appended.
	void (PS2E_CALLBACK* Write)( const wchar_t* fmt, ... );

	// Appends an automatic newline to the specified formatted output.
	void (PS2E_CALLBACK* WriteLn)( const wchar_t* fmt, ... );

	// This function always appends a newline.
	void (PS2E_CALLBACK* Error)( const wchar_t* fmt, ... );

	// This function always appends a newline.
	void (PS2E_CALLBACK* Warning)( const wchar_t* fmt, ... );

	void* reserved[4];

} PS2E_ConsoleWriterWideAPI;


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
//  PS2E_MenuItemInfo
// --------------------------------------------------------------------------------------
typedef struct _PS2E_MenuItemInfo
{
	const char*			LabelText;
	const char*			HelpText;

	// Optional image displayed with the menu option.  The emulator may not support
	// this option, or may choose to ignore or resize the image if the size parameters
	// are outside a valid threshold.
	const PS2E_Image*	Image;

	// Specifies the style of the menu, either Normal, Checked, Radio, or Separator.
	// This option is overridden if the SubMenu field is non-NULL (in such case the
	// menu assumes submenu mode).
	PS2E_MenuItemStyle	Style;

	// Specifies the handle of a sub menu to bind to this menu.  If NULL, the menu is
	// created normally.  If non-NULL, the menu item will use sub-menu mode and will
	// ignore the Style field.
	PS2E_MenuHandle		SubMenu;

	// Menu that this item is attached to.  When this struct is passed into AddMenuItem,
	// the menu item will be automatically appended to the menu specified in this field
	// if the field is non-NULL (if the field is NULL, then no action is taken).
	PS2E_MenuHandle		OwnerMenu;

	// When FALSE the menu item will appear grayed out to the user, and unselectable.
	BOOL				Enabled;

	// Optional user data pointer (or typecast integer value)
	void*				UserPtr;

	// Callback issued when the menu is clicked/activated.  If NULL, the menu will be
	// disabled (grayed).
	PS2E_OnMenuItemClicked* OnClicked;

} PS2E_MenuItemInfo;

// --------------------------------------------------------------------------------------
//  PS2E_MenuItemAPI
// --------------------------------------------------------------------------------------
typedef struct _PS2E_MenuItemAPI
{
	// Allocates a new MenuItem and returns its handle.  The returned item can be added to any
	// menu.
	PS2E_MenuItemHandle (PS2E_CALLBACK* MenuItem_Create)( PS2E_THISPTR thisptr );

	// Deletes the menu item and frees allocated resources.  The menu item will be removed from
	// whatever menu it is attached to.  If the menu item has a SubMenu, the SubMenu is not
	// deleted.
	void (PS2E_CALLBACK* MenuItem_Delete)( PS2E_MenuItemHandle mitem );

	// (Re-)Assigns all properties for a menu.  Assignment generally takes effect immediately.
	void (PS2E_CALLBACK* MenuItem_SetEverything)( PS2E_MenuItemHandle mitem, const PS2E_MenuItemInfo* info );

	// Sets the text label of a menu item.
	void (PS2E_CALLBACK* MenuItem_SetText)( PS2E_MenuItemHandle mitem, const char* text );

	// Assigns the help text for a menu item.  This text is typically shown in a status
	// bar at the bottom of the current window.  This value may be ignored if the emu
	// interface does not have a context for help text.
	void (PS2E_CALLBACK* MenuItem_SetHelpText)( PS2E_MenuItemHandle mitem, const char* helptxt );

	// Gives the menu item an accompanying image (orientation of the image may depend
	// on the operating system platform).
	void (PS2E_CALLBACK* MenuItem_SetImage)( PS2E_MenuItemHandle mitem, const PS2E_Image* image );

	// Gives the menu item an accompanying image (orientation of the image may depend
	// on the operating system platform).
	//
	// Returns:
	//   TRUE if the image was loaded successfully, or FALSE if the image was not found,
	//   could not be opened, or the image data is invalid (not a PNG, or data corrupted).
	BOOL (PS2E_CALLBACK* MenuItem_SetImagePng_FromFile)( PS2E_MenuItemHandle mitem, const char* filename );

	// Gives the menu item an accompanying image (orientation of the image may depend on
	// the operating system platform).  Image is loaded from memory using a memory stream
	// reader.  This method is useful for loading image data embedded into the dll.
	//
	// Returns:
	//   TRUE if the image was loaded successfully, or FALSE if the image data is invalid
	//   (not a PNG, or data corrupted).
	BOOL (PS2E_CALLBACK* MenuItem_SetImagePng_FromMemory)( PS2E_MenuItemHandle mitem, const u8* data );

	// Assigns the menu item's style.
	void (PS2E_CALLBACK* MenuItem_SetStyle)( PS2E_MenuItemHandle mitem, PS2E_MenuItemStyle style );

	// Assigns a pointer value that the plugin can use to attach user-defined data to
	// specific menu items.  The value can be any integer typecast if you don't actually
	// need more than an integers worth of data.
	void (PS2E_CALLBACK* MenuItem_SetUserData)( PS2E_MenuItemHandle mitem, void* dataptr );

	// Assigns a submenu to the menu item, causing it to open the sub menu in cascade
	// fashion.  When a submenu is assigned, the Style attribute of the menu will be
	// ignored.  Passing NULL into this function will clear the submenu and return the
	// menu item to whatever its current Style attribute is set to.
	void (PS2E_CALLBACK* MenuItem_SetSubMenu)( PS2E_MenuItemHandle mitem, PS2E_MenuHandle submenu );

	// Assigns the callback function for this menu (important!).  If passed NULL, the menu
	// item will be automatically disabled (grayed out) by the emulator.
	void (PS2E_CALLBACK* MenuItem_SetCallback)( PS2E_MenuItemHandle mitem, PS2E_OnMenuItemClicked* onClickedCallback );

	// Assigns the enabled status of a MenuItem.  Use FALSE to gray out the menuitem option.
	void (PS2E_CALLBACK* MenuItem_Enable)( PS2E_MenuItemHandle mitem, BOOL enable );

	// Returns the current enable status of the specified menu item.
	BOOL (PS2E_CALLBACK* MenuItem_IsEnabled)( PS2E_MenuItemHandle mitem );

	void* reserved[4];

} PS2E_MenuItemAPI;

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
// each plugin via a call to PS2E_PluginLibAPI::EmuOpen().
//
typedef struct _PS2E_SessionInfo
{
	PS2E_HWND window;

	u32* CycleEE;		// current EE cycle count
	u32* CycleIOP;		// current IOP cycle count

	u32  ElfCRC;		// CRC of the ELF header for this app/game

	// Sony's assigned serial number, valid only for CD/CDVD games (ASCII-Z string).
	//   Ex: SLUS-2932
	//
	// (if the running app is not a sony-registered game, the serial will be a zero
	//  length string).
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
	// Brief name of the emulator (ex: "PCSX2") [required]
	// Depending on the design of the emulator, this string may optionally include version
	// information, however that is not recommended since it can inhibit backward support.
	const char*			EmuName;

	// Version information.  All fields besides the emulator's name are optional.
	PS2E_VersionInfo	EmuVersion;

	// Number of Physical Cores, as detected by the emulator.
	// This should always match the real # of cores supported by hardware.
	int		PhysicalCores;

	// Number of Logical Cores, as detected and/or managed by the emulator.
	// This is not necessarily a reflection of real hardware capabilities.  The emu reserves
	// the right to report this value as it deems appropriate, in management of threading
	// resources.
	int		LogicalCores;

	// Specifies the size of the wchar_t of the emulator, in bytes.  Plugin authors should be
	// sure to check this against your own sizeof(wchar_t) before using any API that has
	// a wchar_t parameter (such as the ConsoleWriterWide interface).  wchar_t is a loosely
	// defined type that can range from 8 bits to 32 bits (realistically, although there is
	// no actual limit on size), and can vary between compilers for the same platform.
	int		Sizeof_wchar_t;


	// Reserved area for future expansion of the structure (avoids having to upgrade the
	// plugin api for amending new extensions).
	int		reserved1[6];

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
	// shooting and debug information is best dumped to console or to disk log, or displayed
	// using a native popup window managed by the plugin.
	//
	// Parameters:
	//  icon  - an icon identifier, typically from the PS2E_OSDIconTypes enumeration.  Specific
	//     versions of emulators may provide their own icon extensions.  The emulator will
	//     silently ignore unrecognized icon identifiers, thus retaining cross-compat.
	//
	//  msg   - string message displayed to the user.
	//
	void (PS2E_CALLBACK* OSD_WriteLn)( int icon, const char* msg );

	// ----------------------------------------------------------------------------
	//  Menu / MenuItem Section
	// ----------------------------------------------------------------------------

	void (PS2E_CALLBACK* AddMenuItem)( const PS2E_MenuItemInfo* item );

	// Allocates a new menu handle and returns it.  The returned menu can have any valid existing
	// menu items bound to it, and can be assigned as a submenu to any created MenuItem.  The menu
	// can belong to multiple menu items, however menu items can only belong to a single menu.
	PS2E_MenuHandle (PS2E_CALLBACK* Menu_Create)( PS2E_THISPTR thisptr );

	// Deletes the specified menu and frees its allocated memory resources.  NULL pointers are
	// safely ignored.  Any menu items also attached to this menu will be deleted.  Even if you
	// do not explicitly delete your plugin's menu resources, the emulator will do automatic
	// cleanup after the plugin's instance is freed.
	void (PS2E_CALLBACK* Menu_Delete)( PS2E_MenuHandle handle );

	// Adds the specified menu item to this menu. Menu items can only belong to one menu at a
	// time. If you assign an item to a created menu that already belongs to another menu, it
	// will be removed from the other menu and moved to this one.  To append a menu item to
	// multiple menus, you will need to create multiple instances of the item.
	void (PS2E_CALLBACK* Menu_AddItem)( PS2E_MenuHandle menu, PS2E_MenuItemHandle mitem );

	// Interface for creating menu items and modifying their properties.
	PS2E_MenuItemAPI MenuItem;

	// Provides a set of basic console functions for writing text to the emulator's
	// console.  Some emulators may not support a console, in which case these functions
	// will be NOPs.   For plain and simple to-disk logging, plugins should create and use
	// their own logging facilities.
	PS2E_ConsoleWriterAPI Console;

	// Optional wide-version of the Console API.  Use with caution -- wchar_t is platform and
	// compiler dependent, and so plugin authors should be sure to check the emulator's wchar_t
	// side before using this interface.  See PS2E_ConsoleWriterWideAPI comments for more info.
	PS2E_ConsoleWriterWideAPI ConsoleW;

	void* reserved2[8];

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
	// EmuOpen
	// This function is called by the emulator when an emulation session is started.  The
	// plugin should take this opportunity to bind itself to the given window handle, open
	// necessary audio/video/input devices, etc.
	//
	// Parameters:
	//   session - provides relevant emulation session information.  Provided pointer is
	//      valid until after the subsequent call to EmuClose()
	//
	// Threading: EmuOpen is called from the GUI thread. All other emulation threads are
	//   guaranteed to be suspended or closed at the time of this call (no locks required).
	//
	void (PS2E_CALLBACK* EmuOpen)( PS2E_THISPTR thisptr, const PS2E_SessionInfo *session );

	// EmuClose
	// This function is called by the emulator prior to stopping emulation.  The window
	// handle specified in EmuOpen is guaranteed to be valid at the time EmuClose is called,
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
	// and Thaw while a plugin is in an EmuOpen() state.
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
	// plugin will only receive calls to Freeze and Thaw while a plugin is in an EmuOpen()
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
	// function being called.  Configure is only called outside the context of EmuOpen()
	// (after a call to EmuClose()).
	//
	// Plugin authors should ensure to re-read and re-apply all settings on EmuOpen(),
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
	// defined functions here will recognize the functions as not supported by the plugin.
	void* reserved[12];

} PS2E_LibraryAPI;

// --------------------------------------------------------------------------------------
//  PS2E_ComponentAPI_GS
// --------------------------------------------------------------------------------------
// Thread Safety:
//   Most GS callbacks are issued from the GS thread only, and are always called synchronously
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
	//   This function may be called from either GUI thread or GS thread.  Emulators calling
	//   it from non-GS threads must ensure mutex locking with TakeSnapshot (meaning the
	//   plugin should be free to disregard threading concerns).
	void (PS2E_CALLBACK* GsSetSnapshotsFolder)( PS2E_THISPTR thisptr, const char* folder );

	// TakeSnapshot
	// The GS plugin is to save the current frame into the given target image.  This
	// function is always called immediately after a GSvsync(), ensuring that the current
	// framebuffer is safely intact for capture.
	//
	// Returns TRUE if the snapshot succeeded, or FALSE if it failed (contents of dest
	// are considered indeterminate and will be ignored by the emu).
	BOOL (PS2E_CALLBACK* GsTakeSnapshot)( PS2E_THISPTR thisptr, PS2E_Image* dest );

	// OSD_QueueMessage
	// Queues a message to the GS for display to the user.  The GS can print the message
	// where-ever it pleases, though it's suggested that the messages be printed either
	// near the top or the bottom of the window (and in the black/empty area if the
	// game's display is letterboxed).
	//
	// Parameters:
	//  message      - text to queue (UTF8 format); will always be a single line (emulator
	//    is responsible for pre-processing linebreaks into multiple messages).  The pointer
	//    will become invalid after this call retunrs, so be sure to make a local copy of the
	//    text.
	//
	//  timeout		 - Suggested timeout period, in milliseconds.  This is a hint and need
	//    not be strictly adhered to by the GS.
	//
	void (PS2E_CALLBACK* OSD_QueueMessage)( PS2E_THISPTR thisptr, const char* msg, int timeout );

	// OSD_IconStatus
	// Sets the visibility status of an icon.  Icon placement can be determined by the GS,
	// although it's recommended that the icon be displayed near a corner of the screen, and
	// be displayed in the empty/black areas if present (letterboxing).
	//
	// Parameters:
	//   iconId     - Icon status to change
	//   alpha      - 0.0 is hdden, 1.0 is visible.  Other alpha values may be used as either
	//     transparency or as a scrolling factor (ie, to scroll the icon in and out of view, in
	//     any way the GS plugin sees fit).
	void (PS2E_CALLBACK* OSD_IconStatus)( PS2E_THISPTR thisptr, OSDIconTypes iconId, float alpha );

	// GSvsync
	//
	// Returns FALSE if the plugin encountered a critical error while updating the display;
	// indicating a device or emulation failure that should terminate the current emulation.
	// (if any critical errors accumulated during GStransferTags or GStransferImage, they
	//  should also be handled here by returning FALSE)
	//
	BOOL (PS2E_CALLBACK* GsVsync)(int field);

	// GSwriteRegs
	// Sends a GIFtag and associated register data.  This is the main transfer method for all
	// GIF register data.  REGLIST mode is unpacked into the forat described below.
	//
	// Note that SIGNAL, FINISH, and LABEL tags are handled internally by the emulator in a
	// thread-safe manner -- the GS plugin should ignore those tags when processing.
	//
	// Returns FALSE if the plugin encountered a critical error while setting texture;
	// indicating a device failure.
	//
	// Parameters:
	//   pMem    - pointer to source memory for the register descriptors and register data.
	//      The first 128 bits (1 qwc) is the descriptors unrolled into 16x8 format.  The
	//      following data is (regcnt x tagcnt) QWCs in length.
	//
	//   regcnt  - number of registers per loop packet (register descriptors are filled
	//      low->high).  Valid range is 1->16, and will never be zero.
	//
	//   nloop   - number of loops of register data.  Valid range is 1->32767 (upper 17
	//      bits are always zero).  This value will never be zero.
	void (PS2E_CALLBACK* GsWriteRegs)(const u128 *pMem, int regcnt, int nloop);

	// GSwritePrim
	// Starts a new prim by sending the specified value to the PRIM register.  The emulator
	// only posts this data to the GS s per the rules of GIFpath processing (note however
	// that packed register data can also contain PRIM writes).
	//
	// Parameters:
	//   primData    - value to write to the PRIM register.  Only the bottom 10 bits are
	//      valid.  Upper bits are always zero.
	void (PS2E_CALLBACK* GsWritePrim)(int primData);

	// GSwriteImage
	// Uploads new image data.  Data uploaded may be in any number of partial chunks, for
	// which the GS is responsible for managing the state machine for writes to GS memory.
	//
	// Plugin authors: Note that it is valid for games to only modify a small portion of a
	// larger texture buffer, or for games to modify several portions of a single large
	// buffer, by using mid-transfer writes to TRXPOS and TRXDIR (TRXPOS writes only become
	// effective once TRXDIR has been written).
	void (PS2E_CALLBACK* GsWriteImage)(const u128 *pMem, int qwc_cnt);

	// GSreadImage
	// This special callback is for implementing the Read mode direction of the GIFpath.
	// The GS plugin writes the texture data as requested by it's internally managed state
	// values for TRXPOS/TRXREG to the buffer provided by pMem.  The buffer size is qwc_cnt
	// and the GS must not write more than that.
	void (PS2E_CALLBACK* GsReadImage)(u128 *pMem, int qwc_cnt);

	void* reserved[8];

} PS2E_ComponentAPI_GS;


// --------------------------------------------------------------------------------------
//  PS2E_McdSizeInfo
// --------------------------------------------------------------------------------------
struct PS2E_McdSizeInfo
{
	u16	SectorSize;					// Size of each sector, in bytes.  (only 512 and 1024 are valid)
	u16 EraseBlockSizeInSectors;	// Size of the erase block, in sectors (max is 16)
	u32	McdSizeInSectors;			// Total size of the card, in sectors (no upper limit)
};

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
	//   False if the card is not available, or True if it is available.
	//
	BOOL (PS2E_CALLBACK* McdIsPresent)( PS2E_THISPTR thisptr, uint port, uint slot );

	// McdGetSectorSize  (can be NULL)
	// Requests memorycard formatting information from the Mcd provider.  See the description of
	// PS2E_McdSizeInfo for details on each field.  If the Mcd provider supports only standard 8MB
	// carts, then this function can be NULL.
	// 
	// Returns:
	//   Assigned values for memorycard sector size and sector count in 'outways.' 
	//
	void (PS2E_CALLBACK* McdGetSizeInfo)( PS2E_THISPTR thisptr, uint port, uint slot, PS2E_McdSizeInfo* outways );

	// McdRead
	// Requests that a block of data be loaded from the memorycard into the specified dest
	// buffer (which is allocated by the caller).  Bytes read should match the requested
	// size.  Reads *must* be performed synchronously (function cannot return until the
	// read op has finished).
	//
	// Returns:
	//   False on failure, and True on success.  Emulator may use GetLastError to retrieve additional
	//   information for logging or displaying to the user.
	//
	BOOL (PS2E_CALLBACK* McdRead)( PS2E_THISPTR thisptr, uint port, uint slot, u8 *dest, u32 adr, int size );

	// McdSave
	// Saves the provided block of data to the memorycard at the specified seek address.
	// Writes *must* be performed synchronously (function cannot return until the write op
	// has finished).  Write cache flushing is optional.
	//
	// Returns:
	//   False on failure, and True on success.  Emulator may use GetLastError to retrieve additional
	//   information for logging or displaying to the user.
	//
	BOOL (PS2E_CALLBACK* McdSave)( PS2E_THISPTR thisptr, uint port, uint slot, const u8 *src, u32 adr, int size );

	// McdEraseBlock
	// Saves "cleared" data to the memorycard at the specified seek address.  Cleared data
	// is a series of 0xff values (all bits set to 1).
	// Writes *must* be performed synchronously (function cannot return until the write op
	// has finished).  Write cache flushing is optional.
	//
	// Returns:
	//   False on failure, and True on success.  Emulator may use GetLastError to retrieve additional
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

enum PS2E_KeyModifiers
{
	PS2E_SHIFT = 1,
	PS2E_CONTROL = 2,
	PS2E_ALT = 4
};

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
//  * Thread affinity is not guaranteed.  Even PadKeyEvent may be called from a thread not
//    belonging to the active window (the window where the GA is output).  Other calls may
//    be made from either the main emu thread or an EE/IOP/GS child thread (if the emulator
//    uses them).
//
typedef struct _PS2E_ComponentAPI_Pad
{
	// Base Component API (inherited structure)
	struct _PS2E_ComponentAPI Base;

	// PadIsPresent
	// Called by the emulator to detect the availability of a pad.  This function will
	// be called frequently -- essentially whenever the SIO port for the pad has its
	// status polled - so its overhead should be minimal when possible.
	//
	// A plugin should behave reasonably when a pad that's not plugged in is polled.
	//
	// Returns:
	//   False if the card/pad is not available, or True if it is available.
	//
	BOOL (PS2E_CALLBACK* PadIsPresent)( PS2E_THISPTR thisptr, uint port, uint slot );

	// PadStartPoll
	// Called by the emulator to start polling the specified pad.
	//
	// Returns:
	//   First byte in response to the poll (Typically 0xff).
	//
	// Threading:
	//   Called from the EEcore thread.  The emulator performs no locking of its own, so
	//   calls to this may occur concurrently with calls to PadUpdate.
	//
	u8 (PS2E_CALLBACK* PadStartPoll)( PS2E_THISPTR thisptr, uint port, uint slot );

	// PadPoll
	// Continues polling the specified pad, sending the given value.
	//
	// Returns:
	//   Next byte in response to the poll.
	//
	// Threading:
	//   Called from the EEcore thread.  The emulator performs no locking of its own, so
	//   calls to this may occur concurrently with calls to PadUpdate.
	//
	u8 (PS2E_CALLBACK* PadPoll)( PS2E_THISPTR thisptr, u8 value );

	// PadKeyEvent
	// Called by the emulator in the gui thread to check for keys being pressed or released.
	//
	// Returns:
	//   PS2E_KeyEvent:  Key being pressed or released.  Should stay valid until next call to
	//                   PadKeyEvent or plugin is closed with EmuClose.
	//
	// Threading:
	//   May be called from any thread.  The emulator performs no locking of its own, so
	//   calls to this may occur concurrently with calls to PadUpdate.
	//
	PS2E_KeyEvent* (PS2E_CALLBACK* PadGetKeyEvent)( PS2E_THISPTR thisptr );

	// PadUpdate
	// This callback is issued from the thread that owns the GSwindow, at roughly 50/60hz,
	// allowing the Pad plugin to use it for update logic that expects thread affinity with
	// the GSwindow.
	//
	// Threading:
	//   Called from the same thread that owns the GSwindow (typically either a GUI thread
	//   or an MTGS thread). The emulator performs no locking of its own, so calls to this
	//   may occur concurrently with calls to PadKeyEvent and PadPoll.
	//
	void (PS2E_CALLBACK* PadUpdate)( PS2E_THISPTR thisptr );

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
