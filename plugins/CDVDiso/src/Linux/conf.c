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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <time.h>
#include <string.h>
#include <zlib.h>
#include <bzlib.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"
#include "CDVDiso.h"
#include "Config.h"

unsigned char Zbuf[CD_FRAMESIZE_RAW * 10 * 2];

extern char *LibName;

extern const unsigned char revision;
extern const unsigned char build;

GtkWidget *FileSel;

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

void _CDRopen()
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

GtkWidget *MsgDlg;

void OnMsg_Ok()
{
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

static void SysMessageLoc(char *fmt, ...)
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

GtkWidget *ConfDlg;
GtkWidget *Edit, *CdEdit;
GtkWidget *FileSel;
GtkWidget *Progress;
GtkWidget *BtnCompress;
GtkWidget *BtnDecompress;
GtkWidget *BtnCreate;
GtkWidget *BtnCreateZ;
GtkWidget *Method;

GList *methodlist;
extern char *methods[];

int stop;

void OnOk(GtkMenuItem * menuitem, gpointer userdata)
{
	char *tmp;

	stop = 1;
	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);
	tmp = gtk_entry_get_text(GTK_ENTRY(CdEdit));
	strcpy(CdDev, tmp);
	SaveConf();
	gtk_widget_destroy(ConfDlg);
	gtk_main_quit();
}

void OnCancel(GtkMenuItem * menuitem, gpointer userdata)
{
	stop = 1;
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

void OnFileSel()
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

void OnStop()
{
	stop = 1;
}

void UpdZmode()
{
	char *tmp;

	tmp = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Method)->entry));
	if (!strcmp(tmp, methods[0])) Zmode = 1;
	else Zmode = 2;
}

char buffer[2352 * 10];

void OnCompress()
{
	struct stat buf;
	u32 lsn;
	u8 cdbuff[10*2352];
	char Zfile[256];
	char *tmp;
	int ret;
	isoFile *src;
	isoFile *dst;

	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);
	UpdZmode();

	if (Zmode == 1) sprintf(Zfile, "%s.Z2", IsoFile);
	if (Zmode == 2) sprintf(Zfile, "%s.BZ2", IsoFile);

	if (stat(Zfile, &buf) != -1)
	{
		/*char str[256];*/
		return;
		/*		sprintf(str, "'%s' already exists, overwrite?", Zfile);
				if (MessageBox(hDlg, str, "Question", MB_YESNO) != IDYES) {
					return;
				}*/
	}

	src = isoOpen(IsoFile);
	if (src == NULL) return;
	dst = isoCreate(Zfile, Zmode == 1 ? ISOFLAGS_Z2 : ISOFLAGS_BZ2);
	if (dst == NULL) return;

	gtk_widget_set_sensitive(BtnCompress, FALSE);
	gtk_widget_set_sensitive(BtnDecompress, FALSE);
	gtk_widget_set_sensitive(BtnCreate, FALSE);
	gtk_widget_set_sensitive(BtnCreateZ, FALSE);
	stop = 0;

	for (lsn = 0; lsn < src->blocks; lsn++)
	{
		printf("block %d ", lsn);
		putchar(13);
		fflush(stdout);
		ret = isoReadBlock(src, cdbuff, lsn);
		if (ret == -1) break;
		ret = isoWriteBlock(dst, cdbuff, lsn);
		if (ret == -1) break;

		gtk_progress_bar_update(GTK_PROGRESS_BAR(Progress), (lsn * 100) / src->blocks);
		while (gtk_events_pending()) gtk_main_iteration();
		if (stop) break;
	}
	isoClose(src);
	isoClose(dst);

	if (!stop) gtk_entry_set_text(GTK_ENTRY(Edit), IsoFile);

	gtk_widget_set_sensitive(BtnCompress, TRUE);
	gtk_widget_set_sensitive(BtnDecompress, TRUE);
	gtk_widget_set_sensitive(BtnCreate, TRUE);
	gtk_widget_set_sensitive(BtnCreateZ, TRUE);

	if (!stop)
	{
		if (ret == -1)
		{
			SysMessageLoc("Error compressing iso image");
		}
		else
		{
			SysMessageLoc("Iso image compressed OK");
		}
	}
}

