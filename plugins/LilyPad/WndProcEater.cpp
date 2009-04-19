#include "WndProcEater.h"

static HWND hWndEaten = 0;
static WNDPROC eatenWndProc = 0;

struct ExtraWndProcInfo {
	ExtraWndProc proc;
	DWORD flags;
};

static ExtraWndProcInfo* extraProcs = 0;
static int numExtraProcs = 0;

void ReleaseExtraProc(ExtraWndProc proc) {
	// Probably isn't needed, but just in case...
	// Creating and destroying the mutex adds some inefficiency,
	// but this function is only called on emulation start and on focus/unfocus.
	HANDLE hMutex = CreateMutexA(0, 0, "LilyPad");
	if (hMutex) WaitForSingleObject(hMutex, 100);

	for (int i=0; i<numExtraProcs; i++) {
		if (extraProcs[i].proc == proc) {
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
	while (numExtraProcs) ReleaseExtraProc(extraProcs[0].proc);
}

extern int deviceUpdateQueued;
LRESULT CALLBACK OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ExtraWndProcResult res = CONTINUE_BLISSFULLY;
	LRESULT out = 0;
	// Here because want it for binding, even when no keyboard mode is selected.
	if (uMsg == WM_GETDLGCODE) {
		return DLGC_WANTALLKEYS | CallWindowProc(eatenWndProc, hWnd, uMsg, wParam, lParam);
	}

	for (int i=0; i<numExtraProcs; i++) {
		// Note:  Second bit of deviceUpdateQueued is only set when I receive a device change
		// notification, which is handled in the GS thread in one of the extraProcs, so this
		// is all I need to prevent bad things from happening while updating devices.  No mutex needed.
		if ((deviceUpdateQueued&2) && (extraProcs[i].flags & EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES)) continue;

		ExtraWndProcResult res2 = extraProcs[i].proc(hWnd, uMsg, wParam, lParam, &out);
		if (res2 != res) {
			if (res2 == CONTINUE_BLISSFULLY_AND_RELEASE_PROC) {
				ReleaseExtraProc(extraProcs[i].proc);
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


int EatWndProc(HWND hWnd, ExtraWndProc proc, DWORD flags) {
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
		extraProcs = (ExtraWndProcInfo*) realloc(extraProcs, sizeof(ExtraWndProcInfo)*(numExtraProcs+1));
		extraProcs[numExtraProcs].proc = proc;
		extraProcs[numExtraProcs].flags = flags;
		numExtraProcs++;
	}

	if (hMutex) {
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}

	return hWndEaten == hWnd;
}
