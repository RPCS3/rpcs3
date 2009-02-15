/* 
 *	Copyright (C) 2007 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

class xpadApp : public CWinApp
{
public:
	xpadApp();

public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

// ps1

#define PSE_LT_PAD 8

// MOUSE SCPH-1030
#define PSE_PAD_TYPE_MOUSE			1
// NEGCON - 16 button analog controller SLPH-00001
#define PSE_PAD_TYPE_NEGCON			2
// GUN CONTROLLER - gun controller SLPH-00014 from Konami
#define PSE_PAD_TYPE_GUN			3
// STANDARD PAD SCPH-1080, SCPH-1150
#define PSE_PAD_TYPE_STANDARD		4
// ANALOG JOYSTICK SCPH-1110
#define PSE_PAD_TYPE_ANALOGJOY		5
// GUNCON - gun controller SLPH-00034 from Namco
#define PSE_PAD_TYPE_GUNCON			6
// ANALOG CONTROLLER SCPH-1150
#define PSE_PAD_TYPE_ANALOGPAD		7

struct PadDataS
{
	// controler type - fill it withe predefined values above
	BYTE type;
	
	// status of buttons - every controller fills this field
	WORD status;
	
	// for analog pad fill those next 4 bytes
	// values are analog in range 0-255 where 128 is center position
	BYTE rightJoyX, rightJoyY, leftJoyX, leftJoyY;

	// for mouse fill those next 2 bytes
	// values are in range -128 - 127
	BYTE moveX, moveY;

	BYTE reserved[91];
} ;

// ps2

#define PS2E_LT_PAD 0x02
#define PS2E_PAD_VERSION 0x0002

struct KeyEvent
{
	UINT32 key;
	UINT32 event;
};

#define KEYPRESS	1
#define KEYRELEASE	2
