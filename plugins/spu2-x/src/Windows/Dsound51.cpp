/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Originally based on SPU2ghz v1.9 beta, by David Quintana.
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
#include "DPLII.h"
#include "dialogs.h"

#include <dsound.h>
#include <mmsystem.h>
#include <audiodefs.h>

#include <stdexcept>

static ds_device_data devices[32];
static int ndevs;
static GUID DevGuid;		// currently employed GUID.
static bool haveGuid;

class DSound51_Driver;

class DSound51: public SndOutModule
{
	friend class DSound51_Driver;
private:
	static const int MAX_BUFFER_COUNT = 8;

	static const int PacketsPerBuffer = 1;
	static const int BufferSize = SndOutPacketSize * PacketsPerBuffer * 3;
	static const int BufferSizeBytes = BufferSize << 1;

	DSound51_Driver* m_driver;

public:
	s32 Init(SndBuffer* sb);
	void Close();
	const char* GetIdent() const;
	const char* GetLongName() const;
	int GetEmptySampleCount() const;
	bool Is51Out() const;
	s32 Test() const;
	virtual void Configure(HWND parent);

protected:
	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

} DS51;

// Internal DSound 5.1 control structures.
class DSound51_Driver
{
	SndBuffer *buff;
	u32 numBuffers;		// cached copy of our configuration setting.
	DPLII dpl2;

	int channel;
	int myLastWrite;

	bool dsound_running;
	HANDLE thread;
	DWORD tid;

	IDirectSound8* dsound;
	IDirectSoundBuffer8* buffer;
	IDirectSoundNotify8* buffer_notify;
	HANDLE buffer_events[DSound51::MAX_BUFFER_COUNT];

	WAVEFORMATEXTENSIBLE wfx;


public:
	DSound51_Driver( SndBuffer* sb );
	~DSound51_Driver();

	DWORD WorkerThread();
	int GetEmptySampleCount() const;
};

static DWORD CALLBACK RThread(DSound51_Driver* obj)
{
	return obj->WorkerThread();
}

static BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext )
{
	//strcpy(devices[ndevs].name,lpcstrDescription);
	_snprintf_s(devices[ndevs].name,255,"%s",lpcstrDescription);

	if(lpGuid)
	{
		devices[ndevs].guid=*lpGuid;
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

DSound51_Driver::DSound51_Driver( SndBuffer *sb ) :
	buff( sb ),
	numBuffers( Config_DSound51.NumBuffers ),
	dpl2( Config_DSound51.LowpassLFE, SampleRate )
{
	//
	// Initialize DSound
	//
	GUID cGuid;
	bool success  = false;

	if( (strlen(Config_DSound51.Device)>0)&&(!FAILED(GUIDFromString(Config_DSound51.Device,&cGuid))))
	{
		if( !FAILED( DirectSoundCreate8(&cGuid,&dsound,NULL) ) )
			success = true;
	}

	// if the GUID failed, just open up the default dsound driver:
	if( !success )
	{
		verifyc(DirectSoundCreate8(NULL,&dsound,NULL));
	}

	verifyc(dsound->SetCooperativeLevel(GetDesktopWindow(),DSSCL_PRIORITY));
	IDirectSoundBuffer* buffer_;
	DSBUFFERDESC desc;

	// Set up WAV format structure.

	memset(&wfx, 0, sizeof(WAVEFORMATEX));
	wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfx.Format.nSamplesPerSec = SampleRate;
	wfx.Format.nChannels=6;
	wfx.Format.wBitsPerSample = 16;
	wfx.Format.nBlockAlign = wfx.Format.nChannels*wfx.Format.wBitsPerSample/8;
	wfx.Format.nAvgBytesPerSec = SampleRate * wfx.Format.nBlockAlign;
	wfx.Format.cbSize=22;
	wfx.Samples.wValidBitsPerSample=0;
	wfx.dwChannelMask=SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT;
	wfx.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;

	verifyc(dsound->SetSpeakerConfig(DSSPEAKER_5POINT1));

	// Set up DSBUFFERDESC structure.

	memset(&desc, 0, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;// _CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
	desc.dwBufferBytes = DSound51::BufferSizeBytes * numBuffers;
	desc.lpwfxFormat = &wfx.Format;

	desc.dwFlags |=DSBCAPS_LOCSOFTWARE;
	desc.dwFlags|=DSBCAPS_GLOBALFOCUS;

	verifyc(dsound->CreateSoundBuffer(&desc,&buffer_,0));
	verifyc(buffer_->QueryInterface(IID_IDirectSoundBuffer8,(void**)&buffer));
	buffer_->Release();

	verifyc(buffer->QueryInterface(IID_IDirectSoundNotify8,(void**)&buffer_notify));

	DSBPOSITIONNOTIFY not[DSound51::MAX_BUFFER_COUNT];

	for(u32 i=0;i<numBuffers;i++)
	{
		buffer_events[i]=CreateEvent(NULL,FALSE,FALSE,NULL);
		not[i].dwOffset=(wfx.Format.nBlockAlign*2 + DSound51::BufferSizeBytes*(i+1))%desc.dwBufferBytes;
		not[i].hEventNotify=buffer_events[i];
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
	dsound_running=true;
	thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread,this,0,&tid);
	SetThreadPriority(thread,THREAD_PRIORITY_TIME_CRITICAL);
}


DSound51_Driver::~DSound51_Driver()
{
	// Stop Thread
	fprintf(stderr," * SPU2: Waiting for DSound thread to finish...");
	dsound_running = false;

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
			if( buffer_events[i] == NULL ) continue;
			CloseHandle( buffer_events[i] );
			buffer_events[i] = NULL;
		}

		SAFE_RELEASE( buffer_notify );
		SAFE_RELEASE( buffer );
	}
	SAFE_RELEASE( dsound );

}

DWORD DSound51_Driver::WorkerThread()
{
	while( dsound_running )
	{
		u32 rv = WaitForMultipleObjects(numBuffers,buffer_events,FALSE,200);

		s16* p1, *oldp1;
		LPVOID p2;
		DWORD s1,s2;

		u32 poffset = DSound51::BufferSizeBytes * rv;

#ifdef _DEBUG
		verifyc(buffer->Lock(poffset,DSound51::BufferSizeBytes,(LPVOID*)&p1,&s1,&p2,&s2,0));
#else
		if( FAILED(buffer->Lock(poffset,DSound51::BufferSizeBytes,(LPVOID*)&p1,&s1,&p2,&s2,0) ) )
		{
			fputs( " * SPU2 : Directsound Warning > Buffer lock failure.  You may need to increase the DSound buffer count.\n", stderr );
			continue;
		}
#endif
		oldp1 = p1;

		for(int p=0; p<DSound51::PacketsPerBuffer; p++, p1+=SndOutPacketSize )
		{
			s32 temp[SndOutPacketSize];
			s32* s = temp;
			s16* t = p1;

			buff->ReadSamples( temp );
			for(int j=0;j<SndOutPacketSize/2;j++)
			{
				// DPL2 code here: inputs s[0] and s[1]. outputs t[0] to t[5]
				dpl2.Convert( t, s[0], s[1] );

				t+=6;
				s+=2;
			}
		}

#ifndef PUBLIC
		verifyc(buffer->Unlock(oldp1,s1,p2,s2));
#else
		buffer->Unlock(oldp1,s1,p2,s2);
#endif

		// Set the write pointer to the beginning of the next block.
		myLastWrite = (poffset + DSound51::BufferSizeBytes) % (DSound51::BufferSizeBytes*numBuffers);
	}
	return 0;
}

int DSound51_Driver::GetEmptySampleCount() const
{
	DWORD play, write;
	buffer->GetCurrentPosition( &play, &write );

	// Note: Dsound's write cursor is bogus.  Use our own instead:

	int empty = play - myLastWrite;
	if( empty < 0 )
		empty = -empty;

	return empty / 6;
}

s32 DSound51::Init(SndBuffer* sb)
{
	if( m_driver == NULL )
		m_driver = new DSound51_Driver( sb );

	return 0;
}

void DSound51::Close()
{
	SAFE_DELETE_OBJ( m_driver );
}

BOOL CALLBACK DSound51::ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	int tSel=0;

	switch(uMsg)
	{
		case WM_INITDIALOG:

			haveGuid = ! FAILED(GUIDFromString(Config_DSound51.Device,&DevGuid));
			SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_RESETCONTENT,0,0);

			ndevs=0;
			DirectSoundEnumerate(DSEnumCallback,NULL);

			tSel=-1;
			for(int i=0;i<ndevs;i++)
			{
				SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_ADDSTRING,0,(LPARAM)devices[i].name);
				if(haveGuid && IsEqualGUID(devices[i].guid,DevGuid))
				{
					tSel=i;
				}
			}

			if(tSel>=0)
			{
				SendMessage(GetDlgItem(hWnd,IDC_DS_DEVICE),CB_SETCURSEL,tSel,0);
			}

			INIT_SLIDER(IDC_LEFT_GAIN_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_RIGHT_GAIN_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_RLEFT_GAIN_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_RRIGHT_GAIN_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_CENTER_GAIN_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_LFE_SLIDER,0,512,64,16,8);
			INIT_SLIDER(IDC_LR_CENTER_SLIDER,0,512,64,16,8);

			AssignSliderValue( hWnd, IDC_LEFT_GAIN_SLIDER, IDC_LEFT_GAIN_EDIT, Config_DSound51.GainL );
			AssignSliderValue( hWnd, IDC_RIGHT_GAIN_SLIDER, IDC_RIGHT_GAIN_EDIT, Config_DSound51.GainR );
			AssignSliderValue( hWnd, IDC_RLEFT_GAIN_SLIDER, IDC_RLEFT_GAIN_EDIT, Config_DSound51.GainSL );
			AssignSliderValue( hWnd, IDC_RRIGHT_GAIN_SLIDER, IDC_RRIGHT_GAIN_EDIT, Config_DSound51.GainSR );
			AssignSliderValue( hWnd, IDC_CENTER_GAIN_SLIDER, IDC_CENTER_GAIN_EDIT, Config_DSound51.GainC);
			AssignSliderValue( hWnd, IDC_LFE_SLIDER, IDC_LFE_EDIT, Config_DSound51.GainLFE);
			AssignSliderValue( hWnd, IDC_LR_CENTER_SLIDER, IDC_LR_CENTER_EDIT, Config_DSound51.AddCLR );

			char temp[128];
			INIT_SLIDER( IDC_BUFFERS_SLIDER, 2, MAX_BUFFER_COUNT, 2, 1, 1 );
			SendMessage(GetDlgItem(hWnd,IDC_BUFFERS_SLIDER),TBM_SETPOS,TRUE,Config_DSound51.NumBuffers);
			sprintf_s(temp, 128, "%d (%d ms latency)",Config_DSound51.NumBuffers, 1000 / (96000 / (Config_DSound51.NumBuffers * BufferSize)));
			SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL2),temp);
			break;

		case WM_COMMAND:
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
							Config_DSound51.Device[0]=0; // clear device name to ""
						}
						else
						{
							sprintf_s(Config_DSound51.Device,256,"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
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

							Config_DSound51.NumBuffers = GetSliderValue( hWnd, IDC_BUFFERS_SLIDER );
							Config_DSound51.GainL = GetSliderValue( hWnd, IDC_LEFT_GAIN_SLIDER );
							Config_DSound51.GainR = GetSliderValue( hWnd, IDC_RIGHT_GAIN_SLIDER );
							Config_DSound51.GainSL = GetSliderValue( hWnd, IDC_RLEFT_GAIN_SLIDER );
							Config_DSound51.GainSR = GetSliderValue( hWnd, IDC_RRIGHT_GAIN_SLIDER );
							Config_DSound51.GainLFE = GetSliderValue( hWnd, IDC_LFE_SLIDER );
							Config_DSound51.GainC = GetSliderValue( hWnd, IDC_CENTER_GAIN_SLIDER );
							Config_DSound51.AddCLR = GetSliderValue( hWnd, IDC_LR_CENTER_SLIDER );

							if( Config_DSound51.NumBuffers < 2 ) Config_DSound51.NumBuffers = 2;
							if( Config_DSound51.NumBuffers > MAX_BUFFER_COUNT ) Config_DSound51.NumBuffers = MAX_BUFFER_COUNT;
						}
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
					if( wmEvent < 2 ) wmEvent = 2;
					if( wmEvent > MAX_BUFFER_COUNT ) wmEvent = MAX_BUFFER_COUNT;
					SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
					sprintf_s(temp,128,"%d (%d ms latency)",wmEvent, 1000 / (96000 / (wmEvent * BufferSize)));
					SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL2),temp);
					break;
				default:
					return FALSE;
			}
			break;

		case WM_VSCROLL:
			HANDLE_SCROLL_MESSAGE(IDC_LEFT_GAIN_SLIDER,IDC_LEFT_GAIN_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_RIGHT_GAIN_SLIDER,IDC_RIGHT_GAIN_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_RLEFT_GAIN_SLIDER,IDC_RLEFT_GAIN_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_RRIGHT_GAIN_SLIDER,IDC_RRIGHT_GAIN_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_CENTER_GAIN_SLIDER,IDC_CENTER_GAIN_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_LFE_SLIDER,IDC_LFE_EDIT);
			HANDLE_SCROLL_MESSAGE(IDC_LR_CENTER_SLIDER,IDC_LR_CENTER_EDIT);

		default:
			return FALSE;
	}
	return TRUE;
}

void DSound51::Configure(HWND parent)
{
	INT_PTR ret;
	ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DSOUND51),GetActiveWindow(),(DLGPROC)ConfigProc,1);
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),"Error Opening the config dialog.","OMG ERROR!",MB_OK,0);
		return;
	}
}

s32 DSound51::Test() const
{
	return 0;
}

bool DSound51::Is51Out() const { return true; }

int DSound51::GetEmptySampleCount() const
{
	if( m_driver == NULL ) return 0;
	return m_driver->GetEmptySampleCount();
}

const char* DSound51::GetIdent() const
{
	return "dsound51";
}

const char* DSound51::GetLongName() const
{
	return "DSound 5.1 (Experimental)";
}

SndOutModule *DSound51Out=&DS51;
