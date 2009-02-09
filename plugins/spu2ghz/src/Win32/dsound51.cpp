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
#define _WIN32_DCOM
#include "spu2.h"
#include "dialogs.h"
#include <initguid.h>
#include <windows.h>
#include <dsound.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include <assert.h>
#include <Mmreg.h>
#include <ks.h>
#include <KSMEDIA.H>

#include "lowpass.h"

#define ENABLE_DPLII

struct ds_device_data {
	char name[256];
	GUID guid;
	bool hasGuid;
};

static ds_device_data devices[32];
static int ndevs;
static GUID DevGuid;		// currently employed GUID.
static bool haveGuid;

extern HRESULT GUIDFromString(const char *str, LPGUID guid);

class DSound51: public SndOutModule
{
private:
	#	define MAX_BUFFER_COUNT 8

	static const int PacketsPerBuffer = 1;
	static const int BufferSize = SndOutPacketSize * PacketsPerBuffer * 3;
	static const int BufferSizeBytes = BufferSize << 1;


	FILE *voicelog;

	u32 numBuffers;		// cached copy of our configuration setting.
	int channel;
	int myLastWrite;

	bool dsound_running;
	HANDLE thread;
	DWORD tid;

	IDirectSound8* dsound;
	IDirectSoundBuffer8* buffer;
	IDirectSoundNotify8* buffer_notify;
	HANDLE buffer_events[MAX_BUFFER_COUNT];

	WAVEFORMATEXTENSIBLE wfx;

	SndBuffer *buff;

	s32 LAccum;
	s32 RAccum;
	s32 ANum;

	s32 LBuff[128];
	s32 RBuff[128];

	LPF_data lpf_l;
	LPF_data lpf_r;

	void Convert(s16 *obuffer, s32 ValL, s32 ValR)
	{
		static u8 bufdone=1;
		static s32 Gfl=0,Gfr=0;

		static s32 spdif_data[6];
		static s32 LMax=0,RMax=0;

		if(PlayMode&4)
		{
			spdif_get_samples(spdif_data); 
		}
		else
		{
			spdif_data[0]=0;
			spdif_data[1]=0;
			spdif_data[2]=0;
			spdif_data[3]=0;
			spdif_data[4]=0;
			spdif_data[5]=0;
		}

#ifdef ENABLE_DPLII
#ifdef USE_AVERAGING
		LAccum+=abs(ValL);
		RAccum+=abs(ValR);
		ANum++;
		
		if(ANum>=512)
		{
			LMax=0;RMax=0;

			LAccum/=ANum;
			RAccum/=ANum;
			ANum=0;

			for(int i=0;i<127;i++)
			{
				LMax+=LBuff[i];
				RMax+=RBuff[i];
				LBuff[i]=LBuff[i+1];
				RBuff[i]=RBuff[i+1];
			}
			LBuff[127]=LAccum;
			RBuff[127]=RAccum;
			LMax+=LAccum;
			RMax+=RAccum;

			s32 TL = (LMax>>15)+1;
			s32 TR = (RMax>>15)+1;

			Gfl=(RMax)/(TL);
			Gfr=(LMax)/(TR);

			if(Gfl>255) Gfl=255;
			if(Gfr>255) Gfr=255;
		}
#else
		
		if(ValL>LMax) LMax = ValL;
		if(-ValL>LMax) LMax = -ValL;
		if(ValR>RMax) RMax = ValR;
		if(-ValR>RMax) RMax = -ValR;
		ANum++;
		if(ANum>=128)
		{
			// shift into a 21 bit value
			const u8 shift = SndOutVolumeShift-5;

			ANum=0;
			LAccum = ((LAccum * 224) + (LMax>>shift))>>8;
			RAccum = ((RAccum * 224) + (RMax>>shift))>>8;

			LMax=0;
			RMax=0;

			if(LAccum<1) LAccum=1;
			if(RAccum<1) RAccum=1;

			Gfl=(RAccum)*255/(LAccum);
			Gfr=(LAccum)*255/(RAccum);

			int gMax = max(Gfl,Gfr);

			Gfl=Gfl*255/gMax;
			Gfr=Gfr*255/gMax;

			if(Gfl>255) Gfl=255;
			if(Gfr>255) Gfr=255;
			if(Gfl<1) Gfl=1;
			if(Gfr<1) Gfr=1;
		}
#endif

		Gfr = 1; Gfl = 1;

		s32 L,R,C,LFE,SL,SR,LL,LR;

		extern double pow_2_31;

		// shift Values into 12 bits:
		u8 shift2 = SndOutVolumeShift + 4;

		LL = (s32)(LPF(&lpf_l,(ValL>>shift2)/pow_2_31)*pow_2_31);
		LR = (s32)(LPF(&lpf_r,(ValR>>shift2)/pow_2_31)*pow_2_31);
		LFE = (LL + LR)>>4;

		C=(ValL+ValR)>>1; //16.8

		ValL-=C;//16.8
		ValR-=C;//16.8

		L=ValL>>SndOutVolumeShift; //16.0
		R=ValR>>SndOutVolumeShift; //16.0
		C=C>>SndOutVolumeShift;    //16.0

		s32 VL=(ValL>>4) * Gfl; //16.12
		s32 VR=(ValR>>4) * Gfr;

		SL=(VL/209 - VR/148)>>4; //16.0 (?)
		SR=(VL/148 - VR/209)>>4; //16.0 (?)

		// increase surround stereo separation
		int SC = (SL+SR)>>1; //16.0
		int SLd = SL - SC;   //16.0
		int SRd = SL - SC;   //16.0

		SL = (SLd * 209 + SC * 148)>>8; //16.0
		SR = (SRd * 209 + SC * 148)>>8; //16.0

		obuffer[0]=spdif_data[0] + (((L   * Config_DSound51.GainL  ) + (C * Config_DSound51.AddCLR))>>8);
		obuffer[1]=spdif_data[1] + (((R   * Config_DSound51.GainR  ) + (C * Config_DSound51.AddCLR))>>8);
		obuffer[2]=spdif_data[2] + (((C   * Config_DSound51.GainC  ))>>8);
		obuffer[3]=spdif_data[3] + (((LFE * Config_DSound51.GainLFE))>>8);
		obuffer[4]=spdif_data[4] + (((SL  * Config_DSound51.GainSL ))>>8);
		obuffer[5]=spdif_data[5] + (((SR  * Config_DSound51.GainSR ))>>8);
#else
		obuffer[0]=spdif_data[0]+(ValL>>8);
		obuffer[1]=spdif_data[1]+(ValR>>8);
		obuffer[2]=spdif_data[2];
		obuffer[3]=spdif_data[3];
		obuffer[4]=spdif_data[4];
		obuffer[5]=spdif_data[5];
#endif
	}

#	define STRFY(x) #x

#	define verifyc(x) if(Verifyc(x,STRFY(x))) return -1;

