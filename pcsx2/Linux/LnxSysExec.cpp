/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
#include "Linux.h"
#include "LnxSysExec.h"

bool UseGui = true;

SafeArray<u8>* g_RecoveryState = NULL;
SafeArray<u8>* g_gsRecoveryState = NULL;

bool g_ReturnToGui = false;			// set to exit the execution of the emulator and return control to the GUI
bool g_EmulationInProgress = false;	// Set TRUE if a game is actively running (set to false on reset)

static bool sinit = false;
GtkWidget *FileSel;

void SysPrintf(const char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsnprintf(msg, 511, fmt, list);
	msg[511] = '\0';
	va_end(list);

	Console::Write(msg);
}

static void KeyEvent(keyEvent* ev);

void SysUpdate()
{
	KeyEvent(PAD1keyEvent());
	KeyEvent(PAD2keyEvent());
}

static void TryRecoverFromGsState()
{
	if( g_gsRecoveryState != NULL )
	{
		s32 dummylen;

		memLoadingState eddie( *g_gsRecoveryState );
		eddie.FreezePlugin( "GS", gsSafeFreeze );
		eddie.Freeze( dummylen );		// reads the length value recorded earlier.
		eddie.gsFreeze();
	}
}

void ExecuteCpu()
{
	// Make sure any left-over recovery states are cleaned up.
	safe_delete( g_RecoveryState );

	// Just in case they weren't initialized earlier (no harm in calling this multiple times)
	if (OpenPlugins(NULL) == -1) return;

	// this needs to be called for every new game!
	// (note: sometimes launching games through bios will give a crc of 0)

	if( GSsetGameCRC != NULL ) GSsetGameCRC(ElfCRC, g_ZeroGSOptions);

	TryRecoverFromGsState();

	safe_delete( g_gsRecoveryState );

	// Destroy the window.  Ugly thing.
	gtk_widget_destroy(MainWindow);
	gtk_main_quit();
	
	while (gtk_events_pending()) gtk_main_iteration();

	g_EmulationInProgress = true;
	g_ReturnToGui = false;

	signal(SIGINT, SignalExit);
	signal(SIGPIPE, SignalExit);

	// Optimization: We hardcode two versions of the EE here -- one for recs and one for ints.
	// This is because recs are performance critical, and being able to inline them into the
	// function here helps a small bit (not much but every small bit counts!).

	if (CHECK_EEREC)
	{
		while (!g_ReturnToGui)
		{
			recExecute();
			SysUpdate();
		}
	}
	else
	{
		while (!g_ReturnToGui)
		{
			Cpu->Execute();
			SysUpdate();
		}
	}
}

void RunGui()
{
	StartGui();
}

// Runs an ELF image directly (ISO or ELF program or BIN)
// Used by Run::FromCD and such
void RunExecute(const char* elf_file, bool use_bios)
{

	// (air notes:)
	// If you want to use the new to-memory savestate feature, take a look at the new
	// RunExecute in WinMain.c, and secondly the CpuDlg.c or AdvancedDlg.cpp.  The
	// objects used are SafeArray, memLoadingState, and memSavingState.

	// It's important to make sure to reset the CPU and the plugins correctly, which is
	// where the new RunExecute comes into play.  It can be kind of tricky knowing
	// when to call cpuExecuteBios and loadElfFile, and with what parameters.

	// (or, as an alternative maybe we should switch to wxWidgets and have a unified
	// cross platform gui?)  - Air
	
	// Sounds like a good idea, at this point.

	try
	{
		cpuReset();
	}

	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert( ex.cMessage() );
		return;
	}

	if (OpenPlugins(NULL) == -1)
	{
		RunGui();
		return;
	}

	if (elf_file == NULL)
	{
		if (g_RecoveryState != NULL)
		{
			try
			{
				memLoadingState(*g_RecoveryState).FreezeAll();
			}
			catch (std::runtime_error& ex)
			{
				Msgbox::Alert(
				    "Gamestate recovery failed.  Your game progress will be lost (sorry!)\n"
				    "\nError: %s\n", params ex.what());

				// Take the user back to the GUI...
				safe_delete(g_RecoveryState);
				ClosePlugins();
				return;
			}
			safe_delete(g_RecoveryState);
		}
		else if( g_gsRecoveryState == NULL )
		{
			// Not recovering a state, so need to execute the bios and load the ELF information.

			// if the elf_file is null we use the CDVD elf file.
			// But if the elf_file is an empty string then we boot the bios instead.

			char ename[g_MaxPath];
			ename[0] = 0;
			
			if (!use_bios) GetPS2ElfName(ename);
			loadElfFile(ename);
		}
	}
	else
	{
		// Custom ELF specified (not using CDVD).
		// Run the BIOS and load the ELF.

		loadElfFile(elf_file);
	}
	ExecuteCpu();
}

