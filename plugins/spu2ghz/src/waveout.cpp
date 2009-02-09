//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#include "spu2.h"
#include "dialogs.h"
#include <windows.h>


class WaveOutModule: public SndOutModule
{
private:
#	define MAX_BUFFER_COUNT 8

	static const int PacketsPerBuffer = (1024 / SndOutPacketSize);
	static const int BufferSize = SndOutPacketSize*PacketsPerBuffer;
	static const int BufferSizeBytes = BufferSize << 1;

	u32 numBuffers;
	HWAVEOUT hwodevice;
	WAVEFORMATEX wformat;
	WAVEHDR whbuffer[MAX_BUFFER_COUNT];

	s16* qbuffer;

	#define QBUFFER(x) (qbuffer + BufferSize * (x))

	bool waveout_running;
	HANDLE thread;
	DWORD tid;

	SndBuffer *buff;

	FILE *voicelog;

	char ErrText[256];

	static DWORD CALLBACK RThread(WaveOutModule*obj)
	{
		return obj->Thread();
	}

	DWORD CALLBACK Thread()
	{
		while( waveout_running )
		{
			bool didsomething = false;
			for(u32 i=0;i<numBuffers;i++)
			{
				if(!(whbuffer[i].dwFlags & WHDR_DONE) ) continue;

				WAVEHDR *buf=whbuffer+i;

				buf->dwBytesRecorded = buf->dwBufferLength;

				s16 *t = (s16*)buf->lpData;
				for(int p=0; p<PacketsPerBuffer; p++, t+=SndOutPacketSize )
					buff->ReadSamples( t );

				whbuffer[i].dwFlags&=~WHDR_DONE;
				waveOutWrite(hwodevice,buf,sizeof(WAVEHDR));
				didsomething = true;
			}

			if( didsomething )
				Sleep(1);
			else
				Sleep(0);
		}
		return 0;
	}

public:
	s32 Init(SndBuffer *sb)
	{
		buff = sb;
		numBuffers = Config_WaveOut.NumBuffers;

		MMRESULT woores;

		if (Test()) return -1;

		wformat.wFormatTag=WAVE_FORMAT_PCM;
		wformat.nSamplesPerSec=SampleRate;
		wformat.wBitsPerSample=16;
		wformat.nChannels=2;
		wformat.nBlockAlign=((wformat.wBitsPerSample * wformat.nChannels) / 8);
		wformat.nAvgBytesPerSec=(wformat.nSamplesPerSec * wformat.nBlockAlign);
		wformat.cbSize=0;
		
		qbuffer=new s16[BufferSize*numBuffers];

		woores = waveOutOpen(&hwodevice,WAVE_MAPPER,&wformat,0,0,0);
		if (woores != MMSYSERR_NOERROR)
		{
			waveOutGetErrorText(woores,(char *)&ErrText,255);
			SysMessage("WaveOut Error: %s",ErrText);
			return -1;
		}

		for(u32 i=0;i<numBuffers;i++)
		{
			whbuffer[i].dwBufferLength=BufferSizeBytes;
			whbuffer[i].dwBytesRecorded=BufferSizeBytes;
			whbuffer[i].dwFlags=0;
			whbuffer[i].dwLoops=0;
			whbuffer[i].dwUser=0;
			whbuffer[i].lpData=(LPSTR)QBUFFER(i);
			whbuffer[i].lpNext=0;
			whbuffer[i].reserved=0;
			waveOutPrepareHeader(hwodevice,whbuffer+i,sizeof(WAVEHDR));
			whbuffer[i].dwFlags|=WHDR_DONE; //avoid deadlock
		}

		// Start Thread
		// [Air]: The waveout code does not use wait objects, so setting a time critical
		// priority level is a bad idea.  Standard priority will do fine.  The buffer will get the
		// love it needs and won't suck resources idling pointlessly.  Just don't try to
		// run it in uber-low-latency mode.
		waveout_running=true;
		thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread,this,0,&tid);
		//SetThreadPriority( thread, THREAD_PRIORITY_TIME_CRITICAL );

