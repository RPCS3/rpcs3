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

#define _WIN32_DCOM
#include "Dialogs.h"

#define DIRECTSOUND_VERSION 0x1000
#include <dsound.h>

class DSound : public SndOutModule
{
private:
	static const uint MAX_BUFFER_COUNT = 8;
	static const int PacketsPerBuffer = 1;
	static const int BufferSize = SndOutPacketSize * PacketsPerBuffer;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Configuration Vars

	wstring m_Device;
	u8 m_NumBuffers;
	bool m_DisableGlobalFocus;
	bool m_UseHardware;

	ds_device_data m_devices[32];
	int ndevs;
	GUID DevGuid;		// currently employed GUID.
	bool haveGuid;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Instance vars
	
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
			u32 rv = WaitForMultipleObjects(m_NumBuffers,buffer_events,FALSE,200);
	 
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
		//
		// Initialize DSound
		//
		GUID cGuid;

		try
		{
			if( m_Device.empty() )
				throw std::runtime_error( "screw it" );
			
			// Convert from unicode to ANSI:
			char guid[256];
			sprintf_s( guid, "%S", m_Device.c_str() );
			
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
		desc.dwFlags =  DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;
		desc.dwBufferBytes = BufferSizeBytes * m_NumBuffers;
		desc.lpwfxFormat = &wfx;
	 
		// Try a hardware buffer first, and then fall back on a software buffer if
		// that one fails.
	 
		desc.dwFlags |= m_UseHardware ? DSBCAPS_LOCHARDWARE : DSBCAPS_LOCSOFTWARE;
		desc.dwFlags |= m_DisableGlobalFocus ? DSBCAPS_STICKYFOCUS : DSBCAPS_GLOBALFOCUS;
	 
		if( FAILED(dsound->CreateSoundBuffer(&desc, &buffer_, 0) ) )
		{
			if( m_UseHardware )
			{
				desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
				desc.dwFlags |= m_DisableGlobalFocus ? DSBCAPS_STICKYFOCUS : DSBCAPS_GLOBALFOCUS;

				if( FAILED(dsound->CreateSoundBuffer(&desc, &buffer_, 0) ) )
					throw std::runtime_error( "DirectSound Error: Buffer could not be created." );
			}

			throw std::runtime_error( "DirectSound Error: Buffer could not be created." );
		}
		if(	FAILED(buffer_->QueryInterface(IID_IDirectSoundBuffer8,(void**)&buffer)) )
			throw std::runtime_error( "DirectSound Error: Interface could not be queried." );

		buffer_->Release();
		verifyc( buffer->QueryInterface(IID_IDirectSoundNotify8,(void**)&buffer_notify) );

		DSBPOSITIONNOTIFY not[MAX_BUFFER_COUNT];
	 
		for(uint i=0;i<m_NumBuffers;i++)
		{
			buffer_events[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
			not[i].dwOffset = (wfx.nBlockAlign + BufferSizeBytes*(i+1)) % desc.dwBufferBytes;
			not[i].hEventNotify = buffer_events[i];
		}
	 
		buffer_notify->SetNotificationPositions(m_NumBuffers,not);
	 
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
		 
			for(u32 i=0;i<m_NumBuffers;i++)
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

	bool _DSEnumCallback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext )
	{
		m_devices[ndevs].name = lpcstrDescription;

		if(lpGuid)
		{
			m_devices[ndevs].guid = *lpGuid;
			m_devices[ndevs].hasGuid = true;
		}
		else
		{
			m_devices[ndevs].hasGuid = false;
		}
		ndevs++;

		if(ndevs<32) return TRUE;
		return FALSE;
	}

	bool _ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		int wmId,wmEvent;
		int tSel = 0;

		switch(uMsg)
		{
			case WM_INITDIALOG:
			{
				wchar_t temp[128];
				
				char temp2[192];
				sprintf_s( temp2, "%S", m_Device.c_str() );
				haveGuid = ! FAILED(GUIDFromString(temp2,&DevGuid));
				SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_RESETCONTENT,0,0); 

				ndevs=0;
				DirectSoundEnumerate( DSEnumCallback, NULL );

				tSel=-1;
				for(int i=0;i<ndevs;i++)
				{
					SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_ADDSTRING,0,(LPARAM)m_devices[i].name.c_str());
					if(haveGuid && IsEqualGUID(m_devices[i].guid,DevGuid))
						tSel = i;
				}

				if(tSel>=0)
					SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_SETCURSEL,tSel,0);

