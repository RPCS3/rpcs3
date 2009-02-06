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

#include "GtkGui.h"

using namespace R5900;

void On_Dialog_Cancelled(GtkButton* button, gpointer user_data) {
	gtk_widget_destroy((GtkWidget*)gtk_widget_get_toplevel ((GtkWidget*)button));
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void StartGui() {
	GtkWidget *Menu;
	GtkWidget *Item;
	
	u32 i;

	add_pixmap_directory(".pixmaps");
	MainWindow = create_MainWindow();
	
	if (SVN_REV != 0)
		gtk_window_set_title(GTK_WINDOW(MainWindow), "PCSX2 "PCSX2_VERSION" "SVN_REV" Playground");
	else
		gtk_window_set_title(GTK_WINDOW(MainWindow), "PCSX2 "PCSX2_VERSION" Playground");

	// status bar
	pStatusBar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX(lookup_widget(MainWindow, "status_box")), pStatusBar, TRUE, TRUE, 0);
	gtk_widget_show (pStatusBar);

	gtk_statusbar_push(GTK_STATUSBAR(pStatusBar),0,
                       "F1 - save, F2 - next state, Shift+F2 - prev state, F3 - load, F8 - snapshot");

	// add all the languages
	Item = lookup_widget(MainWindow, "GtkMenuItem_Language");
	Menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(Item), Menu);

	for (i=0; i < langsMax; i++) {
		Item = gtk_check_menu_item_new_with_label(ParseLang(langs[i].lang));
		gtk_widget_show(Item);
		gtk_container_add(GTK_CONTAINER(Menu), Item);
		gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(Item), TRUE);
		if (!strcmp(Config.Lang, langs[i].lang))
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(Item), TRUE);

		gtk_signal_connect(GTK_OBJECT(Item), "activate",
                           GTK_SIGNAL_FUNC(OnLanguage),
                           (gpointer)(uptr)i);
	}

	// check the appropriate menu items
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(MainWindow, "enable_console1")), Config.PsxOut);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(MainWindow, "enable_patches1")), Config.Patch);
	
	// disable anything not implemented or not working properly.
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "patch_browser1")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "patch_finder2")), FALSE);
	#ifndef PCSX2_DEVBUILD
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "GtkMenuItem_Logging")), FALSE);
	#endif
	
	gtk_widget_show_all(MainWindow);
	gtk_window_activate_focus(GTK_WINDOW(MainWindow));
	gtk_main();
}

void RunGui() {
	StartGui();
}

void FixCPUState(void)
{
	//Config.sseMXCSR = LinuxsseMXCSR;
	//Config.sseVUMXCSR = LinuxsseVUMXCSR;
	SetCPUState(Config.sseMXCSR, Config.sseVUMXCSR);
}

void OnDestroy(GtkObject *object, gpointer user_data) {}
	
gboolean OnDelete(GtkWidget       *widget, GdkEvent *event, gpointer user_data)
{
	 pcsx2_exit();
	return (FALSE);
}
int Pcsx2Configure() {
	if (!UseGui) return 0;
	
	configuringplug = TRUE;
	MainWindow = NULL;
	OnConf_Conf(NULL, 0);
	configuringplug = FALSE;
	
	return applychanges;
}

void OnLanguage(GtkMenuItem *menuitem, gpointer user_data) {
	ChangeLanguage(langs[(int)(uptr)user_data].lang);
	gtk_widget_destroy(MainWindow);
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();
	StartGui();
}

void SignalExit(int sig) {
	ClosePlugins();
	pcsx2_exit();
}

void ExecuteCpu()
{
	// Make sure any left-over recovery states are cleaned up.
	safe_delete( g_RecoveryState );
	
	// Destroy the window.  Ugly thing.
	gtk_widget_destroy(MainWindow);
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();

	g_GameInProgress = true;
	m_ReturnToGame = false;
	
	signal(SIGINT, SignalExit);
	signal(SIGPIPE, SignalExit);

	// Make sure any left-over recovery states are cleaned up.
	safe_delete( g_RecoveryState );

	// Just in case they weren't initialized earlier (no harm in calling this multiple times)
	if (OpenPlugins(NULL) == -1) return;

	// this needs to be called for every new game! (note: sometimes launching games through bios will give a crc of 0)
	if( GSsetGameCRC != NULL ) GSsetGameCRC(ElfCRC, g_ZeroGSOptions);

	// Optimization: We hardcode two versions of the EE here -- one for recs and one for ints.
	// This is because recs are performance critical, and being able to inline them into the
	// function here helps a small bit (not much but every small bit counts!).

	g_EmulationInProgress = true;
	g_ReturnToGui = false;

	PCSX2_MEM_PROTECT_BEGIN();

	if( CHECK_EEREC )
	{
		while( !g_ReturnToGui )
		{
			recExecute();
			SysUpdate();
		}
	}
	else
	{
		while( !g_ReturnToGui )
		{
			Cpu->Execute();
			SysUpdate();
		}
	}
	PCSX2_MEM_PROTECT_END();
}