void OnDecompress()
{
#if 0
	struct stat buf;
	FILE *f;
	unsigned long c = 0, p = 0, s;
	char table[256];
	char *tmp;
	int blocks;

	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);

	if (strstr(IsoFile, ".Z") != NULL) Zmode = 1;
	else Zmode = 2;

	strcpy(table, IsoFile);
	if (Zmode == 1) strcat(table, ".table");
	else strcat(table, ".index");

	if (stat(table, &buf) == -1)
	{
		return;
	}
	if (Zmode == 1) c = s = buf.st_size / 6;
	else c = s = (buf.st_size / 4) - 1;
	f = fopen(table, "rb");
	Ztable = (char*)malloc(buf.st_size);
	fread(Ztable, 1, buf.st_size, f);
	fclose(f);

	cdHandle[0] = fopen(IsoFile, "rb");
	if (cdHandle[0] == NULL)
	{
		return;
	}

	if (Zmode == 1) IsoFile[strlen(IsoFile) - 2] = 0;
	else IsoFile[strlen(IsoFile) - 3] = 0;

	f = fopen(IsoFile, "wb");
	if (f == NULL)
	{
		return;
	}

	gtk_widget_set_sensitive(BtnCompress, FALSE);
	gtk_widget_set_sensitive(BtnDecompress, FALSE);
	gtk_widget_set_sensitive(BtnCreate, FALSE);
	gtk_widget_set_sensitive(BtnCreateZ, FALSE);
	stop = 0;

	if (Zmode == 1)
	{
		blocks = 1;
	}
	else
	{
		blocks = 10;
	}

	while (c--)
	{
		unsigned long size, pos, ssize;
		float per;

		if (Zmode == 1)
		{
			pos = *(unsigned long*) & Ztable[p * 6];
			fseek(cdHandle[0], pos, SEEK_SET);

			ssize = *(unsigned short*) & Ztable[p * 6 + 4];
			fread(Zbuf, 1, ssize, cdHandle[0]);
		}
		else
		{
			pos = *(unsigned long*) & Ztable[p * 4];
			fseek(cdHandle[0], pos, SEEK_SET);

			ssize = *(unsigned long*) & Ztable[p * 4 + 4] - pos;
			fread(Zbuf, 1, ssize, cdHandle[0]);
		}

		size = CD_FRAMESIZE_RAW * blocks;
		if (Zmode == 1) uncompress(cdbuffer, &size, Zbuf, ssize);
		else BZ2_bzBuffToBuffDecompress(cdbuffer, (unsigned int*)&size, Zbuf, ssize, 0, 0);

		fwrite(cdbuffer, 1, size, f);

		p++;

		per = ((float)p / s);
		gtk_progress_bar_update(GTK_PROGRESS_BAR(Progress), per);
		while (gtk_events_pending()) gtk_main_iteration();
		if (stop) break;
	}
	if (!stop) gtk_entry_set_text(GTK_ENTRY(Edit), IsoFile);

	fclose(f);
	fclose(cdHandle[0]);
	cdHandle[0] = NULL;
	free(Ztable);
	Ztable = NULL;

	gtk_widget_set_sensitive(BtnCompress, TRUE);
	gtk_widget_set_sensitive(BtnDecompress, TRUE);
	gtk_widget_set_sensitive(BtnCreate, TRUE);
	gtk_widget_set_sensitive(BtnCreateZ, TRUE);

	if (!stop) SysMessageLoc("Iso Image Decompressed OK");
#endif
}

#define CD_LEADOUT	(0xaa)
unsigned char param[4];
int cddev = -1;

union
{
	struct cdrom_msf msf;
	unsigned char buf[CD_FRAMESIZE_RAW];
} cr;

void incSector()
{
	param[2]++;
	if (param[2] == 75)
	{
		param[2] = 0;
		param[1]++;
	}
	if (param[1] == 60)
	{
		param[1] = 0;
		param[0]++;
	}
}

