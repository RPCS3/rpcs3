#include <stdio.h>

#include "CDVD.h"

char *LibName       = "CDVDnull Driver";

const unsigned char version = PS2E_CDVD_VERSION;
const unsigned char revision = 0;
const unsigned char build = 6;


char* CALLBACK PS2EgetLibName() {
	return LibName;
}

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_CDVD;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version << 16) | (revision << 8) | build;
}

#ifdef _WIN32
void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);

	MessageBox(0, tmp, "CDVDnull Msg", 0);
}
#else

void SysMessage(char *fmt, ...) {
	/*GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	MsgDlg = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "GSsoft Msg");
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

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);	

	gtk_main();*/
}

#endif

s32 CALLBACK CDVDinit() {
	return 0;
}

s32 CALLBACK CDVDopen(const char* pTitle) {
	return 0;
}

void CALLBACK CDVDclose() {
}

void CALLBACK CDVDshutdown() {
}

s32 CALLBACK CDVDreadTrack(u32 lsn, int mode) {
	return -1;
}

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer() {
	return NULL;
}

s32 CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {
	return -1;
}

s32 CALLBACK CDVDgetTN(cdvdTN *Buffer) {
	return -1;
}

s32 CALLBACK CDVDgetTD(u8 Track, cdvdTD *Buffer) {
	return -1;
}

s32 CALLBACK CDVDgetTOC(void* toc) {
	return -1;
}

s32 CALLBACK CDVDgetDiskType() {
	return CDVD_TYPE_NODISC;
}

s32 CALLBACK CDVDgetTrayStatus() {
	return CDVD_TRAY_CLOSE;
}

s32 CALLBACK CDVDctrlTrayOpen() {
	return 0;
}

s32 CALLBACK CDVDctrlTrayClose() {
	return 0;
}

void CALLBACK CDVDconfigure() {
	SysMessage("Nothing to Configure");
}

void CALLBACK CDVDabout() {
	SysMessage("%s %d.%d", LibName, revision, build);
}

s32 CALLBACK CDVDtest() {
	return 0;
}
