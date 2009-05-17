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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
#include "Linux.h"
#include "LnxSysExec.h"
#include "HostGui.h"

static bool sinit = false;
GtkWidget *FileSel;
static uptr current_offset = 0;
static uptr offset_counter = 0;
bool Slots[5] = { false, false, false, false, false };

void InstallLinuxExceptionHandler()
{
	struct sigaction sa;
	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &SysPageFaultExceptionFilter;
	sigaction(SIGSEGV, &sa, NULL); 
}

void ReleaseLinuxExceptionHandler()
{
	// Code this later.
}

static const uptr m_pagemask = getpagesize()-1;

// Linux implementation of SIGSEGV handler.  Bind it using sigaction().
void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * )
{
	// get bad virtual address
	uptr offset = (u8*)info->si_addr - psM;
	
	if (offset != current_offset)
	{
		current_offset = offset;
		offset_counter = 0;
	}
	else
	{
		offset_counter++;
		if (offset_counter > 500) 
		{
			DevCon::Status( "Offset 0x%x endlessly repeating. Aborting.", params offset );
			assert( false );
		}
	}

	if (offset>=Ps2MemSize::Base)
	{
		// Bad mojo!  Completely invalid address.
		// Instigate a crash or abort emulation or something.
		DevCon::Status( "Offset 0x%x invalid. Legit SIGSEGV. Aborting.", params offset );
		assert( false );
	}
	
	mmap_ClearCpuBlock( offset & ~m_pagemask );
}

bool ParseCommandLine(int argc, char *argv[], char *file)
{
	int i = 1;
	g_Startup.BootMode = BootMode_Bios;
	
	while (i < argc)
	{
		char* token = argv[i++];

		if (stricmp(token, "-help") == 0 || stricmp(token, "--help") == 0 || stricmp(token, "-h") == 0)
		{
			//Msgbox::Alert( phelpmsg );
			return false;
		}
		else if (stricmp(token, "-nogui") == 0)
		{
			g_Startup.NoGui = FALSE;
		}
		else if (stricmp(token, "-loadgs") == 0)
		{
			g_pRunGSState = argv[i++];
		}			
		else if( strcmp(token, "-bootmode" ) == 0) 
		{
			token = argv[i++];
			g_Startup.BootMode = (StartupMode)atoi( token);
			g_Startup.Enabled = true;
		}
		else if (stricmp(token, "-gs") == 0)
		{
			token = argv[i++];
			g_Startup.gsdll = token;
		}
		else if (stricmp(token, "-cdvd") == 0)
		{
			token = argv[i++];
			g_Startup.cdvddll = token;
		}
		else if (stricmp(token, "-spu") == 0)
		{
			token = argv[i++];
			g_Startup.spudll = token;
		}
		else if (stricmp(token, "-pad") == 0)
		{
			token = argv[i++];
			g_Startup.pad1dll = token;
			g_Startup.pad2dll = token;
		}
		else if (stricmp(token, "-pad1") == 0)
		{
			token = argv[i++];
			g_Startup.pad1dll = token;
		}
		else if (stricmp(token, "-pad2") == 0)
		{
			token = argv[i++];
			g_Startup.pad2dll = token;
		}
		else if (stricmp(token, "-loadgs") == 0)
		{
			token = argv[i++];
			g_pRunGSState = token;
		}
		else
		{
			file = token;
			printf("opening file %s\n", file);
		}
	}
	return true;
}
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

static std::string str_Default( "default" );

void RunGui()
{
	PCSX2_MEM_PROTECT_BEGIN();

	LoadPatch( str_Default );
	if( g_Startup.NoGui || g_Startup.Enabled )
	{
		// Initially bypass GUI and start PCSX2 directly.
		// Manually load plugins using the user's configured image (if non-elf).
		
		if( g_Startup.Enabled && (g_Startup.BootMode != BootMode_Elf) )
		{
			if (OpenPlugins(g_Startup.ImageName) == -1)
				return;
		}

		SysPrepareExecution(
			(g_Startup.BootMode == BootMode_Elf) ? g_Startup.ImageName : NULL, 
			(g_Startup.BootMode == BootMode_Bios)
		);
	}
	
	StartGui();
	
	PCSX2_MEM_PROTECT_END();
}

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
	//if (States_Load(i)) ExecuteCpu();
	States_Load(i);
	RefreshMenuSlots();
}