void RunExecute( const char* elf_file, bool use_bios )
{
	
	// (air notes:)
	// If you want to use the new to-memory savestate feature, take a look at the new
	// RunExecute in WinMain.c, and secondly the CpuDlg.c or AdvancedDlg.cpp.  The
	// objects used are MemoryAlloc, memLoadingState, and memSavingState.

	// It's important to make sure to reset the CPU and the plugins correctly, which is
	// where the new RunExecute comes into play.  It can be kind of tricky knowing
	// when to call cpuExecuteBios and loadElfFile, and with what parameters.

	// (or, as an alternative maybe we should switch to wxWidgets and have a unified
	// cross platform gui?)  - Air

	try
	{
		cpuReset();
	}
	
	catch( std::exception& ex )
	{
		Msgbox::Alert( ex.what() );
		return;
	}

	if (OpenPlugins(NULL) == -1) 
	{
		RunGui(); 
		return;
	}
	
	if (elf_file == NULL )
	{
		if (g_RecoveryState != NULL)
		{
			try
			{
				memLoadingState( *g_RecoveryState ).FreezeAll();
			}
			catch( std::runtime_error& ex )
			{
				Msgbox::Alert(
					"Gamestate recovery failed.  Your game progress will be lost (sorry!)\n"
					"\nError: %s\n", params ex.what() );

				// Take the user back to the GUI...
				safe_delete( g_RecoveryState );
				ClosePlugins();
				return;
			}
			safe_delete( g_RecoveryState );
		}
		else
		{
			// Not recovering a state, so need to execute the bios and load the ELF information.

			// if the elf_file is null we use the CDVD elf file.
			// But if the elf_file is an empty string then we boot the bios instead.

			char ename[g_MaxPath];
			ename[0] = 0;
			if( !use_bios )
				GetPS2ElfName( ename );

			loadElfFile( ename );
		}
	}
	else
	{
		// Custom ELF specified (not using CDVD).
		// Run the BIOS and load the ELF.

		loadElfFile( elf_file );
	}
	
	FixCPUState();

	ExecuteCpu();
}

void OnFile_RunCD(GtkMenuItem *menuitem, gpointer user_data) {
	safe_free( g_RecoveryState );
	ResetPlugins();
	RunExecute( NULL );
}

void OnRunElf_Ok(GtkButton* button, gpointer user_data) {
	gchar *File;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(elfname, File);
	gtk_widget_destroy(FileSel);
	
	RunExecute(elfname);
}

void OnRunElf_Cancel(GtkButton* button, gpointer user_data) {
	gtk_widget_destroy(FileSel);
}

void OnFile_LoadElf(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Ok,*Cancel;

	FileSel = gtk_file_selection_new("Select Psx Elf File");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnRunElf_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect (GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnRunElf_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}
void pcsx2_exit()
{
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char plugin[g_MaxPath];

	// with this the problem with plugins that are linked with the pthread
	// library is solved

	dir = opendir(Config.PluginsDir);
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			sprintf (plugin, "%s%s", Config.PluginsDir, ent->d_name);

			if (strstr(plugin, ".so") == NULL) continue;
			Handle = dlopen(plugin, RTLD_NOW);
			if (Handle == NULL) continue;
		}
	}

	printf("PCSX2 Quitting\n");
	
	if (UseGui)
	{
		gtk_main_quit();
		SysClose();
		gtk_exit(0);
	}
	else
	{
		SysClose();
		exit(0);
	}
}
void OnFile_Exit(GtkMenuItem *menuitem, gpointer user_data) 
{
	pcsx2_exit();
}