	static DWORD CALLBACK RThread(DSound51*obj)
	{
		return obj->Thread();
	}

	int __forceinline Verifyc(HRESULT hr,const char* fn)
	{
		if(FAILED(hr))
		{
			SysMessage("ERROR: Call to %s Failed.",fn);
			return -1;
		}
		return 0;
	}

	DWORD Thread()
	{
		while( dsound_running )
		{
			u32 rv = WaitForMultipleObjects(numBuffers,buffer_events,FALSE,200);
	 
			s16* p1, *oldp1;
			LPVOID p2;
			DWORD s1,s2;
	 
			u32 poffset=BufferSizeBytes * rv;

#ifdef _DEBUG
			verifyc(buffer->Lock(poffset,BufferSizeBytes,(LPVOID*)&p1,&s1,&p2,&s2,0));
#else
			if( FAILED(buffer->Lock(poffset,BufferSizeBytes,(LPVOID*)&p1,&s1,&p2,&s2,0) ) )
			{
				fputs( " * SPU2 : Directsound Warning > Buffer lock failure.  You may need to increase the DSound buffer count.\n", stderr );
				continue;
			}
#endif
			oldp1 = p1;

			for(int p=0; p<PacketsPerBuffer; p++, p1+=SndOutPacketSize )
			{
				s32 temp[SndOutPacketSize];
				s32* s = temp;
				s16* t = p1;

				buff->ReadSamples( temp );
				for(int j=0;j<SndOutPacketSize/2;j++)
				{
					// DPL2 code here: inputs s[0] and s[1]. outputs t[0] to t[5]
					Convert(t,s[0],s[1]);

					t+=6;
					s+=2;
				}				
			}

#ifndef PUBLIC
			verifyc(buffer->Unlock(oldp1,s1,p2,s2));
#else
			buffer->Unlock(oldp1,s1,p2,s2);
#endif
			verifyc(buffer->Unlock(oldp1,s1,p2,s2));

			// Set the write pointer to the beginning of the next block.
			myLastWrite = (poffset + BufferSizeBytes) % (BufferSizeBytes*numBuffers);
		}
		return 0;
	}

public:
	s32  Init(SndBuffer *sb)
	{
		buff = sb;
		numBuffers = Config_DSound51.NumBuffers;

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
		desc.dwBufferBytes = BufferSizeBytes * numBuffers; 
		desc.lpwfxFormat = &wfx.Format; 
	 
		desc.dwFlags |=DSBCAPS_LOCSOFTWARE;
		desc.dwFlags|=DSBCAPS_GLOBALFOCUS;
	 
		verifyc(dsound->CreateSoundBuffer(&desc,&buffer_,0));
		verifyc(buffer_->QueryInterface(IID_IDirectSoundBuffer8,(void**)&buffer));
		buffer_->Release();
	 
		verifyc(buffer->QueryInterface(IID_IDirectSoundNotify8,(void**)&buffer_notify));

		DSBPOSITIONNOTIFY not[MAX_BUFFER_COUNT];
	 
		for(u32 i=0;i<numBuffers;i++)
		{
			buffer_events[i]=CreateEvent(NULL,FALSE,FALSE,NULL);
			not[i].dwOffset=(wfx.Format.nBlockAlign*2 + BufferSizeBytes*(i+1))%desc.dwBufferBytes;
			not[i].hEventNotify=buffer_events[i];
		}
	 
		buffer_notify->SetNotificationPositions(numBuffers,not);
	 
		LPVOID p1=0,p2=0;
		DWORD s1=0,s2=0;
	 
		verifyc(buffer->Lock(0,desc.dwBufferBytes,&p1,&s1,&p2,&s2,0));
		assert(p2==0);
		memset(p1,0,s1);
		verifyc(buffer->Unlock(p1,s1,p2,s2));
	 
		LPF_init(&lpf_l,Config_DSound51.LowpassLFE,SampleRate);
		LPF_init(&lpf_r,Config_DSound51.LowpassLFE,SampleRate);

		//Play the buffer !
		verifyc(buffer->Play(0,0,DSBPLAY_LOOPING));

		// Start Thread
		myLastWrite = 0;
		dsound_running=true;
		thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread,this,0,&tid);
		SetThreadPriority(thread,THREAD_PRIORITY_TIME_CRITICAL);

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
				if( buffer_events[i] == NULL ) continue;
				CloseHandle( buffer_events[i] );
				buffer_events[i] = NULL;
			}
		 
			SAFE_RELEASE( buffer_notify );
			SAFE_RELEASE( buffer );
		}
		SAFE_RELEASE( dsound );
	}

