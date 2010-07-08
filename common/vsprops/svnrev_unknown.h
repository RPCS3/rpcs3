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

// svnrev_genric.h --> svnrev.h
//
// This file acts as a placebo for people who do not have TortoiseSVN installed.
// It provides "empty" revision information to the Pcsx2 Playground projects in
// the absence of real revisions derived from the repository being built.
//
// This file does not affect application/dll builds in any significant manner,
// other than the lack of automatic revision tags inserted into the app (which
// is very convenient but hardly necessary).
//
// See svn_template.h for more information on how the process of revision
// templating works.
//
// If you would like to enable automatic revisin tagging, TortoiseSVN can be
// downloaded from http://tortoisesvn.tigris.org

#define SVN_REV_UNKNOWN

// The following defines are included so that code will still compile even if it
// doesn't check for the SVN_REV_UNKNOWN define.

#define SVN_REV 0
#define SVN_MODS 0

