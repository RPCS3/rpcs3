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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkdialog.h>
//#include <gtk/gtkmessagedialog.h>
#include <signal.h>
#include <sys/time.h>

#include "support.h"
#include "callbacks.h"
#include "interface.h"
#include "Linux.h"

#include "Paths.h"


static int needReset = 1;
int confret;
int confplug=0;
extern int RunExe;

_PS2EgetLibType		PS2EgetLibType = NULL;
_PS2EgetLibVersion2	PS2EgetLibVersion2 = NULL;
_PS2EgetLibName		PS2EgetLibName = NULL;

// Helper Functions
void FindPlugins();

// Functions Callbacks
void OnFile_LoadElf(GtkMenuItem *menuitem, gpointer user_data);
void OnFile_Exit(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Run(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Reset(GtkMenuItem *menuitem, gpointer user_data);
void OnEmu_Arguments(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Gs(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Pads(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Conf(GtkMenuItem *menuitem, gpointer user_data);
void OnLanguage(GtkMenuItem *menuitem, gpointer user_data);
void OnHelp_Help();
void OnHelp_About(GtkMenuItem *menuitem, gpointer user_data);

GtkWidget *Window;
GtkWidget* pStatusBar = NULL;
GtkWidget *CmdLine;	//2002-09-28 (Florin)
GtkWidget *ConfDlg;
GtkWidget *AboutDlg;
GtkWidget *DebugWnd;
GtkWidget *LogDlg;
GtkWidget *FileSel;

GtkAccelGroup *AccelGroup;

typedef struct {
	GtkWidget *Combo;
	GList *glist;
	char plist[255][255];
	int plugins;
} PluginConf;

PluginConf GSConfS;
PluginConf PAD1ConfS;
PluginConf PAD2ConfS;
PluginConf SPU2ConfS;
PluginConf CDVDConfS;
PluginConf DEV9ConfS;
PluginConf USBConfS;
PluginConf FWConfS;
PluginConf BiosConfS;

void StartGui() {
	GtkWidget *Menu;
	GtkWidget *Item;
    GtkWidget* vbox;
	int i;

    add_pixmap_directory(".pixmaps");

	Window = create_MainWindow();

#ifdef PCSX2_VIRTUAL_MEM
    gtk_window_set_title(GTK_WINDOW(Window), "PCSX2 "PCSX2_VERSION" Watermoose VM");
#else
    gtk_window_set_title(GTK_WINDOW(Window), "PCSX2 "PCSX2_VERSION" Watermoose");
#endif

    // status bar
    pStatusBar = gtk_statusbar_new ();
    gtk_box_pack_start (GTK_BOX(lookup_widget(Window, "status_box")), pStatusBar, TRUE, TRUE, 0);
    gtk_widget_show (pStatusBar);

    gtk_statusbar_push(GTK_STATUSBAR(pStatusBar),0,
                       "F1 - save, F2 - next state, Shift+F2 - prev state, F3 - load, F8 - snapshot");

    // add all the languages
	Item = lookup_widget(Window, "GtkMenuItem_Language");
	Menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(Item), Menu);

	for (i=0; i<langsMax; i++) {
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
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(Window, "enable_console1")), Config.PsxOut);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(Window, "enable_patches1")), Config.Patch);

	gtk_widget_show_all(Window);
    gtk_window_activate_focus(GTK_WINDOW(Window));
	gtk_main();
}

void RunGui() {
	StartGui();
}

int destroy=0;

void OnDestroy(GtkObject *object, gpointer user_data) {
	if (!destroy) OnFile_Exit(NULL, user_data);
}

int Pcsx2Configure() {
	if (!UseGui) return 0;
	confplug = 1;
	Window = NULL;
	OnConf_Conf(NULL, 0);
	confplug = 0;
	return confret;
}

int efile=0;
char elfname[256];

void OnLanguage(GtkMenuItem *menuitem, gpointer user_data) {
	ChangeLanguage(langs[(int)(uptr)user_data].lang);
	destroy=1;
	gtk_widget_destroy(Window);
	destroy=0;
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();
	StartGui();
}

void SignalExit(int sig) {
	ClosePlugins();
	OnFile_Exit(NULL, 0);
}

extern int g_ZeroGSOptions;
void RunExecute(int run)
{
    if (needReset == 1) {
		SysReset();
	}

	destroy=1;
	gtk_widget_destroy(Window);
	destroy=0;
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();

	if (OpenPlugins(NULL) == -1) {
		RunGui(); return;
	}
	signal(SIGINT, SignalExit);
	signal(SIGPIPE, SignalExit);

	if (needReset == 1) { 
		
        if( RunExe == 0 )
            cpuExecuteBios();
        if(!efile)
            efile=GetPS2ElfName(elfname);
        loadElfFile(elfname);

		//if (efile == 2)
		//	efile=GetPS2ElfName(elfname);
		//if (efile)
		//	loadElfFile(elfname);
        RunExe = 0;
		efile=0;
		needReset = 0;
	}

    // this needs to be called for every new game! (note: sometimes launching games through bios will give a crc of 0)
	if( GSsetGameCRC != NULL )
		GSsetGameCRC(ElfCRC, g_ZeroGSOptions);

	if (run) Cpu->Execute();
}

void OnFile_RunCD(GtkMenuItem *menuitem, gpointer user_data) {
    needReset = 1;
	efile = 0;
	RunExecute(1);
}

void OnRunElf_Ok(GtkButton* button, gpointer user_data) {
	gchar *File;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(elfname, File);
	gtk_widget_destroy(FileSel);
	needReset = 1;
	efile = 1;
	RunExecute(1);
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

void OnFile_Exit(GtkMenuItem *menuitem, gpointer user_data) {
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char plugin[256];

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

	printf(_("PCSX2 Quitting\n"));
	if (UseGui) gtk_main_quit();
	SysClose();
	if (UseGui) gtk_exit(0);
	else exit(0);
}

void OnEmu_Run(GtkMenuItem *menuitem, gpointer user_data)
{
    if(needReset == 1)
        RunExe = 1;
    efile = 0;
	RunExecute(1);
}

void OnEmu_Reset(GtkMenuItem *menuitem, gpointer user_data)
{
    ResetPlugins();
    needReset = 1;
    efile = 0;
}

int Slots[5] = { -1, -1, -1, -1, -1 };

void ResetMenuSlots(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Item;
	char str[256];
	int i;

	for (i=0; i<5; i++) {
		sprintf(str, "GtkMenuItem_LoadSlot%d", i+1);
		Item = lookup_widget(Window, str);
		if (Slots[i] == -1) 
			gtk_widget_set_sensitive(Item, FALSE);
		else
			gtk_widget_set_sensitive(Item, TRUE);
	}
}

void UpdateMenuSlots(GtkMenuItem *menuitem, gpointer user_data) {
	char str[256];
	int i;

	for (i=0; i<5; i++) {
		sprintf(str, SSTATES_DIR "/%8.8X.%3.3d", ElfCRC, i);
		Slots[i] = CheckState(str);
	}
}

void States_Load(int num) {
	char Text[256];
	int ret;

	efile = 2;
	RunExecute(0);

	sprintf (Text, SSTATES_DIR "/%8.8X.%3.3d", ElfCRC, num);
	ret = LoadState(Text);
/*	if (ret == 0)
		 sprintf (Text, _("*PCSX2*: Loaded State %d"), num+1);
	else sprintf (Text, _("*PCSX2*: Error Loading State %d"), num+1);
	GPU_displayText(Text);*/

	Cpu->Execute();
}

void States_Save(int num) {
	char Text[256];
	int ret;

	sprintf (Text, SSTATES_DIR "/%8.8X.%3.3d", ElfCRC, num);
	ret = SaveState(Text);
    if (ret == 0)
		 sprintf(Text, _("*PCSX2*: Saving State %d"), num+1);
	else sprintf(Text, _("*PCSX2*: Error Saving State %d"), num+1);
	//StatusSet(Text);

    RunExecute(1);
}

void OnStates_Load1(GtkMenuItem *menuitem, gpointer user_data) { States_Load(0); } 
void OnStates_Load2(GtkMenuItem *menuitem, gpointer user_data) { States_Load(1); } 
void OnStates_Load3(GtkMenuItem *menuitem, gpointer user_data) { States_Load(2); } 
void OnStates_Load4(GtkMenuItem *menuitem, gpointer user_data) { States_Load(3); } 
void OnStates_Load5(GtkMenuItem *menuitem, gpointer user_data) { States_Load(4); } 

void OnLoadOther_Ok(GtkButton* button, gpointer user_data) {
	gchar *File;
	char str[256];
	char Text[256];
	int ret;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);

	efile = 2;
	RunExecute(0);

	ret = LoadState(str);
/*	if (ret == 0)
		 sprintf (Text, _("*PCSX*: Loaded State %s"), str);
	else sprintf (Text, _("*PCSX*: Error Loading State %s"), str);
	GPU_displayText(Text);*/

	Cpu->Execute();
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
	char str[256];
	char Text[256];
	int ret;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(str, File);
	gtk_widget_destroy(FileSel);
	RunExecute(0);

	ret = SaveState(str);
/*	if (ret == 0)
		 sprintf (Text, _("*PCSX*: Saved State %s"), str);
	else sprintf (Text, _("*PCSX*: Error Saving State %s"), str);
	GPU_displayText(Text);*/

	Cpu->Execute();
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
	GtkWidget *widgetCmdLine;
	char *str;

	widgetCmdLine = lookup_widget(CmdLine, "GtkEntry_dCMDLINE");
	str = (char*)gtk_entry_get_text(GTK_ENTRY(widgetCmdLine));
	memcpy(args, str, 256);

	gtk_widget_destroy(CmdLine);
	gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnArguments_Cancel(GtkButton* button, gpointer user_data) {
	gtk_widget_destroy(CmdLine);
	gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnEmu_Arguments(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *widgetCmdLine;

	CmdLine = create_CmdLine();
	gtk_window_set_title(GTK_WINDOW(CmdLine), _("Program arguments"));

	widgetCmdLine = lookup_widget(CmdLine, "GtkEntry_dCMDLINE");
	gtk_entry_set_text(GTK_ENTRY(widgetCmdLine), args);
						//args exported by ElfHeader.h
	gtk_widget_show_all(CmdLine);
	gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}
//-------------------

void OnConf_Gs(GtkMenuItem *menuitem, gpointer user_data)
{
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
    GSconfigure();
    chdir(file);
	gtk_widget_set_sensitive(Window, TRUE);
}

void OnConf_Pads(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	PAD1configure();
	if (strcmp(Config.PAD1, Config.PAD2)) PAD2configure();
    chdir(file);
	gtk_widget_set_sensitive(Window, TRUE);
}

void OnConf_Spu2(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	SPU2configure();
	gtk_widget_set_sensitive(Window, TRUE);
    chdir(file);
}

void OnConf_Cdvd(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	CDVDconfigure();
	gtk_widget_set_sensitive(Window, TRUE);
    chdir(file);
}

void OnConf_Dev9(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	DEV9configure();
	gtk_widget_set_sensitive(Window, TRUE);
    chdir(file);
}

void OnConf_Usb(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	USBconfigure();
	gtk_widget_set_sensitive(Window, TRUE);
    chdir(file);
}

void OnConf_Fw(GtkMenuItem *menuitem, gpointer user_data) {
    char file[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(Window, FALSE);
	FWconfigure();
	gtk_widget_set_sensitive(Window, TRUE);
    chdir(file);
}

GtkWidget *CpuDlg;

void OnCpu_Ok(GtkButton *button, gpointer user_data) {
	GtkWidget *Btn;
	long t;
	u32 newopts = 0;

	Cpu->Shutdown();
	vu0Shutdown();
	vu1Shutdown();

	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec"))) ) {
		newopts |= PCSX2_EEREC;
    }
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec"))) )
		newopts |= PCSX2_VU0REC;
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec"))) )
		newopts |= PCSX2_VU1REC;
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS"))) )
		newopts |= PCSX2_GSMULTITHREAD;
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_CpuDC"))) )
		newopts |= PCSX2_DUALCORE;
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitNormal"))) )
		newopts |= PCSX2_FRAMELIMIT_NORMAL;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitLimit"))) )
		newopts |= PCSX2_FRAMELIMIT_LIMIT;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_LimitFS"))) )
		newopts |= PCSX2_FRAMELIMIT_SKIP;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkRadioButton_VUSkip"))) )
		newopts |= PCSX2_FRAMELIMIT_VUSKIP;

	if( (Config.Options&PCSX2_GSMULTITHREAD) ^ (newopts&PCSX2_GSMULTITHREAD) ) {
		Config.Options = newopts;
		SaveConfig();
        SysMessage("Restart Pcsx2");
		exit(0);
	}
	
	if( newopts & PCSX2_EEREC ) newopts |= PCSX2_COP2REC;
	
	Config.Options = newopts;
	
	UpdateVSyncRate();
	SaveConfig();
	
	cpuRestartCPU();

	gtk_widget_destroy(CpuDlg);

	if (Window) gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnCpu_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(CpuDlg);
	if (Window) gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}


void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *Btn;
    char str[512];

	CpuDlg = create_CpuDlg();
	gtk_window_set_title(GTK_WINDOW(CpuDlg), _("Configuration"));

    if(!cpucaps.hasStreamingSIMDExtensions) {
        Config.Options &= (PCSX2_VU0REC|PCSX2_VU1REC);//disable the config just in case
    }
    if(!cpucaps.hasMultimediaExtensions) {
        Config.Options &= ~(PCSX2_EEREC|PCSX2_VU0REC|PCSX2_VU1REC|PCSX2_COP2REC);//return to interpreter mode       
    }

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_EERec")), !!CHECK_EEREC);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU0rec")), !!CHECK_VU0REC);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_VU1rec")), !!CHECK_VU1REC);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_MTGS")), !!CHECK_MULTIGS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(CpuDlg, "GtkCheckButton_CpuDC")), !!CHECK_DUALCORE);
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

    //GtkLabel_CpuVendor
    //GtkLabel_Family
    //GtkLabel_CpuSpeed
    //GtkLabel_Features
	gtk_widget_show_all(CpuDlg);
	if (Window) gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}

