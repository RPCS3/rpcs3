#ifndef _RECORD_H_
#define _RECORD_H_

#ifdef _WINDOWS
void RecordStart();
void RecordBuffer(unsigned char* pSound,long lBytes);
void RecordStop();
BOOL CALLBACK RecordDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

#endif

