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

#pragma once

#define PLUGINtypedefs
#define PLUGINfuncs

#include "PS2Edefs.h"
#include "PluginCallbacks.h"

#include "Utilities/Threading.h"

#include <wx/dynlib.h>

#ifdef _MSC_VER

// Disabling C4673: throwing 'Exception::Blah' the following types will not be considered at the catch site
//   The warning is bugged, and happens even though we're properly inheriting classes with
//   'virtual' qualifiers.  But since the warning is potentially useful elsewhere, I disable
//   it only for the scope of these exceptions.

#	pragma warning(push)
#	pragma warning(disable:4673)
#endif

struct PluginInfo
{
	const char* shortname;
	PluginsEnum_t id;
	int typemask;
	int version;			// minimum version required / supported

	wxString GetShortname() const
	{
		return fromUTF8( shortname );
	}
};

// --------------------------------------------------------------------------------------
//  Plugin-related Exceptions
// --------------------------------------------------------------------------------------
namespace Exception
{
	// Exception thrown when a corrupted or truncated savestate is encountered.
	class SaveStateLoadError : public virtual BadStream
	{
	public:
		DEFINE_STREAM_EXCEPTION( SaveStateLoadError, wxLt("Load failed: The savestate appears to be corrupt or incomplete.") )
	};

	class PluginError : public virtual RuntimeError
	{
	public:
		PluginsEnum_t PluginId;

	public:
		DEFINE_EXCEPTION_COPYTORS( PluginError )

		PluginError() {}
		PluginError( PluginsEnum_t pid, const char* msg="Generic plugin error" )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// Plugin load errors occur when initially trying to load plugins during the
	// creation of a PluginManager object.  The error may either be due to non-existence,
	// corruption, or incompatible versioning.
	class PluginLoadError : public virtual PluginError, public virtual BadStream
	{
	public:
		DEFINE_EXCEPTION_COPYTORS_COVARIANT( PluginLoadError )

		PluginLoadError( PluginsEnum_t pid, const wxString& objname, const char* eng );

		PluginLoadError( PluginsEnum_t pid, const wxString& objname,
			const wxString& eng_msg, const wxString& xlt_msg );

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// Thrown when a plugin fails it's init() callback.  The meaning of this error is entirely
	// dependent on the plugin and, in most cases probably never happens (most plugins do little
	// more than a couple basic memory reservations during init)
	class PluginInitError : public virtual PluginError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS_COVARIANT( PluginInitError )

		explicit PluginInitError( PluginsEnum_t pid,
			const char* msg=wxLt("%s plugin failed to initialize.  Your system may have insufficient memory or resources needed.") )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}
	};

	// Plugin failed to open.  Typically this is a non-critical error that means the plugin has
	// not been configured properly by the user, but may also be indicative of a system
	class PluginOpenError : public virtual PluginError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS_COVARIANT( PluginOpenError )

		explicit PluginOpenError( PluginsEnum_t pid,
			const char* msg=wxLt("%s plugin failed to open.  Your computer may have insufficient resources, or incompatible hardware/drivers.") )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}
	};
	
	// This exception is thrown when a plugin returns an error while trying to save itself.
	// Typically this should be a very rare occurance since a plugin typically shoudn't
	// be doing memory allocations or file access during state saving.
	//
	class FreezePluginFailure : public virtual PluginError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( FreezePluginFailure )

		explicit FreezePluginFailure( PluginsEnum_t pid)
		{
			PluginId = pid;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	class ThawPluginFailure : public virtual PluginError, public virtual SaveStateLoadError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( ThawPluginFailure )

		explicit ThawPluginFailure( PluginsEnum_t pid )
		{
			PluginId = pid;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};
};

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

// --------------------------------------------------------------------------------------
//  LegacyPluginAPI_Common
// --------------------------------------------------------------------------------------
// Important: Contents of this structure must match the order of the contents of the
// s_MethMessCommon[] array defined in Plugins.cpp.
//
// Note: Open is excluded from this list because the GS and CDVD have custom signatures >_<
//
struct LegacyPluginAPI_Common
{
	s32  (CALLBACK* Init)();
	void (CALLBACK* Close)();
	void (CALLBACK* Shutdown)();

	void (CALLBACK* KeyEvent)( keyEvent* evt );
	void (CALLBACK* SetSettingsDir)( const char* dir );
	s32  (CALLBACK* Freeze)(int mode, freezeData *data);
	s32  (CALLBACK* Test)();
	void (CALLBACK* Configure)();
	void (CALLBACK* About)();

	LegacyPluginAPI_Common()
	{
		memzero( *this );
	}
};

class SaveStateBase;
class SysMtgsThread;

// --------------------------------------------------------------------------------------
//  PluginBindings
// --------------------------------------------------------------------------------------
// This structure is intended to be the "future" of PCSX2's plugin interface, and will hopefully
// make the current PluginManager largely obsolete (with the exception of the general Load/Unload
// management facilities)
//
class SysPluginBindings
{
protected:
	PS2E_ComponentAPI_Mcd* Mcd;

public:
	SysPluginBindings()
	{
		Mcd = NULL;
	}

