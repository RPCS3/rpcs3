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

#include "ConfigDlg.h"

using namespace std;
using namespace R5900;

bool applychanges = FALSE;

static void FindComboText(GtkWidget *combo, char plist[255][255], GList *list, char *conf)
{
	if (strlen(conf) > 0) SetActiveComboItem(GTK_COMBO_BOX(combo), plist, list, conf);
}


static bool GetComboText(GtkWidget *combo, char plist[255][255], char *conf)
{
	int i;

	char *tmp = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));

	if (tmp == NULL) return FALSE;
	for (i = 2;i < 255;i += 2)
	{
		if (!strcmp(tmp, plist[i-1]))
		{
			strcpy(conf, plist[i-2]);
			break;
		}
	}
	return TRUE;
}

static void ConfPlugin(PluginConf confs, char* plugin, const char* name)
{
	void *drv;
	void (*conf)();
	char file[g_MaxPath], file2[g_MaxPath];

	GetComboText(confs.Combo, confs.plist, plugin);
	strcpy(file, Config.PluginsDir);
	strcat(file, plugin);
	
	drv = SysLoadLibrary(file);
//#ifndef LOCAL_PLUGIN_INIS
//	chdir(Config.PluginsDir); /* change dirs so that plugins can find their config file*/
//#endif
	if (drv == NULL) return;

	conf = (void (*)()) SysLoadSym(drv, name);
	if (SysLibError() == NULL) conf();
//#ifndef LOCAL_PLUGIN_INIS
//	chdir(MAIN_DIR); /* change back*/
//#endif
	SysCloseLibrary(drv);
}


static void TestPlugin(PluginConf confs, char* plugin, const char* name)
{
	void *drv;
	s32(* (*conf)())();
	char file[g_MaxPath];
	int ret = 0;

	GetComboText(confs.Combo, confs.plist, plugin);
	strcpy(file, Config.PluginsDir);
	strcat(file, plugin);

	drv = SysLoadLibrary(file);
	if (drv == NULL) return;

	conf = (s32(* (*)())()) SysLoadSym(drv, name);
	if (SysLibError() == NULL) ret = (s32) conf();
	SysCloseLibrary(drv);

	if (ret == 0)
		Msgbox::Alert("This plugin reports that should work correctly");
	else
		Msgbox::Alert("This plugin reports that should not work correctly");
}

