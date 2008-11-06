#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "zerospu2.h"
#include "resource.h"

/////////////////////////////////////
// use DirectSound for the sound
#include <dsound.h>

#define SOUNDSIZE 76800//4*48*73
HWND hWMain;

LPDIRECTSOUND lpDS;
LPDIRECTSOUNDBUFFER lpDSBPRIMARY = NULL;
LPDIRECTSOUNDBUFFER lpDSBSECONDARY1 = NULL;
LPDIRECTSOUNDBUFFER lpDSBSECONDARY2 = NULL;
DSBUFFERDESC        dsbd;
DSBUFFERDESC        dsbdesc;
DSCAPS              dscaps;
DSBCAPS             dsbcaps;

unsigned long LastWrite=0xffffffff;
unsigned long LastWriteS=0xffffffff;
unsigned long LastPlay=0;

int SetupSound()
{
    HRESULT dsval;WAVEFORMATEX pcmwf;

    dsval = DirectSoundCreate(NULL,&lpDS,NULL);
    if(dsval!=DS_OK)  {
        MessageBox(NULL,"DirectSoundCreate!","Error",MB_OK);
        return -1;
    }

    if(DS_OK!=IDirectSound_SetCooperativeLevel(lpDS,hWMain, DSSCL_PRIORITY)) {
        if(DS_OK!=IDirectSound_SetCooperativeLevel(lpDS,hWMain, DSSCL_NORMAL)) {
            MessageBox(NULL,"SetCooperativeLevel!","Error",MB_OK);
            return -1;
        }
    }

    memset(&dsbd,0,sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);                                     // NT4 hack! sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;                 
    dsbd.dwBufferBytes = 0; 
    dsbd.lpwfxFormat = NULL;

    dsval=IDirectSound_CreateSoundBuffer(lpDS,&dsbd,&lpDSBPRIMARY,NULL);
    if(dsval!=DS_OK) {
        MessageBox(NULL, "CreateSoundBuffer (Primary)", "Error",MB_OK);
        return -1;
    }

    memset(&pcmwf, 0, sizeof(WAVEFORMATEX));
    pcmwf.wFormatTag = WAVE_FORMAT_PCM;

    pcmwf.nChannels = 2; 
    pcmwf.nBlockAlign = 4;

    pcmwf.nSamplesPerSec = 48000;

    pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
    pcmwf.wBitsPerSample = 16;

    dsval=IDirectSoundBuffer_SetFormat(lpDSBPRIMARY,&pcmwf);
    if(dsval!=DS_OK) {
        MessageBox(NULL, "SetFormat!", "Error",MB_OK);
        return -1;
    }

    dscaps.dwSize = sizeof(DSCAPS);
    dsbcaps.dwSize = sizeof(DSBCAPS);
    IDirectSound_GetCaps(lpDS,&dscaps);
    IDirectSoundBuffer_GetCaps(lpDSBPRIMARY,&dsbcaps);

    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);                                  // NT4 hack! sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_LOCHARDWARE | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    dsbdesc.dwBufferBytes = SOUNDSIZE;
    dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

    dsval=IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc,&lpDSBSECONDARY1,NULL);
    if(dsval!=DS_OK) {
        dsbdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
        dsval=IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc,&lpDSBSECONDARY1,NULL);
        if(dsval!=DS_OK) {
            MessageBox(NULL,"CreateSoundBuffer (Secondary1)", "Error",MB_OK);
            return -1;
        }
    }

    dsval=IDirectSoundBuffer_Play(lpDSBPRIMARY,0,0,DSBPLAY_LOOPING);
    if(dsval!=DS_OK) {
        MessageBox(NULL,"Play (Primary)","Error",MB_OK);
        return -1;
    }

    dsval=IDirectSoundBuffer_Play(lpDSBSECONDARY1,0,0,DSBPLAY_LOOPING);
    if(dsval!=DS_OK) {
        MessageBox(NULL,"Play (Secondary1)","Error",MB_OK);
        return -1;
    }

    LastWrite=0x00000000;LastPlay=0;                      // init some play vars
    return 0;
}

void RemoveSound()
{ 
    int iRes;

    if(lpDSBSECONDARY1!=NULL) {
        IDirectSoundBuffer_Stop(lpDSBSECONDARY1);
        iRes=IDirectSoundBuffer_Release(lpDSBSECONDARY1);
        // FF says such a loop is bad... Demo says it's good... Pete doesn't care
        while(iRes!=0) iRes=IDirectSoundBuffer_Release(lpDSBSECONDARY1);
        lpDSBSECONDARY1=NULL;
    }

    if(lpDSBPRIMARY!=NULL) {
        IDirectSoundBuffer_Stop(lpDSBPRIMARY);
        iRes=IDirectSoundBuffer_Release(lpDSBPRIMARY);
        // FF says such a loop is bad... Demo says it's good... Pete doesn't care
        while(iRes!=0) iRes=IDirectSoundBuffer_Release(lpDSBPRIMARY);
        lpDSBPRIMARY=NULL;
    }

    if(lpDS!=NULL) {
        iRes=IDirectSound_Release(lpDS);
        // FF says such a loop is bad... Demo says it's good... Pete doesn't care
        while(iRes!=0) iRes=IDirectSound_Release(lpDS);
        lpDS=NULL;
    }

}

