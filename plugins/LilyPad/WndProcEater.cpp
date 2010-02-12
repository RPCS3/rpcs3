#include "Global.h"
#include "WndProcEater.h"

WndProcEater::WndProcEater()
{
	hWndEaten = 0;
	eatenWndProc = 0;

	extraProcs = 0;
	numExtraProcs = 0;

	hMutex = CreateMutex(0, 0, L"LilyPad");
}

WndProcEater::~WndProcEater() throw()
{
	if (hMutex) {
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}
}

void WndProcEater::ReleaseExtraProc(ExtraWndProc proc) {
	// Probably isn't needed, but just in case...
	if (hMutex) WaitForSingleObject(hMutex, 100);

	//printf( "(Lilypad) Regurgitating! -> 0x%x\n", proc );

	for (int i=0; i<numExtraProcs; i++) {
		if (extraProcs[i].proc == proc) {
			extraProcs[i] = extraProcs[--numExtraProcs];
			break;
		}
	}
	if (!numExtraProcs && eatenWndProc) {
		free(extraProcs);
		extraProcs = 0;
		// As numExtraProcs is 0, won't cause recursion if called from Release().
		Release();
	}
}

void WndProcEater::Release() {
	while (numExtraProcs) ReleaseExtraProc(extraProcs[0].proc);
	if (hWndEaten && IsWindow(hWndEaten)) {
		RemoveProp(hWndEaten, L"LilyHaxxor");
		SetWindowLongPtr(hWndEaten, GWLP_WNDPROC, (LONG_PTR)eatenWndProc);
		hWndEaten = 0;
		eatenWndProc = 0;
	}
}

LRESULT WndProcEater::_OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( hWnd != hWndEaten )
		fprintf( stderr, "Totally mismatched window handles on OverrideWndProc!\n" );

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
		// if ((deviceUpdateQueued&2) && (extraProcs[i].flags & EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES)) continue;

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
			Release();
		}
		if (res == CONTINUE_BLISSFULLY)
			out = CallWindowProc(eatenWndProc, hWnd, uMsg, wParam, lParam);
		else if (res == USE_DEFAULT_WND_PROC) 
			out = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return out;
}

static LRESULT CALLBACK OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WndProcEater* obj = (WndProcEater*)GetProp(hWnd, L"LilyHaxxor");
	return (obj == NULL) ?
		DefWindowProc(hWnd, uMsg, wParam, lParam) :
		obj->_OverrideWndProc( hWnd, uMsg, wParam, lParam );
}

bool WndProcEater::SetWndHandle(HWND hWnd)
{
	if(hWnd == hWndEaten) return true;

	//printf( "(Lilypad) (Re)-Setting window handle! -> this=0x%08x, hWnd=0x%08x\n", this, hWnd );
	
	Release();
	SetProp(hWnd, L"LilyHaxxor", (HANDLE)this);

	eatenWndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)OverrideWndProc);
	hWndEaten = (eatenWndProc) ? hWnd : 0;

	return !!hWndEaten;
}

void WndProcEater::Eat(ExtraWndProc proc, DWORD flags) {

	// check if Subclassing failed to init during SetWndHandle
	if (!hWndEaten) return;

	// Probably isn't needed, but just in case...
	if (hMutex) WaitForSingleObject(hMutex, 100);

	//printf( "(Lilypad) EatingWndProc! -> 0x%x\n", proc );

	extraProcs = (ExtraWndProcInfo*) realloc(extraProcs, sizeof(ExtraWndProcInfo)*(numExtraProcs+1));
	extraProcs[numExtraProcs].proc = proc;
	extraProcs[numExtraProcs].flags = flags;
	numExtraProcs++;
}
