#include "Global.h"

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
int EatWndProc(HWND hWnd, ExtraWndProc proc);

void ReleaseExtraProc(ExtraWndProc proc);
void ReleaseEatenProc();