#define FindComboText(combo,list,conf) \
	if (strlen(conf) > 0) { \
		int i; \
		for (i=2;i<255;i+=2) { \
			if (!strcmp(conf, list[i-2])) { \
				gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), list[i-1]); \
				break; \
			} \
		} \
	}

#define GetComboText(combo,list,conf) \
	{ \
	int i; \
	char *tmp = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)); \
	for (i=2;i<255;i+=2) { \
		if (!strcmp(tmp, list[i-1])) { \
			strcpy(conf, list[i-2]); \
			break; \
		} \
	} \
	}

void OnConfConf_Ok(GtkButton *button, gpointer user_data) {
	GetComboText(GSConfS.Combo, GSConfS.plist, Config.GS)
	GetComboText(PAD1ConfS.Combo, PAD1ConfS.plist, Config.PAD1);
	GetComboText(PAD2ConfS.Combo, PAD2ConfS.plist, Config.PAD2);
	GetComboText(SPU2ConfS.Combo, SPU2ConfS.plist, Config.SPU2);
	GetComboText(CDVDConfS.Combo, CDVDConfS.plist, Config.CDVD);
	GetComboText(DEV9ConfS.Combo, DEV9ConfS.plist, Config.DEV9);
	GetComboText(USBConfS.Combo,  USBConfS.plist,  Config.USB);
	GetComboText(FWConfS.Combo,   FWConfS.plist,   Config.FW);
	GetComboText(BiosConfS.Combo, BiosConfS.plist, Config.Bios);

	SaveConfig();

	if (confplug == 0) {
		ReleasePlugins();
		LoadPlugins();
    }

	needReset = 1;
	gtk_widget_destroy(ConfDlg);
	if (Window) gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
	confret = 1;
}

