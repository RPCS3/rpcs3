#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <signal.h>

#include "interface.h"
#include "support.h"
#include "callbacks.h"
#include "DEV9.h"
#include "Config.h"

GtkWidget *MsgDlg;

void OnMsg_Ok() {
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void cfgSysMessage(char *fmt, ...) {
	GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	MsgDlg = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "DEV9 Msg");
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

	gtk_main();
}

GtkWidget *About;

void OnAbout_Ok(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CFGabout() {
	About = create_About();
	gtk_widget_show_all(About);
	gtk_main();
}

GtkWidget *Conf;
GtkWidget *ComboEth;
GtkWidget *ComboHdd;
GList *ethdevs;
GList *hdddevs;

void OnConf_Ok(GtkButton *button, gpointer user_data) {
	gchar *str;

	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboEth)->entry));
	strcpy(config.Eth, str);
	str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ComboHdd)->entry));
	strcpy(config.Hdd, str);

	SaveConf();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void GetEthDevs() {
/*	struct ifconf ifc;
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) return;
*/
	ethdevs = g_list_append(ethdevs, "disabled");
	ethdevs = g_list_append(ethdevs, "lo");
	ethdevs = g_list_append(ethdevs, "eth0");
}

void GetHddDevs() {
/*	struct ifconf ifc;
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) return;
*/
	hdddevs = g_list_append(hdddevs, HDD_DEF);
}

void CFGconfigure() {
	Conf = create_Config();

	LoadConf();

	ComboEth = lookup_widget(Conf, "GtkCombo_Eth");
	ethdevs = NULL;
	GetEthDevs();
	if (ethdevs) {
		gtk_combo_set_popdown_strings(GTK_COMBO(ComboEth), ethdevs);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboEth)->entry), config.Eth);
	}

	ComboHdd = lookup_widget(Conf, "GtkCombo_Hdd");
	hdddevs = NULL;
	GetHddDevs();
	if (hdddevs) {
		gtk_combo_set_popdown_strings(GTK_COMBO(ComboHdd), hdddevs);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ComboHdd)->entry), config.Hdd);
	}

	gtk_widget_show_all(Conf);
	gtk_main();
}

long CFGmessage(char *msg) {
	cfgSysMessage(msg);

	return 0;
}

int main(int argc, char *argv[]) {
	gtk_init(NULL, NULL);

	if (!strcmp(argv[1], "configure")) {
		CFGconfigure();
	} else if (!strcmp(argv[1], "about")) {
		CFGabout();
	} else if (!strcmp(argv[1], "message")) {
		CFGmessage(argv[2]);
	}

	return 0;
}
