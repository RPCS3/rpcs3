/*  conversionbox.c
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

#include <windows.h>
#include <windowsx.h> // ComboBox_AddString(), CheckDlgButton()
#include <windef.h> // NULL
// #include <commctrl.h>
#include <stdio.h> // sprintf()
#include <string.h> // strcpy()
#include <sys/types.h> // off64_t
#include "isofile.h"
#include "isocompress.h" // compressdesc[]
#include "imagetype.h" // imagedata[]
#include "multifile.h" // multinames[]
#include "toc.h"
#include "progressbox.h"
#include "mainbox.h"
#include "screens.h" // DLG_..., IDC_...
#include "conversionbox.h"

HWND conversionboxwindow;

void ConversionBoxDestroy()
{
	if (conversionboxwindow != NULL)
	{
		EndDialog(conversionboxwindow, FALSE);
		conversionboxwindow = NULL;
	} // ENDIF- Do we have a Main Window still?
} // END ConversionBoxDestroy()

void ConversionBoxUnfocus()
{
	// gtk_widget_set_sensitive(conversionbox.file, FALSE);
	// gtk_widget_set_sensitive(conversionbox.selectbutton, FALSE);
	// gtk_widget_set_sensitive(conversionbox.compress, FALSE);
	// gtk_widget_set_sensitive(conversionbox.multi, FALSE);
	// gtk_widget_set_sensitive(conversionbox.okbutton, FALSE);
	// gtk_widget_set_sensitive(conversionbox.cancelbutton, FALSE);
	ShowWindow(conversionboxwindow, SW_HIDE);
} // END ConversionBoxUnfocus()

void ConversionBoxFileEvent()
{
	int returnval;
	char templine[256];
	struct IsoFile *tempfile;
	GetDlgItemText(conversionboxwindow, IDC_0402, templine, 256);
	returnval = IsIsoFile(templine);
	if (returnval == -1)
	{
		SetDlgItemText(conversionboxwindow, IDC_0404, "File Type: ---");
		return;
	} // ENDIF- Not a name of any sort?

	if (returnval == -2)
	{
		SetDlgItemText(conversionboxwindow, IDC_0404, "File Type: Not a file");
		return;
	} // ENDIF- Not a regular file?
	if (returnval == -3)
	{
		SetDlgItemText(conversionboxwindow, IDC_0404, "File Type: Not a valid image file");
		return;
	} // ENDIF- Not a regular file?

	if (returnval == -4)
	{
		SetDlgItemText(conversionboxwindow, IDC_0404, "File Type: Missing Table File (will rebuild)");
		return;
	} // ENDIF- Not a regular file?
	tempfile = IsoFileOpenForRead(templine);
	sprintf(templine, "File Type: %s%s%s",
	        multinames[tempfile->multi],
	        tempfile->imagename,
	        compressdesc[tempfile->compress]);
	SetDlgItemText(conversionboxwindow, IDC_0404, templine);
	tempfile = IsoFileClose(tempfile);
	return;
} // END ConversionBoxFileEvent()

void ConversionBoxRefocus()
{
	ConversionBoxFileEvent();
	// gtk_widget_set_sensitive(conversionbox.file, TRUE);
	// gtk_widget_set_sensitive(conversionbox.selectbutton, TRUE);
	// gtk_widget_set_sensitive(conversionbox.compress, TRUE);
	// gtk_widget_set_sensitive(conversionbox.multi, TRUE);
	// gtk_widget_set_sensitive(conversionbox.okbutton, TRUE);
	// gtk_widget_set_sensitive(conversionbox.cancelbutton, TRUE);
	// gtk_window_set_focus(GTK_WINDOW(conversionbox.window), conversionbox.file);
	ShowWindow(conversionboxwindow, SW_SHOW);
	SetActiveWindow(conversionboxwindow);
} // END ConversionBoxRefocus()

void ConversionBoxCancelEvent()
{
	// ShowWindow(conversionboxwindow, SW_HIDE);
	ConversionBoxDestroy();
	MainBoxRefocus();
	return;
} // END ConversionBoxCancelEvent()

void ConversionBoxOKEvent()
{
	char templine[256];
	char tempblock[2352];
	char filename[256];
	HWND tempitem;
	int compressmethod;
	int multi;
	struct IsoFile *fromfile;
	struct IsoFile *tofile;
	int i;
	off64_t endsector;
	int stop;
	int retval;
	ConversionBoxUnfocus();
	GetDlgItemText(conversionboxwindow, IDC_0402, filename, 256);
	if (IsIsoFile(filename) < 0)
	{
		ConversionBoxRefocus();
		MessageBox(conversionboxwindow,
		           "Not a Valid Image File.",
		           "CDVDisoEFP Message",
		           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
		return;
	} // ENDIF- Not an Iso File? Stop early.
	tempitem = GetDlgItem(conversionboxwindow, IDC_0406);
	compressmethod = ComboBox_GetCurSel(tempitem);
	tempitem = NULL;
	if (compressmethod > 0)  compressmethod += 2;

	multi = 0;
	if (IsDlgButtonChecked(conversionboxwindow, IDC_0408))  multi = 1;
	fromfile = NULL;
	fromfile = IsoFileOpenForRead(filename);
	if (fromfile == NULL)
	{
		ConversionBoxRefocus();
		MessageBox(conversionboxwindow,
		           "Cannot opening the source file",
		           "CDVDisoEFP Message",
		           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
		return;
	} // ENDIF- Not an Iso File? Stop early.

	if ((compressmethod == fromfile->compress) &&
	        (multi == fromfile->multi))
	{
		fromfile = IsoFileClose(fromfile);
		ConversionBoxRefocus();
		MessageBox(conversionboxwindow,
		           "Compress/Multifile methods match - no need to convert",
		           "CDVDisoEFP Message",
		           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
		return;
	} // ENDIF- Not an Iso File? Stop early.
	tofile = IsoFileOpenForWrite(filename,
	                             GetImageTypeConvertTo(fromfile->imagetype),
	                             multi,
	                             compressmethod);
	if (tofile == NULL)
	{
		fromfile = IsoFileClose(fromfile);
		ConversionBoxRefocus();
		MessageBox(conversionboxwindow,
		           "Cannot create the new file",
		           "CDVDisoEFP Message",
		           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
		return;
	} // ENDIF- Not an Iso File? Stop early.
	if (fromfile->multi == 1)
	{
		i = 0;
		while ((i < 10) &&
		        (IsoFileSeek(fromfile, fromfile->multisectorend[i] + 1) == 0))  i++;
		endsector = fromfile->multisectorend[fromfile->multiend];
	}
	else
	{
		endsector = fromfile->filesectorsize;
	} // ENDIF- Get ending sector from multifile? (Or single file?)
	IsoFileSeek(fromfile, 0);
	// Open Progress Bar
	sprintf(templine, "%s: %s%s -> %s%s",
	        filename,
	        multinames[fromfile->multi],
	        compressdesc[fromfile->compress],
	        multinames[tofile->multi],
	        compressdesc[tofile->compress]);
	ProgressBoxStart(templine, endsector);

	tofile->cdvdtype = fromfile->cdvdtype;
	for (i = 0; i < 2048; i++)  tofile->toc[i] = fromfile->toc[i];
	stop = 0;
	mainboxstop = 0;
	progressboxstop = 0;
	while ((stop == 0) && (tofile->sectorpos < endsector))
	{
		retval = IsoFileRead(fromfile, tempblock);
		if (retval < 0)
		{
			MessageBox(conversionboxwindow,
			           "Trouble reading source file",
			           "CDVDisoEFP Message",
			           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
			stop = 1;
		}
		else
		{
			retval = IsoFileWrite(tofile, tempblock);
			if (retval < 0)
			{
				MessageBox(conversionboxwindow,
				           "Trouble writing new file",
				           "CDVDisoEFP Message",
				           MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
				stop = 1;
			} // ENDIF- Trouble writing out the next block?
		} // ENDIF- Trouble reading in the next block?
		ProgressBoxTick(tofile->sectorpos);
		if (mainboxstop != 0)  stop = 2;
		if (progressboxstop != 0)  stop = 2;
	} // ENDWHILE- Not stopped for some reason...
	ProgressBoxStop();
	if (stop == 0)
	{
		if (tofile->multi == 1)  tofile->name[tofile->multipos] = '0'; // First file
		strcpy(templine, tofile->name);
		// fromfile = IsoFileCloseAndDelete(fromfile);
		fromfile = IsoFileClose(fromfile);

		IsoSaveTOC(tofile);
		tofile = IsoFileClose(tofile);
		SetDlgItemText(mainboxwindow, IDC_0202, templine);

	}
	else
	{
		fromfile = IsoFileClose(fromfile);
		tofile = IsoFileCloseAndDelete(tofile);
	} // ENDIF- Did we succeed in the transfer?
	ConversionBoxRefocus();
	if (stop == 0)  ConversionBoxCancelEvent();
	return;
} // END ConversionBoxOKEvent()

void ConversionBoxBrowseEvent()
{
	OPENFILENAME filebox;
	char newfilename[256];
	BOOL returnbool;

	filebox.lStructSize = sizeof(filebox);
	filebox.hwndOwner = conversionboxwindow;
	filebox.hInstance = NULL;
	filebox.lpstrFilter = fileselection;
	filebox.lpstrCustomFilter = NULL;
	filebox.nFilterIndex = 0;
	filebox.lpstrFile = newfilename;
	filebox.nMaxFile = 256;
	filebox.lpstrFileTitle = NULL;
	filebox.nMaxFileTitle = 0;
	filebox.lpstrInitialDir = NULL;
	filebox.lpstrTitle = NULL;
	filebox.Flags = OFN_FILEMUSTEXIST
	                | OFN_NOCHANGEDIR
	                | OFN_HIDEREADONLY;
	filebox.nFileOffset = 0;
	filebox.nFileExtension = 0;
	filebox.lpstrDefExt = NULL;
	filebox.lCustData = 0;
	filebox.lpfnHook = NULL;
	filebox.lpTemplateName = NULL;

	GetDlgItemText(conversionboxwindow, IDC_0402, newfilename, 256);
	returnbool = GetOpenFileName(&filebox);
	if (returnbool != FALSE)
	{
		SetDlgItemText(conversionboxwindow, IDC_0402, newfilename);
	} // ENDIF- User actually selected a name? Save it.

	return;
} // END ConversionBoxBrowseEvent()

void ConversionBoxDisplay()
{
	char templine[256];
	HWND itemptr;
	int itemcount;
	// Adjust window position?
	// Pull the name from the Main Window... for a starting place.
	GetDlgItemText(mainboxwindow, IDC_0202, templine, 256);
	SetDlgItemText(conversionboxwindow, IDC_0402, templine);
	// ConversionBoxFileEvent(); // Needed?
	itemptr = GetDlgItem(conversionboxwindow, IDC_0406); // Compression Combo
	itemcount = 0;
	while (compressnames[itemcount] != NULL)
	{
		ComboBox_AddString(itemptr, compressnames[itemcount]);
		itemcount++;
	} // ENDWHILE- loading compression types into combo box
	ComboBox_SetCurSel(itemptr, 0); // First Selection?
	itemptr = NULL;

	CheckDlgButton(conversionboxwindow, IDC_0408, FALSE); // Start unchecked

	return;
} // END ConversionBoxDisplay()

BOOL CALLBACK ConversionBoxCallback(HWND window,
                                    UINT msg,
                                    WPARAM param,
                                    LPARAM param2)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			conversionboxwindow = window;
			ConversionBoxDisplay(); // Final touches to this window
			return(FALSE); // Let Windows display this window
			break;
		case WM_CLOSE: // The "X" in the upper right corner was hit.
			ConversionBoxCancelEvent();
			return(TRUE);
			break;

		case WM_COMMAND:
			switch (LOWORD(param))
			{
				case IDC_0402: // Filename Edit Box
					ConversionBoxFileEvent(); // Describe the File's current type...
					return(FALSE); // Let Windows edit the text.
					break;
				case IDC_0403: // "Browse" Button
					ConversionBoxBrowseEvent();
					return(TRUE);
					break;

				case IDC_0409: // "Change File" Button
					ConversionBoxOKEvent();
					return(TRUE);
					break;
				case IDC_0410: // "Cancel" Button
					ConversionBoxCancelEvent();
					return(TRUE);
					break;
			} // ENDSWITCH param- Which object got the message?
	} // ENDSWITCH msg- what message has been sent to this window?

	return(FALSE);
} // END ConversionBoxEventLoop()
