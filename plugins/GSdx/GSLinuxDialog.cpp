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
	GtkWidget *swthreads_label, *swthreads_text;
	GtkWidget *filter_check, *logz_check, *paltex_check, *fba_check, *aa_check, *win_check;
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
	gtk_combo_box_append_text(GTK_COMBO_BOX(res_combo_box), "1280x960@60");
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

	swthreads_label = gtk_label_new("Software renderer threads:");
	swthreads_text = gtk_entry_new();
	char buf[5];
	sprintf(buf, "%d", theApp.GetConfig("swthreads", 1));

	gtk_entry_set_text(GTK_ENTRY(swthreads_text), buf);
	gtk_container_add(GTK_CONTAINER(main_box), swthreads_label);
	gtk_container_add(GTK_CONTAINER(main_box), swthreads_text);



	filter_check = gtk_check_button_new_with_label("Texture Filtering");
	logz_check = gtk_check_button_new_with_label("Logarithmic Z");
	paltex_check = gtk_check_button_new_with_label("Allow 8 bit textures");
	fba_check = gtk_check_button_new_with_label("Alpha correction (FBA)");
	aa_check = gtk_check_button_new_with_label("Edge anti-aliasing");
	win_check = gtk_check_button_new_with_label("Disable Effects Processing");

	gtk_container_add(GTK_CONTAINER(main_box), filter_check);
	gtk_container_add(GTK_CONTAINER(main_box), logz_check);
	gtk_container_add(GTK_CONTAINER(main_box), paltex_check);
	gtk_container_add(GTK_CONTAINER(main_box), fba_check);
	gtk_container_add(GTK_CONTAINER(main_box), aa_check);
	gtk_container_add(GTK_CONTAINER(main_box), win_check);

	// Filter should be 3 states, not 2.
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_check), theApp.GetConfig("filter", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logz_check), theApp.GetConfig("logz", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paltex_check), theApp.GetConfig("paltex", 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fba_check), theApp.GetConfig("fba", 1));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aa_check), theApp.GetConfig("aa1", 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win_check), theApp.GetConfig("windowed", 1));

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
		#endif
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(res_combo_box)) != -1) {
			int resolution = gtk_combo_box_get_active(GTK_COMBO_BOX(res_combo_box));
			switch (resolution) {
				case 0: theApp.SetConfig("w", 640); theApp.SetConfig("h", 480); break;
				case 1: theApp.SetConfig("w", 800); theApp.SetConfig("h", 600); break;
				case 2: theApp.SetConfig("w", 1024); theApp.SetConfig("h", 768); break;
				case 3: theApp.SetConfig("w", 1280); theApp.SetConfig("h", 960); break;
				default: theApp.SetConfig("w", 640); theApp.SetConfig("h", 480);
			}

		}

		theApp.SetConfig("swthreads", atoi((char*)gtk_entry_get_text(GTK_ENTRY(swthreads_text))) );

        theApp.SetConfig("filter", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_check)));
        theApp.SetConfig("logz", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logz_check)));
        theApp.SetConfig("paltex", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paltex_check)));
        theApp.SetConfig("fba", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fba_check)));
        theApp.SetConfig("aa1", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(aa_check)));
        theApp.SetConfig("windowed", (int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win_check)));

		gtk_widget_destroy (dialog);

		return true;
	}

	gtk_widget_destroy (dialog);

	return false;
}
