/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
 
#include <wx/wx.h>
#include <wx/image.h>

#include "wxHelpers.h"
#include "CheckedStaticBox.h"

class frmLogging: public wxDialogWithHelpers
{
public:
	frmLogging( wxWindow* parent, int id, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

protected:
	enum LogChecks
	{
		LogID_StdOut = 100,
		LogID_Symbols
	};

	/////////////////////////////////////////////////////////////////////////////////////
	//
	class iopLogOptionsPanel : public CheckedStaticBox
	{
	protected:
		enum LogCheckIDs
		{
			LogID_IopBox = 100,
			LogID_Disasm,
			LogID_Memory,
			LogID_Hardware,
			LogID_Bios,
			LogID_DMA,
			LogID_Pad,
			LogID_Cdrom,
			LogID_GPU
		};
		
	public:
		iopLogOptionsPanel( wxWindow* parent );
		void OnLogChecked(wxCommandEvent &event);

	};
	
	/////////////////////////////////////////////////////////////////////////////////////
	//
	class eeLogOptionsPanel : public CheckedStaticBox
	{
	protected:
		enum LogCheckIDs
		{
			// Group boxes and misc logs
			LogID_EEBox = 100,
			LogID_Disasm,
			LogID_Hardware,
			LogID_Memory,
			LogID_Bios,
			LogID_ELF,

			// Disasm section
			LogID_CPU = 200,
			LogID_FPU,
			LogID_VU0,
			LogID_COP0,
			LogID_VU_Macro,

			LogID_Registers = 300,
			LogID_DMA,
			LogID_VIF,
			LogID_SPR,
			LogID_GIF,
			LogID_SIF,
			LogID_IPU,
			LogID_RPC
		};

	public:
		eeLogOptionsPanel( wxWindow* parent );
		void OnLogChecked(wxCommandEvent &event);

	protected:
		class DisasmPanel : public CheckedStaticBox
		{
		public:
			DisasmPanel( wxWindow* parent );
			void OnLogChecked(wxCommandEvent &event);
		};

		class HwPanel : public CheckedStaticBox
		{
		public:
			HwPanel( wxWindow* parent );
			void OnLogChecked(wxCommandEvent &event);
		};
	};


public:
	void LogChecked(wxCommandEvent &event);
	
protected:
	//DECLARE_EVENT_TABLE()

};