				INIT_SLIDER( IDC_BUFFERS_SLIDER, 2, MAX_BUFFER_COUNT, 2, 1, 1 );
				SendMessage(GetDlgItem(hWnd,IDC_BUFFERS_SLIDER),TBM_SETPOS,TRUE,m_NumBuffers); 
				swprintf_s(temp, L"%d (%d ms latency)",m_NumBuffers, 1000 / (96000 / (m_NumBuffers * BufferSize)));
				SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
				
				SET_CHECK( IDC_GLOBALFOCUS_DISABLE, m_DisableGlobalFocus );
				SET_CHECK( IDC_USE_HARDWARE, m_UseHardware );
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
						
						if(!m_devices[i].hasGuid)
						{
							m_Device[0] = 0; // clear device name to ""
						}
						else
						{
							swprintf_s(temp, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
								m_devices[i].guid.Data1,
								m_devices[i].guid.Data2,
								m_devices[i].guid.Data3,
								m_devices[i].guid.Data4[0],
								m_devices[i].guid.Data4[1],
								m_devices[i].guid.Data4[2],
								m_devices[i].guid.Data4[3],
								m_devices[i].guid.Data4[4],
								m_devices[i].guid.Data4[5],
								m_devices[i].guid.Data4[6],
								m_devices[i].guid.Data4[7]
							);
							m_Device = temp;
						}

						m_NumBuffers = (int)SendMessage( GetDlgItem( hWnd, IDC_BUFFERS_SLIDER ), TBM_GETPOS, 0, 0 );

						if( m_NumBuffers < 2 ) m_NumBuffers = 2;
						if( m_NumBuffers > MAX_BUFFER_COUNT ) m_NumBuffers = MAX_BUFFER_COUNT;

						EndDialog(hWnd,0);
					}
					break;

					case IDCANCEL:
						EndDialog(hWnd,0);
					break;

					HANDLE_CHECK( IDC_GLOBALFOCUS_DISABLE, m_DisableGlobalFocus );
					HANDLE_CHECK( IDC_USE_HARDWARE, m_UseHardware );

					default:
						return FALSE;
				}
			}
			break;

			case WM_HSCROLL:
			{
				wmId    = LOWORD(wParam); 
				wmEvent = HIWORD(wParam); 
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
					{
						wchar_t temp[128];
						if( wmEvent < 2 ) wmEvent = 2;
						if( wmEvent > MAX_BUFFER_COUNT ) wmEvent = MAX_BUFFER_COUNT;
						SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
						swprintf_s(temp,L"%d (%d ms latency)",wmEvent, 1000 / (96000 / (wmEvent * BufferSize)));
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

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext );

public:
	virtual void Configure(HWND parent)
	{
		INT_PTR ret;
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DSOUND),GetActiveWindow(),(DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBoxEx(GetActiveWindow(),L"Error Opening the config dialog.",L"OMG ERROR!",MB_OK,0);
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
		return L"dsound";
	}

	const wchar_t* GetLongName() const
	{
		return L"DirectSound (nice)";
	}
	
	void ReadSettings()
	{
		CfgReadStr( L"DSOUNDOUT", L"Device", m_Device, 254, L"default" );
		m_NumBuffers = CfgReadInt( L"DSOUNDOUT", L"Buffer_Count", 5 );
		m_DisableGlobalFocus = CfgReadBool( L"DSOUNDOUT", L"Disable_Global_Focus", false );
		m_UseHardware = CfgReadBool( L"DSOUNDOUT", L"Use_Hardware", false );

		Clampify( m_NumBuffers, (u8)3, (u8)8 );
	}

	void WriteSettings() const
	{
		CfgWriteStr( L"DSOUNDOUT", L"Device", m_Device.empty() ? L"default" : m_Device );
		CfgWriteInt( L"DSOUNDOUT", L"Buffer_Count", m_NumBuffers );
		CfgWriteBool( L"DSOUNDOUT", L"Disable_Global_Focus", m_DisableGlobalFocus );
		CfgWriteBool( L"DSOUNDOUT", L"Use_Hardware", m_UseHardware );
	}

} static DS;

BOOL CALLBACK DSound::ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return DS._ConfigProc( hWnd, uMsg, wParam, lParam );
}	

BOOL CALLBACK DSound::DSEnumCallback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext )
{
	jASSUME( DSoundOut != NULL );
	return DS._DSEnumCallback( lpGuid, lpcstrDescription, lpcstrModule, lpContext );
}

SndOutModule *DSoundOut = &DS;
