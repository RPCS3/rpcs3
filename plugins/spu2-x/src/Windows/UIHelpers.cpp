/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "Dialogs.h"

int SendDialogMsg( HWND hwnd, int dlgId, UINT code, WPARAM wParam, LPARAM lParam)
{
	return SendMessage( GetDlgItem(hwnd,dlgId), code, wParam, lParam );
}

HRESULT GUIDFromString(const char *str, LPGUID guid)
{
	// "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"

	struct T{	// this is a hack because for some reason sscanf writes too much :/
		GUID g;
		int k;
	} t;

	int r = sscanf_s(str,"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		&t.g.Data1,
		&t.g.Data2,
		&t.g.Data3,
		&t.g.Data4[0],
		&t.g.Data4[1],
		&t.g.Data4[2],
		&t.g.Data4[3],
		&t.g.Data4[4],
		&t.g.Data4[5],
		&t.g.Data4[6],
		&t.g.Data4[7]
	);

	if(r!=11) return -1;

	*guid = t.g;
	return 0;
}

__forceinline void Verifyc(HRESULT hr, const char* fn)
{
	if(FAILED(hr))
	{
		assert( 0 );
		throw std::runtime_error( "DirectSound returned an error from %s" );
	}
}

void AssignSliderValue( HWND idcwnd, HWND hwndDisplay, int value )
{
	value = std::min( std::max( value, 0 ), 512 );
	SendMessage(idcwnd,TBM_SETPOS,TRUE,value);

	wchar_t tbox[32];
	swprintf_s( tbox, L"%d", value );
	SetWindowText( hwndDisplay, tbox );
}

void AssignSliderValue( HWND hWnd, int idc, int editbox, int value )
{
	AssignSliderValue( GetDlgItem( hWnd, idc ), GetDlgItem( hWnd, editbox ), value );
}

// Generic slider/scroller message handler.  This is succient so long as you
// don't need some kind of secondary event handling functionality, such as
// updating a custom label.
BOOL DoHandleScrollMessage( HWND hwndDisplay, WPARAM wParam, LPARAM lParam )
{
	int wmId    = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	static char temp[64];
	switch(wmId)
	{
		//case TB_ENDTRACK:
		//case TB_THUMBPOSITION:
		case TB_LINEUP:
		case TB_LINEDOWN:
		case TB_PAGEUP:
		case TB_PAGEDOWN:
			wmEvent = (int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
		case TB_THUMBTRACK:
			AssignSliderValue( (HWND)lParam, hwndDisplay, wmEvent );
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

int GetSliderValue( HWND hWnd, int idc )
{
	int retval = (int)SendMessage( GetDlgItem( hWnd, idc ), TBM_GETPOS, 0, 0 );
	return GetClamped( retval, 0, 512 );
}

