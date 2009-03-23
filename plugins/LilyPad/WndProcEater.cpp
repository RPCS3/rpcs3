#include "WndProcEater.h"

static HWND hWndEaten = 0;
static WNDPROC eatenWndProc = 0;
static ExtraWndProc* extraProcs = 0;
static int numExtraProcs = 0;

void ReleaseExtraProc(ExtraWndProc proc) {
	// Probably isn't needed, but just in case...
	// Creating and destroying the mutex adds some inefficiency,
	// but this function is only called on emulation start and on focus/unfocus.
	HANDLE hMutex = CreateMutexA(0, 0, "LilyPad");
	if (hMutex) WaitForSingleObject(hMutex, 100);

	for (int i=0; i<numExtraProcs; i++) {
		if (extraProcs[i] == proc) {
			extraProcs[i] = extraProcs[--numExtraProcs];
			break;
		}
	}
	if (!numExtraProcs && eatenWndProc) {
		free(extraProcs);
		extraProcs = 0;
		if (hWndEaten && IsWindow(hWndEaten)) {
			SetWindowLongPtr(hWndEaten, GWLP_WNDPROC, (LONG_PTR)eatenWndProc);
		}
		hWndEaten = 0;
		eatenWndProc = 0;
	}

	if (hMutex) {
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}
}

void ReleaseEatenProc() {
	while (numExtraProcs) ReleaseExtraProc(extraProcs[0]);
}

LRESULT CALLBACK OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ExtraWndProcResult res = CONTINUE_BLISSFULLY;
	LRESULT out = 0;
	// Here because want it for binding, even when no keyboard mode is selected.
	if (uMsg == WM_GETDLGCODE) {
		return DLGC_WANTALLKEYS | CallWindowProc(eatenWndProc, hWnd, uMsg, wParam, lParam);
	}
	for (int i=0; i<numExtraProcs; i++) {
		ExtraWndProcResult res2 = extraProcs[i](hWnd, uMsg, wParam, lParam, &out);
		if (res2 != res) {
			if (res2 == CONTINUE_BLISSFULLY_AND_RELEASE_PROC) {
				ReleaseExtraProc(extraProcs[i]);
				i--;
			}
			else if (res2 > res) res = res2;
		}
	}
	if (res != NO_WND_PROC) {
		if (out == WM_DESTROY) {
			ReleaseEatenProc();
		}
		if (res == CONTINUE_BLISSFULLY)
			out = CallWindowProc(eatenWndProc, hWnd, uMsg, wParam, lParam);
		else if (res == USE_DEFAULT_WND_PROC) 
			out = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return out;
}


int EatWndProc(HWND hWnd, ExtraWndProc proc) {
	// Probably isn't needed, but just in case...
	// Creating and destroying the mutex adds some inefficiency,
	// but this function is only called on emulation start and on focus/unfocus.
	HANDLE hMutex = CreateMutexA(0, 0, "LilyPad");
	if (hMutex) WaitForSingleObject(hMutex, 100);

	if (hWnd != hWndEaten) {
		ReleaseEatenProc();
		eatenWndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OverrideWndProc);
		// ???
		if (eatenWndProc)
			hWndEaten = hWnd;
	}
	if (hWndEaten == hWnd) {
		extraProcs = (ExtraWndProc*) realloc(extraProcs, sizeof(ExtraWndProc)*(numExtraProcs+1));
		extraProcs[numExtraProcs++] = proc;
	}

	if (hMutex) {
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}

	return hWndEaten == hWnd;
}
