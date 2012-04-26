/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Global.h"
#include "Dialogs.h"
#include "Config.h"
#include "soundtouch/SoundTouch.h"

namespace SoundtouchCfg
{
void ClampValues()
{
	Clampify( SequenceLenMS, SequenceLen_Min, SequenceLen_Max );
	Clampify( SeekWindowMS, SeekWindow_Min, SeekWindow_Max );
	Clampify( OverlapMS, Overlap_Min, Overlap_Max );
}

void ApplySettings( soundtouch::SoundTouch& sndtouch )
{
	sndtouch.setSetting( SETTING_SEQUENCE_MS,	SequenceLenMS );
	sndtouch.setSetting( SETTING_SEEKWINDOW_MS,	SeekWindowMS );
	sndtouch.setSetting( SETTING_OVERLAP_MS,	OverlapMS );
}

void ReadSettings()
{
	SequenceLenMS	= CfgReadInt( L"SOUNDTOUCH", L"SequenceLengthMS", 30 );
	SeekWindowMS	= CfgReadInt( L"SOUNDTOUCH", L"SeekWindowMS", 20 );
	OverlapMS		= CfgReadInt( L"SOUNDTOUCH", L"OverlapMS", 10 );

	ClampValues();
	WriteSettings();
}

void WriteSettings()
{
	CfgWriteInt( L"SOUNDTOUCH", L"SequenceLengthMS", SequenceLenMS );
	CfgWriteInt( L"SOUNDTOUCH", L"SeekWindowMS", SeekWindowMS );
	CfgWriteInt( L"SOUNDTOUCH", L"OverlapMS", OverlapMS );
}

static GtkWidget *seq_label, *seek_label, *over_label;
static GtkWidget *seq_slide, *seek_slide, *over_slide;

void restore_defaults()
{
    gtk_range_set_value(GTK_RANGE(seq_slide), 30);
    gtk_range_set_value(GTK_RANGE(seek_slide), 20);
    gtk_range_set_value(GTK_RANGE(over_slide), 10);
}

void DisplayDialog()
{
	int return_value;
    GtkWidget *dialog, *main_label, *main_frame, *main_box;
    GtkWidget *default_button;

	ReadSettings();

    /* Create the widgets */
    dialog = gtk_dialog_new_with_buttons (
		"Advanced Settings",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

    main_label = gtk_label_new ("These are advanced configuration options fine tuning time stretching behavior. Larger values are better for slowdown, while smaller values are better for speedup (more then 60 fps.). All options are in microseconds.");
    gtk_label_set_line_wrap (GTK_LABEL(main_label), true);

	default_button = gtk_button_new_with_label("Reset to Defaults");

	seq_label = gtk_label_new("Sequence Length");
    seq_slide = gtk_hscale_new_with_range(SequenceLen_Min, SequenceLen_Max, 2);
    gtk_range_set_value(GTK_RANGE(seq_slide), SequenceLenMS);

    seek_label = gtk_label_new("Seek Window Size");
    seek_slide = gtk_hscale_new_with_range(SeekWindow_Min, SeekWindow_Max, 1);
    gtk_range_set_value(GTK_RANGE(seek_slide), SeekWindowMS);

    over_label = gtk_label_new("Overlap");
    over_slide = gtk_hscale_new_with_range(Overlap_Min, Overlap_Max, 1);
    gtk_range_set_value(GTK_RANGE(over_slide), OverlapMS);

    main_box = gtk_vbox_new(false, 5);
    main_frame = gtk_frame_new ("Spu2-X Config");

	gtk_container_add(GTK_CONTAINER (main_box), default_button);
	gtk_container_add(GTK_CONTAINER (main_box), seq_label);
	gtk_container_add(GTK_CONTAINER (main_box), seq_slide);
	gtk_container_add(GTK_CONTAINER (main_box), seek_label);
	gtk_container_add(GTK_CONTAINER (main_box), seek_slide);
	gtk_container_add(GTK_CONTAINER (main_box), over_label);
	gtk_container_add(GTK_CONTAINER (main_box), over_slide);
    gtk_container_add(GTK_CONTAINER(main_frame), main_box);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_label);
    gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_frame);
    gtk_widget_show_all (dialog);

    g_signal_connect_swapped(GTK_OBJECT (default_button), "clicked", G_CALLBACK(restore_defaults), default_button);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
    	SequenceLenMS = gtk_range_get_value(GTK_RANGE(seq_slide));;
    	SeekWindowMS = gtk_range_get_value(GTK_RANGE(seek_slide));;
    	OverlapMS = gtk_range_get_value(GTK_RANGE(over_slide));;
    }

    gtk_widget_destroy (dialog);

	WriteSettings();
}
}
