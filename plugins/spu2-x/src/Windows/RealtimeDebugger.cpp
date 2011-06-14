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

bool debugDialogOpen=false;
HWND hDebugDialog=NULL;

#ifdef PCSX2_DEVBUILD

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
	if(lCount>=(SampleRate/100)) // Increase to SampleRate/200 for smooth display.
	{
		HDC hdc = GetDC(hDebugDialog);

		if(!hf)
		{
			hf = CreateFont( 12, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Lucida Console" );
		}

		SelectObject(hdc,hf);
		SelectObject(hdc,GetStockObject(DC_BRUSH));
		SelectObject(hdc,GetStockObject(DC_PEN));

		for(int c=0;c<2;c++)
		{
			V_Core& cx(Cores[c]);
			V_CoreDebug& cd(DebugCores[c]);

			for(int v=0;v<24;v++)
			{
				int cc = c*2 + (v/12);
				int vv = v % 12;
				int IX = 8+128*cc;
				int IY = 8+ 48*vv;
				V_Voice& vc(cx.Voices[v]);
				V_VoiceDebug& vcd(cd.Voices[v] );

				SetDCBrushColor(hdc,RGB(  0,  0,  0));
				if((vc.ADSR.Phase>0)&&(vc.ADSR.Phase<6))
				{
					SetDCBrushColor(hdc,RGB(  0,  0,128));
				}
				/*
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
				}*/

				FillRectangle(hdc,IX,IY,124,46);

				SetDCPenColor(hdc,RGB(  255,  128,  32));

				DrawRectangle(hdc,IX,IY,124,46);

				SetDCBrushColor(hdc,RGB(  0,255,  0));

				int vl = abs(((vc.Volume.Left.Value >> 16) * 38) >> 15);
				int vr = abs(((vc.Volume.Right.Value >> 16) * 38) >> 15);

				FillRectangle(hdc,IX+58,IY+42 - vl, 4, vl);
				FillRectangle(hdc,IX+62,IY+42 - vr, 4, vr);

				int adsr = ((vc.ADSR.Value>>16) * 38) / 32768;

				FillRectangle(hdc,IX+66,IY+42 - adsr, 4, adsr);

				int peak = (vcd.displayPeak * 38) / 32768;

				if(vcd.displayPeak >= 32700) // leave a little bit of margin
				{
					SetDCBrushColor(hdc,RGB(  255, 0,  0));
				}

				FillRectangle(hdc,IX+70,IY+42 - peak, 4, peak);

				if(vc.ADSR.Value>0)
				{
					if(vc.SBuffer)
					for(int i=0;i<28;i++)
					{
						int val = ((int)vc.SBuffer[i] * 20) / 32768;

						int y=0;

						if(val>0)
						{
							y=val;
						}
						else
							val=-val;
		
						if(val!=0)
						{
							FillRectangle(hdc,IX+90+i,IY+24-y, 1, val);
						}
					}
				}

				SetTextColor(hdc,RGB(  0,255,  0));
				SetBkColor  (hdc,RGB(  0,  0,  0));

				static wchar_t t[256];

				swprintf_s(t,L"%06x",vc.StartA);
				TextOut(hdc,IX+4,IY+4,t,6);

				swprintf_s(t,L"%06x",vc.NextA);
				TextOut(hdc,IX+4,IY+18,t,6);

				swprintf_s(t,L"%06x",vc.LoopStartA);
				TextOut(hdc,IX+4,IY+32,t,6);

				vcd.displayPeak = 0;

				if(vcd.lastSetStartA != vc.StartA)
				{
					printf(" *** Warning! Core %d Voice %d: StartA should be %06x, and is %06x.\n",
						c,v,vcd.lastSetStartA,vc.StartA);
					vcd.lastSetStartA = vc.StartA;
				}
			}

			// top now: 400
			int JX = 8 + c * 256;
			int JY = 584;

			SetDCBrushColor(hdc,RGB(  0,  0,  0));
			SetDCPenColor(hdc,RGB(  255,  128,  32));

			FillRectangle(hdc,JX,JY,252,60);
			DrawRectangle(hdc,JX,JY,252,60);

			SetTextColor(hdc,RGB(255,255,255));
			SetBkColor  (hdc,RGB(  0,  0,  0));

			static wchar_t t[256];
			TextOut(hdc,JX+4,JY+ 4,L"REVB",4);
			TextOut(hdc,JX+4,JY+18,L"IRQE",4);
			TextOut(hdc,JX+4,JY+32,L"ADMA",4);
			swprintf_s(t,L"DMA%s",c==0 ? "4" : "7");
			TextOut(hdc,JX+4,JY+46,t, 4);
			
			SetTextColor(hdc,RGB(  0,255,  0));
			memset(t, 0, sizeof(t));
			swprintf_s(t,L"ESA %x",cx.EffectsStartA);
			TextOut(hdc,JX+56,JY+ 4,t, 9);
			memset(t, 0, sizeof(t));
			swprintf_s(t,L"EEA %x",cx.EffectsEndA);
			TextOut(hdc,JX+128,JY+ 4,t, 9);
            memset(t, 0, sizeof(t));
			swprintf_s(t,L"(%x)",cx.EffectsBufferSize);
			TextOut(hdc,JX+200,JY+ 4,t,7);

            memset(t, 0, sizeof(t));
			swprintf_s(t,L"IRQA %x",cx.IRQA);
			TextOut(hdc,JX+56,JY+18,t, 12);

			SetTextColor(hdc,RGB(255,255,255));
			SetDCBrushColor(hdc,RGB(  255,0,  0));

			if(cx.FxEnable)
			{
				FillRectangle(hdc,JX+40,JY+4,10,10);
			}
			if(cx.IRQEnable)
			{
				FillRectangle(hdc,JX+40,JY+18,10,10);
			}
			if(cx.AutoDMACtrl != 0)
			{
				FillRectangle(hdc,JX+40,JY+32,10,10);

				for(int j=0;j<64;j++)
				{
					int i=j*256/64;
					int val = (cd.admaWaveformL[i] * 26) / 32768;
					int y=0;

					if(val>0)
						y=val;
					else
						val=-val;

					if(val!=0)
					{
						FillRectangle(hdc,JX+60+j,JY+30-y, 1, val);
					}
				}

				for(int j=0;j<64;j++)
				{
					int i=j*256/64;
					int val = (cd.admaWaveformR[i] * 26) / 32768;
					int y=0;

					if(val>0)
						y=val;
					else
						val=-val;

					if(val!=0)
					{
						FillRectangle(hdc,JX+136+j,JY+30-y, 1, val);
					}
				}
			}
			if(cd.dmaFlag > 0) // So it shows x times this is called, since dmas are so fast
			{
				static wchar_t t[256];
				swprintf_s(t,L"size = %d",cd.lastsize);
				
				TextOut(hdc,JX+64,JY+46,t,wcslen(t));
				FillRectangle(hdc,JX+40,JY+46,10,10);
				cd.dmaFlag--;
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
#else
void UpdateDebugDialog()
{
	// Release mode. Nothing to do
}
#endif
