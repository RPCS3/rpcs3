/*  GSnull
 *  Copyright (C) 2004-2009 PCSX2 Team
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
 */
 
 /*  GSnull
 *  Copyright (C) 2004-2009 PCSX2 Team
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
 */
 
 struct GIF_CTRL
 {
	 u32 RST:1, // GIF reset
	 u32 reserved:2,
	 u32 PSE:1, // Temporary Transfer Stop
	 u32 reserved2:28
 };
 
 struct GIF_MODE
 {
	 u32 M3R:1,
	 u32 reserved:1,
	 u32 IMT:1,
	 u32 reserved2:29
 };
 
 struct GIF_STAT
 {
	 u32 M3R:1,
	 u32 M3P:1,
	 u32 IMT:1,
	 u32 PSE:1,
	 u32 reserved:1,
	 u32 IP3:1,
	 u32 P3Q:1,
	 u32 P2Q:1,
	 u32 P1Q:1,
	 u32 OPH:1,
	 u32 APATH:2,
	 u32 DIR:1,
	 u32 reserved2:11,
	 u32 FQC:5,
	 y32 reserved3:3
 };
 
 struct GIF_CNT
 {
	 u32 LOOPCNT:15,
	 u32 reserved:1,
	 u32 REGCNT:4,
	 u32 VUADDR:10,
	 u32 rese3rved2:2
 };
 
 struct GIF_P3CNT
 {
	 u32 PS3CNT:15,
	 u32 reserved:17
 };
 
 struct GIF_P3TAG
 {
	 u32 LOOPCNT:15,
	 u32 EOP:1,
	 u32 reserved:16
 };
 
 