/*  progressbox.h
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


#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H


#include <sys/types.h> // off64_t

#include <gtk/gtkwidget.h> // GtkWidget


extern const char *compressnames[];

struct ProgressBoxData {
  GtkWidget *window; // GtkWindow
  GtkWidget *desc; // GtkLabel - What are we showing progress on?
  GtkWidget *bar; // GtkProgressBar

  off64_t max;
  gdouble gmax;
  off64_t lastpct;

  int stop; // Someone pressed the Stop button
};

extern struct ProgressBoxData progressbox;


extern void ProgressBoxStart(char *description, off64_t maximum);
extern void ProgressBoxTick(off64_t current);
extern void ProgressBoxStop();
extern void ProgressBoxDestroy();
extern void ProgressBoxDisplay();


#endif /* MAINBOX_H */
