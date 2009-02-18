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

#define _WIN32_DCOM
#include "spu2.h"
#include "dialogs.h"

#define DIRECTSOUND_VERSION 0x1000
#include <dsound.h>

static ds_device_data devices[32];
static int ndevs;
static GUID DevGuid;		// currently employed GUID.
static bool haveGuid;

class DSound: public SndOutModule
{
private:
	static const uint MAX_BUFFER_COUNT = 8;

	static const int PacketsPerBuffer = 1;
	static const int BufferSize = SndOutPacketSize * PacketsPerBuffer;


	u32 numBuffers;		// cached copy of our configuration setting.
	int channel;
	int myLastWrite;	// last write position, in bytes

	bool dsound_running;
	HANDLE thread;
	DWORD tid;

	IDirectSound8* dsound;
	IDirectSoundBuffer8* buffer;
	IDirectSoundNotify8* buffer_notify;
	HANDLE buffer_events[MAX_BUFFER_COUNT];

	WAVEFORMATEX wfx;

	HANDLE waitEvent;

	template< typename T >
	static DWORD CALLBACK RThread( DSound* obj )
	{
		return obj->Thread<T>();
	}

	template< typename T >
	DWORD CALLBACK Thread()
	{
		static const int BufferSizeBytes = BufferSize * sizeof( T );

		while( dsound_running )
		{
			u32 rv = WaitForMultipleObjects(numBuffers,buffer_events,FALSE,200);
	 
			T* p1, *oldp1;
			LPVOID p2;
			DWORD s1,s2;
	 
			u32 poffset = BufferSizeBytes * rv;

			if( FAILED(buffer->Lock(poffset,BufferSizeBytes,(LPVOID*)&p1,&s1,&p2,&s2,0) ) )
			{
				assert( 0 );
				fputs( " * SPU2 : Directsound Warning > Buffer lock failure.  You may need to increase\n\tyour configured DSound buffer count.\n", stderr );
				continue;
			}
			oldp1 = p1;

			for(int p=0; p<PacketsPerBuffer; p++, p1+=SndOutPacketSize )
				SndBuffer::ReadSamples( p1 );

			buffer->Unlock( oldp1, s1, p2, s2 );

			// Set the write pointer to the beginning of the next block.
			myLastWrite = (poffset + BufferSizeBytes) & ~BufferSizeBytes;
		}
		return 0;
	}

public:
	s32 Init()
	{
		numBuffers = Config_DSoundOut.NumBuffers;

		//
		// Initialize DSound
		//
		GUID cGuid;

		try
		{
			if( Config_DSoundOut.Device.empty() )
				throw std::runtime_error( "screw it" );
			
			// Convert from unicode to ANSI:
			char guid[256];
			sprintf_s( guid, "%S", Config_DSoundOut.Device.c_str() );
			
			if( (FAILED(GUIDFromString( guid, &cGuid ))) ||
				FAILED( DirectSoundCreate8(&cGuid,&dsound,NULL) ) )
					throw std::runtime_error( "try again?" );
		}
		catch( std::runtime_error& )
		{
			// if the GUID failed, just open up the default dsound driver:
			if( FAILED(DirectSoundCreate8(NULL,&dsound,NULL) ) )
				throw std::runtime_error( "DirectSound failed to initialize!" );
		}

		if( FAILED(dsound->SetCooperativeLevel(GetDesktopWindow(),DSSCL_PRIORITY)) )
			throw std::runtime_error( "DirectSound Error: Cooperative level could not be set." );
		
		// Determine the user's speaker configuration, and select an expansion option as needed.
		// FAIL : Directsound doesn't appear to support audio expansion >_<
		
		DWORD speakerConfig = 2;
		//dsound->GetSpeakerConfig( &speakerConfig );

		IDirectSoundBuffer* buffer_;
 		DSBUFFERDESC desc; 
	 
		// Set up WAV format structure. 
	 
		memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
		wfx.wFormatTag		= WAVE_FORMAT_PCM;
		wfx.nSamplesPerSec	= SampleRate;
		wfx.nChannels		= (WORD)speakerConfig;
		wfx.wBitsPerSample	= 16;
		wfx.nBlockAlign		= 2*(WORD)speakerConfig;
		wfx.nAvgBytesPerSec	= SampleRate * wfx.nBlockAlign;
		wfx.cbSize			= 0;

		uint BufferSizeBytes = BufferSize * wfx.nBlockAlign;
	 
		// Set up DSBUFFERDESC structure. 
	 
		memset(&desc, 0, sizeof(DSBUFFERDESC)); 
		desc.dwSize = sizeof(DSBUFFERDESC); 
		desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;// _CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY; 
		desc.dwBufferBytes = BufferSizeBytes * numBuffers;
		desc.lpwfxFormat = &wfx;
	 
		desc.dwFlags |= DSBCAPS_LOCSOFTWARE;
		desc.dwFlags |= DSBCAPS_GLOBALFOCUS;
	 
		if( FAILED(dsound->CreateSoundBuffer(&desc,&buffer_,0) ) )
			throw std::runtime_error( "DirectSound Error: Interface could not be queried." );
		
		if(	FAILED(buffer_->QueryInterface(IID_IDirectSoundBuffer8,(void**)&buffer)) )
			throw std::runtime_error( "DirectSound Error: Interface could not be queried." );

		buffer_->Release();
		verifyc( buffer->QueryInterface(IID_IDirectSoundNotify8,(void**)&buffer_notify) );

		DSBPOSITIONNOTIFY not[MAX_BUFFER_COUNT];
	 
		for(u32 i=0;i<numBuffers;i++)
		{
			// [Air] note: wfx.nBlockAlign modifier was *10 -- seems excessive to me but maybe
			// it was needed for some quirky driver?  Theoretically we want the notification as soon
			// as possible after the buffer has finished playing.

			buffer_events[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
			not[i].dwOffset = (wfx.nBlockAlign + BufferSizeBytes*(i+1)) % desc.dwBufferBytes;
			not[i].hEventNotify = buffer_events[i];
		}
	 
		buffer_notify->SetNotificationPositions(numBuffers,not);
	 
		LPVOID p1=0,p2=0;
		DWORD s1=0,s2=0;
	 
		verifyc(buffer->Lock(0,desc.dwBufferBytes,&p1,&s1,&p2,&s2,0));
		assert(p2==0);
		memset(p1,0,s1);
		verifyc(buffer->Unlock(p1,s1,p2,s2));
	 
		//Play the buffer !
		verifyc(buffer->Play(0,0,DSBPLAY_LOOPING));

		// Start Thread
		myLastWrite = 0;
		dsound_running = true;
		thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread<StereoOut16>,this,0,&tid);
		SetThreadPriority(thread,THREAD_PRIORITY_ABOVE_NORMAL);

		return 0;
	}