class RecoveryMemSavingState : public memSavingState, Sealed
{
public:
	virtual ~RecoveryMemSavingState() { }
	RecoveryMemSavingState() : memSavingState( *g_RecoveryState )
	{
	}

	void gsFreeze()
	{
		if (g_gsRecoveryState != NULL)
		{
			// just copy the data from src to dst.
			// the normal savestate doesn't expect a length prefix for internal structures,
			// so don't copy that part.
			const u32 pluginlen = *((u32*)g_gsRecoveryState->GetPtr());
			const u32 gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
			memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(pluginlen+8), gslen );
			m_idx += gslen;
		}
		else
			memSavingState::gsFreeze();
	}

	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
	{
		if ((freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL))
		{
			// Gs data is already in memory, so just copy from src to dest:
			// length of the GS data is stored as the first u32, so use that to run the copy:
			const u32 len = *((u32*)g_gsRecoveryState->GetPtr());
			memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(), len+4 );
			m_idx += len+4;
		}
		else
			memSavingState::FreezePlugin( name, freezer );
	}
};

class RecoveryZipSavingState : public gzSavingState, Sealed
{
public:
	virtual ~RecoveryZipSavingState() { }
	RecoveryZipSavingState( const string& filename ) : gzSavingState( filename )
	{
	}

	void gsFreeze()
	{
		if (g_gsRecoveryState != NULL)
		{
			// read data from the gsRecoveryState allocation instead of the GS, since the gs
			// info was invalidated when the plugin was shut down.

			// the normal savestate doesn't expect a length prefix for internal structures,
			// so don't copy that part.

			u32& pluginlen = *((u32*)g_gsRecoveryState->GetPtr(0));
			u32& gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
			gzwrite( m_file, g_gsRecoveryState->GetPtr(pluginlen+4), gslen );
		}
		else
			gzSavingState::gsFreeze();
	}

	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
	{
		if ((freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL))
		{
			// Gs data is already in memory, so just copy from there into the gzip file.
			// length of the GS data is stored as the first u32, so use that to run the copy:
			u32& len = *((u32*)g_gsRecoveryState->GetPtr());
			gzwrite( m_file, g_gsRecoveryState->GetPtr(), len+4 );
		}
		else
			gzSavingState::FreezePlugin( name, freezer );
	}
};

bool isSlotUsed(int num)
{
	if (ElfCRC == 0) 
		return false;
	else
		return Path::isFile(SaveState::GetFilename( num ));
}

