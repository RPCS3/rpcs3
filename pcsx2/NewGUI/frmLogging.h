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
 
#include <wx/wx.h>
#include <wx/image.h>

#include "wxHelpers.h"

#pragma once

class frmLogging: public wxDialogWithHelpers
{
public:
	frmLogging( wxWindow* parent, int id, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

protected:

	// DECLARE_EVENT_TABLE();
	enum {
		EE_CPU_LOG = 100,
		EE_MEM_LOG,
		EE_HW_LOG,
		EE_DMA_LOG,
		EE_BIOS_LOG,
		EE_ELF_LOG,
		EE_FPU_LOG,
		EE_MMI_LOG,
		EE_VU0_LOG,
		EE_COP0_LOG,
		EE_VIF_LOG,
		EE_SPR_LOG,
		EE_GIF_LOG,
		EE_SIF_LOG,
		EE_IPU_LOG,
		EE_VU_MACRO_LOG,
		EE_RPC_LOG,
		
		IOP_IOP_LOG,
		IOP_MEM_LOG,
		IOP_HW_LOG,
		IOP_BIOS_LOG,
		IOP_DMA_LOG,
		IOP_PAD_LOG,
		IOP_GTE_LOG,
		IOP_CDR_LOG,
		IOP_GPU_LOG,
		
		STDOUT_LOG,
		SYMS_LOG
	} LogChecks;
	
public:
	void LogChecked(wxCommandEvent &event);
};