void OnConf_Gs(GtkMenuItem *menuitem, gpointer user_data)
{
	char file[255];

//	chdir(Config.PluginsDir);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	GSconfigure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Pads(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	PAD1configure();
	if (strcmp(Config.PAD1, Config.PAD2)) PAD2configure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Spu2(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	SPU2configure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Cdvd(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	CDVDconfigure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Dev9(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	DEV9configure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Usb(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	USBconfigure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnConf_Fw(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_set_sensitive(MainWindow, FALSE);
	FWconfigure();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void SetActiveComboItem(GtkComboBox *widget, char plist[255][255], GList *list, char *conf)
{
	GList *temp;
	int i = 0, pindex = 0, item = -1;

	if (strlen(conf) > 0)
	{
		for (i = 2;i < 255;i += 2)
		{
			if (!strcmp(conf, plist[i-2]))
			{
				pindex = i - 1;
				break;
			}
		}
	}

	i = 0;
	temp = list;

	while (temp)
	{
		if (!strcmp(plist[pindex], (char*)temp->data))
			item = i;

		temp = temp->next;
		i++;
	}

	if (item <= 0) item = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), item);
}

void OnConfConf_Ok(GtkButton *button, gpointer user_data)
{
	applychanges = TRUE;

	if (!GetComboText(GSConfS.Combo, GSConfS.plist, Config.GS))
		applychanges = FALSE;
	if (!GetComboText(PAD1ConfS.Combo, PAD1ConfS.plist, Config.PAD1))
		applychanges = FALSE;
	if (!GetComboText(PAD2ConfS.Combo, PAD2ConfS.plist, Config.PAD2))
		applychanges = FALSE;
	if (!GetComboText(SPU2ConfS.Combo, SPU2ConfS.plist, Config.SPU2))
		applychanges = FALSE;
	if (!GetComboText(CDVDConfS.Combo, CDVDConfS.plist, Config.CDVD))
		applychanges = FALSE;
	if (!GetComboText(DEV9ConfS.Combo, DEV9ConfS.plist, Config.DEV9))
		applychanges = FALSE;
	if (!GetComboText(USBConfS.Combo,  USBConfS.plist,  Config.USB))
		applychanges = FALSE;
	if (!GetComboText(FWConfS.Combo,   FWConfS.plist,   Config.FW))
		applychanges = FALSE;
	if (!GetComboText(BiosConfS.Combo, BiosConfS.plist, Config.Bios))
		applychanges = FALSE;

	SaveConfig();

	if (configuringplug == FALSE)
	{
		ReleasePlugins();
		LoadPlugins();
	}

	gtk_widget_destroy(ConfDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void OnConfConf_GsConf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(GSConfS, Config.GS, "GSconfigure");
}

void OnConfConf_GsTest(GtkButton *button, gpointer user_data)
{
	TestPlugin(GSConfS, Config.GS, "GStest");
}

void OnConfConf_GsAbout(GtkButton *button, gpointer user_data)
{
	ConfPlugin(GSConfS, Config.GS, "GSabout");
}

void OnConfConf_Pad1Conf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(PAD1ConfS, Config.PAD1, "PADconfigure");
}

void OnConfConf_Pad1Test(GtkButton *button, gpointer user_data)
{
	TestPlugin(PAD1ConfS, Config.PAD1, "PADtest");
}

void OnConfConf_Pad1About(GtkButton *button, gpointer user_data)
{
	ConfPlugin(PAD1ConfS, Config.PAD1, "PADabout");
}

void OnConfConf_Pad2Conf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(PAD2ConfS, Config.PAD2, "PADconfigure");
}

void OnConfConf_Pad2Test(GtkButton *button, gpointer user_data)
{
	TestPlugin(PAD2ConfS, Config.PAD2, "PADtest");
}

void OnConfConf_Pad2About(GtkButton *button, gpointer user_data)
{
	ConfPlugin(PAD2ConfS, Config.PAD2, "PADabout");
}

void OnConfConf_Spu2Conf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(SPU2ConfS, Config.SPU2, "SPU2configure");
}

void OnConfConf_Spu2Test(GtkButton *button, gpointer user_data)
{
	TestPlugin(SPU2ConfS, Config.SPU2, "SPU2test");
}

void OnConfConf_Spu2About(GtkButton *button, gpointer user_data)
{
	ConfPlugin(SPU2ConfS, Config.SPU2, "SPU2about");
}

void OnConfConf_CdvdConf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(CDVDConfS, Config.CDVD, "CDVDconfigure");
}

void OnConfConf_CdvdTest(GtkButton *button, gpointer user_data)
{
	TestPlugin(CDVDConfS, Config.CDVD, "CDVDtest");
}

void OnConfConf_CdvdAbout(GtkButton *button, gpointer user_data)
{
	ConfPlugin(CDVDConfS, Config.CDVD, "CDVDabout");
}

void OnConfConf_Dev9Conf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(DEV9ConfS, Config.DEV9, "DEV9configure");
}

void OnConfConf_Dev9Test(GtkButton *button, gpointer user_data)
{
	TestPlugin(DEV9ConfS, Config.DEV9, "DEV9test");
}

void OnConfConf_Dev9About(GtkButton *button, gpointer user_data)
{
	ConfPlugin(DEV9ConfS, Config.DEV9, "DEV9about");
}

void OnConfConf_UsbConf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(USBConfS, Config.USB, "USBconfigure");
}

void OnConfConf_UsbTest(GtkButton *button, gpointer user_data)
{
	TestPlugin(USBConfS, Config.USB, "USBtest");
}

void OnConfConf_UsbAbout(GtkButton *button, gpointer user_data)
{
	ConfPlugin(USBConfS, Config.USB, "USBabout");
}

void OnConfConf_FWConf(GtkButton *button, gpointer user_data)
{
	ConfPlugin(FWConfS, Config.FW, "FWconfigure");
}

void OnConfConf_FWTest(GtkButton *button, gpointer user_data)
{
	TestPlugin(FWConfS, Config.FW, "FWtest");
}

void OnConfConf_FWAbout(GtkButton *button, gpointer user_data)
{
	ConfPlugin(FWConfS, Config.FW, "FWabout");
}


void SetComboToGList(GtkComboBox *widget, GList *list)
{
	GList *temp;

	while (gtk_combo_box_get_active_text(widget) != NULL)
	{
		gtk_combo_box_remove_text(GTK_COMBO_BOX(widget), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	}

	temp = list;
	while (temp != NULL)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), (char*)temp->data);

		temp = temp->next;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
}

static void ConfCreatePConf(const char *name, PluginConf *confs, char *config)
{
	char tmp[50];

	sprintf(tmp, "GtkCombo_%s", name);
	confs->Combo = lookup_widget(ConfDlg, tmp);
	SetComboToGList(GTK_COMBO_BOX(confs->Combo), confs->PluginNameList);
	FindComboText(confs->Combo, confs->plist, confs->PluginNameList, config);
}

