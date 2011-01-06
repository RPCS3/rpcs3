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


class WaveOutModule: public SndOutModule
{
private:
	static const uint MAX_BUFFER_COUNT = 8;

	static const int PacketsPerBuffer = (1024 / SndOutPacketSize);
	static const int BufferSize = SndOutPacketSize*PacketsPerBuffer;

	u32 numBuffers;
	HWAVEOUT hwodevice;
	WAVEFORMATEX wformat;
	WAVEHDR whbuffer[MAX_BUFFER_COUNT];

	StereoOut16* qbuffer;

	#define QBUFFER(x) (qbuffer + BufferSize * (x))

	bool waveout_running;
	HANDLE thread;
	DWORD tid;

	wchar_t ErrText[256];

	template< typename T >
	DWORD CALLBACK Thread()
	{
		static const int BufferSizeBytes = BufferSize * sizeof( T );

		while( waveout_running )
		{
			bool didsomething = false;
			for(u32 i=0;i<numBuffers;i++)
			{
				if(!(whbuffer[i].dwFlags & WHDR_DONE) ) continue;

				WAVEHDR *buf = whbuffer+i;

				buf->dwBytesRecorded = buf->dwBufferLength;

				T* t = (T*)buf->lpData;
				for(int p=0; p<PacketsPerBuffer; p++, t+=SndOutPacketSize )
					SndBuffer::ReadSamples( t );

				whbuffer[i].dwFlags &= ~WHDR_DONE;
				waveOutWrite( hwodevice, buf, sizeof(WAVEHDR) );
				didsomething = true;
			}

			if( didsomething )
				Sleep(1);
			else
				Sleep(0);
		}
		return 0;
	}

	template< typename T >
	static DWORD CALLBACK RThread(WaveOutModule*obj)
	{
		return obj->Thread<T>();
	}

public:
	s32 Init()
	{
		numBuffers = Config_WaveOut.NumBuffers;

		MMRESULT woores;

		if (Test()) return -1;

		// TODO : Use dsound to determine the speaker configuration, and expand audio from there.

		#if 0
		int speakerConfig;

		//if( StereoExpansionEnabled )
			speakerConfig = 2;  // better not mess with this in wavout :p (rama)

		// Any windows driver should support stereo at the software level, I should think!
		jASSUME( speakerConfig > 1 );
		LPTHREAD_START_ROUTINE threadproc;

		switch( speakerConfig )
		{
		case 2:
			ConLog( "* SPU2 > Using normal 2 speaker stereo output.\n" );
			threadproc = (LPTHREAD_START_ROUTINE)&RThread<StereoOut16>;
			speakerConfig = 2;
		break;

		case 4:
			ConLog( "* SPU2 > 4 speaker expansion enabled [quadraphenia]\n" );
			threadproc = (LPTHREAD_START_ROUTINE)&RThread<StereoQuadOut16>;
			speakerConfig = 4;
		break;

		case 6:
		case 7:
			ConLog( "* SPU2 > 5.1 speaker expansion enabled.\n" );
			threadproc = (LPTHREAD_START_ROUTINE)&RThread<Stereo51Out16>;
			speakerConfig = 6;
		break;

		default:
			ConLog( "* SPU2 > 7.1 speaker expansion enabled.\n" );
			threadproc = (LPTHREAD_START_ROUTINE)&RThread<Stereo51Out16>;
			speakerConfig = 8;
		break;
		}
		#endif

		wformat.wFormatTag		= WAVE_FORMAT_PCM;
		wformat.nSamplesPerSec	= SampleRate;
		wformat.wBitsPerSample	= 16;
		wformat.nChannels		= 2;
		wformat.nBlockAlign		= ((wformat.wBitsPerSample * wformat.nChannels) / 8);
		wformat.nAvgBytesPerSec	= (wformat.nSamplesPerSec * wformat.nBlockAlign);
		wformat.cbSize			= 0;

		qbuffer = new StereoOut16[BufferSize*numBuffers];

		woores = waveOutOpen(&hwodevice,WAVE_MAPPER,&wformat,0,0,0);
		if (woores != MMSYSERR_NOERROR)
		{
			waveOutGetErrorText(woores,(wchar_t *)&ErrText,255);
			SysMessage("WaveOut Error: %s",ErrText);
			return -1;
		}

		const int BufferSizeBytes = wformat.nBlockAlign * BufferSize;

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
		waveout_running = true;
		thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread<StereoOut16>,this,0,&tid);

		return 0;
	}

	void Close()
	{
		// Stop Thread
		fprintf(stderr,"* SPU2-X: Waiting for waveOut thread to finish...");
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

		safe_delete_array( qbuffer );
	}

private:

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		int wmId,wmEvent;

		switch(uMsg)
		{
			case WM_INITDIALOG:

				wchar_t temp[128];
				INIT_SLIDER( IDC_BUFFERS_SLIDER, 3, MAX_BUFFER_COUNT, 2, 1, 1 );
				SendMessage(GetDlgItem(hWnd,IDC_BUFFERS_SLIDER),TBM_SETPOS,TRUE,Config_WaveOut.NumBuffers);
				swprintf_s(temp, 128, L"%d (%d ms latency)",Config_WaveOut.NumBuffers, 1000 / (96000 / (Config_WaveOut.NumBuffers * BufferSize)));
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
						swprintf_s(temp, L"%d (%d ms latency)",wmEvent, 1000 / (96000 / (wmEvent * BufferSize)));
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
	virtual void Configure(uptr parent)
	{
		INT_PTR ret;
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_WAVEOUT), (HWND)parent, (DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBox((HWND)parent, L"Error Opening the config dialog.", L"OMG ERROR!", MB_OK | MB_SETFOREGROUND);
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

	int GetEmptySampleCount()
	{
		int result = 0;
		for(int i=0;i<MAX_BUFFER_COUNT;i++)
		{
			result += (whbuffer[i].dwFlags & WHDR_DONE) ? BufferSize : 0;
		}
		return result;
	}

	const wchar_t* GetIdent() const
	{
		return L"waveout";
	}

	const wchar_t* GetLongName() const
	{
		return L"waveOut (Laggy)";
	}

	void ReadSettings()
	{
	}

	void WriteSettings() const
	{
	}

} static WO;

SndOutModule *WaveOut = &WO;