void OnConfConf_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(ConfDlg);
	if (Window) gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
	confret = 0;
}

#define ConfPlugin(src, confs, plugin, name) \
	void *drv; \
	src conf; \
	char file[256]; \
	GetComboText(confs.Combo, confs.plist, plugin); \
    strcpy(file, Config.PluginsDir); \
	strcat(file, plugin); \
	drv = SysLoadLibrary(file); \
    getcwd(file, ARRAYSIZE(file)); /* store current dir */  \
    chdir(Config.PluginsDir); /* change dirs so that plugins can find their config file*/  \
	if (drv == NULL) return; \
	conf = (src) SysLoadSym(drv, name); \
	if (SysLibError() == NULL) conf(); \
    chdir(file); /* change back*/       \
    SysCloseLibrary(drv);

#define TestPlugin(src, confs, plugin, name) \
	void *drv; \
	src conf; \
	int ret = 0; \
	char file[256]; \
	GetComboText(confs.Combo, confs.plist, plugin); \
	strcpy(file, Config.PluginsDir); \
	strcat(file, plugin); \
	drv = SysLoadLibrary(file); \
	if (drv == NULL) return; \
	conf = (src) SysLoadSym(drv, name); \
	if (SysLibError() == NULL) { \
		ret = conf(); \
		if (ret == 0) \
			 SysMessage(_("This plugin reports that should work correctly")); \
		else SysMessage(_("This plugin reports that should not work correctly")); \
	} \
	SysCloseLibrary(drv);