void States_Load(const string& file, int num = -1)
{
	if( !Path::isFile( file ) )
	{
		Console::Notice( "Saveslot %d is empty.", params num );
		return;
	}
	try
	{
		char Text[g_MaxPath];
		gzLoadingState joe( file );		// this'll throw an StateLoadError_Recoverable.

		// Make sure the cpu and plugins are ready to be state-ified!
		cpuReset();
		OpenPlugins( NULL );

		joe.FreezeAll();

		if( num != -1 )
			sprintf (Text, _("*PCSX2*: Loaded State %d"), num);
		else
			sprintf (Text, _("*PCSX2*: Loaded State %s"), file.c_str());

		//StatusBar_Notice( Text );
		Console::Notice(Text);

		if( GSsetGameCRC != NULL ) GSsetGameCRC(ElfCRC, g_ZeroGSOptions);
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		if( num != -1 )
			Console::Notice( "Could not load savestate from slot %d.\n\n%s", params num, ex.cMessage() );
		else
			Console::Notice( "Could not load savestate file: %s.\n\n%s", params file.c_str(), ex.cMessage() );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.

		string message;

		if( num != -1 )
			ssprintf( message,
				"Encountered an error while loading savestate from slot %d.\n", num );
		else
			ssprintf( message,
				"Encountered an error while loading savestate from file: %s.\n", file.c_str() );

		if( g_EmulationInProgress )
			message += "Since the savestate load was incomplete, the emulator has been reset.\n";

		message += "\nError: " + ex.Message();

		Msgbox::Alert( message.c_str() );
		SysClose();
		return;
	}

	// Start emulating!
	ExecuteCpu();
}

void States_Load(int num)
{
	States_Load( SaveState::GetFilename( num ), num );
}

void States_Save( const string& file, int num = -1 )
{
	try
	{
		string text;
		RecoveryZipSavingState( file ).FreezeAll();
		if( num != -1 )
			ssprintf( text, _( "State saved to slot %d" ), num );
		else
			ssprintf( text, _( "State saved to file: %s" ), file.c_str() );

		Console::Notice(text.c_str());
	}
	catch( Exception::BaseException& ex )
	{
		string message;

		if( num != -1 )
			ssprintf( message, "An error occurred while trying to save to slot %d\n", num );
		else
			ssprintf( message, "An error occurred while trying to save to file: %s\n", file.c_str() );

		message += "Your emulation state has not been saved!\n\nError: " + ex.Message();

		Console::Error( message.c_str() );
	}
	CheckSlots();
}

void States_Save(int num)
{
	if( g_RecoveryState != NULL )
	{
		// State is already saved into memory, and the emulator (and in-progress flag)
		// have likely been cleared out.  So save from the Recovery buffer instead of
		// doing a "standard" save:

		string text;
		SaveState::GetFilename( text, num );
		gzFile fileptr = gzopen( text.c_str(), "wb" );
		if( fileptr == NULL )
		{
			Msgbox::Alert( _("File permissions error while trying to save to slot %d"), params num );
			return;
		}
		gzwrite( fileptr, &g_SaveVersion, sizeof( u32 ) );
		gzwrite( fileptr, g_RecoveryState->GetPtr(), g_RecoveryState->GetSizeInBytes() );
		gzclose( fileptr );
		return;
	}

	if( !g_EmulationInProgress )
	{
		Msgbox::Alert( "You need to start a game first before you can save it's state." );
		return;
	}

	States_Save( SaveState::GetFilename( num ), num );
}

class JustGsSavingState : public memSavingState, Sealed
{
public:
	virtual ~JustGsSavingState() { }
	JustGsSavingState() : memSavingState( *g_gsRecoveryState )
	{
	}

	// This special override saves the gs info to m_idx+4, and then goes back and
	// writes in the length of data saved.
	void gsFreeze()
	{
		int oldmidx = m_idx;
		m_idx += 4;
		memSavingState::gsFreeze();
		if( IsSaving() )
		{
			s32& len = *((s32*)m_memory.GetPtr( oldmidx ));
			len = (m_idx - oldmidx)-4;
		}
	}
};

void OnStates_Load(GtkMenuItem *menuitem, gpointer user_data)
{
	char *name;
	int i;
	
	if (GTK_BIN (menuitem)->child)
	{
		GtkWidget *child = GTK_BIN (menuitem)->child;
  
		if (GTK_IS_LABEL (child)) 
			gtk_label_get (GTK_LABEL (child), &name);
		else
			return;
	}
	
	sscanf(name, "Slot %d", &i);
	States_Load(i);
}

