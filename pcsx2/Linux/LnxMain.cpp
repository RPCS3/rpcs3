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

#include "LnxMain.h"

using namespace R5900;

DIR *dir;
GtkWidget *FileSel;
GtkWidget *MsgDlg;
bool configuringplug = FALSE;
const char* g_pRunGSState = NULL;

int efile = 0;
char elfname[g_MaxPath];
bool Slots[5] = { false, false, false, false, false };

#ifdef PCSX2_DEVBUILD
TESTRUNARGS g_TestRun;
#endif


int main(int argc, char *argv[])
{
	char *file = NULL;
	char elfname[g_MaxPath];

	efile = 0;
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, "Langs");
	textdomain(PACKAGE);
#endif

	printf("\n");
	mkdir(CONFIG_DIR, 0755);

	strcpy(cfgfile, CONFIG_DIR "/pcsx2.cfg");

#ifdef PCSX2_DEVBUILD
	memset(&g_TestRun, 0, sizeof(g_TestRun));
#endif
	
	if (!ParseCommandLine(argc, argv, file)) return 0;

#ifdef PCSX2_DEVBUILD
	g_TestRun.efile = efile;
	g_TestRun.ptitle = file;
#endif

	// make gtk thread safe if using MTGS
	if (CHECK_MULTIGS)
	{
		g_thread_init(NULL);
		gdk_threads_init();
	}

	if (UseGui)
	{
		gtk_init(NULL, NULL);
	}

	if (LoadConfig() == -1)
	{

		memset(&Config, 0, sizeof(Config));
		strcpy(Config.BiosDir,    DEFAULT_BIOS_DIR "/");
		strcpy(Config.PluginsDir, DEFAULT_PLUGINS_DIR "/");
		strcpy(Config.Mcd1, MEMCARDS_DIR "/" DEFAULT_MEMCARD1);
		strcpy(Config.Mcd2, MEMCARDS_DIR "/" DEFAULT_MEMCARD2);
		Config.Patch = 0;
		Config.PsxOut = 1;
		Config.Options = PCSX2_EEREC | PCSX2_VU0REC | PCSX2_VU1REC | PCSX2_FRAMELIMIT_LIMIT;
		Config.sseMXCSR = DEFAULT_sseMXCSR;
		Config.sseVUMXCSR = DEFAULT_sseVUMXCSR;
		Config.eeOptions = DEFAULT_eeOptions;
		Config.vuOptions = DEFAULT_vuOptions;

		Msgbox::Alert("Pcsx2 needs to be configured");
		Pcsx2Configure();

		LoadConfig();
	}

	InitLanguages();

	if (Config.PsxOut)
	{
		// output the help commands
		Console::WriteLn("\tF1 - save state");
		Console::WriteLn("\t(Shift +) F2 - cycle states");
		Console::WriteLn("\tF3 - load state");

#ifdef PCSX2_DEVBUILD
		Console::WriteLn("\tF10 - dump performance counters");
		Console::WriteLn("\tF11 - save GS state");
		Console::WriteLn("\tF12 - dump hardware registers");
#endif
	}

	if (!SysInit()) return 1;

#ifdef PCSX2_DEVBUILD
	if (g_pRunGSState)
	{
		LoadGSState(g_pRunGSState);
		SysClose();
		return 0;
	}
#endif
	
	if (UseGui && (file == NULL))
	{
		StartGui();
		return 0;
	}

	if (OpenPlugins(file) == -1) return -1;

	SysReset();

	cpuExecuteBios();
	if (file) strcpy(elfname, file);
	if (!efile) efile = GetPS2ElfName(elfname);
	loadElfFile(elfname);

	ExecuteCpu();

	return 0;
}

