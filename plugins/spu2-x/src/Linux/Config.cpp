/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "Dialogs.h"
#include "Config.h"

#ifdef PCSX2_DEVBUILD
static const int LATENCY_MAX = 3000;
#else
static const int LATENCY_MAX = 750;
#endif

static const int LATENCY_MIN = 40;

int AutoDMAPlayRate[2] = {0,0};

// Default settings.

// MIXING
int Interpolation = 4;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
		3. hermite interpolation
		4. catmull-rom interpolation
*/

bool EffectsDisabled = false;
float FinalVolume;
bool postprocess_filter_enabled = true;
bool postprocess_filter_dealias = false;
bool _visual_debug_enabled = false; // windows only feature

// OUTPUT
u32 OutputModule = 0;
int SndOutLatencyMS = 300;
int SynchMode = 0; // Time Stretch, Async or Disabled
static u32 OutputAPI = 0;

int numSpeakers = 0;
int dplLevel = 0;
/*****************************************************************************/

void ReadSettings()
{
	// For some reason this can be called before we know what ini file we're writing to.
	// Lets not try to read it if that happens.
	if (!pathSet) 
	{
		FileLog("Read called without the path set.\n");
		return;
	}
	
	Interpolation = CfgReadInt( L"MIXING",L"Interpolation", 4 );
	EffectsDisabled = CfgReadBool( L"MIXING", L"Disable_Effects", false );
	postprocess_filter_dealias = CfgReadBool( L"MIXING", L"DealiasFilter", false );
	FinalVolume = ((float)CfgReadInt( L"MIXING", L"FinalVolume", 100 )) / 100;
		if ( FinalVolume > 1.0f) FinalVolume = 1.0f;

	wxString temp;
	CfgReadStr( L"OUTPUT", L"Output_Module", temp, PortaudioOut->GetIdent() );
	OutputModule = FindOutputModuleById( temp.c_str() );// find the driver index of this module

	// find current API
	CfgReadStr( L"PORTAUDIO", L"HostApi", temp, L"ALSA" );
	OutputAPI = -1;
	if (temp == L"ALSA") OutputAPI = 0;
	if (temp == L"OSS")  OutputAPI = 1;
	if (temp == L"JACK") OutputAPI = 2;

	SndOutLatencyMS = CfgReadInt(L"OUTPUT",L"Latency", 300);
	SynchMode = CfgReadInt( L"OUTPUT", L"Synch_Mode", 0);

	PortaudioOut->ReadSettings();
	SoundtouchCfg::ReadSettings();
	DebugConfig::ReadSettings();

	// Sanity Checks
	// -------------

	Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );

	WriteSettings();
	spuConfig->Flush();
}

/*****************************************************************************/

void WriteSettings()
{
	if (!pathSet) 
	{
		FileLog("Write called without the path set.\n");
		return;
	}
	
	CfgWriteInt(L"MIXING",L"Interpolation",Interpolation);
	CfgWriteBool(L"MIXING",L"Disable_Effects",EffectsDisabled);
	CfgWriteBool(L"MIXING",L"DealiasFilter",postprocess_filter_dealias);
	CfgWriteInt(L"MIXING",L"FinalVolume",(int)(FinalVolume * 100 +0.5f));

	CfgWriteStr(L"OUTPUT",L"Output_Module", mods[OutputModule]->GetIdent() );
	CfgWriteInt(L"OUTPUT",L"Latency", SndOutLatencyMS);
	CfgWriteInt(L"OUTPUT",L"Synch_Mode", SynchMode);

	PortaudioOut->WriteSettings();
	SoundtouchCfg::WriteSettings();
	DebugConfig::WriteSettings();
}

void advanced_dialog()
{
	SoundtouchCfg::DisplayDialog();
}

void debug_dialog()
{
	DebugConfig::DisplayDialog();
}