void OnLoadOther_Ok(GtkButton* button, gpointer user_data)
{
	gchar *File;
	char str[g_MaxPath];

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	//if (States_Load(str)) ExecuteCpu();
	States_Load(str);
	RefreshMenuSlots();
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

bool SysInit()
{
	if (sinit) return true;
	sinit = true;

	mkdir(SSTATES_DIR, 0755);
	mkdir(MEMCARDS_DIR, 0755);

	mkdir(LOGS_DIR, 0755);

#ifdef PCSX2_DEVBUILD
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
	if (sinit == false) return;
	cpuShutdown();
	ClosePlugins( true );
	ReleasePlugins();

	if (emuLog != NULL)
	{
		fclose(emuLog);
		emuLog = NULL;
	}
	sinit = false;
	
	// Precautionary extra shutdown stuff.
	SysEndExecution();
	g_EmulationInProgress = false;
}

namespace HostSys
{
	void *LoadLibrary(const char *lib)
	{
		return dlopen(lib, RTLD_NOW);
	}

	void *LoadSym(void *lib, const char *sym)
	{
		return dlsym(lib, sym);
	}

	const char *LibError()
	{
		return dlerror();
	}

	void CloseLibrary(void *lib)
	{
		dlclose(lib);
	}

	void *Mmap(uptr base, u32 size)
	{
		u8 *Mem;
		Mem = (u8*)mmap((uptr*)base, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		if (Mem == MAP_FAILED) Console::Notice("Mmap Failed!");

		return Mem;
	}

	void Munmap(uptr base, u32 size)
	{
		munmap((uptr*)base, size);
	}

	void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution )
	{
		// Breakpoint this to trap potentially inappropriate use of page protection, which would
		// be caused by failed aligned directives on global vars  (dunno if it's happening or not yet)
		/*Console::Error(
			"*PCSX2/Linux Warning* Inappropriate use of page protection detected.\n"
			"\tbaseaddr not page aligned: 0x%08X", params (uptr)baseaddr
		);*/

		int lnxmode = 0;

		// make sure base and size are aligned to the system page size:
		size = (size + m_pagemask) & ~m_pagemask;
		baseaddr = (void*)( ((uptr)baseaddr) & ~m_pagemask );

		switch( mode )
		{
			case Protect_NoAccess: break;
			case Protect_ReadOnly: lnxmode = PROT_READ; break;
			case Protect_ReadWrite: lnxmode = PROT_READ | PROT_WRITE; break;
		}

		if( allowExecution ) lnxmode |= PROT_EXEC;
		mprotect( baseaddr, size, lnxmode );
	}
}

namespace HostGui
{
	// Sets the status bar message without mirroring the output to the console.
	void SetStatusMsg( const string& text )
	{
		// don't try this in Visual C++ folks!
		gtk_statusbar_push(GTK_STATUSBAR(pStatusBar), 0, text.c_str());
	}

	void Notice( const string& text )
	{
		// mirror output to the console!
		Console::Status( text.c_str() );
		SetStatusMsg( text );
	}

	void ResetMenuSlots()
	{
		for (int i = 0; i < 5; i++)
		{
			Slots[i] = States_isSlotUsed(i);
		}
	}

	void BeginExecution()
	{
		// Destroy the window.  Ugly thing.
		gtk_widget_destroy(MainWindow);
		gtk_main_quit();

		while (gtk_events_pending()) gtk_main_iteration();

		signal(SIGINT, SignalExit);
		signal(SIGPIPE, SignalExit);

		// no try/catch needed since no cleanup needed.. ?
		SysExecute();
	}

/* Quick macros for checking shift, control, alt, and caps lock. */
#define SHIFT_EVT(evt) ((evt == XK_Shift_L) || (evt == XK_Shift_R))
#define CTRL_EVT(evt) ((evt == XK_Control_L) || (evt == XK_Control_R))
#define ALT_EVT(evt) ((evt == XK_Alt_L) || (evt == XK_Alt_R))
#define CAPS_LOCK_EVT(evt) (evt == XK_Caps_Lock)

        void __fastcall KeyEvent(keyEvent* ev)
        {
		struct KeyModifiers *keymod = &keymodifiers;

                if (ev == NULL) return;

                if (GSkeyEvent != NULL) GSkeyEvent(ev);

                if (ev->evt == KEYPRESS)
                {
			if (SHIFT_EVT(ev->key)) keymod->shift = TRUE;
			if (CTRL_EVT(ev->key)) keymod->control = TRUE;
			if (ALT_EVT(ev->key)) keymod->alt = TRUE;
			if (CAPS_LOCK_EVT(ev->key)) keymod->capslock = TRUE;

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
                                                ProcessFKeys(ev->key - XK_F1 + 1, keymod);
                                        }
                                        catch (Exception::CpuStateShutdown&)
                                        {
                                                // Woops!  Something was unrecoverable.  Bummer.
                                                // Let's give the user a RunGui!

                                                g_EmulationInProgress = false;
                                                SysEndExecution();
                                        }
                                        break;

                                case XK_Tab:
                                        CycleFrameLimit(0);
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
                                        SysEndExecution();

                                        if (g_Startup.NoGui) exit(0);

                                        // fixme: The GUI is now capable of receiving control back from the
                                        // emulator.  Which means that when we call SysEscapeExecute() here, the
                                        // emulation loop in ExecuteCpu() will exit.  You should be able to set it
                                        // up so that it returns control to the existing GTK event loop, instead of
                                        // always starting a new one via RunGui().  (but could take some trial and
                                        // error)  -- (air)
                                        
                                        // Easier said then done; running gtk in two threads at the same time can't be
                                        // done, and working around that is pretty fiddly.
                                        RunGui();
                                        break;

                                default:
                                        GSkeyEvent(ev);
                                        break;
                        }
                }
                else if (ev->evt == KEYRELEASE)
                {
			if (SHIFT_EVT(ev->key)) keymod->shift = FALSE;
			if (CTRL_EVT(ev->key)) keymod->control = FALSE;
			if (ALT_EVT(ev->key)) keymod->alt = FALSE;
			if (CAPS_LOCK_EVT(ev->key)) keymod->capslock = FALSE;
                }

                return;
        }

}