long CDR_open(void)
{
	if (cddev != -1)
		return 0;
	cddev = open(CdDev, O_RDONLY);
	if (cddev == -1)
	{
		printf("CDR: Could not open %s\n", CdDev);
		return -1;
	}

	return 0;
}

long CDR_close(void)
{
	if (cddev == -1) return 0;
	close(cddev);
	cddev = -1;

	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
long CDR_getTN(unsigned char *buffer)
{
	struct cdrom_tochdr toc;

	if (ioctl(cddev, CDROMREADTOCHDR, &toc) == -1) return -1;

	buffer[0] = toc.cdth_trk0;	// start track
	buffer[1] = toc.cdth_trk1;	// end track

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
long CDR_getTD(unsigned char track, unsigned char *buffer)
{
	struct cdrom_tocentry entry;

	if (track == 0) track = 0xaa; // total time
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(cddev, CDROMREADTOCENTRY, &entry) == -1) return -1;

	buffer[0] = entry.cdte_addr.msf.minute;	/* minute */
	buffer[1] = entry.cdte_addr.msf.second;	/* second */
	buffer[2] = entry.cdte_addr.msf.frame;	/* frame */

	return 0;
}

// read track
// time:
//  byte 0 - minute
//  byte 1 - second
//  byte 2 - frame
char *CDR_readTrack(unsigned char *time)
{
	cr.msf.cdmsf_min0 = time[0];
	cr.msf.cdmsf_sec0 = time[1];
	cr.msf.cdmsf_frame0 = time[2];

	if (ioctl(cddev, CDROMREADRAW, &cr) == -1) return NULL;
	return cr.buf;
}


void OnCreate()
{
	FILE *f;
	struct stat buf;
	struct tm *Tm;
	time_t Ttime;
	unsigned long ftrack, ltrack;
	unsigned long p = 0, s;
	unsigned char *buffer;
	unsigned char bufferz[2352];
	unsigned char start[4], end[4];
	char *tmp;
#ifdef VERBOSE
	unsigned long count = 0;
	int i = 0;
#endif

	memset(bufferz, 0, sizeof(bufferz));
	ftrack = 1;
	ltrack = CD_LEADOUT;

	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);

	if (stat(IsoFile, &buf) == 0)
	{
		printf("File %s Already exists\n", IsoFile);
		return;
	}

	if (CDR_open() == -1)
	{
		return;
	}

	if (CDR_getTD(ftrack, start) == -1)
	{
		printf("Error getting TD\n");

		CDR_close();
		return;
	}
	if (CDR_getTD(ltrack, end) == -1)
	{
		printf("Error getting TD\n");

		CDR_close();
		return;
	}

	f = fopen(IsoFile, "wb");
	if (f == NULL)
	{
		CDR_close();
		printf("Error opening %s", IsoFile);
		return;
	}

	printf("Making Iso: from %2.2d:%2.2d:%2.2d to %2.2d:%2.2d:%2.2d\n",
	       start[0], start[1], start[2], end[0], end[1], end[2]);

	memcpy(param, start, 3);

	time(&Ttime);

	stop = 0;
	s = MSF2SECT(end[0], end[1], end[2]);
	gtk_widget_set_sensitive(BtnCompress, FALSE);
	gtk_widget_set_sensitive(BtnDecompress, FALSE);
	gtk_widget_set_sensitive(BtnCreate, FALSE);
	gtk_widget_set_sensitive(BtnCreateZ, FALSE);

	for (;;)   /* loop until end */
	{
		float per;

		if ((param[0] == end[0]) & (param[1] == end[1]) & (param[2] == end[2]))
			break;
		buffer = CDR_readTrack(param);
		if (buffer == NULL)
		{
			int i;

			for (i = 0; i < 10; i++)
			{
				buffer = CDR_readTrack(param);
				if (buffer != NULL) break;
			}
			if (buffer == NULL)
			{
				printf("Error Reading %2d:%2d:%2d\n", param[0], param[1], param[2]);
				buffer = bufferz;
				buffer[12] = param[0];
				buffer[13] = param[1];
				buffer[14] = param[2];
				buffer[15] = 0x2;
			}
		}
		fwrite(buffer, 1, 2352, f);
#ifdef VERBOSE
		count += CD_FRAMESIZE_RAW;

		printf("reading %2d:%2d:%2d ", param[0], param[1], param[2]);
		if ((time(NULL) - Ttime) != 0)
		{
			i = (count / 1024) / (time(NULL) - Ttime);
			printf("( %5dKbytes/s, %dX)", i, i / 150);
		}
		putchar(13);
		fflush(stdout);
#endif

		incSector();

		p++;
		per = ((float)p / s);
		gtk_progress_bar_update(GTK_PROGRESS_BAR(Progress), per);
		while (gtk_events_pending()) gtk_main_iteration();
		if (stop) break;
	}

	Ttime = time(NULL) - Ttime;
	Tm = gmtime(&Ttime);
	printf("\nTotal Time used: %d:%d:%d\n", Tm->tm_hour, Tm->tm_min,
	       Tm->tm_sec);

	CDR_close();
	fclose(f);

	gtk_widget_set_sensitive(BtnCompress, TRUE);
	gtk_widget_set_sensitive(BtnDecompress, TRUE);
	gtk_widget_set_sensitive(BtnCreate, TRUE);
	gtk_widget_set_sensitive(BtnCreateZ, TRUE);

	if (!stop) SysMessageLoc("Iso Image Created OK");
}

