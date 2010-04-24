/*  CDVDiso
 *  Copyright (C) 2002-2004  CDVDiso Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Config.h"

const char *s_strIniPath="./inis/CDVDiso.ini";
GtkWidget *AboutDlg, *ConfDlg, *MsgDlg, *FileSel;
GtkWidget *Edit, *CdEdit;

void LoadConf()
{
	FILE *f;
	char cfg[256];

	//sprintf(cfg, "%s/.PS2E/CDVDiso.cfg", getenv("HOME"));
	strcpy(cfg, s_strIniPath);
	f = fopen(cfg, "r");
	
	if (f == NULL)
	{
		printf("Unable to load %s\n", cfg);
		strcpy(IsoFile, DEV_DEF);
		strcpy(CdDev, CDDEV_DEF);
		BlockDump = 0;
		SaveConf();
		return;
	}
	
	fscanf(f, "IsoFile = %[^\n]\n", IsoFile);
	fscanf(f, "CdDev   = %[^\n]\n", CdDev);
	fscanf(f, "BlockDump   = %d\n", &BlockDump);
	
	if (!strncmp(IsoFile, "CdDev   =", 9)) *IsoFile = 0; // quick fix
	if (*CdDev == 0) strcpy(CdDev, CDDEV_DEF);
	
	fclose(f);
}

void SaveConf()
{
	FILE *f;
	char cfg[256];

	//sprintf(cfg, "%s/.PS2E", getenv("HOME"));
	
	//mkdir(cfg, 0755);
	//sprintf(cfg, "%s/.PS2E/CDVDiso.cfg", getenv("HOME"));
	strcpy(cfg, s_strIniPath);
	
	f = fopen(cfg, "w");
	if (f == NULL)
	{
		printf("Unable to save %s\n", cfg);
		return;
	}
	
	fprintf(f, "IsoFile = %s\n", IsoFile);
	fprintf(f, "CdDev   = %s\n", CdDev);
	fprintf(f, "BlockDump   = %d\n", BlockDump);
	fclose(f);
}

void SysMessage(char *fmt, ...)
{
	va_list list;
	char tmp[256];
	char cmd[256];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	sprintf(cmd, "message \"%s\"", tmp);
	SysMessageLoc(tmp);
}

void OnFile_Ok()
{
	gchar *File;
	
	gtk_widget_hide(FileSel);
	File = gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	strcpy(IsoFile, File);
	gtk_main_quit();
}

void OnFile_Cancel()
{
	gtk_widget_hide(FileSel);
	gtk_main_quit();
}

void CfgOpenFile()
{
	GtkWidget *Ok, *Cancel;
	
	FileSel = gtk_file_selection_new("Select Iso File");

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked",
	                   GTK_SIGNAL_FUNC(OnFile_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect(GTK_OBJECT(Cancel), "clicked",
	                   GTK_SIGNAL_FUNC(OnFile_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);

	gtk_main();

	SaveConf();
}

void OnMsg_Ok()
{
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessageLoc(char *fmt, ...)
{
	GtkWidget *Ok, *Txt;
	GtkWidget *Box, *Box1;
	va_list list;
	int w;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	w = strlen(msg) * 6 + 20;

	MsgDlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize(MsgDlg, w, 70);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "cdriso Msg");
	gtk_container_set_border_width(GTK_CONTAINER(MsgDlg), 0);

	Box = gtk_vbox_new(0, 0);
	gtk_container_add(GTK_CONTAINER(MsgDlg), Box);
	gtk_widget_show(Box);

	Txt = gtk_label_new(msg);

	gtk_box_pack_start(GTK_BOX(Box), Txt, FALSE, FALSE, 5);
	gtk_widget_show(Txt);

	Box1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), Box1, FALSE, FALSE, 0);
	gtk_widget_show(Box1);

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);

	gtk_main();
}

void OnOk (GtkButton *button,  gpointer user_data)
{
	char *tmp;

	stop = true;
	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);
	tmp = gtk_entry_get_text(GTK_ENTRY(CdEdit));
	strcpy(CdDev, tmp);
	
	if is_checked(ConfDlg, "checkBlockDump")
		BlockDump = 1;
	else 
		BlockDump = 0;
	
	SaveConf();
	gtk_widget_destroy(ConfDlg);
	gtk_main_quit();
}

void OnCancel(GtkButton *button,  gpointer user_data)
{
	stop = true;
	gtk_widget_destroy(ConfDlg);
	gtk_main_quit();
}

void OnFileSel_Ok()
{
	gchar *File;

	File = gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSel));
	gtk_entry_set_text(GTK_ENTRY(Edit), File);
	gtk_widget_destroy(FileSel);
}

void OnFileSel_Cancel()
{
	gtk_widget_destroy(FileSel);
}

void OnFileSel(GtkButton *button,  gpointer user_data)
{
	GtkWidget *Ok, *Cancel;

	FileSel = gtk_file_selection_new("Select Psx Iso File");
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSel), IsoFile);

	Ok = GTK_FILE_SELECTION(FileSel)->ok_button;
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnFileSel_Ok), NULL);
	gtk_widget_show(Ok);

	Cancel = GTK_FILE_SELECTION(FileSel)->cancel_button;
	gtk_signal_connect(GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(OnFileSel_Cancel), NULL);
	gtk_widget_show(Cancel);

	gtk_widget_show(FileSel);
	gdk_window_raise(FileSel->window);
}


EXPORT_C_(void) CDVDconfigure()
{
	int i;

	LoadConf();

	ConfDlg = create_Config();

	Edit = lookup_widget(ConfDlg, "GtkEntry_Iso");
	gtk_entry_set_text(GTK_ENTRY(Edit), IsoFile);
	CdEdit = lookup_widget(ConfDlg, "GtkEntry_CdDev");
	gtk_entry_set_text(GTK_ENTRY(CdEdit), CdDev);

	Progress = lookup_widget(ConfDlg, "GtkProgressBar_Progress");

	BtnCompress   = lookup_widget(ConfDlg, "GtkButton_Compress");
	BtnDecompress = lookup_widget(ConfDlg, "GtkButton_Decompress");
	BtnCreate     = lookup_widget(ConfDlg, "GtkButton_Create");
	BtnCreateZ    = lookup_widget(ConfDlg, "GtkButton_CreateZ");

	methodlist = NULL;
	for (i = 0; i < 2; i++)
		methodlist = g_list_append(methodlist, methods[i]);
		
	Method = lookup_widget(ConfDlg, "GtkCombo_Method");
	gtk_combo_set_popdown_strings(GTK_COMBO(Method), methodlist);
	if (strstr(IsoFile, ".Z") != NULL)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Method)->entry), methods[0]);
	else 
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Method)->entry), methods[1]);
	
	set_checked(ConfDlg, "checkBlockDump", (BlockDump == 1));
	
	gtk_widget_show_all(ConfDlg);
	gtk_main();

	return 0;
}

void OnAboutOk(GtkMenuItem * menuitem, gpointer userdata)
{
	gtk_widget_hide(AboutDlg);
	gtk_main_quit();
}

EXPORT_C_(void) CDVDabout()
{
	GtkWidget *Label;
	GtkWidget *Ok;
	GtkWidget *Box, *BBox;
	char AboutText[255];

	sprintf(AboutText, "%s %d.%d\n", LibName, revision, build);

	AboutDlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize(AboutDlg, 260, 80);
	gtk_window_set_title(GTK_WINDOW(AboutDlg), "CDVD About Dialog");
	gtk_window_set_position(GTK_WINDOW(AboutDlg), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(AboutDlg), 10);

	Box = gtk_vbox_new(0, 0);
	gtk_container_add(GTK_CONTAINER(AboutDlg), Box);
	gtk_widget_show(Box);

	Label = gtk_label_new(AboutText);
	gtk_box_pack_start(GTK_BOX(Box), Label, FALSE, FALSE, 0);
	gtk_widget_show(Label);

	BBox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), BBox, FALSE, FALSE, 0);
	gtk_widget_show(BBox);

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(Ok), "clicked",
	                   GTK_SIGNAL_FUNC(OnAboutOk), NULL);
	gtk_container_add(GTK_CONTAINER(BBox), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(AboutDlg);
	gtk_main();
}