void UpdateConfDlg()
{
	FindPlugins();

	ConfCreatePConf("Gs", &GSConfS, Config.GS);
	ConfCreatePConf("Pad1", &PAD1ConfS, Config.PAD1);
	ConfCreatePConf("Pad2", &PAD2ConfS, Config.PAD2);
	ConfCreatePConf("Spu2", &SPU2ConfS, Config.SPU2);
	ConfCreatePConf("Cdvd", &CDVDConfS, Config.CDVD);
	ConfCreatePConf("Dev9", &DEV9ConfS, Config.DEV9);
	ConfCreatePConf("Usb",  &USBConfS, Config.USB);
	ConfCreatePConf("FW",  &FWConfS, Config.FW);
	ConfCreatePConf("Bios", &BiosConfS, Config.Bios);
}

void GetDirectory(GtkWidget *topWindow, const char *message, char *reply)
{
	gchar *File;
	GtkWidget *dialog;
	gint result;

	dialog = gtk_file_chooser_dialog_new(message, GTK_WINDOW(topWindow), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (result)
	{
		case(GTK_RESPONSE_OK):
			File = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			strcpy(reply, File);
			if (reply[strlen(reply)-1] != '/')
				strcat(reply, "/");

		default:
			gtk_widget_destroy(dialog);
	}
}

void OnConfConf_PluginsPath(GtkButton *button, gpointer user_data)
{
	char reply[g_MaxPath];

	GetDirectory(ConfDlg, "Choose the Plugin Directory:", reply);
	strcpy(Config.PluginsDir, reply);

	UpdateConfDlg();
}

void OnConfConf_BiosPath(GtkButton *button, gpointer user_data)
{
	char reply[g_MaxPath];

	GetDirectory(ConfDlg, "Choose the Bios Directory:", reply);
	strcpy(Config.BiosDir, reply);

	UpdateConfDlg();
}

void OnConf_Conf(GtkMenuItem *menuitem, gpointer user_data)
{
	FindPlugins();

	ConfDlg = create_ConfDlg();
	gtk_window_set_title(GTK_WINDOW(ConfDlg), "Configuration");

	UpdateConfDlg();

	gtk_widget_show_all(ConfDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

static void ComboAddPlugin(char name[g_MaxPath], PluginConf *confs, u32 version, struct dirent *ent)
{
	sprintf(name, "%s %ld.%ld.%ld", PS2EgetLibName(), (version >> 8)&0xff , version&0xff, (version >> 24)&0xff);
	confs->plugins += 2;
	strcpy(confs->plist[confs->plugins-1], name);
	strcpy(confs->plist[confs->plugins-2], ent->d_name);
	confs->PluginNameList = g_list_append(confs->PluginNameList, confs->plist[confs->plugins-1]);
}

void FindPlugins()
{
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char plugin[g_MaxPath], name[g_MaxPath];

	GSConfS.plugins  = 0; CDVDConfS.plugins = 0; DEV9ConfS.plugins = 0; 
	PAD1ConfS.plugins = 0; PAD2ConfS.plugins = 0; SPU2ConfS.plugins = 0; 
	USBConfS.plugins = 0; FWConfS.plugins = 0; BiosConfS.plugins = 0;
	GSConfS.PluginNameList  = NULL; CDVDConfS.PluginNameList = NULL; DEV9ConfS.PluginNameList = NULL;
	PAD1ConfS.PluginNameList = NULL; PAD2ConfS.PluginNameList = NULL; SPU2ConfS.PluginNameList = NULL;
	USBConfS.PluginNameList = NULL; FWConfS.PluginNameList = NULL; BiosConfS.PluginNameList = NULL;

	dir = opendir(Config.PluginsDir);
	if (dir == NULL)
	{
		Msgbox::Alert("Could not open '%s' directory", params Config.PluginsDir);
		return;
	}
	while ((ent = readdir(dir)) != NULL)
	{
		u32 version;
		u32 type;

		sprintf(plugin, "%s%s", Config.PluginsDir, ent->d_name);

		if (strstr(plugin, ".so") == NULL) continue;
		Handle = SysLoadLibrary(plugin);
		if (Handle == NULL)
		{
			Console::Error("Can't open %s: %s\n", params ent->d_name, SysLibError());
			continue;
		}
		
		PS2EgetLibType = (_PS2EgetLibType) SysLoadSym(Handle, "PS2EgetLibType");
		PS2EgetLibName = (_PS2EgetLibName) SysLoadSym(Handle, "PS2EgetLibName");
		PS2EgetLibVersion2 = (_PS2EgetLibVersion2) SysLoadSym(Handle, "PS2EgetLibVersion2");

		if ((PS2EgetLibType == NULL) || (PS2EgetLibName == NULL) || (PS2EgetLibVersion2 == NULL))
		{
			if (PS2EgetLibType == NULL) Console::Error("PS2EgetLibType==NULL for %s", params ent->d_name);
			if (PS2EgetLibName == NULL) Console::Error("PS2EgetLibName==NULL for %s", params ent->d_name);
			if (PS2EgetLibVersion2 == NULL) Console::Error("PS2EgetLibVersion2==NULL for %s", params ent->d_name);
			continue;
		}

		type = PS2EgetLibType();

		if (type & PS2E_LT_GS)
		{
			version = PS2EgetLibVersion2(PS2E_LT_GS);

			if (((version >> 16)&0xff) == PS2E_GS_VERSION)
				ComboAddPlugin(name, &GSConfS, version, ent);
			else
				Console::Notice("Plugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_GS_VERSION);
		}
		if (type & PS2E_LT_PAD)
		{
			_PADquery query;

			query = (_PADquery)dlsym(Handle, "PADquery");
			version = PS2EgetLibVersion2(PS2E_LT_PAD);

			if (((version >> 16)&0xff) == PS2E_PAD_VERSION && query)
			{
				if (query() & 0x1) ComboAddPlugin(name, &PAD1ConfS, version, ent);
				if (query() & 0x2) ComboAddPlugin(name, &PAD2ConfS, version, ent);
			}
			else
				Console::Notice("Plugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_PAD_VERSION);
		}
		if (type & PS2E_LT_SPU2)
		{
			version = PS2EgetLibVersion2(PS2E_LT_SPU2);

			if (((version >> 16)&0xff) == PS2E_SPU2_VERSION)
				ComboAddPlugin(name, &SPU2ConfS, version, ent);
			else
				Console::Notice("Plugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_SPU2_VERSION);
		}
		if (type & PS2E_LT_CDVD)
		{
			version = PS2EgetLibVersion2(PS2E_LT_CDVD);

			if (((version >> 16)&0xff) == PS2E_CDVD_VERSION)
				ComboAddPlugin(name, &CDVDConfS, version, ent);
			else
				Console::Notice("Plugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_CDVD_VERSION);
		}
		if (type & PS2E_LT_DEV9)
		{
			version = PS2EgetLibVersion2(PS2E_LT_DEV9);

			if (((version >> 16)&0xff) == PS2E_DEV9_VERSION)
				ComboAddPlugin(name, &DEV9ConfS, version, ent);
			else
				Console::Notice("DEV9Plugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_DEV9_VERSION);
		}
		if (type & PS2E_LT_USB)
		{
			version = PS2EgetLibVersion2(PS2E_LT_USB);

			if (((version >> 16)&0xff) == PS2E_USB_VERSION)
				ComboAddPlugin(name, &USBConfS, version, ent);
			else
				Console::Notice("USBPlugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_USB_VERSION);
		}
		if (type & PS2E_LT_FW)
		{
			version = PS2EgetLibVersion2(PS2E_LT_FW);

			if (((version >> 16)&0xff) == PS2E_FW_VERSION)
				ComboAddPlugin(name, &FWConfS, version, ent);
			else
				Console::Notice("FWPlugin %s: Version %x != %x", params plugin, (version >> 16)&0xff, PS2E_FW_VERSION);
		}
	}
	closedir(dir);

	dir = opendir(Config.BiosDir);
	if (dir == NULL)
	{
		Msgbox::Alert("Could not open '%s' directory", params Config.BiosDir);
		return;
	}

	while ((ent = readdir(dir)) != NULL)
	{
		struct stat buf;
		char description[50];				//2002-09-28 (Florin)

		sprintf(plugin, "%s%s", Config.BiosDir, ent->d_name);
		if (stat(plugin, &buf) == -1) continue;
		if (buf.st_size > (1024*4096)) continue;	//2002-09-28 (Florin)
		if (!IsBIOS(ent->d_name, description)) continue;//2002-09-28 (Florin)

		BiosConfS.plugins += 2;
		snprintf(BiosConfS.plist[BiosConfS.plugins-1], sizeof(BiosConfS.plist[0]), "%s (", description);
		strncat(BiosConfS.plist[BiosConfS.plugins-1], ent->d_name, min(sizeof(BiosConfS.plist[0] - 2), strlen(ent->d_name)));
		strcat(BiosConfS.plist[BiosConfS.plugins-1], ")");
		strcpy(BiosConfS.plist[BiosConfS.plugins-2], ent->d_name);
		BiosConfS.PluginNameList = g_list_append(BiosConfS.PluginNameList, BiosConfS.plist[BiosConfS.plugins-1]);
	}
	closedir(dir);
}
