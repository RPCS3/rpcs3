/***************************************************************************
                            cfg.c  -  description
                             -------------------
    begin                : Sun Nov 16 2003
    copyright            : (C) 2003 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2003/11/16 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#define _IN_CFG
#include "externals.h"

////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// read config from registry

void ReadConfig(void)
{
 HKEY myKey;DWORD temp,type,size;

 // init values

 iCD_AD=-1;            
 iCD_TA=-1;               
 iCD_LU=-1;
 iRType=0;
 iUseSpeedLimit=0;
 iSpeedLimit=2;
 iNoWait=0;
 iMaxRetry=5;
 iShowReadErr=0;
 iUsePPF=0;
 iUseSubReading=0;
 iUseDataCache=0;
 iCheckTrayStatus=0;
 memset(szPPF,0,260);
 memset(szSUBF,0,260);

 // read values

 if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\CDVD\\CDVDPeops",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
  {
   size = 4;
   if(RegQueryValueEx(myKey,"Adapter",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_AD=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"Target",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_TA=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"LUN",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCD_LU=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseCaching",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseCaching=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseDataCache",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseDataCache=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseSpeedLimit",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseSpeedLimit=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"SpeedLimit",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iSpeedLimit=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"NoWait",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iNoWait=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"CheckTrayStatus",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iCheckTrayStatus=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"MaxRetry",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iMaxRetry=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"ShowReadErr",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iShowReadErr=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UsePPF",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUsePPF=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseSubReading",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseSubReading=(int)temp;
   size=259;
   RegQueryValueEx(myKey,"PPFFile",0,&type,(LPBYTE)szPPF,&size);
   size=259;
   RegQueryValueEx(myKey,"SCFile",0,&type,(LPBYTE)szSUBF,&size);

   RegCloseKey(myKey);
  }
  
 // disabled for now 
 iUsePPF=0; 
}

////////////////////////////////////////////////////////////////////////
// write user config

void WriteConfig(void)
{
 HKEY myKey;DWORD myDisp,temp;
                                   
 RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\CDVD\\CDVDPeops",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&myDisp);
 temp=iInterfaceMode;
 RegSetValueEx(myKey,"InterfaceMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_AD;
 RegSetValueEx(myKey,"Adapter",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_TA;
 RegSetValueEx(myKey,"Target",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCD_LU;
 RegSetValueEx(myKey,"LUN",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseCaching;
 RegSetValueEx(myKey,"UseCaching",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseDataCache;
 RegSetValueEx(myKey,"UseDataCache",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseSpeedLimit;
 RegSetValueEx(myKey,"UseSpeedLimit",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iSpeedLimit;
 RegSetValueEx(myKey,"SpeedLimit",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iNoWait;
 RegSetValueEx(myKey,"NoWait",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iCheckTrayStatus ;
 RegSetValueEx(myKey,"CheckTrayStatus",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iMaxRetry;
 RegSetValueEx(myKey,"MaxRetry",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iShowReadErr;
 RegSetValueEx(myKey,"ShowReadErr",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUsePPF;
 RegSetValueEx(myKey,"UsePPF",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseSubReading;
 RegSetValueEx(myKey,"UseSubReading",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));

 RegSetValueEx(myKey,"PPFFile",0,REG_BINARY,(LPBYTE)szPPF,259);  
 RegSetValueEx(myKey,"SCFile",0,REG_BINARY,(LPBYTE)szSUBF,259);  

 RegCloseKey(myKey);
}

////////////////////////////////////////////////////////////////////////
// choose ppf/sbi/m3s file name

void OnChooseFile(HWND hW,int iFType)
{
 OPENFILENAME ofn;char szB[260];BOOL b;

 ofn.lStructSize=sizeof(OPENFILENAME); 
 ofn.hwndOwner=hW;                      
 ofn.hInstance=NULL; 
 if(iFType==0)      ofn.lpstrFilter="PPF Files\0*.PPF\0\0\0"; 
 else if(iFType==1) ofn.lpstrFilter="SBI Files\0*.SBI\0M3S Files\0*.M3S\0\0\0"; 
 else if(iFType==2) ofn.lpstrFilter="SUB Files\0*.SUB\0\0\0"; 
 else if(iFType==3) ofn.lpstrFilter="SBI Files\0*.SBI\0\0\0"; 
 else               ofn.lpstrFilter="M3S Files\0*.M3S\0\0\0"; 

 ofn.lpstrCustomFilter=NULL; 
 ofn.nMaxCustFilter=0;
 ofn.nFilterIndex=0; 
 if(iFType==0)      GetDlgItemText(hW,IDC_PPFFILE,szB,259);
 else if(iFType==1) GetDlgItemText(hW,IDC_SUBFILE,szB,259);
 else if(iFType==2) GetDlgItemText(hW,IDC_SUBFILEEDIT,szB,259);
 else if(iFType==3) GetDlgItemText(hW,IDC_OUTFILEEDIT,szB,259);
 else               GetDlgItemText(hW,IDC_OUTFILEEDIT,szB,259);

 ofn.lpstrFile=szB;
 ofn.nMaxFile=259; 
 ofn.lpstrFileTitle=NULL;
 ofn.nMaxFileTitle=0; 
 ofn.lpstrInitialDir=NULL;
 ofn.lpstrTitle=NULL; 
 if(iFType<3)
  ofn.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_HIDEREADONLY;     
 else
  ofn.Flags=OFN_CREATEPROMPT|OFN_NOCHANGEDIR|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;    
 ofn.nFileOffset=0; 
 ofn.nFileExtension=0;
 ofn.lpstrDefExt=0; 
 ofn.lCustData=0;
 ofn.lpfnHook=NULL; 
 ofn.lpTemplateName=NULL;
    
 if(iFType<3)
      b=GetOpenFileName(&ofn);
 else b=GetSaveFileName(&ofn);
                                                  
 if(b)
  {
   if(iFType==0)      SetDlgItemText(hW,IDC_PPFFILE,szB);
   else if(iFType==1) SetDlgItemText(hW,IDC_SUBFILE,szB);
   else if(iFType==2) SetDlgItemText(hW,IDC_SUBFILEEDIT,szB);
   else if(iFType==3) SetDlgItemText(hW,IDC_OUTFILEEDIT,szB);
   else               SetDlgItemText(hW,IDC_OUTFILEEDIT,szB);
  }
}

////////////////////////////////////////////////////////////////////////
// file drive combo

void EnumDrives(HWND hW)
{
 HWND hWC;char szB[256];int i=0,k=0,iNum;
 char * p, * pBuf, * pN;

 hWC=GetDlgItem(hW,IDC_DRIVE);
 ComboBox_ResetContent(hWC);
 ComboBox_AddString(hWC,"NONE");                       // add always existing 'none'

 wsprintf(szB,"[%d:%d:%d",iCD_AD,iCD_TA,iCD_LU);       // make current user info text

 pN=pBuf=(char *)malloc(32768);
 memset(pBuf,0,32768);
 iNum=GetGenCDDrives(pBuf);                            // get the system cd drives list

 for(i=0;i<iNum;i++)                                   // loop drives
  {
   ComboBox_AddString(hWC,pN);                         // -> add drive name
   p=strchr(pN,']');
   if(p)
    {
     *p=0;
     if(strcmp(szB,pN)==0) k=i+1;                      // -> is it the current user drive? sel it
     *p=']';
    }
   pN+=strlen(pN)+1;                                   // next drive in buffer
  }

 free(pBuf);

 ComboBox_SetCurSel(hWC,k);                            // do the drive sel
}

////////////////////////////////////////////////////////////////////////
// get curr selected drive

void GetCDRInfos(HWND hW,int * iA, int * iT,int * iL)
{
 HWND hWC=GetDlgItem(hW,IDC_DRIVE);
 char szB[256];int i;char * p;

 i=ComboBox_GetCurSel(hWC);
 if(i<=0)                                              // none selected
  {
   *iA=-1;*iT=-1;*iL=-1;
   MessageBox(hW,"Please select a cdrom drive!","Config error",MB_OK|MB_ICONINFORMATION);
   return;
  }

 ComboBox_GetLBText(hWC,i,szB);                        // get cd text
 p=szB+1;
 *iA=atoi(p);                                          // get AD,TA,LU
 p=strchr(szB,':')+1;
 *iT=atoi(p);
 p=strchr(p,':')+1;
 *iL=atoi(p);
}

////////////////////////////////////////////////////////////////////////
// interface mode has changed

void OnIMode(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_IMODE);
 int iM  = ComboBox_GetCurSel(hWC);

 GetCDRInfos(hW,&iCD_AD,&iCD_TA,&iCD_LU);              // get sel drive
 CloseGenInterface();                                  // close current interface
 iInterfaceMode=iM;                                    // set new interface mode
 OpenGenInterface();                                   // open new interface
 ComboBox_SetCurSel(hWC,iInterfaceMode);               // sel interface again (maybe it was not supported on open)
 EnumDrives(hW);                                       // enum drives again
}

////////////////////////////////////////////////////////////////////////
// cache mode has changed

void OnCache(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_CACHE);
 if(ComboBox_GetCurSel(hWC)<=0)
      ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);
 else ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_SHOW);
}

////////////////////////////////////////////////////////////////////////
// show/hide files depending on subc mode

void ShowSubFileStuff(HWND hW)
{
 HWND hWC=GetDlgItem(hW,IDC_SUBCHAN0);
 int iShow,iSel=ComboBox_GetCurSel(hWC);

 if(iSel==2) iShow=SW_SHOW;
 else        iShow=SW_HIDE; 

 ShowWindow(GetDlgItem(hW,IDC_SFSTATIC),iShow);
 ShowWindow(GetDlgItem(hW,IDC_SUBFILE),iShow);
 ShowWindow(GetDlgItem(hW,IDC_CHOOSESUBF),iShow);

 if(iSel==1)
  {
   ComboBox_SetCurSel(GetDlgItem(hW,IDC_CACHE),0);
   ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);
  }
}

////////////////////////////////////////////////////////////////////////
// init dialog

BOOL OnInitCDRDialog(HWND hW) 
{
 HWND hWC;int i=0;

 ReadConfig();                                         // read config

 hWC=GetDlgItem(hW,IDC_IMODE);                         // interface
 ComboBox_AddString(hWC,"NONE");
 ComboBox_AddString(hWC,"W9X/ME - ASPI scsi commands");
 ComboBox_AddString(hWC,"W2K/XP - IOCTL scsi commands");
 
// not supported with my dvd drive - DISABLED! 
// ComboBox_AddString(hWC,"W2K/XP - IOCTL raw reading");

 ComboBox_SetCurSel(hWC,iInterfaceMode);

 EnumDrives(hW);                                       // enum drives

 hWC=GetDlgItem(hW,IDC_CACHE);                         // caching
 ComboBox_AddString(hWC,"None - reads one sector");
 ComboBox_AddString(hWC,"Read ahead - fast, reads more sectors at once");
 ComboBox_AddString(hWC,"Async read - faster, additional asynchronous reads");
 ComboBox_AddString(hWC,"Thread read - fast with IOCTL, always async reads");
 ComboBox_AddString(hWC,"Smooth read - for drives with ps2 cd/dvd reading troubles");
 ComboBox_SetCurSel(hWC,iUseCaching);

 if(iUseDataCache)
  CheckDlgButton(hW,IDC_DATACACHE,TRUE);
 if(!iUseCaching)
  ShowWindow(GetDlgItem(hW,IDC_DATACACHE),SW_HIDE);

 if(iUseSpeedLimit)                                    // speed limit
  CheckDlgButton(hW,IDC_SPEEDLIMIT,TRUE);

 if(iNoWait)                                           // wait for drive
  CheckDlgButton(hW,IDC_NOWAIT,TRUE);
     
 if(iCheckTrayStatus)                                  // tray status
  CheckDlgButton(hW,IDC_TRAYSTATE,TRUE);

 SetDlgItemInt(hW,IDC_RETRY,iMaxRetry,FALSE);          // retry on error
 if(iMaxRetry)      CheckDlgButton(hW,IDC_TRYAGAIN,TRUE);
 if(iShowReadErr)   CheckDlgButton(hW,IDC_SHOWREADERR,TRUE);

 hWC=GetDlgItem(hW,IDC_SUBCHAN0);                      // subchannel mode
 ComboBox_AddString(hWC,"Don't read subchannels");
 ComboBox_AddString(hWC,"Read subchannels (slow, few drives support it, best chances with BE mode)");
 ComboBox_AddString(hWC,"Use subchannel SBI/M3S info file (recommended)");
 ComboBox_SetCurSel(hWC,iUseSubReading);

 ShowSubFileStuff(hW);                                 // show/hide subc controls

 hWC=GetDlgItem(hW,IDC_SPEED);                         // speed limit
 ComboBox_AddString(hWC,"2 X");
 ComboBox_AddString(hWC,"4 X");
 ComboBox_AddString(hWC,"8 X");
 ComboBox_AddString(hWC,"16 X");

 i=0;
 if(iSpeedLimit==4)  i=1;
 if(iSpeedLimit==8)  i=2;
 if(iSpeedLimit==16) i=3;

 ComboBox_SetCurSel(hWC,i);

 if(iUsePPF) CheckDlgButton(hW,IDC_USEPPF,TRUE);       // ppf
 SetDlgItemText(hW,IDC_PPFFILE,szPPF);
 SetDlgItemText(hW,IDC_SUBFILE,szSUBF);

 return TRUE;	
}

////////////////////////////////////////////////////////////////////////

void OnCDROK(HWND hW) 
{
 int iA,iT,iL,iR;
 HWND hWC=GetDlgItem(hW,IDC_RTYPE);

 GetCDRInfos(hW,&iA,&iT,&iL);
 if(iA==-1) return;

 hWC=GetDlgItem(hW,IDC_CACHE);
 iUseCaching=ComboBox_GetCurSel(hWC);
 if(iUseCaching<0) iUseCaching=0;
 if(iUseCaching>4) iUseCaching=4;

 iCD_AD=iA;iCD_TA=iT;iCD_LU=iL;

 if(IsDlgButtonChecked(hW,IDC_SPEEDLIMIT))
      iUseSpeedLimit=1;
 else iUseSpeedLimit=0;

 iUseSubReading=0;
 hWC=GetDlgItem(hW,IDC_SUBCHAN0);
 iUseSubReading=ComboBox_GetCurSel(hWC);
 if(iUseSubReading<0) iUseSubReading=0;
 if(iUseSubReading>2) iUseSubReading=2;
 if(iUseSubReading==1) iUseCaching=0;

 if(IsDlgButtonChecked(hW,IDC_DATACACHE))
      iUseDataCache=1;
 else iUseDataCache=0;
 if(iUseCaching==0) iUseDataCache=0;

 if(IsDlgButtonChecked(hW,IDC_NOWAIT))
      iNoWait=1;
 else iNoWait=0;

 if(IsDlgButtonChecked(hW,IDC_TRAYSTATE))
      iCheckTrayStatus=1;
 else iCheckTrayStatus=0;

 iMaxRetry=GetDlgItemInt(hW,IDC_RETRY,NULL,FALSE);
 if(iMaxRetry<1)  iMaxRetry=1;
 if(iMaxRetry>10) iMaxRetry=10;
 if(!IsDlgButtonChecked(hW,IDC_TRYAGAIN)) iMaxRetry=0;

 if(IsDlgButtonChecked(hW,IDC_SHOWREADERR))
      iShowReadErr=1;
 else iShowReadErr=0;

 hWC=GetDlgItem(hW,IDC_SPEED);
 iR=ComboBox_GetCurSel(hWC);

 iSpeedLimit=2;
 if(iR==1) iSpeedLimit=4;
 if(iR==2) iSpeedLimit=8;
 if(iR==3) iSpeedLimit=16;

 if(IsDlgButtonChecked(hW,IDC_USEPPF))
      iUsePPF=1;
 else iUsePPF=0;

 GetDlgItemText(hW,IDC_PPFFILE,szPPF,259);
 GetDlgItemText(hW,IDC_SUBFILE,szSUBF,259);

 WriteConfig();                                        // write registry

 EndDialog(hW,TRUE);
}

////////////////////////////////////////////////////////////////////////

void OnCDRCancel(HWND hW) 
{
 EndDialog(hW,FALSE);
}

////////////////////////////////////////////////////////////////////////

BOOL CALLBACK CDRDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_INITDIALOG:
     return OnInitCDRDialog(hW);

   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDC_SUBCHAN0:   if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {ShowSubFileStuff(hW);return TRUE;}
       case IDC_IMODE:      if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {OnIMode(hW);return TRUE;}
                            break;
       case IDC_CACHE:      if(HIWORD(wParam)==CBN_SELCHANGE) 
                             {OnCache(hW);return TRUE;}
                            break;
       case IDCANCEL:       OnCDRCancel(hW); return TRUE;
       case IDOK:           OnCDROK(hW);     return TRUE;
       case IDC_CHOOSEFILE: OnChooseFile(hW,0);return TRUE;
       case IDC_CHOOSESUBF: OnChooseFile(hW,1);return TRUE;
      }
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDCANCEL: EndDialog(hW,FALSE);return TRUE;
       case IDOK:     EndDialog(hW,FALSE);return TRUE;
      }
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////