	void Close()
	{
		// Stop Thread
		fprintf(stderr," * SPU2: Waiting for DSound thread to finish...");
		dsound_running=false;
			
		WaitForSingleObject(thread,INFINITE);
		CloseHandle(thread);

		fprintf(stderr," Done.\n");

		//
		// Clean up
		//
		if( buffer != NULL )
		{
			buffer->Stop();
		 
			for(u32 i=0;i<numBuffers;i++)
			{
				if( buffer_events[i] != NULL )
					CloseHandle(buffer_events[i]);
				buffer_events[i] = NULL;
			}

			SAFE_RELEASE( buffer_notify );
			SAFE_RELEASE( buffer );
		}

		SAFE_RELEASE( dsound );
	}

private:

	static BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext )
	{
		devices[ndevs].name = lpcstrDescription;

		if(lpGuid)
		{
			devices[ndevs].guid = *lpGuid;
			devices[ndevs].hasGuid = true;
		}
		else
		{
		devices[ndevs].hasGuid = false;
		}
		ndevs++;

		if(ndevs<32) return TRUE;
		return FALSE;
	}

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		int wmId,wmEvent;
		int tSel=0;

		switch(uMsg)
		{
			case WM_INITDIALOG:
			{
				wchar_t temp[128];
				
				char temp2[192];
				sprintf_s( temp2, "%S", Config_DSoundOut.Device.c_str() );
				haveGuid = ! FAILED(GUIDFromString(temp2,&DevGuid));
				SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_RESETCONTENT,0,0); 

				ndevs=0;
				DirectSoundEnumerate( DSEnumCallback, NULL );

				tSel=-1;
				for(int i=0;i<ndevs;i++)
				{
					SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_ADDSTRING,0,(LPARAM)devices[i].name.c_str());
					if(haveGuid && IsEqualGUID(devices[i].guid,DevGuid))
					{
						tSel=i;
					}
				}

				if(tSel>=0)
				{
					SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_SETCURSEL,tSel,0);
				}

				INIT_SLIDER( IDC_BUFFERS_SLIDER, 2, MAX_BUFFER_COUNT, 2, 1, 1 );
				SendMessage(GetDlgItem(hWnd,IDC_BUFFERS_SLIDER),TBM_SETPOS,TRUE,Config_DSoundOut.NumBuffers); 
				swprintf_s(temp, _T("%d (%d ms latency)"),Config_DSoundOut.NumBuffers, 1000 / (96000 / (Config_DSoundOut.NumBuffers * BufferSize)));
				SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
			}
			break;

			case WM_COMMAND:
			{
				wchar_t temp[128];

				wmId    = LOWORD(wParam); 
				wmEvent = HIWORD(wParam); 
				// Parse the menu selections:
				switch (wmId)
				{
					case IDOK:
						{
							int i = (int)SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_GETCURSEL,0,0);
							
							if(!devices[i].hasGuid)
							{
								Config_DSoundOut.Device[0] = 0; // clear device name to ""
							}
							else
							{
								swprintf_s(temp, _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
									devices[i].guid.Data1,
									devices[i].guid.Data2,
									devices[i].guid.Data3,
									devices[i].guid.Data4[0],
									devices[i].guid.Data4[1],
									devices[i].guid.Data4[2],
									devices[i].guid.Data4[3],
									devices[i].guid.Data4[4],
									devices[i].guid.Data4[5],
									devices[i].guid.Data4[6],
									devices[i].guid.Data4[7]
								);
								Config_DSoundOut.Device = temp;
							}

							Config_DSoundOut.NumBuffers = (int)SendMessage( GetDlgItem( hWnd, IDC_BUFFERS_SLIDER ), TBM_GETPOS, 0, 0 );

							if( Config_DSoundOut.NumBuffers < 2 ) Config_DSoundOut.NumBuffers = 2;
							if( Config_DSoundOut.NumBuffers > MAX_BUFFER_COUNT ) Config_DSoundOut.NumBuffers = MAX_BUFFER_COUNT;
						}
						EndDialog(hWnd,0);
						break;
					case IDCANCEL:
						EndDialog(hWnd,0);
						break;
					default:
						return FALSE;
				}
			}
			break;

			case WM_HSCROLL:
			{
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
					{
						wchar_t temp[128];
						if( wmEvent < 2 ) wmEvent = 2;
						if( wmEvent > MAX_BUFFER_COUNT ) wmEvent = MAX_BUFFER_COUNT;
						SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
						swprintf_s(temp,_T("%d (%d ms latency)"),wmEvent, 1000 / (96000 / (wmEvent * BufferSize)));
						SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
						break;
					}
					default:
						return FALSE;
				}
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
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DSOUND),GetActiveWindow(),(DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBoxEx(GetActiveWindow(),_T("Error Opening the config dialog."),_T("OMG ERROR!"),MB_OK,0);
			return;
		}
	}

	virtual bool Is51Out() const { return false; }

	s32 Test() const
	{
		return 0;
	}

	int GetEmptySampleCount() const
	{
		DWORD play, write;
		buffer->GetCurrentPosition( &play, &write );

		// Note: Dsound's write cursor is bogus.  Use our own instead:

		int empty = play - myLastWrite;
		if( empty < 0 )
			empty = -empty;

		return empty / 2;
	}

	const wchar_t* GetIdent() const
	{
		return _T("dsound");
	}

	const wchar_t* GetLongName() const
	{
		return _T("DirectSound (nice)");
	}

} DS;

SndOutModule *DSoundOut=&DS;
