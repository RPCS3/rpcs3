/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007 
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

#include <string.h>
#include <gtk/gtk.h>
#include <pthread.h>

#define JOYSTICK_SUPPORT
#ifdef JOYSTICK_SUPPORT
#include <SDL/SDL.h>
#endif

#include "zeropad.h"

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

Display *GSdsp;
static pthread_spinlock_t s_mutexStatus;
static u32 s_keyPress[2], s_keyRelease[2]; // thread safe
static u32 s_bSDLInit = false;

// holds all joystick info
class JoystickInfo
{
public:
    JoystickInfo();
    ~JoystickInfo() { Destroy(); }
    
    void Destroy(); 
    // opens handles to all possible joysticks
    static void EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks);
    
    bool Init(int id, bool bStartThread=true); // opens a handle and gets information
    void Assign(int pad); // assigns a joystick to a pad

    void TestForce();

    const string& GetName() { return devname; }
    int GetNumButtons() { return numbuttons; }
    int GetNumAxes() { return numaxes; }
    int GetNumPOV() { return numpov; }
    int GetId() { return _id; }
    int GetPAD() { return pad; }
    int GetDeadzone(int axis) { return deadzone; }

    void SaveState();
    int GetButtonState(int i) { return vbutstate[i]; }
    int GetAxisState(int i) { return vaxisstate[i]; }
    void SetButtonState(int i, int state) { vbutstate[i] = state; }
    void SetAxisState(int i, int value) { vaxisstate[i] = value; }
#ifdef JOYSTICK_SUPPORT
    SDL_Joystick* GetJoy() { return joy; }
#endif

private:

    string devname; // pretty device name
    int _id;
    int numbuttons, numaxes, numpov;
    int axisrange, deadzone;
    int pad;
    
    vector<int> vbutstate, vaxisstate;
    
#ifdef JOYSTICK_SUPPORT
    SDL_Joystick* joy;
#endif
};

static vector<JoystickInfo*> s_vjoysticks;

extern string s_strIniPath;

void SaveConfig()
{
    int i, j;
    FILE *f;
    char cfg[255];

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg,"w");
    if (f == NULL) {
        printf("ZeroPAD: failed to save ini %s\n", s_strIniPath.c_str());
        return;
    }
    
	for (j=0; j<2; j++) {
		for (i=0; i<PADKEYS; i++) {
			fprintf(f, "[%d][%d] = 0x%lx\n", j, i, conf.keys[j][i]);
		}
	}
	fprintf(f, "log = %d\n", conf.log);
    fprintf(f, "options = %d\n", conf.options);
    fclose(f);
}

static char* s_pGuiKeyMap[] = { "L2", "R2", "L1", "R1",
                                "Triangle", "Circle", "Cross", "Square",
                                "Select", "L3", "R3", "Start",
                                "Up", "Right", "Down", "Left",
                                "Lx", "Rx", "Ly", "Ry" };

string GetLabelFromButton(const char* buttonname)
{
    string label = "e";
    label += buttonname;
    return label;
}

void LoadConfig() {
    FILE *f;
	char str[256];
    char cfg[255];
	int i, j;

    memset(&conf, 0, sizeof(conf));
	conf.keys[0][0] = XK_a;			// L2
	conf.keys[0][1] = XK_semicolon;			// R2
	conf.keys[0][2] = XK_w;			// L1
	conf.keys[0][3] = XK_p;			// R1
	conf.keys[0][4] = XK_i;			// TRIANGLE
	conf.keys[0][5] = XK_l;			// CIRCLE
	conf.keys[0][6] = XK_k;			// CROSS
	conf.keys[0][7] = XK_j;			// SQUARE
	conf.keys[0][8] = XK_v;	// SELECT
	conf.keys[0][11] = XK_n;// START
	conf.keys[0][12] = XK_e;	// UP
	conf.keys[0][13] = XK_f;	// RIGHT
	conf.keys[0][14] = XK_d;	// DOWN
	conf.keys[0][15] = XK_s;	// LEFT
	conf.log = 0;

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg, "r");
    if (f == NULL) {
        printf("ZeroPAD: failed to load ini %s\n", s_strIniPath.c_str());
        SaveConfig();//save and return
        return;
    }

	for (j=0; j<2; j++) {
		for (i=0; i<PADKEYS; i++) {
			sprintf(str, "[%d][%d] = 0x%%x\n", j, i);
			fscanf(f, str, &conf.keys[j][i]);
		}
	}
    fscanf(f, "log = %d\n", &conf.log);
    fscanf(f, "options = %d\n", &conf.options);
    fclose(f);
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

    MsgDlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
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

