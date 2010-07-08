/*  PCSX2 - PS2 Emulator for PCs
*  Copyright (C) 2002-2010  PCSX2 Dev Team
*
*  PCSX2 is free software: you can redistribute it and/or modify it under the terms
*  of the GNU Lesser General Public License as published by the Free Software Found-
*  ation, either version 3 of the License, or (at your option) any later version.
*
*  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*  PURPOSE.  See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with PCSX2.
*  If not, see <http://www.gnu.org/licenses/>.
*/

// svnrev_template.h --> svnrev.h
//
// This file acts as a template for the automatic SVN revision/version tag.
// It is used by the utility SubWCrev.exe to create an "svnrev.h" file for
// whichever project is being compiled (as indicated by command line options
// passed to SubWCRev.exe during the project's pre-build step).
//
// The SubWCRev.exe utility is part of TortoiseSVN and requires several DLLs
// installed by TortoiseSVN, so it will only be available if you have TortoiseSVN
// installed on your system.  If you do not have it installed, a generic template
// is used instead (see svnrev_generic.h).  Having TortoiseSVN is handy but not
// necessary.  If you do not have it installed, everything will still compile
// fine except without the SVN revision tagged to the application/dll version.
//
// TortoiseSVN can be downloaded from http://tortoisesvn.tigris.org

#define SVN_REV $WCREV$
#define SVN_MODS $WCMODS?1:0$