void InitLanguages()
{
	char *lang;
	int i = 1;

	if (Config.Lang[0] == 0)
	{
		strcpy(Config.Lang, "en");
	}

	langs = (_langs*)malloc(sizeof(_langs));
	strcpy(langs[0].lang, "en");
	dir = opendir(LANGS_DIR);

	while ((lang = GetLanguageNext()) != NULL)
	{
		langs = (_langs*)realloc(langs, sizeof(_langs) * (i + 1));
		strcpy(langs[i].lang, lang);
		i++;
	}

	CloseLanguages();
	langsMax = i;
}

char *GetLanguageNext()
{
	struct dirent *ent;

	if (dir == NULL) return NULL;
	for (;;)
	{
		ent = readdir(dir);
		if (ent == NULL) return NULL;

		if (!strcmp(ent->d_name, ".")) continue;
		if (!strcmp(ent->d_name, "..")) continue;
		break;
	}

	return ent->d_name;
}

void CloseLanguages()
{
	if (dir) closedir(dir);
}

void ChangeLanguage(char *lang)
{
	strcpy(Config.Lang, lang);
	SaveConfig();
}

void OnMsg_Ok()
{
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}


void On_Dialog_Cancelled(GtkButton* button, gpointer user_data)
{
	gtk_widget_destroy((GtkWidget*)gtk_widget_get_toplevel((GtkWidget*)button));
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void StartGui()
{
	GtkWidget *Menu;
	GtkWidget *Item;

	u32 i;

	add_pixmap_directory(".pixmaps");
	MainWindow = create_MainWindow();

	if (SVN_REV != 0)
		gtk_window_set_title(GTK_WINDOW(MainWindow), "PCSX2 "PCSX2_VERSION" "SVN_REV);
	else
		gtk_window_set_title(GTK_WINDOW(MainWindow), "PCSX2 "PCSX2_VERSION);

	// status bar
	pStatusBar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(lookup_widget(MainWindow, "status_box")), pStatusBar, TRUE, TRUE, 0);
	gtk_widget_show(pStatusBar);

	StatusBar_SetMsg( "F1 - save, F2 - next state, Shift+F2 - prev state, F3 - load, F8 - snapshot");

	// add all the languages
	Item = lookup_widget(MainWindow, "GtkMenuItem_Language");
	Menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(Item), Menu);

	for (i = 0; i < langsMax; i++)
	{
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
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "GtkMenuItem_EnterDebugger")), FALSE);
	//gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "GtkMenuItem_Memcards")), FALSE);
#ifndef PCSX2_DEVBUILD
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget(MainWindow, "GtkMenuItem_Logging")), FALSE);
#endif
	
	CheckSlots();
	
	gtk_widget_show_all(MainWindow);
	gtk_window_activate_focus(GTK_WINDOW(MainWindow));
	gtk_main();
}

void OnDestroy(GtkObject *object, gpointer user_data) {}

gboolean OnDelete(GtkWidget       *widget, GdkEvent *event, gpointer user_data)
{
	pcsx2_exit();
	return (FALSE);
}

int Pcsx2Configure()
{
	if (!UseGui) return 0;

	configuringplug = TRUE;
	MainWindow = NULL;
	OnConf_Conf(NULL, 0);
	configuringplug = FALSE;

	return applychanges;
}

void OnLanguage(GtkMenuItem *menuitem, gpointer user_data)
{
	ChangeLanguage(langs[(int)(uptr)user_data].lang);
	gtk_widget_destroy(MainWindow);
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();
	StartGui();
}

void OnFile_RunCD(GtkMenuItem *menuitem, gpointer user_data)
{
	safe_free(g_RecoveryState);
	ResetPlugins();
	RunExecute(NULL);
}

void OnRunElf_Ok(GtkButton* button, gpointer user_data)
{
	gchar *File;

	File = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(elfname, File);
	gtk_widget_destroy(FileSel);

	RunExecute(elfname);
}

void OnRunElf_Cancel(GtkButton* button, gpointer user_data)
{
	gtk_widget_destroy(FileSel);
}