s32  _PADopen(void *pDsp)
{
    GSdsp = *(Display**)pDsp;
    pthread_spin_init(&s_mutexStatus, PTHREAD_PROCESS_PRIVATE);
    s_keyPress[0] = s_keyPress[1] = 0;
    s_keyRelease[0] = s_keyRelease[1] = 0;
    XAutoRepeatOff(GSdsp);

    JoystickInfo::EnumerateJoysticks(s_vjoysticks);

	return 0;
}

void _PADclose()
{
    pthread_spin_destroy(&s_mutexStatus);
    XAutoRepeatOn(GSdsp);

    vector<JoystickInfo*>::iterator it;
    FORIT(it, s_vjoysticks) delete *it;
    s_vjoysticks.clear();
}

void _PADupdate(int pad)
{
    pthread_spin_lock(&s_mutexStatus);
    status[pad] |= s_keyRelease[pad];
    status[pad] &= ~s_keyPress[pad];
    s_keyRelease[pad] = 0;
    s_keyPress[pad] = 0;
    pthread_spin_unlock(&s_mutexStatus);
}

int _GetJoystickIdFromPAD(int pad)
{
    // select the right joystick id
    int joyid = -1;
    for(int i = 0; i < PADKEYS; ++i) {
        if( IS_JOYSTICK(conf.keys[pad][i]) || IS_JOYBUTTONS(conf.keys[pad][i]) ) {
            joyid = PAD_GETJOYID(conf.keys[pad][i]);
            break;
        }
    }

    return joyid;
}

void CALLBACK PADupdate(int pad)
{
    int i;
    XEvent E;
    int keyPress=0,keyRelease=0;
    KeySym key;

    // keyboard input
    while (XPending(GSdsp) > 0) {
		XNextEvent(GSdsp, &E);
		switch (E.type) {
        case KeyPress:
            //_KeyPress(pad, XLookupKeysym((XKeyEvent *)&E, 0)); break;
            key = XLookupKeysym((XKeyEvent *)&E, 0);
            for (i=0; i<PADKEYS; i++) {
                if (key == conf.keys[pad][i]) {
                    keyPress|=(1<<i);
                    keyRelease&=~(1<<i);
                    break;
                }
            }
            
            event.evt = KEYPRESS;
            event.key = key;
            break;
        case KeyRelease:
            key = XLookupKeysym((XKeyEvent *)&E, 0);
            //_KeyRelease(pad, XLookupKeysym((XKeyEvent *)&E, 0));
            for (i=0; i<PADKEYS; i++) {
                if (key == conf.keys[pad][i]) {
                    keyPress&=~(1<<i);
                    keyRelease|= (1<<i);
                    break;
                }
            }
            
            event.evt = KEYRELEASE;
            event.key = key;
            break;
            
        case FocusIn:
            XAutoRepeatOff(GSdsp);
            break;
            
        case FocusOut:
            XAutoRepeatOn(GSdsp);
            break;
		}
    }

    // joystick info
#ifdef JOYSTICK_SUPPORT

    SDL_JoystickUpdate();
    for (int i=0; i<PADKEYS; i++) {
        int key = conf.keys[pad][i];
        JoystickInfo* pjoy = NULL;
        
        if( IS_JOYBUTTONS(key) ) {
            int joyid = PAD_GETJOYID(key);
            if( joyid >= 0 && joyid < (int)s_vjoysticks.size()) {
                pjoy = s_vjoysticks[joyid];
                if( SDL_JoystickGetButton((pjoy)->GetJoy(), PAD_GETJOYBUTTON(key)) ) {
                    status[(pjoy)->GetPAD()] &= ~(1<<i); // pressed
                }
                else
                    status[(pjoy)->GetPAD()] |= (1<<i); // pressed
            }
        }
        else if( IS_JOYSTICK(key) ) {
            int joyid = PAD_GETJOYID(key);
            if( joyid >= 0 && joyid < (int)s_vjoysticks.size()) {

                pjoy = s_vjoysticks[joyid];
                int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), PAD_GETJOYSTICK_AXIS(key));
                int pad = (pjoy)->GetPAD();
                switch(i) {
                case PAD_LX:
                    if( abs(value) > (pjoy)->GetDeadzone(value) ) {
                        g_lanalog[pad].x = value/256;
                        if( conf.options&PADOPTION_REVERTLX )
                            g_lanalog[pad].x = -g_lanalog[pad].x;
                        g_lanalog[pad].x += 0x80;
                    }
                    else g_lanalog[pad].x = 0x80;
                    break;
                case PAD_LY:
                    if( abs(value) > (pjoy)->GetDeadzone(value) ) {
                        g_lanalog[pad].y = value/256;
                        if( conf.options&PADOPTION_REVERTLX )
                            g_lanalog[pad].y = -g_lanalog[pad].y;
                        g_lanalog[pad].y += 0x80;
                            }
                    else g_lanalog[pad].y = 0x80;
                    break;
                case PAD_RX:
                    if( abs(value) > (pjoy)->GetDeadzone(value) ) {
                        g_ranalog[pad].x = value/256;
                        if( conf.options&PADOPTION_REVERTLX )
                            g_ranalog[pad].x = -g_ranalog[pad].x;
                        g_ranalog[pad].x += 0x80;
                    }
                    else g_ranalog[pad].x = 0x80;
                    break;
                case PAD_RY:
                    if( abs(value) > (pjoy)->GetDeadzone(value) ) {
                        g_ranalog[pad].y = value/256;
                        if( conf.options&PADOPTION_REVERTLX )
                            g_ranalog[pad].y = -g_ranalog[pad].y;
                                g_ranalog[pad].y += 0x80;
                    }
                    else g_ranalog[pad].y = 0x80;
                    break;
                }
            }
        }
        else if( IS_POV(key) ) {
            int joyid = PAD_GETJOYID(key);
            if( joyid >= 0 && joyid < (int)s_vjoysticks.size()) {

                pjoy = s_vjoysticks[joyid];

                int value = SDL_JoystickGetAxis((pjoy)->GetJoy(), PAD_GETJOYSTICK_AXIS(key));
                int pad = (pjoy)->GetPAD();
                if( PAD_GETPOVSIGN(key) && (value<-2048) )
                    status[pad] &= ~(1<<i);
                else if( !PAD_GETPOVSIGN(key) && (value>2048) )
                    status[pad] &= ~(1<<i);
                else
                    status[pad] |= (1<<i);
            }
        }
    }
