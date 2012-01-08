/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <gtk/gtk.h>
#include "GSdx.h"
#include "GSLinuxLogo.h"

static void SysMessage(const char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg) - 1] = 0;

	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

GtkWidget* CreateRenderComboBox()
{
	GtkWidget  *render_combo_box;
	int renderer_box_position = 0;
	
	render_combo_box = gtk_combo_box_new_text ();

	for(size_t i = 6; i < theApp.m_gs_renderers.size(); i++)
	{
		const GSSetting& s = theApp.m_gs_renderers[i];

		string label = s.name;

		if(!s.note.empty()) label += format(" (%s)", s.note.c_str());

		// (dev only) for any NULL stuff
		if (i >= 7 && i <= 9) label += " (debug only)";
		// (experimental) for opengl stuff
		if (i == 10 || i == 11) label += " (experimental)";

		gtk_combo_box_append_text(GTK_COMBO_BOX(render_combo_box), label.c_str());
	}

	switch (theApp.GetConfig("renderer", 0)) {
		// Note the value are based on m_gs_renderers vector on GSdx.cpp
		case 7 : renderer_box_position = 0; break;
		case 8 : renderer_box_position = 1; break;
		case 10: renderer_box_position = 2; break;
		case 11: renderer_box_position = 3; break;
		case 12: renderer_box_position = 4; break;
		case 13: renderer_box_position = 5; break;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(render_combo_box), renderer_box_position);
	return render_combo_box;
}

GtkWidget* CreateInterlaceComboBox()
{
	GtkWidget  *interlace_combo_box;
	interlace_combo_box = gtk_combo_box_new_text ();

	for(size_t i = 0; i < theApp.m_gs_interlace.size(); i++)
	{
		const GSSetting& s = theApp.m_gs_interlace[i];

		string label = s.name;

		if(!s.note.empty()) label += format(" (%s)", s.note.c_str());

		gtk_combo_box_append_text(GTK_COMBO_BOX(interlace_combo_box), label.c_str());
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(interlace_combo_box), theApp.GetConfig("interlace", 0));
	return interlace_combo_box;
}

GtkWidget* CreateMsaaComboBox()
{
	GtkWidget  *msaa_combo_box;
	msaa_combo_box = gtk_combo_box_new_text ();
	
	// For now, let's just put in the same vaues that show up in the windows combo box.
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "Custom");
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "3x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "4x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "5x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(msaa_combo_box), "6x");

	gtk_combo_box_set_active(GTK_COMBO_BOX(msaa_combo_box), theApp.GetConfig("msaa", 0));
	return msaa_combo_box;
}

GtkWidget *render_combo_box;
GtkWidget *filter_check, *logz_check, *paltex_check, *fba_check, *aa_check,  *native_res_check;
GtkWidget *msaa_combo_box, *resx_spin, *resy_spin;
		
void toggle_widget_states( GtkWidget *widget, gpointer callback_data )
{
	int render_type;
	bool hardware_render = false, software_render = false, sdl_render = false, null_render = false;
	
	render_type = gtk_combo_box_get_active(GTK_COMBO_BOX(render_combo_box));
	hardware_render = (render_type == 1 || render_type == 4 || render_type == 7 || render_type == 13);
	
	if (hardware_render)
	{
		gtk_widget_set_sensitive(filter_check, true);
		gtk_widget_set_sensitive(logz_check, true);
		gtk_widget_set_sensitive(paltex_check, true);
		gtk_widget_set_sensitive(fba_check, true);
		gtk_widget_set_sensitive(native_res_check, true);
		
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(native_res_check)))
		{
			gtk_widget_set_sensitive(msaa_combo_box, false);
			gtk_widget_set_sensitive(resx_spin, false);
			gtk_widget_set_sensitive(resy_spin, false);
		}
		else
		{
			gtk_widget_set_sensitive(msaa_combo_box, true);
			
			if (gtk_combo_box_get_active(GTK_COMBO_BOX(msaa_combo_box)) == 0)
			{
				gtk_widget_set_sensitive(resx_spin, true);
				gtk_widget_set_sensitive(resy_spin, true);
			}
			else
			{
				gtk_widget_set_sensitive(resx_spin, false);
				gtk_widget_set_sensitive(resy_spin, false);
			}
		}
	}
	else
	{
		gtk_widget_set_sensitive(filter_check, false);
		gtk_widget_set_sensitive(logz_check, false);
		gtk_widget_set_sensitive(paltex_check, false);
		gtk_widget_set_sensitive(fba_check, false);
		
		gtk_widget_set_sensitive(native_res_check, false);
		gtk_widget_set_sensitive(msaa_combo_box, false);
		gtk_widget_set_sensitive(resx_spin, false);
		gtk_widget_set_sensitive(resy_spin, false);
	}
}