		return 0;
	}

	void Close() 
	{
		// Stop Thread
		fprintf(stderr," * SPU2: Waiting for waveOut thread to finish...");
		waveout_running=false;
			
		WaitForSingleObject(thread,INFINITE);
		CloseHandle(thread);

		fprintf(stderr," Done.\n");

		//
		// Clean up
		//
		waveOutReset(hwodevice);
		for(u32 i=0;i<numBuffers;i++)
		{
			waveOutUnprepareHeader(hwodevice,&whbuffer[i],sizeof(WAVEHDR));
		}
		waveOutClose(hwodevice);

		SAFE_DELETE_ARRAY( qbuffer );
	}

private:

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		int wmId,wmEvent;
		int tSel=0;

		switch(uMsg)
		{
			case WM_INITDIALOG:

				char temp[128];
				INIT_SLIDER( IDC_BUFFERS_SLIDER, 3, MAX_BUFFER_COUNT, 2, 1, 1 );
				SendMessage(GetDlgItem(hWnd,IDC_BUFFERS_SLIDER),TBM_SETPOS,TRUE,Config_WaveOut.NumBuffers); 
				sprintf_s(temp, 128, "%d (%d ms latency)",Config_WaveOut.NumBuffers, 1000 / (96000 / (Config_WaveOut.NumBuffers * BufferSize)));
				SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);

				break;
			case WM_COMMAND:
				wmId    = LOWORD(wParam); 
				wmEvent = HIWORD(wParam); 
				// Parse the menu selections:
				switch (wmId)
				{
					case IDOK:
						{
							Config_WaveOut.NumBuffers = (int)SendMessage( GetDlgItem( hWnd, IDC_BUFFERS_SLIDER ), TBM_GETPOS, 0, 0 );

							if( Config_WaveOut.NumBuffers < 3 ) Config_WaveOut.NumBuffers = 3;
							if( Config_WaveOut.NumBuffers > MAX_BUFFER_COUNT ) Config_WaveOut.NumBuffers = MAX_BUFFER_COUNT;
						}
						EndDialog(hWnd,0);
						break;
					case IDCANCEL:
						EndDialog(hWnd,0);
						break;
					default:
						return FALSE;
				}
				break;

			case WM_HSCROLL:
				wmId    = LOWORD(wParam); 
				wmEvent = HIWORD(wParam); 
				switch(wmId) {
					//case TB_ENDTRACK:
					//case TB_THUMBPOSITION:
					case TB_LINEUP:
					case TB_LINEDOWN:
					case TB_PAGEUP:
					case TB_PAGEDOWN:
						wmEvent=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
					case TB_THUMBTRACK:
						if( wmEvent < 3 ) wmEvent = 3;
						if( wmEvent > MAX_BUFFER_COUNT ) wmEvent = MAX_BUFFER_COUNT;
						SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
						sprintf_s(temp,128,"%d (%d ms latency)",wmEvent, 1000 / (96000 / (wmEvent * BufferSize)));
						SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
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

public:
	virtual void Configure(HWND parent)
	{
		INT_PTR ret;
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_WAVEOUT),GetActiveWindow(),(DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBoxEx(GetActiveWindow(),"Error Opening the config dialog.","OMG ERROR!",MB_OK,0);
			return;
		}
	}

	virtual bool Is51Out() const { return false; }

	s32 Test() const
	{
		if (waveOutGetNumDevs() == 0) {
			SysMessage("No waveOut Devices Present\n"); return -1;
		}
		return 0;
	}

	int GetEmptySampleCount() const
	{
		int result = 0;
		for(int i=0;i<MAX_BUFFER_COUNT;i++)
		{
			result += (whbuffer[i].dwFlags & WHDR_DONE) ? BufferSize : 0;
		}
		return result;
	}

	const char* GetIdent() const
	{
		return "waveout";
	}

	const char* GetLongName() const
	{
		return "waveOut (Laggy)";
	}

} WO;

SndOutModule *WaveOut=&WO;