void OnConfConf_GsConf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_GSconfigure, GSConfS, Config.GS, "GSconfigure");
}

void OnConfConf_GsTest(GtkButton *button, gpointer user_data) {
	TestPlugin(_GStest, GSConfS, Config.GS, "GStest");
}

void OnConfConf_GsAbout(GtkButton *button, gpointer user_data) {
	ConfPlugin(_GSabout, GSConfS, Config.GS, "GSabout");
}

void OnConfConf_Pad1Conf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_PADconfigure, PAD1ConfS, Config.PAD1, "PADconfigure");
}

void OnConfConf_Pad1Test(GtkButton *button, gpointer user_data) {
	TestPlugin(_PADtest, PAD1ConfS, Config.PAD1, "PADtest");
}

void OnConfConf_Pad1About(GtkButton *button, gpointer user_data) {
	ConfPlugin(_PADabout, PAD1ConfS, Config.PAD1, "PADabout");
}

void OnConfConf_Pad2Conf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_PADconfigure, PAD2ConfS, Config.PAD2, "PADconfigure");
}

void OnConfConf_Pad2Test(GtkButton *button, gpointer user_data) {
	TestPlugin(_PADtest, PAD2ConfS, Config.PAD2, "PADtest");
}

void OnConfConf_Pad2About(GtkButton *button, gpointer user_data) {
	ConfPlugin(_PADabout, PAD2ConfS, Config.PAD2, "PADabout");
}

void OnConfConf_Spu2Conf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_SPU2configure, SPU2ConfS, Config.SPU2, "SPU2configure");
}

void OnConfConf_Spu2Test(GtkButton *button, gpointer user_data) {
	TestPlugin(_SPU2test, SPU2ConfS, Config.SPU2, "SPU2test");
}

void OnConfConf_Spu2About(GtkButton *button, gpointer user_data) {
	ConfPlugin(_SPU2about, SPU2ConfS, Config.SPU2, "SPU2about");
}

void OnConfConf_CdvdConf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_CDVDconfigure, CDVDConfS, Config.CDVD, "CDVDconfigure");
}

void OnConfConf_CdvdTest(GtkButton *button, gpointer user_data) {
	TestPlugin(_CDVDtest, CDVDConfS, Config.CDVD, "CDVDtest");
}

void OnConfConf_CdvdAbout(GtkButton *button, gpointer user_data) {
	ConfPlugin(_CDVDabout, CDVDConfS, Config.CDVD, "CDVDabout");
}

void OnConfConf_Dev9Conf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_DEV9configure, DEV9ConfS, Config.DEV9, "DEV9configure");
}

void OnConfConf_Dev9Test(GtkButton *button, gpointer user_data) {
	TestPlugin(_DEV9test, DEV9ConfS, Config.DEV9, "DEV9test");
}

void OnConfConf_Dev9About(GtkButton *button, gpointer user_data) {
	ConfPlugin(_DEV9about, DEV9ConfS, Config.DEV9, "DEV9about");
}

void OnConfConf_UsbConf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_USBconfigure, USBConfS, Config.USB, "USBconfigure");
}

void OnConfConf_UsbTest(GtkButton *button, gpointer user_data) {
	TestPlugin(_USBtest, USBConfS, Config.USB, "USBtest");
}

void OnConfConf_UsbAbout(GtkButton *button, gpointer user_data) {
	ConfPlugin(_USBabout, USBConfS, Config.USB, "USBabout");
}

void OnConfConf_FWConf(GtkButton *button, gpointer user_data) {
	ConfPlugin(_FWconfigure, FWConfS, Config.FW, "FWconfigure");
}

void OnConfConf_FWTest(GtkButton *button, gpointer user_data) {
	TestPlugin(_FWtest, FWConfS, Config.FW, "FWtest");
}

void OnConfConf_FWAbout(GtkButton *button, gpointer user_data) {
	ConfPlugin(_FWabout, FWConfS, Config.FW, "FWabout");
}

