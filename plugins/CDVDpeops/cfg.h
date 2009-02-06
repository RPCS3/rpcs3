/***************************************************************************
                            cfg.h  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Pete Bernert
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
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

void ReadConfig(void);
void WriteConfig(void);
void OnChooseFile(HWND hW,int iFType);
void EnumDrives(HWND hW);
void GetCDRInfos(HWND hW,int * iA, int * iT,int * iL);
void OnIMode(HWND hW);
void OnCache(HWND hW);
void ShowSubFileStuff(HWND hW);
BOOL OnInitCDRDialog(HWND hW);
void OnCDRAuto(HWND hW);
void ShowProgress(HWND hW,long lact,long lmin,long lmax);
void WriteDiffSub(FILE * xfile,int i,unsigned char * lpX,int iM,BOOL b3Min);
BOOL OnCreateSubFileEx(HWND hW,HWND hWX,BOOL b3Min);
BOOL OnCreateSubEx(HWND hW,HWND hWX,int iM,BOOL b3Min);
void StartSubReading(HWND hW,HWND hWX);
BOOL CALLBACK SubDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnCreateSub(HWND hW);
void OnCDROK(HWND hW);
void OnCDRCancel(HWND hW);
BOOL CALLBACK CDRDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