void OnEmu_Run(GtkMenuItem *menuitem, gpointer user_data)
{
	if( g_EmulationInProgress )
		ExecuteCpu();
	else
		RunExecute( NULL, true );	// boots bios if no savestate is to be recovered

}

void OnEmu_Reset(GtkMenuItem *menuitem, gpointer user_data)
{
	SysReset();
}

 
 void ResetMenuSlots(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Item;
	char str[g_MaxPath];
	int i;
 
	for (i=0; i<5; i++) {
		sprintf(str, "GtkMenuItem_LoadSlot%d", i+1);
		Item = lookup_widget(MainWindow, str);
		if (Slots[i] == -1) 
			gtk_widget_set_sensitive(Item, FALSE);
		else
			gtk_widget_set_sensitive(Item, TRUE);
	}
 }

/*void UpdateMenuSlots(GtkMenuItem *menuitem, gpointer user_data) {
	char str[g_MaxPath];
	int i = 0;

	for (i=0; i<5; i++) {
		sprintf(str, SSTATES_DIR "/%8.8X.%3.3d", ElfCRC, i);
		Slots[i] = CheckState(str);
	}
}*/

void States_Load(string file, int num = -1 )
{
	efile = 2;
	try
	{
		// when we init joe it'll throw an UnsupportedStateVersion.
		// So reset the cpu afterward so that trying to load a bum save
		// doesn't fry the current emulation state.
		gzLoadingState joe( file );

		// Make sure the cpu and plugins are ready to be state-ified!
		cpuReset();
		OpenPlugins( NULL );

		joe.FreezeAll();
	}
	catch( Exception::UnsupportedStateVersion& )
	{
		if( num != -1 )
			Msgbox::Alert("Savestate slot %d is an unsupported version." , params num);
		else
			Msgbox::Alert( "%s : This is an unsupported savestate version." , params file.c_str());

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely...

		return;
	}
	catch( std::exception& ex )
	{
		if (num != -1)
			Console::Error("Error occured while trying to load savestate slot %d", params num);
		else
			Console::Error("Error occured while trying to load savestate file: %d", params file.c_str());

		Console::Error( "%s", params ex.what() );

		// The emulation state is ruined.  Might as well give them a popup and start the gui.

		Msgbox::Alert( 
			"An error occured while trying to load the savestate data.\n"
			"Pcsx2 emulation state has been reset."
		);

		cpuShutdown();
		return;
	}

	ExecuteCpu();
}

void States_Load(int num) {
	string Text;

	SaveState::GetFilename( Text, num );

	struct stat buf;
	if( stat(Text.c_str(), &buf ) == -1 )
	{
		Console::Notice( "Saveslot %d is empty.", params num );
		return;
	}
	States_Load( Text, num );
}

void States_Save( string file, int num = -1 )
{
	try
	{
		gzSavingState(file).FreezeAll();
		if( num != -1 )
			Console::Notice("State saved to slot %d", params num );
		else
			Console::Notice( "State saved to file: %s", params file.c_str() );
	}
	catch( std::exception& ex )
	{
		if( num != -1 )
			Msgbox::Alert("An error occurred while trying to save to slot %d", params num );
		else
			Msgbox::Alert("An error occurred while trying to save to file: %s", params file.c_str() );

		Console::Error("Save state request failed with the following error:" );
		Console::Error( "%s", params ex.what() );
	}
}

void States_Save(int num) {
	string Text;

	SaveState::GetFilename( Text, num );
	States_Save( Text, num );
}

void OnStates_Load1(GtkMenuItem *menuitem, gpointer user_data) { States_Load(0); } 
void OnStates_Load2(GtkMenuItem *menuitem, gpointer user_data) { States_Load(1); } 
void OnStates_Load3(GtkMenuItem *menuitem, gpointer user_data) { States_Load(2); } 
void OnStates_Load4(GtkMenuItem *menuitem, gpointer user_data) { States_Load(3); } 
void OnStates_Load5(GtkMenuItem *menuitem, gpointer user_data) { States_Load(4); } 

void OnLoadOther_Ok(GtkButton* button, gpointer user_data) {
	gchar *File;
	char str[g_MaxPath];

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	States_Load( str );
}

void OnLoadOther_Cancel(GtkButton* button, gpointer user_data) {
	gtk_widget_destroy(FileSel);
}