#define ConfCreatePConf(name, type) \
	type##ConfS.Combo = lookup_widget(ConfDlg, "GtkCombo_" name); \
    gtk_combo_set_popdown_strings(GTK_COMBO(type##ConfS.Combo), type##ConfS.glist); \
	FindComboText(type##ConfS.Combo, type##ConfS.plist, Config.type); \

void UpdateConfDlg() {
	FindPlugins();

	ConfCreatePConf("Gs", GS);
	ConfCreatePConf("Pad1", PAD1);
	ConfCreatePConf("Pad2", PAD2);
	ConfCreatePConf("Spu2", SPU2);
	ConfCreatePConf("Cdvd", CDVD);
	ConfCreatePConf("Dev9", DEV9);
	ConfCreatePConf("Usb",  USB);
	ConfCreatePConf("FW",  FW);
	ConfCreatePConf("Bios", Bios);
}

void OnPluginsPath_Ok() {
	gchar *File;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(Config.PluginsDir, File);
	if (Config.PluginsDir[strlen(Config.PluginsDir)-1] != '/')
		strcat(Config.PluginsDir, "/");

	UpdateConfDlg();

	gtk_widget_destroy(FileSel);
}

void OnPluginsPath_Cancel() {
	gtk_widget_destroy(FileSel);
}

void OnConfConf_PluginsPath(GtkButton *button, gpointer user_data) {
	GtkWidget *Ok,*Cancel;

	FileSel = gtk_file_selection_new(_("Select Plugins Directory"));

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnPluginsPath_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect (GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnPluginsPath_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}

void OnBiosPath_Ok() {
	gchar *File;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(Config.BiosDir, File);
	if (Config.BiosDir[strlen(Config.BiosDir)-1] != '/')
		strcat(Config.BiosDir, "/");

	UpdateConfDlg();

	gtk_widget_destroy(FileSel);
}

void OnBiosPath_Cancel() {
	gtk_widget_destroy(FileSel);
}

void OnConfConf_BiosPath(GtkButton *button, gpointer user_data) {
	GtkWidget *Ok,*Cancel;

	FileSel = gtk_file_selection_new(_("Select Bios Directory"));

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnBiosPath_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect (GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnBiosPath_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}

void OnConf_Conf(GtkMenuItem *menuitem, gpointer user_data) {
	FindPlugins();

	ConfDlg = create_ConfDlg();
	gtk_window_set_title(GTK_WINDOW(ConfDlg), _("Configuration"));

	UpdateConfDlg();

	gtk_widget_show_all(ConfDlg);
	if (Window) gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}

GtkWidget *CmdLine;
GtkWidget *ListDV;
GtkListStore *ListDVModel;
GtkWidget *SetPCDlg, *SetPCEntry;
GtkWidget *SetBPADlg, *SetBPAEntry;
GtkWidget *SetBPCDlg, *SetBPCEntry;
GtkWidget *DumpCDlg, *DumpCTEntry, *DumpCFEntry;
GtkWidget *DumpRDlg, *DumpRTEntry, *DumpRFEntry;
GtkWidget *MemWriteDlg, *MemEntry, *DataEntry;
GtkAdjustment *DebugAdj;
static u32 dPC;
static u32 dBPA = -1;
static u32 dBPC = -1;
static char nullAddr[256];
int DebugMode; // 0 - EE | 1 - IOP

#include "R3000A.h"
#include "PsxMem.h"

void UpdateDebugger() {

	char *str;
	int i;
	DebugAdj->value = (gfloat)dPC/4;
	gtk_list_store_clear(ListDVModel);

	for (i=0; i<23; i++) {
		GtkTreeIter iter;
		u32 *mem;
		u32 pc = dPC + i*4;
		if (DebugMode) {
			mem = (u32*)PSXM(pc);
		} else
			mem = PSM(pc);
		if (mem == NULL) { sprintf(nullAddr, "%8.8lX:\tNULL MEMORY", pc); str = nullAddr; }
		else str = disR5900Fasm(*mem, pc);
		gtk_list_store_append(ListDVModel, &iter);
		gtk_list_store_set(ListDVModel, &iter, 0, str, -1);

	}
}

void OnDebug_Close(GtkButton *button, gpointer user_data) {
	ClosePlugins();
	gtk_widget_destroy(DebugWnd);
	gtk_main_quit();
	gtk_widget_set_sensitive(Window, TRUE);
}

void OnDebug_ScrollChange(GtkAdjustment *adj) {
	dPC = (u32)adj->value*4;
	dPC&= ~0x3;
	
	UpdateDebugger();
}

void OnSetPC_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetPCEntry));

	sscanf(str, "%lx", &dPC);
	dPC&= ~0x3;

	gtk_widget_destroy(SetPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetPC_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetPC(GtkButton *button, gpointer user_data) {
	SetPCDlg = create_SetPCDlg();
	
	SetPCEntry = lookup_widget(SetPCDlg, "GtkEntry_dPC");

	gtk_widget_show_all(SetPCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnSetBPA_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetBPAEntry));

	sscanf(str, "%lx", &dBPA);
	dBPA&= ~0x3;

	gtk_widget_destroy(SetBPADlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetBPA_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetBPADlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetBPA(GtkButton *button, gpointer user_data) {
	SetBPADlg = create_SetBPADlg();
	
	SetBPAEntry = lookup_widget(SetBPADlg, "GtkEntry_BPA");

	gtk_widget_show_all(SetBPADlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnSetBPC_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetBPCEntry));

	sscanf(str, "%lx", &dBPC);

	gtk_widget_destroy(SetBPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetBPC_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetBPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetBPC(GtkButton *button, gpointer user_data) {
	SetBPCDlg = create_SetBPCDlg();
	
	SetBPCEntry = lookup_widget(SetBPCDlg, "GtkEntry_BPC");

	gtk_widget_show_all(SetBPCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDebug_ClearBPs(GtkButton *button, gpointer user_data) {
	dBPA = -1;
	dBPC = -1;
}

void OnDumpC_Ok(GtkButton *button, gpointer user_data) {
	FILE *f;
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpCFEntry));
	u32 addrf, addrt;

	sscanf(str, "%lx", &addrf); addrf&=~0x3;
	str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpCTEntry));
	sscanf(str, "%lx", &addrt); addrt&=~0x3;

	f = fopen("dump.txt", "w");
	if (f == NULL) return;

	while (addrf != addrt) {
		u32 *mem;

		if (DebugMode) {
			mem = PSXM(addrf);
		} else {
			mem = PSM(addrf);
		}
		if (mem == NULL) { sprintf(nullAddr, "%8.8lX:\tNULL MEMORY", addrf); str = nullAddr; }
		else str = disR5900Fasm(*mem, addrf);

		fprintf(f, "%s\n", str);
		addrf+= 4;
	}

	fclose(f);

	gtk_widget_destroy(DumpCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDumpC_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(DumpCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_DumpCode(GtkButton *button, gpointer user_data) {
	DumpCDlg = create_DumpCDlg();
	
	DumpCFEntry = lookup_widget(DumpCDlg, "GtkEntry_DumpCF");
	DumpCTEntry = lookup_widget(DumpCDlg, "GtkEntry_DumpCT");

	gtk_widget_show_all(DumpCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDumpR_Ok(GtkButton *button, gpointer user_data) {
	FILE *f;
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpRFEntry));
	u32 addrf, addrt;

	sscanf(str, "%lx", &addrf); addrf&=~0x3;
	str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpRTEntry));
	sscanf(str, "%lx", &addrt); addrt&=~0x3;

	f = fopen("dump.txt", "w");
	if (f == NULL) return;

	while (addrf != addrt) {
		u32 *mem;
		u32 out;

		if (DebugMode) {
			mem = PSXM(addrf);
		} else {
			mem = PSM(addrf);
		}
		if (mem == NULL) out = 0;
		else out = *mem;

		fwrite(&out, 4, 1, f);
		addrf+= 4;
	}

	fclose(f);

	gtk_widget_destroy(DumpRDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDumpR_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(DumpRDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_RawDump(GtkButton *button, gpointer user_data) {
	DumpRDlg = create_DumpRDlg();
	
	DumpRFEntry = lookup_widget(DumpRDlg, "GtkEntry_DumpRF");
	DumpRTEntry = lookup_widget(DumpRDlg, "GtkEntry_DumpRT");

	gtk_widget_show_all(DumpRDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDebug_Step(GtkButton *button, gpointer user_data) {
	Cpu->Step();
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_Skip(GtkButton *button, gpointer user_data) {
	cpuRegs.pc+= 4;
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

int HasBreakPoint(u32 pc) {
	if (pc == dBPA) return 1;
	if (DebugMode == 0) {
		if ((cpuRegs.cycle - 10) <= dBPC &&
			(cpuRegs.cycle + 10) >= dBPC) return 1;
	} else {
		if ((psxRegs.cycle - 100) <= dBPC &&
			(psxRegs.cycle + 100) >= dBPC) return 1;
	}
	return 0;
}

void OnDebug_Go(GtkButton *button, gpointer user_data) {
	for (;;) {
		if (HasBreakPoint(cpuRegs.pc)) break;
		Cpu->Step();
	}
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_Log(GtkButton *button, gpointer user_data) {
#ifdef PCSX2_DEVBUILD
	Log = 1 - Log;
#endif
}

void OnDebug_EEMode(GtkToggleButton *togglebutton, gpointer user_data) {
	DebugMode = 0;
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_IOPMode(GtkToggleButton *togglebutton, gpointer user_data) {
	DebugMode = 1;
	dPC = psxRegs.pc;
	UpdateDebugger();
}

void OnMemWrite32_Ok(GtkButton *button, gpointer user_data) {
	char *mem  = (char*)gtk_entry_get_text(GTK_ENTRY(MemEntry));
	char *data = (char*)gtk_entry_get_text(GTK_ENTRY(DataEntry));

	printf("memWrite32: %s, %s\n", mem, data);
	memWrite32(strtol(mem, (char**)NULL, 0), strtol(data, (char**)NULL, 0));

	gtk_widget_destroy(MemWriteDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnMemWrite32_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(MemWriteDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_memWrite32(GtkButton *button, gpointer user_data) {
	MemWriteDlg = create_MemWrite32();

	MemEntry = lookup_widget(MemWriteDlg, "GtkEntry_Mem");
	DataEntry = lookup_widget(MemWriteDlg, "GtkEntry_Data");

	gtk_widget_show_all(MemWriteDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();

	UpdateDebugger();
}

void OnDebug_Debugger(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *scroll;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

	if (OpenPlugins(NULL) == -1) return;
	if (needReset) { SysReset(); needReset = 0; }

	if (!efile)
		efile=GetPS2ElfName(elfname);
	if (efile)
		loadElfFile(elfname);
	efile=0;

	dPC = cpuRegs.pc;

	DebugWnd = create_DebugWnd();

	ListDVModel = gtk_list_store_new (1, G_TYPE_STRING);
	ListDV = lookup_widget(DebugWnd, "GtkList_DisView");
	gtk_tree_view_set_model(GTK_TREE_VIEW(ListDV), GTK_TREE_MODEL(ListDVModel));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("heading", renderer,
	                                                   "text", 0,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ListDV), column);

	scroll = lookup_widget(DebugWnd, "GtkVScrollbar_VList");

	DebugAdj = GTK_RANGE(scroll)->adjustment;
	DebugAdj->lower = (gfloat)0x00000000/4;
	DebugAdj->upper = (gfloat)0xffffffff/4;
	DebugAdj->step_increment = (gfloat)1;
	DebugAdj->page_increment = (gfloat)20;
	DebugAdj->page_size = (gfloat)23;

	gtk_signal_connect(GTK_OBJECT(DebugAdj),
	                   "value_changed", GTK_SIGNAL_FUNC(OnDebug_ScrollChange),
					   NULL);

	UpdateDebugger();

	gtk_widget_show_all(DebugWnd);
	gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}

void OnLogging_Ok(GtkButton *button, gpointer user_data) {
	GtkWidget *Btn;
	char str[32];
	int i, ret;

#ifdef PCSX2_DEVBUILD
	for (i=0; i<17; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		ret = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));
		if (ret) varLog|= 1<<i;
		else varLog&=~(1<<i);
	}

	for (i=20; i<29; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		ret = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));
		if (ret) varLog|= 1<<i;
		else varLog&=~(1<<i);
	}

	for (i=30; i<32; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		ret = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));
		if (ret) varLog|= 1<<i;
		else varLog&=~(1<<i);
	}

	Btn = lookup_widget(LogDlg, "Log");
	Log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));

    SaveConfig();
#endif

	gtk_widget_destroy(LogDlg);
	gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnLogging_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(LogDlg);
	gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnDebug_Logging(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *Btn;
	char str[32];
	int i;

	LogDlg = create_Logging();

	for (i=0; i<17; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), varLog & (1<<i));
	}

	for (i=20; i<29; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), varLog & (1<<i));
	}

	for (i=30; i<32; i++) {
		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), varLog & (1<<i));
	}

	Btn = lookup_widget(LogDlg, "Log");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), Log);

	gtk_widget_show_all(LogDlg);
	gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}

void OnHelp_Help() {
}

void OnHelpAbout_Ok(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(AboutDlg);
	gtk_widget_set_sensitive(Window, TRUE);
	gtk_main_quit();
}

void OnHelp_About(GtkMenuItem *menuitem, gpointer user_data) {
	char str[256];
	GtkWidget *Label;

	AboutDlg = create_AboutDlg();
	gtk_window_set_title(GTK_WINDOW(AboutDlg), _("About"));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelVersion");
	sprintf(str, _("PCSX2 For Linux\nVersion %s"), PCSX2_VERSION);
	gtk_label_set_text(GTK_LABEL(Label), str);

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelAuthors");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelAuthors));

	Label = lookup_widget(AboutDlg, "GtkAbout_LabelGreets");
	gtk_label_set_text(GTK_LABEL(Label), _(LabelGreets));

	gtk_widget_show_all(AboutDlg);
	gtk_widget_set_sensitive(Window, FALSE);
	gtk_main();
}

#define ComboAddPlugin(type) { \
    sprintf (name, "%s %ld.%ld.%ld", PS2EgetLibName(), (version>>8)&0xff ,version&0xff, (version>>24)&0xff); \
	type##ConfS.plugins+=2; \
	strcpy(type##ConfS.plist[type##ConfS.plugins-1], name); \
	strcpy(type##ConfS.plist[type##ConfS.plugins-2], ent->d_name); \
	type##ConfS.glist = g_list_append(type##ConfS.glist, type##ConfS.plist[type##ConfS.plugins-1]); \
}

void FindPlugins() {
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char plugin[256],name[256];

	GSConfS.plugins  = 0;  CDVDConfS.plugins = 0; DEV9ConfS.plugins = 0;
	PAD1ConfS.plugins = 0; PAD2ConfS.plugins = 0; SPU2ConfS.plugins = 0;
	USBConfS.plugins = 0;  FWConfS.plugins = 0;  BiosConfS.plugins = 0;
	GSConfS.glist  = NULL;  CDVDConfS.glist = NULL; DEV9ConfS.glist = NULL;
	PAD1ConfS.glist = NULL; PAD2ConfS.glist = NULL; SPU2ConfS.glist = NULL;
	USBConfS.glist = NULL;  FWConfS.glist = NULL;  BiosConfS.glist = NULL;

	dir = opendir(Config.PluginsDir);
	if (dir == NULL) {
		SysMessage(_("Could not open '%s' directory"), Config.PluginsDir);
		return;
	}
	while ((ent = readdir(dir)) != NULL) {
		u32 version;
		u32 type;

		sprintf (plugin, "%s%s", Config.PluginsDir, ent->d_name);

		if (strstr(plugin, ".so") == NULL) continue;
		Handle = dlopen(plugin, RTLD_NOW);
		if (Handle == NULL) {
			printf("%s\n", dlerror()); continue;
		}

		PS2EgetLibType = (_PS2EgetLibType) dlsym(Handle, "PS2EgetLibType");
		PS2EgetLibName = (_PS2EgetLibName) dlsym(Handle, "PS2EgetLibName");
		PS2EgetLibVersion2 = (_PS2EgetLibVersion2) dlsym(Handle, "PS2EgetLibVersion2");
		if (PS2EgetLibType == NULL || PS2EgetLibName == NULL || PS2EgetLibVersion2 == NULL)
            continue;
		type = PS2EgetLibType();

		if (type & PS2E_LT_GS) {
			version = PS2EgetLibVersion2(PS2E_LT_GS);
			if (((version >> 16)&0xff) == PS2E_GS_VERSION) {
				ComboAddPlugin(GS);
			}
            else
                SysPrintf("Plugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_GS_VERSION);
		}
		if (type & PS2E_LT_PAD) {
			_PADquery query;

			query = (_PADquery)dlsym(Handle, "PADquery");
			version = PS2EgetLibVersion2(PS2E_LT_PAD);
			if (((version >> 16)&0xff) == PS2E_PAD_VERSION && query) {
				if (query() & 0x1)
					ComboAddPlugin(PAD1);
				if (query() & 0x2)
					ComboAddPlugin(PAD2);
			} else SysPrintf("Plugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_PAD_VERSION);
		}
		if (type & PS2E_LT_SPU2) {
			version = PS2EgetLibVersion2(PS2E_LT_SPU2);
			if (((version >> 16)&0xff) == PS2E_SPU2_VERSION) {
				ComboAddPlugin(SPU2);
			} else SysPrintf("Plugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_SPU2_VERSION);
		}
		if (type & PS2E_LT_CDVD) {
			version = PS2EgetLibVersion2(PS2E_LT_CDVD);
			if (((version >> 16)&0xff) == PS2E_CDVD_VERSION) {
				ComboAddPlugin(CDVD);
			} else SysPrintf("Plugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_CDVD_VERSION);
		}
		if (type & PS2E_LT_DEV9) {
			version = PS2EgetLibVersion2(PS2E_LT_DEV9);
			if (((version >> 16)&0xff) == PS2E_DEV9_VERSION) {
				ComboAddPlugin(DEV9);
			} else SysPrintf("DEV9Plugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_DEV9_VERSION);
		}
		if (type & PS2E_LT_USB) {
			version = PS2EgetLibVersion2(PS2E_LT_USB);
			if (((version >> 16)&0xff) == PS2E_USB_VERSION) {
				ComboAddPlugin(USB);
			} else SysPrintf("USBPlugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_USB_VERSION);
		}
		if (type & PS2E_LT_FW) {
			version = PS2EgetLibVersion2(PS2E_LT_FW);
			if (((version >> 16)&0xff) == PS2E_FW_VERSION) {
				ComboAddPlugin(FW);
			} else SysPrintf("FWPlugin %s: Version %x != %x\n", plugin, (version >> 16)&0xff, PS2E_FW_VERSION);
		}
	}
	closedir(dir);

	dir = opendir(Config.BiosDir);
	if (dir == NULL) {
		SysMessage(_("Could not open '%s' directory"), Config.BiosDir);
		return;
	}

	while ((ent = readdir(dir)) != NULL) {
		struct stat buf;
		char description[50];				//2002-09-28 (Florin)

		sprintf (plugin, "%s%s", Config.BiosDir, ent->d_name);
		if (stat(plugin, &buf) == -1) continue;
//		if (buf.st_size < (1024*512)) continue;
		if (buf.st_size > (1024*4096)) continue;	//2002-09-28 (Florin)
		if (!IsBIOS(ent->d_name, description)) continue;//2002-09-28 (Florin)

		BiosConfS.plugins+=2;
        snprintf(BiosConfS.plist[BiosConfS.plugins-1], sizeof(BiosConfS.plist[0]), "%s (", description);
        strncat(BiosConfS.plist[BiosConfS.plugins-1], ent->d_name, min(sizeof(BiosConfS.plist[0]-2), strlen(ent->d_name)));
        strcat(BiosConfS.plist[BiosConfS.plugins-1], ")");
		strcpy(BiosConfS.plist[BiosConfS.plugins-2], ent->d_name);
		BiosConfS.glist = g_list_append(BiosConfS.glist, BiosConfS.plist[BiosConfS.plugins-1]);
	}
	closedir(dir);
}

GtkWidget *MsgDlg;

void OnMsg_Ok() {
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessage(char *fmt, ...) {
	GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	if (!UseGui) { printf("%s\n",msg); return; }

	MsgDlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), _("PCSX2 Msg"));
	gtk_container_set_border_width(GTK_CONTAINER(MsgDlg), 5);

	Box = gtk_vbox_new(5, 0);
	gtk_container_add(GTK_CONTAINER(MsgDlg), Box);
	gtk_widget_show(Box);

	Txt = gtk_label_new(msg);
	
	gtk_box_pack_start(GTK_BOX(Box), Txt, FALSE, FALSE, 5);
	gtk_widget_show(Txt);

	Box1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), Box1, FALSE, FALSE, 0);
	gtk_widget_show(Box1);

	Ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);
    gtk_widget_grab_focus(Ok);

	gtk_widget_show(MsgDlg);

	gtk_main();
}

void
on_patch_browser1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}

void
on_patch_finder2_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}

void
on_enable_console1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    Config.PsxOut=(int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
    SaveConfig();
}

void
on_enable_patches1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    Config.Patch=(int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
    SaveConfig();
}
