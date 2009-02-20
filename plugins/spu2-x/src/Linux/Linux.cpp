/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
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

#include "Linux.h"

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
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "SPU2null Msg");
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
