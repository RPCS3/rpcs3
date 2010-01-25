/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "CpuUsageProvider.h"

// CpuUsageProvider under Linux  (Doesn't Do Jack, yet!)
//
// Currently this just falls back on the Default, which itself is not implemented under
// linux, as of me writing this.  I'm not sure if it'll be easier to implement the
// default provider (see Utilities/LnxThreads.cpp for details) or to use a custom/manual
// implementation here.

CpuUsageProvider::CpuUsageProvider() :
	m_Implementation( new DefaultCpuUsageProvider() )
{
}

CpuUsageProvider::~CpuUsageProvider() throw()
{
}
