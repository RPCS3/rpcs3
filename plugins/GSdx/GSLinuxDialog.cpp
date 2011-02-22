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

bool RunLinuxDialog()
{
	GtkWidget *dialog;
	GtkWidget *main_frame, *main_box;
	GtkWidget *res_label, *res_combo_box;
	GtkWidget *render_label, *render_combo_box;
	GtkWidget *interlace_label, *interlace_combo_box;
	GtkWidget *aspect_label, *aspect_combo_box;
	GtkWidget *texture_check, *log_check, *an_8_bit_check, *alpha_check, *aa_check, *win_check;
	int return_value;

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

	main_box = gtk_vbox_new(false, 5);
	main_frame = gtk_frame_new ("GSdx Config");
	gtk_container_add (GTK_CONTAINER(main_frame), main_box);

	res_label = gtk_label_new ("Interpolation:");
	res_combo_box = gtk_combo_box_new_text ();
	gtk_combo_box_append_text(GTK_COMBO_BOX(res_combo_box), "640x480@60");
	gtk_combo_box_append_text(GTK_COMBO_BOX(res_combo_box), "800x600@60");
	gtk_combo_box_append_text(GTK_COMBO_BOX(res_combo_box), "1024x768@60");
	gtk_combo_box_append_text(GTK_COMBO_BOX(res_combo_box), "And a few other values like that.");

	// Or whatever the default value is.
	gtk_combo_box_set_active(GTK_COMBO_BOX(res_combo_box), 2);

	gtk_container_add(GTK_CONTAINER(main_box), res_label);
	gtk_container_add(GTK_CONTAINER(main_box), res_combo_box);


	render_label = gtk_label_new ("Renderer:");
	render_combo_box = gtk_combo_box_new_text ();

	for(size_t i = 6; i < theApp.m_gs_renderers.size(); i++)
	{
		const GSSetting& s = theApp.m_gs_renderers[i];

		string label = s.name;

		if(!s.note.empty()) label += format(" (%s)", s.note.c_str());

		gtk_combo_box_append_text(GTK_COMBO_BOX(render_combo_box), label.c_str());
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(render_combo_box), 0);
	gtk_container_add(GTK_CONTAINER(main_box), render_label);
	gtk_container_add(GTK_CONTAINER(main_box), render_combo_box);


	interlace_label = gtk_label_new ("Interlace:");
	interlace_combo_box = gtk_combo_box_new_text ();

	for(size_t i = 0; i < theApp.m_gs_interlace.size(); i++)
	{
		const GSSetting& s = theApp.m_gs_interlace[i];

		string label = s.name;

		if(!s.note.empty()) label += format(" (%s)", s.note.c_str());

		gtk_combo_box_append_text(GTK_COMBO_BOX(interlace_combo_box), label.c_str());
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(interlace_combo_box), 0);
	gtk_container_add(GTK_CONTAINER(main_box), interlace_label);
	gtk_container_add(GTK_CONTAINER(main_box), interlace_combo_box);

	aspect_label = gtk_label_new ("Aspect Ratio:");
	aspect_combo_box = gtk_combo_box_new_text ();

	for(size_t i = 0; i < theApp.m_gs_aspectratio.size(); i++)
	{
		const GSSetting& s = theApp.m_gs_aspectratio[i];

		string label = s.name;

		if(!s.note.empty()) label += format(" (%s)", s.note.c_str());

		gtk_combo_box_append_text(GTK_COMBO_BOX(aspect_combo_box), label.c_str());
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(aspect_combo_box), 0);
	gtk_container_add(GTK_CONTAINER(main_box), aspect_label);
	gtk_container_add(GTK_CONTAINER(main_box), aspect_combo_box);


	texture_check = gtk_check_button_new_with_label("Texture Filtering");
	log_check = gtk_check_button_new_with_label("Logarithmic Z");
	an_8_bit_check = gtk_check_button_new_with_label("Allow 8 bit textures");
	alpha_check = gtk_check_button_new_with_label("Alpha correction (FBA)");
	aa_check = gtk_check_button_new_with_label("Edge anti-aliasing");
	win_check = gtk_check_button_new_with_label("Disable Effects Processing");

	gtk_container_add(GTK_CONTAINER(main_box), texture_check);
	gtk_container_add(GTK_CONTAINER(main_box), log_check);
	gtk_container_add(GTK_CONTAINER(main_box), an_8_bit_check);
	gtk_container_add(GTK_CONTAINER(main_box), alpha_check);
	gtk_container_add(GTK_CONTAINER(main_box), aa_check);
	gtk_container_add(GTK_CONTAINER(main_box), win_check);

	// These should be set to their actual values, not false.
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(texture_check), false);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(log_check), false);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(an_8_bit_check), false);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alpha_check), false);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aa_check), false);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win_check), false);

	gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_frame);
	gtk_widget_show_all (dialog);

	return_value = gtk_dialog_run (GTK_DIALOG (dialog));

	if (return_value == GTK_RESPONSE_ACCEPT)
	{
		// Get all the settings from the dialog box.

		#if 0 // I'll put the right variable names in later.
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(res_combo_box)) != -1)
			resolution = gtk_combo_box_get_active(GTK_COMBO_BOX(res_combo_box));

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(render_combo_box)) != -1)
			renderer = gtk_combo_box_get_active(GTK_COMBO_BOX(render_combo_box));

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(interlace_combo_box)) != -1)
			interlace = gtk_combo_box_get_active(GTK_COMBO_BOX(interlace_combo_box));

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(aspect_combo_box)) != -1)
			aspect = gtk_combo_box_get_active(GTK_COMBO_BOX(aspect_combo_box));


		texture = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(texture_check));
		log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_check));
		8_bit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(an_8_bit_check));
		alpha = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(alpha_check));
		aa = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(aa_check));
		windowed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win_check));

		#endif

		gtk_widget_destroy (dialog);

		return true;
	}

	gtk_widget_destroy (dialog);

	return false;
}
