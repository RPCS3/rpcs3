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
int Interpolation = 1;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
		3. hermite interpolation
		4. catmull-rom interpolation
*/

bool EffectsDisabled = false;
int ReverbBoost = 0;

// OUTPUT
u32 OutputModule = 0;
int SndOutLatencyMS = 150;
int SynchMode = 0; // Time Stretch, Async or Disabled

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
	
	Interpolation = CfgReadInt( L"MIXING",L"Interpolation", 1 );
	EffectsDisabled = CfgReadBool( L"MIXING", L"Disable_Effects", false );
	ReverbBoost = CfgReadInt( L"MIXING",L"Reverb_Boost", 0 );

	wxString temp;
	CfgReadStr( L"OUTPUT", L"Output_Module", temp, PortaudioOut->GetIdent() );
	OutputModule = FindOutputModuleById( temp.c_str() );// find the driver index of this module

	SndOutLatencyMS = CfgReadInt(L"OUTPUT",L"Latency", 150);
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
	CfgWriteInt(L"MIXING",L"Reverb_Boost",ReverbBoost);

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
    GtkWidget *reverb_label, *reverb_box;
    GtkWidget *debug_check;
    GtkWidget *debug_button;

    GtkWidget *output_frame, *output_box;
    GtkWidget *mod_label, *mod_box;
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
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "1 - Linear (simple/nice)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "2 - Cubic (slower/good highs)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "3 - Hermite (slower/better highs)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "4 - Catmull-Rom (slow/hq)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(int_box), Interpolation);

    effects_check = gtk_check_button_new_with_label("Disable Effects Processing");

    reverb_label = gtk_label_new ("Reverb Boost Factor:");
    reverb_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "1X - Normal Reverb Volume");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "2X - Reverb Volume x 2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "4X - Reverb Volume x 4");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "8X - Reverb Volume x 8");
    gtk_combo_box_set_active(GTK_COMBO_BOX(reverb_box), ReverbBoost);

    debug_check = gtk_check_button_new_with_label("Enable Debug Options");
	debug_button = gtk_button_new_with_label("Debug...");

    mod_label = gtk_label_new ("Module:");
    mod_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "0 - No Sound (emulate SPU2 only)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "1 - PortAudio (cross-platform)");
    if (OutputModule == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(mod_box), 0);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(mod_box), 1);

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
	gtk_container_add(GTK_CONTAINER(mixing_box), reverb_label);
	gtk_container_add(GTK_CONTAINER(mixing_box), reverb_box);
	gtk_container_add(GTK_CONTAINER(mixing_box), effects_check);
	gtk_container_add(GTK_CONTAINER(mixing_box), debug_check);
	gtk_container_add(GTK_CONTAINER(mixing_box), debug_button);

	gtk_container_add(GTK_CONTAINER(output_box), mod_label);
	gtk_container_add(GTK_CONTAINER(output_box), mod_box);
	gtk_container_add(GTK_CONTAINER(output_box), sync_label);
	gtk_container_add(GTK_CONTAINER(output_box), sync_box);
	gtk_container_add(GTK_CONTAINER(output_box), latency_label);
	gtk_container_add(GTK_CONTAINER(output_box), latency_slide);
	gtk_container_add(GTK_CONTAINER(output_box), advanced_button);

	gtk_container_add(GTK_CONTAINER(main_box), mixing_frame);
	gtk_container_add(GTK_CONTAINER(main_box), output_frame);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(effects_check), EffectsDisabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debug_check), DebugEnabled);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_frame);
    gtk_widget_show_all (dialog);

    g_signal_connect_swapped(GTK_OBJECT (advanced_button), "clicked", G_CALLBACK(advanced_dialog), advanced_button);
    g_signal_connect_swapped(GTK_OBJECT (debug_button), "clicked", G_CALLBACK(debug_dialog), debug_button);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
		DebugEnabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(debug_check));
    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(int_box)) != -1)
    		Interpolation = gtk_combo_box_get_active(GTK_COMBO_BOX(int_box));

    	EffectsDisabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(effects_check));

    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(reverb_box)) != -1)
			ReverbBoost = gtk_combo_box_get_active(GTK_COMBO_BOX(reverb_box));

    	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mod_box)) != 1)
			OutputModule = 0;
		else
			OutputModule = FindOutputModuleById( PortaudioOut->GetIdent() );

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