#endif
    
    pthread_spin_lock(&s_mutexStatus);
    s_keyPress[pad] |= keyPress;
    s_keyPress[pad] &= ~keyRelease;
    s_keyRelease[pad] |= keyRelease;
    s_keyRelease[pad] &= ~keyPress;
    pthread_spin_unlock(&s_mutexStatus);
}

static GtkWidget *Conf=NULL, *s_devicecombo=NULL;
static int s_selectedpad = 0;

void UpdateConf(int pad)
{
    s_selectedpad = pad;

	int i;
    GtkWidget *Btn;
	for (i=0; i<ArraySize(s_pGuiKeyMap); i++) {
        
        if( s_pGuiKeyMap[i] == NULL )
            continue;

		Btn = lookup_widget(Conf, GetLabelFromButton(s_pGuiKeyMap[i]).c_str());
        if( Btn == NULL ) {
            printf("ZeroPAD: cannot find key %s\n", s_pGuiKeyMap[i]);
            continue;
        }

        string tmp;
        if( IS_KEYBOARD(conf.keys[pad][i]) ) {
            char* pstr = XKeysymToString(PAD_GETKEY(conf.keys[pad][i]));
            if( pstr != NULL )
                tmp = pstr;
        }
        else if( IS_JOYBUTTONS(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JBut %d", PAD_GETJOYBUTTON(conf.keys[pad][i]));
        }
        else if( IS_JOYSTICK(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JAxis %d", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]));
        }
        else if( IS_POV(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JPOV %d%s", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]), PAD_GETPOVSIGN(conf.keys[pad][i])?"-":"+");
        }

        if (tmp.size() > 0) {
            gtk_entry_set_text(GTK_ENTRY(Btn), tmp.c_str());
        }
		else
			gtk_entry_set_text(GTK_ENTRY(Btn), "Unknown");

        gtk_object_set_user_data(GTK_OBJECT(Btn), (void*)(PADKEYS*pad+i));
	}

    // check bounds
    int joyid = _GetJoystickIdFromPAD(pad);
    if( joyid < 0 || joyid >= (int)s_vjoysticks.size() ) {
        // get first unused joystick
        for(joyid = 0; joyid < s_vjoysticks.size(); ++joyid) {
            if( s_vjoysticks[joyid]->GetPAD() < 0 )
                break;
        }
    }
    
    if( joyid >= 0 && joyid < (int)s_vjoysticks.size() ) {
        // select the combo
        gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), joyid);
    }
    else gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), s_vjoysticks.size()); // no gamepad

    int padopts = conf.options>>(16*pad);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverselx")), padopts&PADOPTION_REVERTLX);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversely")), padopts&PADOPTION_REVERTLY);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverserx")), padopts&PADOPTION_REVERTRX);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversery")), padopts&PADOPTION_REVERTRY);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "forcefeedback")), padopts&PADOPTION_FORCEFEEDBACK);
}