int SoundGetBytesBuffered()
{
    unsigned long cplay,cwrite;

    if(LastWrite==0xffffffff) return 0;

    IDirectSoundBuffer_GetCurrentPosition(lpDSBSECONDARY1,&cplay,&cwrite);

    if(cplay>SOUNDSIZE) return SOUNDSIZE;

    if(cplay<LastWrite) return LastWrite-cplay;
    return (SOUNDSIZE-cplay)+LastWrite;
}

void SoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
    LPVOID lpvPtr1, lpvPtr2;
    unsigned long dwBytes1,dwBytes2; 
    unsigned long *lpSS, *lpSD;
    unsigned long dw,cplay,cwrite;
    HRESULT hr;
    unsigned long status;

    IDirectSoundBuffer_GetStatus(lpDSBSECONDARY1,&status);
    if(status&DSBSTATUS_BUFFERLOST) {
        if(IDirectSoundBuffer_Restore(lpDSBSECONDARY1)!=DS_OK) return;
        IDirectSoundBuffer_Play(lpDSBSECONDARY1,0,0,DSBPLAY_LOOPING);
    }

    IDirectSoundBuffer_GetCurrentPosition(lpDSBSECONDARY1,&cplay,&cwrite);

    if(LastWrite==0xffffffff) LastWrite=cwrite;

    hr=IDirectSoundBuffer_Lock(lpDSBSECONDARY1,LastWrite,lBytes,
        &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0);

    if(hr!=DS_OK) {LastWrite=0xffffffff;return;}

    lpSD=(unsigned long *)lpvPtr1;
    dw=dwBytes1>>2;

    lpSS=(unsigned long *)pSound;
    while(dw) {
        *lpSD++=*lpSS++;
        dw--;
    }

    if(lpvPtr2) {
        lpSD=(unsigned long *)lpvPtr2;
        dw=dwBytes2>>2;
        while(dw) {
            *lpSD++=*lpSS++;
            dw--;
        }
    }

    IDirectSoundBuffer_Unlock(lpDSBSECONDARY1,lpvPtr1,dwBytes1,lpvPtr2,dwBytes2);
    LastWrite+=lBytes;
    if(LastWrite>=SOUNDSIZE) LastWrite-=SOUNDSIZE;
    LastPlay=cplay;
}

/////////
// GUI //
/////////
HINSTANCE hInst;

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "SPU2NULL Msg", 0);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
	switch(uMsg) {
		case WM_INITDIALOG:
			LoadConfig();
            if (conf.Log)        CheckDlgButton(hW, IDC_LOGGING, TRUE);
            //if( conf.options & OPTION_FFXHACK) CheckDlgButton(hW, IDC_FFXHACK, TRUE);
            if( conf.options & OPTION_REALTIME) CheckDlgButton(hW, IDC_REALTIME, TRUE);
            if( conf.options & OPTION_TIMESTRETCH) CheckDlgButton(hW, IDC_TIMESTRETCH, TRUE);
            if( conf.options & OPTION_MUTE) CheckDlgButton(hW, IDC_MUTESOUND, TRUE);
            if( conf.options & OPTION_RECORDING) CheckDlgButton(hW, IDC_SNDRECORDING, TRUE);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
                    conf.options = 0;
					if (IsDlgButtonChecked(hW, IDC_LOGGING))
						 conf.Log = 1;
                    else conf.Log = 0;
//                    if( IsDlgButtonChecked(hW, IDC_FFXHACK) )
//                        conf.options |= OPTION_FFXHACK;
                    if( IsDlgButtonChecked(hW, IDC_REALTIME) )
                        conf.options |= OPTION_REALTIME;
                    if( IsDlgButtonChecked(hW, IDC_TIMESTRETCH) )
                        conf.options |= OPTION_TIMESTRETCH;
                    if( IsDlgButtonChecked(hW, IDC_MUTESOUND) )
                        conf.options |= OPTION_MUTE;
                    if( IsDlgButtonChecked(hW, IDC_SNDRECORDING) )
                        conf.options |= OPTION_RECORDING;
					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK SPU2configure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_CONFIG),
              GetActiveWindow(),  
              (DLGPROC)ConfigureDlgProc); 
}

void CALLBACK SPU2about() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT),
              GetActiveWindow(),  
              (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

extern HINSTANCE hInst;
void SaveConfig() 
{
    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\zerospu2.ini");
	sprintf(szValue,"%u",Conf1->Log);
    WritePrivateProfileString("Interface", "Logging",szValue,szIniFile);
    sprintf(szValue,"%u",Conf1->options);
    WritePrivateProfileString("Interface", "Options",szValue,szIniFile);

}

void LoadConfig()
{
    FILE *fp;
    Config *Conf1 = &conf;
	char *szTemp;
	char szIniFile[256], szValue[256];
  
	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\zerospu2.ini");
    fp=fopen("inis\\zerospu2.ini","rt");//check if usbnull.ini really exists
	if (!fp)
	{
		CreateDirectory("inis",NULL); 
        memset(&conf, 0, sizeof(conf));
        conf.Log = 0;//default value
        conf.options = OPTION_TIMESTRETCH;
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);
	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
	Conf1->Log = strtoul(szValue, NULL, 10);
    GetPrivateProfileString("Interface", "Options", NULL, szValue, 20, szIniFile);
    Conf1->options = strtoul(szValue, NULL, 10);
	return ;

}

