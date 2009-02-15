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


#include "SPU2.h"
#include "Dialogs.h"
#include "RegTable.h"


static bool debugDialogOpen=false;
static HWND hDebugDialog=NULL;

static BOOL CALLBACK DebugProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;

	switch(uMsg)
	{
		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			{
				debugDialogOpen=true;
			}
		break;
		
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
				case IDCANCEL:
					debugDialogOpen=false;
					EndDialog(hWnd,0);
					break;
				default:
					return FALSE;
			}
		break;
		
		default:
			return FALSE;
	}
	return TRUE;
}

#ifndef PUBLIC

int FillRectangle(HDC dc, int left, int top, int width, int height)
{
	RECT r = { left, top, left+width, top+height };

	return FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

BOOL DrawRectangle(HDC dc, int left, int top, int width, int height)
{
	RECT r = { left, top, left+width, top+height };

	POINT p[5] = {
		{ r.left, r.top },
		{ r.right, r.top },
		{ r.right, r.bottom },
		{ r.left, r.bottom },
		{ r.left, r.top },
	};

	return Polyline(dc, p, 5);
}


HFONT hf = NULL;
int lCount=0;
void UpdateDebugDialog()
{
	if(!debugDialogOpen) return;

	lCount++;
	if(lCount>=(SampleRate/10))
	{
		HDC hdc = GetDC(hDebugDialog);

		if(!hf)
		{
			hf = CreateFont( 8, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Lucida Console") );
		}

		SelectObject(hdc,hf);
		SelectObject(hdc,GetStockObject(DC_BRUSH));
		SelectObject(hdc,GetStockObject(DC_PEN));

		for(int c=0;c<2;c++)
		{
			for(int v=0;v<24;v++)
			{
				int IX = 8+256*c;
				int IY = 8+ 32*v;
				V_Voice& vc(Cores[c].Voices[v]);
				V_VoiceDebug& vcd( DebugCores[c].Voices[v] );

				SetDCBrushColor(hdc,RGB(  0,  0,  0));
				if((vc.ADSR.Phase>0)&&(vc.ADSR.Phase<6))
				{
					SetDCBrushColor(hdc,RGB(  0,  0,128));
				}
				else
				{
					if(vcd.lastStopReason==1)
					{
						SetDCBrushColor(hdc,RGB(128,  0,  0));
					}
					if(vcd.lastStopReason==2)
					{
						SetDCBrushColor(hdc,RGB(  0,128,  0));
					}
				}

				FillRectangle(hdc,IX,IY,252,30);

				SetDCPenColor(hdc,RGB(  255,  128,  32));

				DrawRectangle(hdc,IX,IY,252,30);

				SetDCBrushColor  (hdc,RGB(  0,255,  0));

				int vl = abs(((vc.VolumeL.Value >> 16) * 24) >> 15);
				int vr = abs(((vc.VolumeR.Value >> 16) * 24) >> 15);

				FillRectangle(hdc,IX+38,IY+26 - vl, 4, vl);
				FillRectangle(hdc,IX+42,IY+26 - vr, 4, vr);

				int adsr = (vc.ADSR.Value>>16) * 24 / 32768;

				FillRectangle(hdc,IX+48,IY+26 - adsr, 4, adsr);

				int peak = vcd.displayPeak * 24 / 32768;

				FillRectangle(hdc,IX+56,IY+26 - peak, 4, peak);

				SetTextColor(hdc,RGB(  0,255,  0));
				SetBkColor  (hdc,RGB(  0,  0,  0));

				static wchar_t t[1024];

				swprintf_s(t,_T("%06x"),vc.StartA);
				TextOut(hdc,IX+4,IY+3,t,6);

				swprintf_s(t,_T("%06x"),vc.NextA);
				TextOut(hdc,IX+4,IY+12,t,6);

				swprintf_s(t,_T("%06x"),vc.LoopStartA);
				TextOut(hdc,IX+4,IY+21,t,6);

				vcd.displayPeak = 0;

				if(vcd.lastSetStartA != vc.StartA)
				{
					printf(" *** Warning! Core %d Voice %d: StartA should be %06x, and is %06x.\n",
						c,v,vcd.lastSetStartA,vc.StartA);
					vcd.lastSetStartA = vc.StartA;
				}
			}
		}
		ReleaseDC(hDebugDialog,hdc);
		lCount=0;
	}

	MSG msg;
	while(PeekMessage(&msg,hDebugDialog,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
#endif
