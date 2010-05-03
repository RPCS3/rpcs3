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

#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__

namespace R3000A
{
	extern const char *biosA0n[256];
	extern const char *biosB0n[256];
	extern const char *biosC0n[256];

	void psxBiosInit();
	void psxBiosShutdown();
	void psxBiosException();
	void psxBiosFreeze(int Mode);

	extern void (*biosA0[256])();
	extern void (*biosB0[256])();
	extern void (*biosC0[256])();

	extern void bios_write();
	extern void bios_printf();

}
#endif /* __PSXBIOS_H__ */