void OnStates_LoadOther(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Ok,*Cancel;

	FileSel = gtk_file_selection_new(_("Select State File"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSel), SSTATES_DIR "/");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnLoadOther_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect (GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnLoadOther_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
} 

void OnStates_Save1(GtkMenuItem *menuitem, gpointer user_data) { States_Save(0); } 
void OnStates_Save2(GtkMenuItem *menuitem, gpointer user_data) { States_Save(1); } 
void OnStates_Save3(GtkMenuItem *menuitem, gpointer user_data) { States_Save(2); } 
void OnStates_Save4(GtkMenuItem *menuitem, gpointer user_data) { States_Save(3); } 
void OnStates_Save5(GtkMenuItem *menuitem, gpointer user_data) { States_Save(4); } 

void OnSaveOther_Ok(GtkButton* button, gpointer user_data) {
	gchar *File;
	char str[g_MaxPath];

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	States_Save( str );
}

void OnSaveOther_Cancel(GtkButton* button, gpointer user_data) {
	gtk_widget_destroy(FileSel);
}

void OnStates_SaveOther(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Ok,*Cancel;

	FileSel = gtk_file_selection_new(_("Select State File"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSel), SSTATES_DIR "/");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnSaveOther_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect (GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnSaveOther_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
} 

//2002-09-28 (Florin)
void OnArguments_Ok(GtkButton *button, gpointer user_data) {
	char *str;

	str = (char*)gtk_entry_get_text(GTK_ENTRY(widgetCmdLine));
	memcpy(args, str, g_MaxPath);

	gtk_widget_destroy(CmdLine);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnEmu_Arguments(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *widgetCmdLine;

	CmdLine = create_CmdLine();
	gtk_window_set_title(GTK_WINDOW(CmdLine), _("Program arguments"));

	widgetCmdLine = lookup_widget(CmdLine, "GtkEntry_dCMDLINE");
	
	gtk_entry_set_text(GTK_ENTRY(widgetCmdLine), args);
	gtk_widget_show_all(CmdLine);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void OnCpu_Ok(GtkButton *button, gpointer user_data) {
	u32 newopts = 0;

	//Cpu->Shutdown();
	//vu0Shutdown();
	//vu1Shutdown();
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec"))))
		newopts |= PCSX2_EEREC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec"))))
		newopts |= PCSX2_VU0REC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec"))))
		newopts |= PCSX2_VU1REC;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS"))))
		newopts |= PCSX2_GSMULTITHREAD;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitNormal"))))
		newopts |= PCSX2_FRAMELIMIT_NORMAL;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitLimit"))))
		newopts |= PCSX2_FRAMELIMIT_LIMIT;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitFS"))))
		newopts |= PCSX2_FRAMELIMIT_SKIP;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_VUSkip"))))
		newopts |= PCSX2_FRAMELIMIT_VUSKIP;
	
	Config.CustomFps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")));
	Config.CustomFrameSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")));
	Config.CustomConsecutiveFrames = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")));
	Config.CustomConsecutiveSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")));
	
	if (Config.Options != newopts)
	{
		SysRestorableReset();

		if( (Config.Options&PCSX2_GSMULTITHREAD) ^ (newopts&PCSX2_GSMULTITHREAD) )
		{
			// gotta shut down *all* the plugins.
			ResetPlugins();
		}
		Config.Options = newopts;
	}
	else
		UpdateVSyncRate();

	SaveConfig();
	
	gtk_widget_destroy(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data)
{
	char str[512];

	CpuDlg = create_CpuDlg();
	gtk_window_set_title(GTK_WINDOW(CpuDlg), _("Configuration"));
	
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec")), !!CHECK_EEREC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec")), !!CHECK_VU0REC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec")), !!CHECK_VU1REC);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS")), !!CHECK_MULTIGS);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitNormal")), CHECK_FRAMELIMIT==PCSX2_FRAMELIMIT_NORMAL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitLimit")), CHECK_FRAMELIMIT==PCSX2_FRAMELIMIT_LIMIT);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitFS")), CHECK_FRAMELIMIT==PCSX2_FRAMELIMIT_SKIP);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_VUSkip")), CHECK_FRAMELIMIT==PCSX2_FRAMELIMIT_VUSKIP);
    
	sprintf(str, "Cpu Vendor:     %s", cpuinfo.x86ID);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuVendor")), str);
	sprintf(str, "Familly:   %s", cpuinfo.x86Fam);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Family")), str);
	sprintf(str, "Cpu Speed:   %d MHZ", cpuinfo.cpuspeed);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuSpeed")), str);

	strcpy(str,"Features:    ");
	if(cpucaps.hasMultimediaExtensions) strcat(str,"MMX");
	if(cpucaps.hasStreamingSIMDExtensions) strcat(str,",SSE");
	if(cpucaps.hasStreamingSIMD2Extensions) strcat(str,",SSE2");
	if(cpucaps.hasStreamingSIMD3Extensions) strcat(str,",SSE3");
	if(cpucaps.hasAMD64BitArchitecture) strcat(str,",x86-64");
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Features")), str);
	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")), (gdouble)Config.CustomFps);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")), (gdouble)Config.CustomFrameSkip);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")), (gdouble)Config.CustomConsecutiveFrames);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")), (gdouble)Config.CustomConsecutiveSkip);

	gtk_widget_show_all(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void OnLogging_Ok(GtkButton *button, gpointer user_data) {
#ifdef PCSX2_DEVBUILD
	GtkWidget *Btn;
	char str[32];
	int i, ret;
	
	
	for (i=0; i<32; i++) {
		if (((i > 16) && (i < 20)) || (i == 29))
			continue;
		
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		ret = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));
		if (ret) varLog|= 1<<i;
		else varLog&=~(1<<i);
	}
	
    SaveConfig();
