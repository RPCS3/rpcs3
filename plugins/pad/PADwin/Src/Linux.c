#include <string.h>
#include <gtk/gtk.h>

#include "PAD.h"
#include "interface.h"
#include "support.h"

Display *GSdsp;
XEvent E;


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

	gtk_main();
}

void _PADupdate() {
	int key;

    XLockDisplay(GSdsp);
    while (XPending(GSdsp) > 0) {
		XNextEvent(GSdsp, &E);
		switch (E.type) {
	    	case KeyPress:
				key = XLookupKeysym((XKeyEvent *)&E, 0);
				_KeyPress(key);
				break;
                
		    case KeyRelease:
				key = XLookupKeysym((XKeyEvent *)&E, 0);
				_KeyRelease(key);
				break;
                
	    	case FocusIn:
				XAutoRepeatOff(GSdsp);
				break;

	    	case FocusOut:
				XAutoRepeatOn(GSdsp);
				break;
		}
    }
    XUnlockDisplay(GSdsp);
}

s32  _PADopen(void *pDsp) {
	GSdsp = *(Display**)pDsp;
    XAutoRepeatOff(GSdsp);

	return 0;
}

void _PADclose() {
    XAutoRepeatOn(GSdsp);
}


GtkWidget *Conf;
GtkWidget *Btn;
//GtkWidget *Analog;
char name[32];

int padn;

void UpdateConf() {
	int i;

	for (i=0; i<24; i++) {
	    char *tmp;

		sprintf(name, "Key%d", i);
		Btn = lookup_widget(Conf, name);
	   	tmp = XKeysymToString(conf.keys[padn][i]);
		if (tmp != NULL)
			gtk_label_set_text(GTK_LABEL(GTK_BIN(Btn)->child), tmp);
		else
			gtk_label_set_text(GTK_LABEL(GTK_BIN(Btn)->child), "Unknown");
		gtk_object_set_user_data(GTK_OBJECT(Btn), &conf.keys[padn][i]);
	}
}

void OnConf_Key(GtkButton *Btn) {
    GdkEvent *ev;
	unsigned long *key = (unsigned long*)gtk_object_get_user_data(GTK_OBJECT(Btn));
    char *tmp;

    for (;;) {
		ev = gdk_event_get();
		if (ev != NULL) {
	    	if (ev->type == GDK_KEY_PRESS) {
    		    *key = ev->key.keyval;
		    	tmp = XKeysymToString(*key);
		    	if (tmp != NULL)
					gtk_label_set_text(GTK_LABEL(GTK_BIN(Btn)->child), tmp);
			    else
					gtk_label_set_text(GTK_LABEL(GTK_BIN(Btn)->child), "Unknown");
				return;
			}
	    }
    }
}

void OnConf_Pad1() {
	padn = 0;
	UpdateConf();
}

void OnConf_Pad2() {
	padn = 1;
	UpdateConf();
}

void OnConf_Ok() {
//	conf.analog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Analog));

	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel() {
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void CALLBACK PADconfigure() {
	LoadConfig();

	Conf = create_Config();

//	Analog = lookup_widget(Conf, "GtkCheckButton_Analog");
//	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Analog), conf.analog);

	padn = 0;
	UpdateConf();

	gtk_widget_show_all(Conf);	
	gtk_main();
}

GtkWidget *About;

void OnAbout_Ok() {
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CALLBACK PADabout() {

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK PADtest() {
	return 0;
}
/*
PADdriver PADx11 = {
	"x11",
	_PADopen,
	_PADclose,
	_PADupdate,
};
*/