void OnFile_LoadElf(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *Ok, *Cancel;

	FileSel = gtk_file_selection_new("Select Psx Elf File");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnRunElf_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect(GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnRunElf_Cancel), NULL);
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
	if (dir != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			sprintf(plugin, "%s%s", Config.PluginsDir, ent->d_name);

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

void SignalExit(int sig)
{
	ClosePlugins();
	pcsx2_exit();
}

void OnFile_Exit(GtkMenuItem *menuitem, gpointer user_data)
{
	pcsx2_exit();
}

void OnEmu_Run(GtkMenuItem *menuitem, gpointer user_data)
{
	if (g_EmulationInProgress)
		ExecuteCpu();
	else
		RunExecute(NULL, true);	// boots bios if no savestate is to be recovered

}

void OnEmu_Reset(GtkMenuItem *menuitem, gpointer user_data)
{
	SysReset();
}

void ResetMenuSlots()
{
	GtkWidget *Item;
	char str[g_MaxPath], str2[g_MaxPath];
	int i;

	for (i = 0; i < 5; i++)
	{
		
		sprintf(str, "load_slot_%d", i);
		sprintf(str2, "save_slot_%d", i);
		Item = lookup_widget(MainWindow, str);
		
		if GTK_IS_WIDGET(Item) 
			gtk_widget_set_sensitive(Item, Slots[i]);
		else
			Console::Error("No such widget: %s", params str);
		
		Item = lookup_widget(MainWindow, str2);
		gtk_widget_set_sensitive(Item, (ElfCRC != 0));
			
	}
}

void CheckSlots()
{
	int i = 0;
	
	if (ElfCRC == 0) Console::Notice("Disabling game slots until a game is loaded.");
	
	for (i=0; i<5; i++) 
	{
		if (isSlotUsed(i))
			Slots[i] = true;
		else
			Slots[i] = false;
	}
	ResetMenuSlots();
}

//2002-09-28 (Florin)
void OnArguments_Ok(GtkButton *button, gpointer user_data)
{
	char *str;

	str = (char*)gtk_entry_get_text(GTK_ENTRY(widgetCmdLine));
	memcpy(args, str, g_MaxPath);

	gtk_widget_destroy(CmdLine);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnEmu_Arguments(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *widgetCmdLine;

	CmdLine = create_CmdLine();
	gtk_window_set_title(GTK_WINDOW(CmdLine), _("Program arguments"));

	widgetCmdLine = lookup_widget(CmdLine, "GtkEntry_dCMDLINE");

	gtk_entry_set_text(GTK_ENTRY(widgetCmdLine), args);
	gtk_widget_show_all(CmdLine);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void OnLogging_Ok(GtkButton *button, gpointer user_data)
{
#ifdef PCSX2_DEVBUILD
	GtkWidget *Btn;
	char str[32];
	int i, ret;


	for (i = 0; i < 32; i++)
	{
		if (((i > 16) && (i < 20)) || (i == 29))
			continue;

		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		ret = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Btn));
		if (ret) varLog |= 1 << i;
		else varLog &= ~(1 << i);
	}

	SaveConfig();
#endif

	gtk_widget_destroy(LogDlg);
	gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnDebug_Logging(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *Btn;
	char str[32];
	int i;

	LogDlg = create_Logging();


	for (i = 0; i < 32; i++)
	{
		if (((i > 16) && (i < 20)) || (i == 29))
			continue;

		sprintf(str, "Log%d", i);
		Btn = lookup_widget(LogDlg, str);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Btn), varLog & (1 << i));
	}

	gtk_widget_show_all(LogDlg);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void on_patch_browser1_activate(GtkMenuItem *menuitem, gpointer user_data) {}

void on_patch_finder2_activate(GtkMenuItem *menuitem, gpointer user_data) {}

void on_enable_console1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	Config.PsxOut = (int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
	SaveConfig();
}

void on_enable_patches1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	Config.Patch = (int)gtk_check_menu_item_get_active((GtkCheckMenuItem*)menuitem);
	SaveConfig();
}