void OnCreateZ()
{
	FILE *f;
	FILE *t;
	struct stat buf;
	struct tm *Tm;
	time_t Ttime;
	unsigned long ftrack, ltrack;
	unsigned long p = 0, s, c = 0;
	unsigned char *buffer;
	unsigned char bufferz[2352];
	unsigned char start[4], end[4];
	char table[256];
	char *tmp;
	int b, blocks;
#ifdef VERBOSE
	unsigned long count = 0;
	int i = 0;
#endif

	memset(bufferz, 0, sizeof(bufferz));
	ftrack = 1;
	ltrack = CD_LEADOUT;

	tmp = gtk_entry_get_text(GTK_ENTRY(Edit));
	strcpy(IsoFile, tmp);

	UpdZmode();

	if (Zmode == 1)
	{
		blocks = 1;
		if (strstr(IsoFile, ".Z") == NULL) strcat(IsoFile, ".Z");
	}
	else
	{
		blocks = 10;
		if (strstr(IsoFile, ".bz") == NULL) strcat(IsoFile, ".bz");
	}
	
	if (stat(IsoFile, &buf) == 0)
	{
		printf("File %s Already exists\n", IsoFile);
		return;
	}

	strcpy(table, IsoFile);
	if (Zmode == 1) 
		strcat(table, ".table");
	else 
		strcat(table, ".index");

	t = fopen(table, "wb");
	
	if (t == NULL) return;
	if (CDR_open() == -1) return;

	if (CDR_getTD(ftrack, start) == -1)
	{
		printf("Error getting TD\n");
		CDR_close();
		return;
	}
	
	if (CDR_getTD(ltrack, end) == -1)
	{
		printf("Error getting TD\n");
		CDR_close();
		return;
	}

	f = fopen(IsoFile, "wb");
	if (f == NULL)
	{
		CDR_close();
		printf("Error opening %s", IsoFile);
		return;
	}

	printf("Making Iso: from %2.2d:%2.2d:%2.2d to %2.2d:%2.2d:%2.2d\n",
	       start[0], start[1], start[2], end[0], end[1], end[2]);

	memcpy(param, start, 3);

	time(&Ttime);

	stop = 0;
	s = MSF2SECT(end[0], end[1], end[2]) / blocks;
	gtk_widget_set_sensitive(BtnCompress, FALSE);
	gtk_widget_set_sensitive(BtnDecompress, FALSE);
	gtk_widget_set_sensitive(BtnCreate, FALSE);
	gtk_widget_set_sensitive(BtnCreateZ, FALSE);

	for (;;)   /* loop until end */
	{
		unsigned long size;
		unsigned char Zbuf[CD_FRAMESIZE_RAW * 10 * 2];
		float per;

		for (b = 0; b < blocks; b++)
		{
			if ((param[0] == end[0]) & (param[1] == end[1]) & (param[2] == end[2]))
				break;
			
			buffer = CDR_readTrack(param);
			if (buffer == NULL)
			{
				int i;

				for (i = 0; i < 10; i++)
				{
					buffer = CDR_readTrack(param);
					if (buffer != NULL) break;
				}
				
				if (buffer == NULL)
				{
					printf("Error Reading %2d:%2d:%2d\n", param[0], param[1], param[2]);
					buffer = bufferz;
					buffer[12] = param[0];
					buffer[13] = param[1];
					buffer[14] = param[2];
					buffer[15] = 0x2;
				}
			}
			memcpy(cdbuffer + b * CD_FRAMESIZE_RAW, buffer, CD_FRAMESIZE_RAW);

			incSector();
		}
		if ((param[0] == end[0]) & (param[1] == end[1]) & (param[2] == end[2]))
			break;

		size = CD_FRAMESIZE_RAW * blocks * 2;
		if (Zmode == 1) 
			compress(Zbuf, &size, cdbuffer, CD_FRAMESIZE_RAW);
		else 
			BZ2_bzBuffToBuffCompress(Zbuf, (unsigned int*)&size, cdbuffer, CD_FRAMESIZE_RAW * 10, 1, 0, 30);

		fwrite(&c, 1, 4, t);
		if (Zmode == 1) fwrite(&size, 1, 2, t);

		fwrite(Zbuf, 1, size, f);

		c += size;
#ifdef VERBOSE
		count += CD_FRAMESIZE_RAW * blocks;

		printf("reading %2d:%2d:%2d ", param[0], param[1], param[2]);
		if ((time(NULL) - Ttime) != 0)
		{
			i = (count / 1024) / (time(NULL) - Ttime);
			printf("( %5dKbytes/s, %dX)", i, i / 150);
		}
		putchar(13);
		fflush(stdout);
#endif

		p++;
		per = ((float)p / s);
		
		gtk_progress_bar_update(GTK_PROGRESS_BAR(Progress), per);
		while (gtk_events_pending()) gtk_main_iteration();
		
		if (stop) break;
	}
	if (Zmode == 2) fwrite(&c, 1, 4, f);

	if (!stop) gtk_entry_set_text(GTK_ENTRY(Edit), IsoFile);

	Ttime = time(NULL) - Ttime;
	Tm = gmtime(&Ttime);
	printf("\nTotal Time used: %d:%d:%d\n", Tm->tm_hour, Tm->tm_min,
	       Tm->tm_sec);

	CDR_close();
	fclose(f);
	fclose(t);

	gtk_widget_set_sensitive(BtnCompress, TRUE);
	gtk_widget_set_sensitive(BtnDecompress, TRUE);
	gtk_widget_set_sensitive(BtnCreate, TRUE);
	gtk_widget_set_sensitive(BtnCreateZ, TRUE);

	if (!stop) SysMessageLoc("Compressed Iso Image Created OK");
}

long CDRconfigure(void)
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
	else gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Method)->entry), methods[1]);

	gtk_widget_show_all(ConfDlg);
	gtk_main();

	return 0;
}

GtkWidget *AboutDlg;

void OnAboutOk(GtkMenuItem * menuitem, gpointer userdata)
{
	gtk_widget_hide(AboutDlg);
	gtk_main_quit();
}

void CDRabout(void)
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

int main(int argc, char *argv[])
{
	if (argc < 2) return 0;

	gtk_init(NULL, NULL);

	if (!strcmp(argv[1], "open"))
		_CDRopen();
	else if (!strcmp(argv[1], "configure"))
		CDRconfigure();
	else if (!strcmp(argv[1], "message"))
	{
		if (argc > 2) SysMessageLoc(argv[2]);
	}
	else
		CDRabout();

	return 0;
}


