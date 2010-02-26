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
 
 // To be continued...
 
 #include "Dialogs.h"
#include <gtk/gtk.h>
#include <cstring>

void __forceinline SysMessage(const char *fmt, ...)
{
    va_list list;
    char msg[512];

    va_start(list, fmt);
    vsprintf(msg, fmt, list);
    va_end(list);

    if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     "%s", msg);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void SysMessage(const wchar_t *fmt, ...)
{
	va_list list;
	va_start(list,fmt);
	wxString msg;
	msg.PrintfV( fmt, list );
	va_end(list);
	
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     "%s", msg.ToUTF8().data());
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void DspUpdate()
{
}

s32 DspLoadLibrary(wchar_t* fileName, int modnum)
{
	return 0;
}

