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

#pragma once

// AppForwardDefs.h
//
// Purpose:
//   This header file is meant to be a dependency-free include that provides a relatively
//   full compliment of forward defines for PCSX2/App and wxwidgets types.  When 
//   forward defined in this way, these types can be used by method and class definitions
//   as either pointers or handles without running into complicated header file 
//   inter-dependence.
//

class MainEmuFrame;
class GSFrame;
class ConsoleLogFrame;
class PipeRedirectionBase;
class AppCoreThread;
class Pcsx2AppMethodEvent;
class IniInterface;

// wxWidgets forward declarations

class wxConfigBase;
class wxFileConfig;
class wxDirPickerCtrl;
class wxFilePickerCtrl;
class wxFileDirPickerEvent;
class wxListBox;
class wxListCtrl;
class wxListView;
class wxListbook;
class wxSpinCtrl;
class wxBookCtrlBase;

class wxListEvent;