void OnConf_Key(GtkButton *button, gpointer user_data)
{
    GdkEvent *ev;
    GtkWidget* label = lookup_widget(Conf, GetLabelFromButton(gtk_button_get_label(button)).c_str());
    if( label == NULL ) {
        printf("couldn't find correct label\n");
        return;
    }
    
    int id = (int)(uptr)gtk_object_get_user_data(GTK_OBJECT(label));
    int pad = id/PADKEYS;
    int key = id%PADKEYS;
    unsigned long *pkey = &conf.keys[pad][key];

    vector<JoystickInfo*>::iterator itjoy;

    // save the states
#ifdef JOYSTICK_SUPPORT
    SDL_JoystickUpdate();
    FORIT(itjoy, s_vjoysticks) (*itjoy)->SaveState();
#endif
    
    for (;;) {
		ev = gdk_event_get();
		if (ev != NULL) {
	    	if (ev->type == GDK_KEY_PRESS) {
    		    *pkey = ev->key.keyval;

		    	char* tmp = XKeysymToString(*pkey);
		    	if (tmp != NULL)
					gtk_entry_set_text(GTK_ENTRY(label), tmp);
			    else
					gtk_entry_set_text(GTK_ENTRY(label), "Unknown");
				return;
			}
	    }

#ifdef JOYSTICK_SUPPORT
        SDL_JoystickUpdate();
        FORIT(itjoy, s_vjoysticks) {

            // MAKE sure to look for changes in the state!!
            
            for(int i = 0; i < (*itjoy)->GetNumButtons(); ++i) {
                int but = SDL_JoystickGetButton((*itjoy)->GetJoy(), i);
                if( but != (*itjoy)->GetButtonState(i) ) {

                    if( !but ) { // released, we don't really want this
                        (*itjoy)->SetButtonState(i, 0);
                        break;
                    }

                    *pkey = PAD_JOYBUTTON((*itjoy)->GetId(), i);
                    char str[32];
                    sprintf(str, "JBut %d", i);
                    gtk_entry_set_text(GTK_ENTRY(label), str);
                    return;
                }
            }

            for(int i = 0; i < (*itjoy)->GetNumAxes(); ++i) {
                int value = SDL_JoystickGetAxis((*itjoy)->GetJoy(), i);

                if( value != (*itjoy)->GetAxisState(i) ) {

                    if( abs(value) <= (*itjoy)->GetAxisState(i)) {// we don't want this
                        // released, we don't really want this
                        (*itjoy)->SetButtonState(i, value);
                        break;
                    }
                    

                    if( abs(value) > 0x3fff ) {
                        if( key < 16 ) { // POV
                            *pkey = PAD_POV((*itjoy)->GetId(), value<0, i);
                            char str[32];
                            sprintf(str, "JPOV %d%s", i, value<0?"-":"+");
                            gtk_entry_set_text(GTK_ENTRY(label), str);
                            return;
                        }
                        else { // axis
                            *pkey = PAD_JOYSTICK((*itjoy)->GetId(), i);
                            char str[32];
                            sprintf(str, "JAxis %d", i);
                            gtk_entry_set_text(GTK_ENTRY(label), str);
                            return;
                        }
                    }
                }
            }
        }
#endif
    }
}

void OnConf_Pad1(GtkButton *button, gpointer user_data)
{
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) )
        UpdateConf(0);
}

void OnConf_Pad2(GtkButton *button, gpointer user_data)
{
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) )
        UpdateConf(1);
}

void OnConf_Ok(GtkButton *button, gpointer user_data)
{
//	conf.analog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Analog));
	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(Conf);
	gtk_main_quit();
    LoadConfig(); // load previous config
}