	bool McdIsPresent( uint port, uint slot );
	void McdRead( uint port, uint slot, u8 *dest, u32 adr, int size );
	void McdSave( uint port, uint slot, const u8 *src, u32 adr, int size );
	void McdEraseBlock( uint port, uint slot, u32 adr );
	u64 McdGetCRC( uint port, uint slot );

	friend class PluginManager;
};

extern SysPluginBindings SysPlugins;

// --------------------------------------------------------------------------------------
//  PluginManager Class
// --------------------------------------------------------------------------------------
//
class PluginManager
{
	DeclareNoncopyableObject( PluginManager );

protected:
	class PluginStatus_t
	{
	public:
		PluginsEnum_t pid;

		bool		IsInitialized;
		bool		IsOpened;

		wxString	Filename;
		wxString	Name;
		wxString	Version;

		LegacyPluginAPI_Common	CommonBindings;
		wxDynamicLibrary		Lib;

	public:
		PluginStatus_t()
		{
			IsInitialized	= false;
			IsOpened		= false;
		}

		PluginStatus_t( PluginsEnum_t _pid, const wxString& srcfile );
		virtual ~PluginStatus_t() throw() { }
		
	protected:
		void BindCommon( PluginsEnum_t pid );
		void BindRequired( PluginsEnum_t pid );
		void BindOptional( PluginsEnum_t pid );
	};

	const PS2E_LibraryAPI*		m_mcdPlugin;
	wxString					m_SettingsFolder;
	Threading::MutexRecursive	m_mtx_PluginStatus;

public:		// hack until we unsuck plugins...
	ScopedPtr<PluginStatus_t>	m_info[PluginId_Count];

public:
	PluginManager();
	virtual ~PluginManager() throw();

	void Load( PluginsEnum_t pid, const wxString& srcfile );
	void Load( const wxString (&folders)[PluginId_Count] );
	void Unload();
	void Unload( PluginsEnum_t pid );

	bool AreLoaded() const;
	bool AreAnyLoaded() const;
	bool AreAnyInitialized() const;
	
	Threading::Mutex& GetMutex() { return m_mtx_PluginStatus; }

	virtual void Init();
	virtual void Shutdown();
	virtual void Open();
	virtual void Open( PluginsEnum_t pid );
	virtual void Close( PluginsEnum_t pid );
	virtual void Close();

	virtual bool IsOpen( PluginsEnum_t pid ) const;
	virtual bool IsInitialized( PluginsEnum_t pid ) const;
	virtual bool IsLoaded( PluginsEnum_t pid ) const;
	
	virtual void Freeze( PluginsEnum_t pid, SaveStateBase& state );
	virtual bool DoFreeze( PluginsEnum_t pid, int mode, freezeData* data );

	virtual bool KeyEvent( const keyEvent& evt );
	virtual void Configure( PluginsEnum_t pid );
	virtual void SetSettingsFolder( const wxString& folder );
	virtual void SendSettingsFolder();

	const wxString GetName( PluginsEnum_t pid ) const;
	const wxString GetVersion( PluginsEnum_t pid ) const;

protected:
	virtual bool NeedsClose() const;
	virtual bool NeedsOpen() const;

	virtual bool NeedsShutdown() const;
	virtual bool NeedsInit() const;
	
	virtual bool NeedsLoad() const;
	virtual bool NeedsUnload() const;

	virtual bool OpenPlugin_GS();
	virtual bool OpenPlugin_CDVD();
	virtual bool OpenPlugin_PAD();
	virtual bool OpenPlugin_SPU2();
	virtual bool OpenPlugin_DEV9();
	virtual bool OpenPlugin_USB();
	virtual bool OpenPlugin_FW();
	
	void _generalclose( PluginsEnum_t pid );

	virtual void ClosePlugin_GS();
	virtual void ClosePlugin_CDVD();
	virtual void ClosePlugin_PAD();
	virtual void ClosePlugin_SPU2();
	virtual void ClosePlugin_DEV9();
	virtual void ClosePlugin_USB();
	virtual void ClosePlugin_FW();

	friend class SysMtgsThread;
};

extern const PluginInfo tbl_PluginInfo[];

// GetPluginManager() is a required external implementation. This function is *NOT*
// provided by the PCSX2 core library.  It provides an interface for the linking User
// Interface apps or DLLs to reference their own instance of PluginManager (also allowing
// them to extend the class and override virtual methods).

extern PluginManager& GetCorePlugins();

// Hack to expose internal MemoryCard plugin:

extern "C" const PS2E_LibraryAPI* FileMcd_InitAPI( const PS2E_EmulatorInfo* emuinfo );

// Per ChickenLiver, this is being used to pass the GS plugins window handle to the Pad plugins.
// So a rename to pDisplay is in the works, but it will not, in fact, be removed.
extern uptr pDsp;

