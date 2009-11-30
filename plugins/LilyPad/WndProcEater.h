#define EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES	1

/* Need this to let window be subclassed multiple times but still clean up nicely.
 */
enum ExtraWndProcResult {
	CONTINUE_BLISSFULLY,
	// Calls ReleaseExtraProc without messing up order.
	CONTINUE_BLISSFULLY_AND_RELEASE_PROC,
	USE_DEFAULT_WND_PROC,
	NO_WND_PROC
};

typedef ExtraWndProcResult (*ExtraWndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *out);

struct ExtraWndProcInfo {
	ExtraWndProc proc;
	DWORD flags;
};

class WndProcEater
{
public:
	HWND hWndEaten;
	WNDPROC eatenWndProc;
	ExtraWndProcInfo* extraProcs;
	int numExtraProcs;

	HANDLE hMutex;

public:
	WndProcEater();
	virtual ~WndProcEater() throw();

	bool SetWndHandle(HWND hWnd);
	void Eat(ExtraWndProc proc, DWORD flags);
	void ReleaseExtraProc(ExtraWndProc proc);
	void Release();
	
	LRESULT _OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern WndProcEater hWndGSProc;
extern WndProcEater hWndTopProc;
extern WndProcEater hWndButtonProc;