void CALLBACK PADconfigure()
{
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/zeropad.ini";
	
	LoadConfig();

	Conf = create_Conf();

    // recreate
    JoystickInfo::EnumerateJoysticks(s_vjoysticks);

    s_devicecombo  = lookup_widget(Conf, "joydevicescombo");

    // fill the combo
    char str[255];
    vector<JoystickInfo*>::iterator it;
    FORIT(it, s_vjoysticks) {
        sprintf(str, "%d: %s - but: %d, axes: %d, pov: %d", (*it)->GetId(), (*it)->GetName().c_str(),
                (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetNumPOV());
        gtk_combo_box_append_text (GTK_COMBO_BOX (s_devicecombo), str);
    }
    gtk_combo_box_append_text (GTK_COMBO_BOX (s_devicecombo), "No Gamepad");

	UpdateConf(0);

	gtk_widget_show_all(Conf);	
	gtk_main();
}

// GUI event handlers
void on_joydevicescombo_changed(GtkComboBox     *combobox, gpointer         user_data)
{
    int joyid = gtk_combo_box_get_active(combobox);

    // unassign every joystick with this pad
    for(int i = 0; i < (int)s_vjoysticks.size(); ++i) {
        if( s_vjoysticks[i]->GetPAD() == s_selectedpad )
            s_vjoysticks[i]->Assign(-1);
    }
    
    if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
        s_vjoysticks[joyid]->Assign(s_selectedpad);    
}

void on_checkbutton_reverselx_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTLX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reversely_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTLY<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reverserx_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTRX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reversery_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTRY<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_forcefeedback_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    int mask = PADOPTION_REVERTLX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) {
        conf.options |= mask;

        int joyid = gtk_combo_box_get_active(GTK_COMBO_BOX(s_devicecombo));
        if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
            s_vjoysticks[joyid]->TestForce();
    }
    else conf.options &= ~mask;
}

GtkWidget *About = NULL;

void OnAbout_Ok(GtkButton *button, gpointer user_data)
{
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

//////////////////////////
// Joystick definitions //
//////////////////////////

// opens handles to all possible joysticks
void JoystickInfo::EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks)
{
#ifdef JOYSTICK_SUPPORT

    if( !s_bSDLInit ) {
        if( SDL_Init(SDL_INIT_JOYSTICK) < 0 )
            return;
        SDL_JoystickEventState(SDL_QUERY);
        s_bSDLInit = true;
    }

    vector<JoystickInfo*>::iterator it;
    FORIT(it, vjoysticks) delete *it;

    vjoysticks.resize(SDL_NumJoysticks());
    for(int i = 0; i < (int)vjoysticks.size(); ++i) {
        vjoysticks[i] = new JoystickInfo();
        vjoysticks[i]->Init(i, true);
    }

    // set the pads
    for(int pad = 0; pad < 2; ++pad) {
        // select the right joystick id
        int joyid = -1;
        for(int i = 0; i < PADKEYS; ++i) {
            if( IS_JOYSTICK(conf.keys[pad][i]) || IS_JOYBUTTONS(conf.keys[pad][i]) ) {
                joyid = PAD_GETJOYID(conf.keys[pad][i]);
                break;
            }
        }

        if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
            s_vjoysticks[joyid]->Assign(pad);
    }

#endif
}

JoystickInfo::JoystickInfo()
{
#ifdef JOYSTICK_SUPPORT
    joy = NULL;
#endif
    _id = -1;
    pad = -1;
    axisrange = 0x7fff;
    deadzone = 2000;
}

void JoystickInfo::Destroy()
{
#ifdef JOYSTICK_SUPPORT
    if( joy != NULL ) {
        if( SDL_JoystickOpened(_id) )
            SDL_JoystickClose(joy);
        joy = NULL;
    }
#endif
}

bool JoystickInfo::Init(int id, bool bStartThread)
{
#ifdef JOYSTICK_SUPPORT
    Destroy();
    _id = id;

    joy = SDL_JoystickOpen(id);
    if( joy == NULL ) {
        printf("failed to open joystick %d\n", id);
        return false;
    }

    numaxes = SDL_JoystickNumAxes(joy);
    numbuttons = SDL_JoystickNumButtons(joy);
    numpov = SDL_JoystickNumHats(joy);
    devname = SDL_JoystickName(id);
    vbutstate.resize(numbuttons);
    vaxisstate.resize(numbuttons);
    
    return true;
#else
    return false;
#endif
}

// assigns a joystick to a pad
void JoystickInfo::Assign(int newpad)
{
    if( pad == newpad )
        return;

    pad = newpad;

    if( pad >= 0 ) {
        for(int i = 0; i < PADKEYS; ++i) {
            if( IS_JOYBUTTONS(conf.keys[pad][i]) ) {
                conf.keys[pad][i] = PAD_JOYBUTTON(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
            }
            else if( IS_JOYSTICK(conf.keys[pad][i]) ) {
                conf.keys[pad][i] = PAD_JOYSTICK(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
            }
        }
    }
}

void JoystickInfo::SaveState()
{
#ifdef JOYSTICK_SUPPORT
    for(int i = 0; i < numbuttons; ++i)
        vbutstate[i] = SDL_JoystickGetButton(joy, i);
    for(int i = 0; i < numaxes; ++i)
        vaxisstate[i] = SDL_JoystickGetAxis(joy, i);
#endif
}

void JoystickInfo::TestForce()
{
}
