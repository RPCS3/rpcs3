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
#include <gtk/gtk.h>

#ifdef PCSX2_DEVBUILD
static const int LATENCY_MAX = 3000;
#else
static const int LATENCY_MAX = 750;
#endif

static const int LATENCY_MIN = 40;

int AutoDMAPlayRate[2] = {0,0};

CONFIG_DSOUNDOUT Config_DSoundOut;
CONFIG_WAVEOUT Config_WaveOut;
CONFIG_XAUDIO2 Config_XAudio2;

// Default settings.

// MIXING
int Interpolation = 1;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
*/
bool EffectsDisabled = false;
int ReverbBoost = 0;

// OUTPUT
u32 OutputModule = FindOutputModuleById( PortaudioOut->GetIdent() );
int SndOutLatencyMS = 160;
bool timeStretchDisabled = false;

/*****************************************************************************/

void ReadSettings()
{
	Interpolation = CfgReadInt( L"MIXING",L"Interpolation", 1 );
	EffectsDisabled = CfgReadBool( L"MIXING", L"Disable_Effects", false );
	ReverbBoost = CfgReadInt( L"MIXING",L"Reverb_Boost", 0 );

	wstring temp;
	CfgReadStr( L"OUTPUT", L"Output_Module", temp, 127, PortaudioOut->GetIdent() );
	OutputModule = FindOutputModuleById( temp.c_str() );// find the driver index of this module
	
	SndOutLatencyMS = CfgReadInt(L"OUTPUT",L"Latency", 150);
	timeStretchDisabled = CfgReadBool( L"OUTPUT", L"Disable_Timestretch", false );

	PortaudioOut->ReadSettings();
	SoundtouchCfg::ReadSettings();
	//DebugConfig::ReadSettings();

	// Sanity Checks
	// -------------

	Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );
	
	WriteSettings();
}

/*****************************************************************************/

void WriteSettings()
{
	CfgWriteInt(L"MIXING",L"Interpolation",Interpolation);
	CfgWriteBool(L"MIXING",L"Disable_Effects",EffectsDisabled);
	CfgWriteInt(L"MIXING",L"Reverb_Boost",ReverbBoost);

	CfgWriteStr(L"OUTPUT",L"Output_Module", mods[OutputModule]->GetIdent() );
	CfgWriteInt(L"OUTPUT",L"Latency", SndOutLatencyMS);
	CfgWriteBool(L"OUTPUT",L"Disable_Timestretch", timeStretchDisabled);

	PortaudioOut->WriteSettings();	
	SoundtouchCfg::WriteSettings();
	//DebugConfig::WriteSettings();
}

void displayDialog();

void configure()
{
	ReadSettings();
	displayDialog();
	WriteSettings();
}

void displayDialog()
{
    GtkWidget *dialog, *main_label;
    int return_value;
    
    GtkWidget *mixing_label;
    
    GtkWidget *int_label, *int_box;
    
    GtkWidget *effects_check;
    
    GtkWidget *reverb_label, *reverb_box;
    
    GtkWidget *output_label;
    
    GtkWidget *mod_label, *mod_box;
    
    GtkWidget *latency_slide;
    GtkWidget *time_check;
    GtkWidget *advanced_button;
    
    GtkWidget *mixing_vbox, *output_vbox, *main_hbox;
	
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
		
    main_label = gtk_label_new ("Spu2-X Config");
    
    mixing_label = gtk_label_new ("Mixing Settings:");
    
    int_label = gtk_label_new ("Interpolation:");
    int_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "0 - Nearest(none/fast)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "1 - Linear (reccommended)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(int_box), "2 - Cubic (slower/maybe better)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(int_box), Interpolation);
    
    effects_check = gtk_check_button_new_with_label("Disable Effects Processing");
    
    reverb_label = gtk_label_new ("Reverb Boost Factor:");
    reverb_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "1X - Normal Reverb Volume");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "2X - Reverb Volume x 2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "4X - Reverb Volume x 4");
    gtk_combo_box_append_text(GTK_COMBO_BOX(reverb_box), "8X - Reverb Volume x 8");
    gtk_combo_box_set_active(GTK_COMBO_BOX(reverb_box), ReverbBoost);
    
    output_label = gtk_label_new ("Output Settings:");
    
    mod_label = gtk_label_new ("Module:");
    mod_box = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "0 - No Sound (emulate SPU2 only)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(mod_box), "1 - PortAudio (cross-platform)");
    if (OutputModule == 0) 
		gtk_combo_box_set_active(GTK_COMBO_BOX(mod_box), 0);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(mod_box), 1);
	
    
    // latency slider
    latency_slide = gtk_hscale_new_with_range(LATENCY_MIN, LATENCY_MAX, 5);
    gtk_range_set_value(GTK_RANGE(latency_slide), SndOutLatencyMS);
    
    time_check = gtk_check_button_new_with_label("Disable Time Stretch");
    
    // Advanced button
    
    mixing_vbox = gtk_vbox_new(false, 5);
    output_vbox = gtk_vbox_new(false, 5);
    main_hbox = gtk_hbox_new(false, 5);
    
	gtk_container_add(GTK_CONTAINER(mixing_vbox), mixing_label);
	gtk_container_add(GTK_CONTAINER(mixing_vbox), int_label);
	gtk_container_add(GTK_CONTAINER(mixing_vbox), int_box);
	gtk_container_add(GTK_CONTAINER(mixing_vbox), effects_check);
	gtk_container_add(GTK_CONTAINER(mixing_vbox), reverb_label);
	gtk_container_add(GTK_CONTAINER(mixing_vbox), reverb_box);
	
	gtk_container_add(GTK_CONTAINER(output_vbox), output_label);
	gtk_container_add(GTK_CONTAINER(output_vbox), mod_label);
	gtk_container_add(GTK_CONTAINER(output_vbox), mod_box);
	gtk_container_add(GTK_CONTAINER(output_vbox), latency_slide);
	gtk_container_add(GTK_CONTAINER(output_vbox), time_check);
	
	gtk_container_add(GTK_CONTAINER(main_hbox), mixing_vbox);
	gtk_container_add(GTK_CONTAINER(main_hbox), output_vbox);
	
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(effects_check), EffectsDisabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_check), timeStretchDisabled);
    
    /* Add all our widgets, and show everything we've added to the dialog. */
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_label);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_hbox);
    gtk_widget_show_all (dialog);
    
    return_value = gtk_dialog_run (GTK_DIALOG (dialog));
    
    if (return_value == GTK_RESPONSE_ACCEPT)
    {
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
    	timeStretchDisabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(time_check));
    }
    
    gtk_widget_destroy (dialog);
    
}