#endif

	gtk_widget_destroy(LogDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnDebug_Logging(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Btn;
	char str[32];
	int i;

	LogDlg = create_Logging();
	
	
	for (i=0; i<32; i++) {
		if (((i > 16) && (i < 20)) || (i == 29))
			continue;
		
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), varLog & (1<<i));
	}

	gtk_widget_show_all(LogDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void OnHelp_Help() {
}

void OnHelpAbout_Ok(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(AboutDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnHelp_About(GtkMenuItem *menuitem, gpointer user_data) {
	char str[g_MaxPath];
	GtkWidget *Label;

	AboutDlg = create_AboutDlg();
 	gtk_window_set_title(GTK_WINDOW(AboutDlg), _("About"));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelVersion");
	
 	// Include the SVN revision
	if (SVN_REV !=0)
		sprintf(str, _("PCSX2 Playground For Linux\nVersion %s %s\n"), PCSX2_VERSION, SVN_REV);
	else
		//Use this instead for a non-svn version
		sprintf(str, _("PCSX2 Playground For Linux\nVersion %s\n"), PCSX2_VERSION);
	
	gtk_label_set_text(GTK_LABEL(Label), str);	
	
	Label = lookup_widget(AboutDlg, "GtkAbout_LabelAuthors");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelAuthors));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelGreets");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelGreets));

	gtk_widget_show_all(AboutDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void on_patch_browser1_activate(GtkMenuItem *menuitem, gpointer user_data) {}

void on_patch_finder2_activate(GtkMenuItem *menuitem, gpointer user_data) {}

void on_enable_console1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    Config.PsxOut=(int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
    SaveConfig();
}

void on_enable_patches1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    Config.Patch=(int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
    SaveConfig();
}

void on_Game_Fixes(GtkMenuItem *menuitem, gpointer user_data) 
{
	GameFixDlg = create_GameFixDlg();
	
	set_checked(GameFixDlg, "check_VU_Add_Sub", (Config.GameFixes & FLAG_VU_ADD_SUB));
	set_checked(GameFixDlg, "check_FPU_Clamp", (Config.GameFixes & FLAG_FPU_CLAMP));
	set_checked(GameFixDlg, "check_VU_Branch", (Config.GameFixes & FLAG_VU_BRANCH));
	
	gtk_widget_show_all(GameFixDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
	}

void on_Game_Fix_OK(GtkButton *button, gpointer user_data) 
{
	
	Config.GameFixes = 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_VU_Add_Sub") ? FLAG_VU_ADD_SUB : 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_FPU_Clamp") ? FLAG_FPU_CLAMP : 0;
	Config.GameFixes |= is_checked(GameFixDlg, "check_VU_Branch") ? FLAG_VU_BRANCH : 0;
	
	SaveConfig();
	gtk_widget_destroy(GameFixDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void on_Speed_Hacks(GtkMenuItem *menuitem, gpointer user_data)
{
	SpeedHacksDlg = create_SpeedHacksDlg();
	
	switch (CHECK_EE_CYCLERATE)
	{
		case 0:
			set_checked(SpeedHacksDlg, "check_default_cycle_rate", true);
			break;
		case 1:
			set_checked(SpeedHacksDlg, "check_1_5_cycle_rate", true);
			break;
		case 2:
			set_checked(SpeedHacksDlg, "check_2_cycle_rate", true);
			break;
		case 3:
			set_checked(SpeedHacksDlg, "check_3_cycle_rate", true);
			break;
		default:
			set_checked(SpeedHacksDlg, "check_default_cycle_rate", true);
			break;
	}
	
        set_checked(SpeedHacksDlg, "check_iop_cycle_rate", CHECK_IOP_CYCLERATE);
	set_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack", CHECK_WAITCYCLE_HACK);
	set_checked(SpeedHacksDlg, "check_ESC_hack",CHECK_ESCAPE_HACK);
	
	gtk_widget_show_all(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}
	
void on_Speed_Hack_OK(GtkButton *button, gpointer user_data)
{
	Config.Hacks = 0;

	if is_checked(SpeedHacksDlg, "check_default_cycle_rate")
		Config.Hacks = 0;
	else if is_checked(SpeedHacksDlg, "check_1_5_cycle_rate")
		Config.Hacks = 1;
	else if is_checked(SpeedHacksDlg, "check_2_cycle_rate")
		Config.Hacks = 2;
	else if is_checked(SpeedHacksDlg, "check_3_cycle_rate")
		Config.Hacks = 3;
		
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_iop_cycle_rate") << 3;  
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_wait_cycles_sync_hack") << 4;  
	Config.Hacks |= is_checked(SpeedHacksDlg, "check_ESC_hack") << 10;  
	
	SaveConfig();

	gtk_widget_destroy(SpeedHacksDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void setAdvancedOptions()
{
	if( !cpucaps.hasStreamingSIMD2Extensions )
	{
		// SSE1 cpus do not support Denormals Are Zero flag.

		Config.sseMXCSR &= ~FLAG_DENORMAL_ZERO;
		Config.sseVUMXCSR &= ~FLAG_DENORMAL_ZERO;
	}
	
	switch((Config.sseMXCSR & 0x6000) >> 13)
	{
		case 0:
			set_checked(AdvDlg, "radio_EE_Round_Near", TRUE);
			break;
		case 1:
			set_checked(AdvDlg, "radio_EE_Round_Negative",TRUE);
			break;
		case 2:
			set_checked(AdvDlg, "radio_EE_Round_Positive", TRUE);
			break;
		case 3:
			set_checked(AdvDlg, "radio_EE_Round_Zero", TRUE);
			break;
	}
	
	switch((Config.sseVUMXCSR & 0x6000) >> 13)
	{
		case 0:
			set_checked(AdvDlg, "radio_VU_Round_Near", TRUE);
			break;
		case 1:
			set_checked(AdvDlg, "radio_VU_Round_Negative",TRUE);
			break;
		case 2:
			set_checked(AdvDlg, "radio_VU_Round_Positive", TRUE);
			break;
		case 3:
			set_checked(AdvDlg, "radio_VU_Round_Zero", TRUE);
			break;
	}
	
	
	switch(Config.eeOptions)
	{
		case FLAG_EE_CLAMP_NONE:
			set_checked(AdvDlg, "radio_EE_Clamp_None", TRUE);
			break;
		case FLAG_EE_CLAMP_NORMAL:
			set_checked(AdvDlg, "radio_EE_Clamp_Normal",TRUE);
			break;
		case FLAG_EE_CLAMP_EXTRA_PRESERVE:
			set_checked(AdvDlg, "radio_EE_Clamp_Extra_Preserve", TRUE);
			break;
	}
	
	switch(Config.vuOptions)
	{
		case FLAG_VU_CLAMP_NONE:
			set_checked(AdvDlg, "radio_VU_Clamp_None", TRUE);
			break;
		case FLAG_VU_CLAMP_NORMAL:
			set_checked(AdvDlg, "radio_VU_Clamp_Normal",TRUE);
			break;
		case FLAG_VU_CLAMP_EXTRA:
			set_checked(AdvDlg, "radio_VU_Clamp_Extra", TRUE);
			break;
		case FLAG_VU_CLAMP_EXTRA_PRESERVE:
			set_checked(AdvDlg, "radio_VU_Clamp_Extra_Preserve", TRUE);
			break;
	}
	set_checked(AdvDlg, "check_EE_Flush_Zero", (Config.sseMXCSR & FLAG_FLUSH_ZERO) ? TRUE : FALSE);
	set_checked(AdvDlg, "check_EE_Denormal_Zero", (Config.sseMXCSR & FLAG_DENORMAL_ZERO) ? TRUE : FALSE);
	
	set_checked(AdvDlg, "check_VU_Flush_Zero", (Config.sseVUMXCSR & FLAG_FLUSH_ZERO) ? TRUE : FALSE);
	set_checked(AdvDlg, "check_VU_Denormal_Zero", (Config.sseVUMXCSR & FLAG_DENORMAL_ZERO) ? TRUE : FALSE);
}
void on_Advanced(GtkMenuItem *menuitem, gpointer user_data)
{
	AdvDlg = create_AdvDlg();
	
	setAdvancedOptions();
	
	gtk_widget_show_all(AdvDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
	}

void on_Advanced_Defaults(GtkButton *button, gpointer user_data)
{
	Config.sseMXCSR = DEFAULT_sseMXCSR;
	Config.sseVUMXCSR = DEFAULT_sseVUMXCSR;
	Config.eeOptions = DEFAULT_eeOptions;
	Config.vuOptions = DEFAULT_vuOptions;
	
	setAdvancedOptions();
      }

void on_Advanced_OK(GtkButton *button, gpointer user_data) 
{
	Config.sseMXCSR &= 0x1fbf;
	Config.sseVUMXCSR &= 0x1fbf;
	Config.eeOptions = 0;
	Config.vuOptions = 0;
	
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Near") ? FLAG_ROUND_NEAR : 0; 
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Negative") ? FLAG_ROUND_NEGATIVE : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Positive") ? FLAG_ROUND_POSITIVE : 0; 
	Config.sseMXCSR |= is_checked(AdvDlg, "radio_EE_Round_Zero") ? FLAG_ROUND_ZERO : 0; 
	
	Config.sseMXCSR |= is_checked(AdvDlg, "check_EE_Denormal_Zero") ? FLAG_DENORMAL_ZERO : 0;
	Config.sseMXCSR |= is_checked(AdvDlg, "check_EE_Flush_Zero") ? FLAG_FLUSH_ZERO : 0;
	
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Near") ? FLAG_ROUND_NEAR : 0; 
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Negative") ? FLAG_ROUND_NEGATIVE : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Positive") ? FLAG_ROUND_POSITIVE : 0; 
	Config.sseVUMXCSR |= is_checked(AdvDlg, "radio_VU_Round_Zero") ? FLAG_ROUND_ZERO : 0; 
	
	Config.sseVUMXCSR |= is_checked(AdvDlg, "check_VU_Denormal_Zero") ? FLAG_DENORMAL_ZERO : 0;
	Config.sseVUMXCSR |= is_checked(AdvDlg, "check_VU_Flush_Zero") ? FLAG_FLUSH_ZERO : 0;
	
	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_None") ? FLAG_EE_CLAMP_NONE : 0; 
	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_Normal") ? FLAG_EE_CLAMP_NORMAL : 0;
	Config.eeOptions |= is_checked(AdvDlg, "radio_EE_Clamp_Extra_Preserve") ? FLAG_EE_CLAMP_EXTRA_PRESERVE : 0; 
	
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_None") ? FLAG_VU_CLAMP_NONE : 0; 
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Normal") ? FLAG_VU_CLAMP_NORMAL : 0;
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Extra") ? FLAG_VU_CLAMP_EXTRA : 0; 
	Config.vuOptions |= is_checked(AdvDlg, "radio_VU_Clamp_Extra_Preserve") ? FLAG_VU_CLAMP_EXTRA_PRESERVE : 0; 
	
	SetCPUState(Config.sseMXCSR, Config.sseVUMXCSR);
	SaveConfig();
	
	gtk_widget_destroy(AdvDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}
