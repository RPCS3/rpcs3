/*  interface.c
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <stddef.h> // NULL
#include <stdio.h> // sprintf()
#include <string.h> // strcmp()

#include <gtk/gtkmain.h> // gtk_init(), gtk_main(), gtk_main_quit()
#include <gtk/gtkwidget.h> // gtk_widget_show_all()

#include "logfile.h"
#include "conf.h"
#include "aboutbox.h"
#include "mainbox.h"
#include "devicebox.h"
#include "selectionbox.h"
#include "progressbox.h"
#include "messagebox.h"
#include "conversionbox.h"
#include "interface.h"


int main(int argc, char *argv[]) {
  if(argc != 2)  return(1);

  gtk_init(NULL, NULL);

  if(!strcmp(argv[1], "about")) {
    AboutBoxDisplay();
    return(0);
  } else if (!strcmp(argv[1], "configure")) {
    OpenLog();
    InitConf();
    LoadConf();
    MainBoxDisplay();
    DeviceBoxDisplay();
    ConversionBoxDisplay();
    ProgressBoxDisplay();
    MessageBoxDisplay();
    SelectionBoxDisplay();

    gtk_widget_show_all(mainbox.window);
    gtk_main();
    CloseLog();
    return(0);
  } // ENDLONGIF- Which display would you like to see?

  return(1); // No Displays chosen? Abort!
} // END main()