void DisplayDialog()
{
    int return_value;

    GtkWidget *dialog;
    GtkWidget *main_frame, *main_box;

    GtkWidget *mixing_frame, *mixing_box;
    GtkWidget *int_label, *int_box;
    GtkWidget *effects_check;
    GtkWidget *dealias_filter;
    GtkWidget *debug_check;
    GtkWidget *debug_button;

    GtkWidget *output_frame, *output_box;
    GtkWidget *mod_label, *mod_box;
    GtkWidget *api_label, *api_box;
    GtkWidget *latency_label, *latency_slide;
    GtkWidget *sync_label, *sync_box;
    GtkWidget *advanced_button;

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"Spu2-X Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

    int_label = gtk_label_new ("Interpolation:");
    int_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "0 - Nearest (fastest/bad quality)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "1 - Linear (simple/okay sound)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "2 - Cubic (artificial highs)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "3 - Hermite (better highs)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "4 - Catmull-Rom (PS2-like/slow)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(int_box), Interpolation);

    effects_check = gtk_check_button_new_with_label("Disable Effects Processing");
    dealias_filter = gtk_check_button_new_with_label("Use the de-alias filter(overemphasizes the highs)");

    debug_check = gtk_check_button_new_with_label("Enable Debug Options");
	debug_button = gtk_button_new_with_label("Debug...");

    mod_label = gtk_label_new ("Module:");
    mod_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "0 - No Sound (emulate SPU2 only)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "1 - PortAudio (cross-platform)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "2 - SDL Audio (recommanded for pulseaudio");
    //gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "3 - Alsa (probably doesn't work)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(mod_box), OutputModule);

    api_label = gtk_label_new ("PortAudio API:");
    api_box = gtk_combo_box_new_text ();
	// In order to keep it the menu light, I only put linux major api
    gtk_combo_box_append_text(GTK_COMBO_BOX(api_box), "0 - ALSA (recommended)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(api_box), "1 - OSS (legacy)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(api_box), "2 - JACK");
    gtk_combo_box_set_active(GTK_COMBO_BOX(api_box), OutputAPI);

    latency_label = gtk_label_new ("Latency:");
    latency_slide = gtk_hscale_new_with_range(LATENCY_MIN, LATENCY_MAX, 5);
    gtk_range_set_value(GTK_RANGE(latency_slide), SndOutLatencyMS);

    sync_label = gtk_label_new ("Synchronization Mode:");
    sync_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(sync_box), "TimeStretch (Recommended)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(sync_box), "Async Mix (Breaks some games!)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(sync_box), "None (Audio can skip.)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sync_box), SynchMode);

	advanced_button = gtk_button_new_with_label("Advanced...");

    main_box = gtk_hbox_new(false, 5);
    main_frame = gtk_frame_new ("Spu2-X Config");
    gtk_container_add (GTK_CONTAINER(main_frame), main_box);

    mixing_box = gtk_vbox_new(false, 5);
    mixing_frame = gtk_frame_new ("Mixing Settings:");
    gtk_container_add (GTK_CONTAINER(mixing_frame), mixing_box);

    output_box = gtk_vbox_new(false, 5);
    output_frame = gtk_frame_new ("Output Settings:");
    gtk_container_add (GTK_CONTAINER(output_frame), output_box);

	gtk_container_add(GTK_CONTAINER(mixing_box), int_label);
	gtk_container_add(GTK_CONTAINER(mixing_box), int_box);
	gtk_container_add(GTK_CONTAINER(mixing_box), effects_check);
	gtk_container_add(GTK_CONTAINER(mixing_box), dealias_filter);
	gtk_container_add(GTK_CONTAINER(mixing_box), debug_check);
	gtk_container_add(GTK_CONTAINER(mixing_box), debug_button);

	gtk_container_add(GTK_CONTAINER(output_box), mod_label);
	gtk_container_add(GTK_CONTAINER(output_box), mod_box);
	gtk_container_add(GTK_CONTAINER(output_box), api_label);
	gtk_container_add(GTK_CONTAINER(output_box), api_box);
	gtk_container_add(GTK_CONTAINER(output_box), sync_label);
	gtk_container_add(GTK_CONTAINER(output_box), sync_box);
	gtk_container_add(GTK_CONTAINER(output_box), latency_label);
	gtk_container_add(GTK_CONTAINER(output_box), latency_slide);
	gtk_container_add(GTK_CONTAINER(output_box), advanced_button);

	gtk_container_add(GTK_CONTAINER(main_box), mixing_frame);
	gtk_container_add(GTK_CONTAINER(main_box), output_frame);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(effects_check), EffectsDisabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dealias_filter), postprocess_filter_dealias);
	//FinalVolume;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debug_check), DebugEnabled);

    gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_frame);
    gtk_widget_show_all (dialog);

    g_signal_connect_swapped(GTK_OBJECT (advanced_button), "clicked", G_CALLBACK(advanced_dialog), advanced_button);
    g_signal_connect_swapped(GTK_OBJECT (debug_button), "clicked", G_CALLBACK(debug_dialog), debug_button);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
		DebugEnabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(debug_check));
		postprocess_filter_dealias = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dealias_filter));
    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(int_box)) != -1)
    		Interpolation = gtk_combo_box_get_active(GTK_COMBO_BOX(int_box));

    	EffectsDisabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(effects_check));
		//FinalVolume;

    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mod_box)) != -1)
			OutputModule = gtk_combo_box_get_active(GTK_COMBO_BOX(mod_box));

    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(api_box)) != -1) {
			OutputAPI = gtk_combo_box_get_active(GTK_COMBO_BOX(api_box));
			switch(OutputAPI) {
				case 0: PortaudioOut->SetApiSettings(L"ALSA"); break;
				case 1: PortaudioOut->SetApiSettings(L"OSS"); break;
				case 2: PortaudioOut->SetApiSettings(L"JACK"); break;
				default: PortaudioOut->SetApiSettings(L"Unknown");
			}
		}

    	SndOutLatencyMS = gtk_range_get_value(GTK_RANGE(latency_slide));
    	
    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(sync_box)) != -1)
			SynchMode = gtk_combo_box_get_active(GTK_COMBO_BOX(sync_box));
    }

    gtk_widget_destroy (dialog);
}

void configure()
{
	initIni();
	ReadSettings();
	DisplayDialog();
	WriteSettings();
	delete spuConfig;
	spuConfig = NULL;
}