private:

	static BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext )
	{
		//strcpy(devices[ndevs].name,lpcstrDescription);
		_snprintf_s(devices[ndevs].name,256,255,"%s",lpcstrDescription);

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

	static void AssignSliderValue( HWND idcwnd, HWND hwndDisplay, int value )
	{
		value = min( max( value, 0 ), 512 );
		SendMessage(idcwnd,TBM_SETPOS,TRUE,value); 
		
		char tbox[24];
		sprintf_s( tbox, 16, "%d", value );
		SetWindowText( hwndDisplay, tbox );
	}

	static void AssignSliderValue( HWND hWnd, int idc, int editbox, int value )
	{
		AssignSliderValue( GetDlgItem( hWnd, idc ), GetDlgItem( hWnd, editbox ), value );
	}

	static BOOL DoHandleScrollMessage(WPARAM wParam, LPARAM lParam, HWND hwndDisplay)
	{
		int wmId    = LOWORD(wParam); 
		int wmEvent = HIWORD(wParam); 
		static char temp[64];
		switch(wmId) {
			//case TB_ENDTRACK:
			//case TB_THUMBPOSITION:
			case TB_LINEUP:
			case TB_LINEDOWN:
			case TB_PAGEUP:
			case TB_PAGEDOWN:
				wmEvent=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			case TB_THUMBTRACK:
				AssignSliderValue( (HWND)lParam, hwndDisplay, wmEvent );
				break;
			default:
				return FALSE;
		}
		return TRUE;
	}

	static int GetSliderValue( HWND hWnd, int idc )
	{
		int retval = (int)SendMessage( GetDlgItem( hWnd, idc ), TBM_GETPOS, 0, 0 );
		return min( max( retval, 0), 512);
	}

#define HANDLE_SCROLL_MESSAGE(idc,idcDisplay) \
			if((HWND)lParam == GetDlgItem(hWnd,idc)) return DoHandleScrollMessage(wParam,lParam,GetDlgItem(hWnd,idcDisplay))

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

public:
	virtual void Configure(HWND parent)
	{
		INT_PTR ret;
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DSOUND51),GetActiveWindow(),(DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBoxEx(GetActiveWindow(),"Error Opening the config dialog.","OMG ERROR!",MB_OK,0);
			return;
		}
	}

	s32  Test() const
	{
		return 0;
	}

	bool Is51Out() const { return true; }

	int GetEmptySampleCount() const
	{
		DWORD play, write;
		buffer->GetCurrentPosition( &play, &write );

		// Note: Dsound's write cursor is bogus.  Use our own instead:

		int empty = play - myLastWrite;
		if( empty < 0 )
			empty = -empty;

		return empty / 6;

	}

	const char* GetIdent() const
	{
		return "dsound51";
	}

	const char* GetLongName() const
	{
		return "DSound 5.1 (Experimental)";
	}
} DS51;

SndOutModule *DSound51Out=&DS51;