void OnLoadOther_Ok(GtkButton* button, gpointer user_data)
{
	gchar *File;
	char str[g_MaxPath];

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	States_Load(str);
}

void OnLoadOther_Cancel(GtkButton* button, gpointer user_data)
{
	gtk_widget_destroy(FileSel);
}

void OnStates_LoadOther(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *Ok, *Cancel;

	FileSel = gtk_file_selection_new(_("Select State File"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSel), SSTATES_DIR "/");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnLoadOther_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect(GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnLoadOther_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}

void OnStates_Save(GtkMenuItem *menuitem, gpointer user_data)
{
	char *name;
	int i;
	
	if (GTK_BIN (menuitem)->child)
	{
		GtkWidget *child = GTK_BIN (menuitem)->child;
  
		if (GTK_IS_LABEL (child)) 
			gtk_label_get (GTK_LABEL (child), &name);
		else
			return;
	}
	
	sscanf(name, "Slot %d", &i);
	States_Save(i);
}

void OnSaveOther_Ok(GtkButton* button, gpointer user_data)
{
	gchar *File;
	char str[g_MaxPath];

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	States_Save(str);
}

void OnSaveOther_Cancel(GtkButton* button, gpointer user_data)
{
	gtk_widget_destroy(FileSel);
}

void OnStates_SaveOther(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *Ok, *Cancel;

	FileSel = gtk_file_selection_new(_("Select State File"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSel), SSTATES_DIR "/");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnSaveOther_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect(GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnSaveOther_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}

/* Quick macros for checking shift, control, alt, and caps lock. */
#define SHIFT_EVT(evt) ((evt == XK_Shift_L) || (evt == XK_Shift_R))
#define CTRL_EVT(evt) ((evt == XK_Control_L) || (evt == XK_Control_L))
#define ALT_EVT(evt) ((evt == XK_Alt_L) || (evt == XK_Alt_R))
#define CAPS_LOCK_EVT(evt) (evt == XK_Caps_Lock)

void KeyEvent(keyEvent* ev)
{
	static int shift = 0;

	if (ev == NULL) return;

	if (GSkeyEvent != NULL) GSkeyEvent(ev);

	if (ev->evt == KEYPRESS)
	{
		if (SHIFT_EVT(ev->key))
			shift = 1;
		if (CAPS_LOCK_EVT(ev->key))
		{
			//Set up anything we want to happen while caps lock is down.
			//Config_hacks_backup = Config.Hacks;
		}

		switch (ev->key)
		{
			case XK_F1:
			case XK_F2:
			case XK_F3:
			case XK_F4:
			case XK_F5:
			case XK_F6:
			case XK_F7:
			case XK_F8:
			case XK_F9:
			case XK_F10:
			case XK_F11:
			case XK_F12:
				try
				{
					ProcessFKeys(ev->key - XK_F1 + 1, shift);
				}
				catch (Exception::CpuStateShutdown&)
				{
					// Woops!  Something was unrecoverable.  Bummer.
					// Let's give the user a RunGui!

					g_EmulationInProgress = false;
					g_ReturnToGui = true;
				}
				break;

			case XK_Escape:
				signal(SIGINT, SIG_DFL);
				signal(SIGPIPE, SIG_DFL);

#ifdef PCSX2_DEVBUILD
				if (g_SaveGSStream >= 3)
				{
					g_SaveGSStream = 4;// gs state
					break;
				}
#endif
				if( Config.closeGSonEsc )
				{
					safe_delete( g_gsRecoveryState );
					safe_delete( g_RecoveryState );
					g_gsRecoveryState = new SafeArray<u8>();
					JustGsSavingState eddie;
					eddie.FreezePlugin( "GS", gsSafeFreeze ) ;
					eddie.gsFreeze();
					PluginsResetGS();
				}

				ClosePlugins();
				
				if (!UseGui) exit(0);

				// fixme: The GUI is now capable of recieving control back from the
				// emulator.  Which means that when I set g_ReturnToGui here, the emulation
				// loop in ExecuteCpu() will exit.  You should be able to set it up so
				// that it returns control to the existing GTK event loop, instead of
				// always starting a new one via RunGui().  (but could take some trial and
				// error)  -- (air)
				
				// Easier said then done; running gtk in two threads at the same time can't be
				// done, and working around that is pretty fiddly.
				g_ReturnToGui = true;
				RunGui();
				break;

			default:
				GSkeyEvent(ev);
				break;
		}
	}
	else if (ev->evt == KEYRELEASE)
	{
		if (SHIFT_EVT(ev->key))
			shift = 0;
		if (CAPS_LOCK_EVT(ev->key))
		{
			//Release caps lock
			//Config_hacks_backup = Config.Hacks;
		}
	}

	return;
}


void SysRestorableReset()
{
	// already reset? and saved?
	if( !g_EmulationInProgress ) return;
	if( g_RecoveryState != NULL ) return;

	try
	{
		g_RecoveryState = new SafeArray<u8>( "Memory Savestate Recovery" );
		RecoveryMemSavingState().FreezeAll();
		safe_delete( g_gsRecoveryState );
		g_EmulationInProgress = false;
	}
	catch( Exception::RuntimeError& ex )
	{
		Msgbox::Alert(
			"Pcsx2 gamestate recovery failed. Some options may have been reverted to protect your game's state.\n"
			"Error: %s", params ex.cMessage() );
		safe_delete( g_RecoveryState );
	}
}

void SysReset()
{
	if (!sinit) return;

	g_EmulationInProgress = false;
	safe_delete( g_RecoveryState );
	safe_delete( g_gsRecoveryState );
	ResetPlugins();

	ElfCRC = 0;

	// Note : No need to call cpuReset() here.  It gets called automatically before the
	// emulator resumes execution.
}

bool SysInit()
{
	if (sinit) return true;
	sinit = true;

	mkdir(SSTATES_DIR, 0755);
	mkdir(MEMCARDS_DIR, 0755);

	mkdir(LOGS_DIR, 0755);

#ifdef PCSX2_DEVBUILD
	if (g_TestRun.plogname != NULL)
		emuLog = fopen(g_TestRun.plogname, "w");
	if (emuLog == NULL)
		emuLog = fopen(LOGS_DIR "/emuLog.txt", "wb");
#endif

	if (emuLog != NULL)
		setvbuf(emuLog, NULL, _IONBF, 0);

	PCSX2_MEM_PROTECT_BEGIN();
	SysDetect();
	if (!SysAllocateMem()) return false;	// critical memory allocation failure;

	SysAllocateDynarecs();
	PCSX2_MEM_PROTECT_END();

	while (LoadPlugins() == -1)
	{
		if (Pcsx2Configure() == FALSE)
		{
			Msgbox::Alert("Configuration failed. Exiting.");
			exit(1);
		}
	}

	return true;
}

void SysClose()
{
	if (sinit == 0) return;
	cpuShutdown();
	ClosePlugins();
	ReleasePlugins();

	if (emuLog != NULL)
	{
		fclose(emuLog);
		emuLog = NULL;
	}
	sinit = 0;
}

void *SysLoadLibrary(const char *lib)
{
	return dlopen(lib, RTLD_NOW);
}

void *SysLoadSym(void *lib, const char *sym)
{
	return dlsym(lib, sym);
}

const char *SysLibError()
{
	return dlerror();
}

void SysCloseLibrary(void *lib)
{
	dlclose(lib);
}

void SysRunGui()
{
	RunGui();
}

void *SysMmap(uptr base, u32 size)
{
	u8 *Mem;
	Mem = (u8*)mmap((uptr*)base, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (Mem == MAP_FAILED) Console::Notice("Mmap Failed!");

	return Mem;
}

void SysMunmap(uptr base, u32 size)
{
	munmap((uptr*)base, size);
}