bool RunLinuxDialog()
{
	GtkWidget *dialog;
	GtkWidget *main_box, *res_box, *hw_box, *sw_box;
	GtkWidget  *native_box, *msaa_box, *resxy_box, *renderer_box, *interlace_box, *threads_box;
	GtkWidget *hw_table, *res_frame, *hw_frame, *sw_frame;
	GtkWidget *interlace_combo_box, *threads_spin;
	GtkWidget *interlace_label, *threads_label, *native_label,  *msaa_label, *rexy_label, *render_label;
	int return_value;
	
	GdkPixbuf* logo_pixmap;
	GtkWidget *logo_image;
	
	/* Create the widgets */
	dialog = gtk_dialog_new_with_buttons (
		"GSdx Config",
		NULL, /* parent window*/
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_REJECT,
		NULL);

	// The main area for the whole dialog box.
	main_box = gtk_vbox_new(false, 5);
	
	// The Internal resolution frame and container.
	res_box = gtk_vbox_new(false, 5);
	res_frame = gtk_frame_new ("OpenGL Internal Resolution (can cause glitches)");
	gtk_container_add(GTK_CONTAINER(res_frame), res_box);
	
	// The hardware mode frame, container, and table.
	hw_box = gtk_vbox_new(false, 5);
	hw_frame = gtk_frame_new ("Hardware Mode Settings");
	gtk_container_add(GTK_CONTAINER(hw_frame), hw_box);
	hw_table = gtk_table_new(2,2, false);
	gtk_container_add(GTK_CONTAINER(hw_box), hw_table);
	
	// The software mode frame and container. (It doesn't have enough in it for a table.)
	sw_box = gtk_vbox_new(false, 5);
	sw_frame = gtk_frame_new ("Software Mode Settings");
	gtk_container_add(GTK_CONTAINER(sw_frame), sw_box);
	
	// Grab a logo, to make things look nice.
	logo_pixmap = gdk_pixbuf_from_pixdata(&gsdx_ogl_logo, false, NULL);
	logo_image = gtk_image_new_from_pixbuf(logo_pixmap);
	//gtk_container_add(GTK_CONTAINER(main_box), logo_image);
	gtk_box_pack_start(GTK_BOX(main_box), logo_image, true, true, 0);
	
	// Create the renderer combo box and label, and stash them in a box.
	render_label = gtk_label_new ("Renderer:");
	render_combo_box = CreateRenderComboBox();
	renderer_box = gtk_hbox_new(false, 5);
	// Use gtk_box_pack_start instead of gtk_container_add so it lines up nicely.
	gtk_box_pack_start(GTK_BOX(renderer_box), render_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(renderer_box), render_combo_box, false, false, 5);
	
	// Create the interlace combo box and label, and stash them in a box.
	interlace_label = gtk_label_new ("Interlacing (F5):");
	interlace_combo_box = CreateInterlaceComboBox();
	interlace_box = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(interlace_box), interlace_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(interlace_box), interlace_combo_box, false, false, 5);

	// Create the threading spin box and label, and stash them in a box. (Yes, we do a lot of that.)
	threads_label = gtk_label_new("Extra rendering threads:");
	threads_spin = gtk_spin_button_new_with_range(0,100,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(threads_spin), theApp.GetConfig("extrathreads", 0));
	threads_box = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(threads_box), threads_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(threads_box), threads_spin, false, false, 5);

	// A bit of funkiness for the resolution box.
	native_label = gtk_label_new("Original PS2 Resolution: ");
	native_res_check = gtk_check_button_new_with_label("Native");
	native_box = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(native_box), native_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(native_box), native_res_check, false, false, 5);
	
	msaa_label = gtk_label_new("Or Use Scaling (broken):");
	msaa_combo_box = CreateMsaaComboBox();
	msaa_box = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(msaa_box), msaa_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(msaa_box), msaa_combo_box, false, false, 5);
	
	rexy_label = gtk_label_new("Custom Resolution:");
	resx_spin = gtk_spin_button_new_with_range(256,8192,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(resx_spin), theApp.GetConfig("resx", 1024));
	resy_spin = gtk_spin_button_new_with_range(256,8192,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(resy_spin), theApp.GetConfig("resy", 1024));
	resxy_box = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(resxy_box), rexy_label, false, false, 5);
	gtk_box_pack_start(GTK_BOX(resxy_box), resx_spin, false, false, 5);
	gtk_box_pack_start(GTK_BOX(resxy_box), resy_spin, false, false, 5);
	
	// Create our checkboxes.
	filter_check = gtk_check_button_new_with_label("Texture Filtering");
	logz_check = gtk_check_button_new_with_label("Logarithmic Z");
	paltex_check = gtk_check_button_new_with_label("Allow 8 bit textures");
	fba_check = gtk_check_button_new_with_label("Alpha correction (FBA)");
	aa_check = gtk_check_button_new_with_label("Edge anti-aliasing (AA1)");
	
	// Set the checkboxes.
	// Filter should have 3 states, not 2.
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_check), theApp.GetConfig("filter", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logz_check), theApp.GetConfig("logz", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paltex_check), theApp.GetConfig("paltex", 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fba_check), theApp.GetConfig("fba", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aa_check), theApp.GetConfig("aa1", 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(native_res_check), theApp.GetConfig("nativeres", 0));
	
	// Populate all those boxes we created earlier with widgets.
	gtk_container_add(GTK_CONTAINER(res_box), native_box);
	gtk_container_add(GTK_CONTAINER(res_box), msaa_box);
	gtk_container_add(GTK_CONTAINER(res_box), resxy_box);
	
	gtk_container_add(GTK_CONTAINER(sw_box), threads_box);
	gtk_container_add(GTK_CONTAINER(sw_box), aa_check);
	
	// Tables are strange. The numbers are for their position: left, right, top, bottom.
	gtk_table_attach_defaults(GTK_TABLE(hw_table), filter_check, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(hw_table), logz_check, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(hw_table), paltex_check, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(hw_table), fba_check, 1, 2, 1, 2);
	
	// Put everything in the big box.
	gtk_container_add(GTK_CONTAINER(main_box), renderer_box);
	gtk_container_add(GTK_CONTAINER(main_box), interlace_box);
	gtk_container_add(GTK_CONTAINER(main_box), res_frame);
	gtk_container_add(GTK_CONTAINER(main_box), hw_frame);
	gtk_container_add(GTK_CONTAINER(main_box), sw_frame);

	g_signal_connect(render_combo_box, "changed", G_CALLBACK(toggle_widget_states), NULL);
	g_signal_connect(msaa_combo_box, "changed", G_CALLBACK(toggle_widget_states), NULL);
	g_signal_connect(native_res_check, "toggled", G_CALLBACK(toggle_widget_states), NULL);
	// Put the box in the dialog and show it to the world.
	gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_box);
	gtk_widget_show_all (dialog);
	toggle_widget_states(NULL, NULL);
	return_value = gtk_dialog_run (GTK_DIALOG (dialog));

	if (return_value == GTK_RESPONSE_ACCEPT)
	{
		int mode_height = 0, mode_width = 0;
		
		mode_width = theApp.GetConfig("ModeWidth", 640);
		mode_height = theApp.GetConfig("ModeHeight", 480);
		theApp.SetConfig("ModeHeight", mode_height);
		theApp.SetConfig("ModeWidth", mode_width);
		
		// Get all the settings from the dialog box.
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(render_combo_box)) != -1) {
			// FIXME test current opengl version supported through glxinfo (OpenGL version string:)
			// Warn the user if 4.2 is not supported and switch back to basic SDL renderer
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(render_combo_box))) {
				// Note the value are based on m_gs_renderers vector on GSdx.cpp
				case 0: theApp.SetConfig("renderer", 7); break;
				case 1: theApp.SetConfig("renderer", 8); break;
				case 2: theApp.SetConfig("renderer", 10); break;
				case 3: theApp.SetConfig("renderer", 11); break;
				case 4: theApp.SetConfig("renderer", 12); break;
				case 5: theApp.SetConfig("renderer", 13); break;
			}
		}

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(interlace_combo_box)) != -1)
			theApp.SetConfig( "interlace", (int)gtk_combo_box_get_active(GTK_COMBO_BOX(interlace_combo_box)));

		theApp.SetConfig("extrathreads", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(threads_spin)));

		theApp.SetConfig("filter", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_check)));
		theApp.SetConfig("logz", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logz_check)));
		theApp.SetConfig("paltex", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paltex_check)));
		theApp.SetConfig("fba", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fba_check)));
		theApp.SetConfig("aa1", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(aa_check)));
		theApp.SetConfig("nativeres", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(native_res_check)));
		
		theApp.SetConfig("msaa", (int)gtk_combo_box_get_active(GTK_COMBO_BOX(msaa_combo_box)));
		theApp.SetConfig("resx", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(resx_spin)));
		theApp.SetConfig("resy", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(resy_spin)));
		
		// Let's just be windowed for the moment.
		theApp.SetConfig("windowed", 1);

		gtk_widget_destroy (dialog);

		return true;
	}

	gtk_widget_destroy (dialog);

	return false;
}
