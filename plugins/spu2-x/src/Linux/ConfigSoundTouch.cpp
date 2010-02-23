/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "Global.h"
#include "Dialogs.h"
#include "Config.h"
#include "SoundTouch/SoundTouch.h"

namespace SoundtouchCfg
{
void ClampValues()
{
	Clampify( SequenceLenMS, SequenceLen_Min, SequenceLen_Max );
	Clampify( SeekWindowMS, SeekWindow_Min, SeekWindow_Max );
	Clampify( OverlapMS, Overlap_Min, Overlap_Max );
}

void ApplySettings( soundtouch::SoundTouch& sndtouch )
{
	sndtouch.setSetting( SETTING_SEQUENCE_MS,	SequenceLenMS );
	sndtouch.setSetting( SETTING_SEEKWINDOW_MS,	SeekWindowMS );
	sndtouch.setSetting( SETTING_OVERLAP_MS,	OverlapMS );
}

void ReadSettings()
{
	SequenceLenMS	= CfgReadInt( L"SOUNDTOUCH", L"SequenceLengthMS", 63 );
	SeekWindowMS	= CfgReadInt( L"SOUNDTOUCH", L"SeekWindowMS", 16 );
	OverlapMS		= CfgReadInt( L"SOUNDTOUCH", L"OverlapMS", 7 );

	ClampValues();
	WriteSettings();		
}

void WriteSettings()
{
	CfgWriteInt( L"SOUNDTOUCH", L"SequenceLengthMS", SequenceLenMS );
	CfgWriteInt( L"SOUNDTOUCH", L"SeekWindowMS", SeekWindowMS );
	CfgWriteInt( L"SOUNDTOUCH", L"OverlapMS", OverlapMS );
}

/*bool CALLBACK SoundtouchCfg::DialogProc(uptr hWnd,u32 uMsg,uptr wParam,uptr lParam)
{
}*/

void OpenDialog( uptr hWnd )
{
	ReadSettings();
	WriteSettings();
}
}
