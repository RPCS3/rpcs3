#include "global.h"
#include "PS2Edefs.h"
#include "Config.h"
#include <math.h>
#include "InputManager.h"
#include "DeviceEnumerator.h"
#include "WndProcEater.h"
#include "KeyboardQueue.h"
#include <Dbt.h>

// Used so don't read input and cleanup input devices at the same time.
CRITICAL_SECTION readInputCriticalSection;

HINSTANCE hInst;
HWND hWnd;

u8 miceEnabled;

int openCount = 0;

HMODULE user32 = 0;

int activeWindow = 0;


int IsWindowMaximized (HWND hWnd) {
	RECT rect;
	if (GetWindowRect(hWnd, &rect)) {
		POINT p;
		p.x = rect.left;
		p.y = rect.top;
		MONITORINFO info;
		memset(&info, 0, sizeof(info));
		info.cbSize = sizeof(info);
		HMONITOR hMonitor;
		if ((hMonitor = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY)) &&
			GetMonitorInfo(hMonitor, &info) &&
			memcmp(&info.rcMonitor, &rect, sizeof(rect)) == 0) {
				return 1;
		}
	}
	return 0;
}

#include <stdio.h>
int bufSize = 0;
static unsigned char outBuf[50];
static unsigned char inBuf[50];

void DEBUG_NEW_SET() {
	if (config.debug) {
		HANDLE hFile = CreateFileA("logs\\padLog.txt", GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
		if (hFile != INVALID_HANDLE_VALUE) {
			int i;
			char temp[1500];
			char *end = temp;
			for (i=0; i<bufSize; i++) {
				sprintf(end, "%02X ", inBuf[i]);
				end = strchr(end, 0);
			}
			end++[0] = '\n';
			for (i=0; i<bufSize; i++) {
				sprintf(end, "%02X ", outBuf[i]);
				end = strchr(end, 0);
			}
			end++[0] = '\n';
			end++[0] = '\n';
			DWORD junk;
			WriteFile(hFile, temp, end-temp, &junk, 0);
			bufSize = 0;
			CloseHandle(hFile);;
		}
	}
}

inline void DEBUG_IN(unsigned char c) {
	if (bufSize < 40) inBuf[bufSize] = c;
}
inline void DEBUG_OUT(unsigned char c) {
	if (bufSize < 40) outBuf[bufSize++] = c;
}

#define MODE_DIGITAL 0x41
#define MODE_ANALOG 0x73
#define MODE_FULL_ANALOG 0x79

struct Stick {
	int horiz;
	int vert;
};

struct ButtonSum {
	int buttons[12];
	Stick sticks[3];
};

class Pad {
public:
	ButtonSum sum, lockedSum;

	Stick rStick, lStick;

	int lockedState;
	u8 vibrate[8];

	u8 umask[2];

	u8 vibrateI[2];
	u8 vibrateVal[2];
	// Digital / Analog / Full Analog (aka DS2 Native)
	u8 mode;
	u8 modeLock;
	// In config mode
	u8 config;
	// Used to keep track of which pads I'm running.
	// Note that initialized pads *can* be disabled.
	// I keep track of state of non-disabled non-initialized
	// pads, but should never be asked for their state.
	u8 initialized;
} pads[2];

u8 Cap (int i) {
	// If negative, zero out i.
	i &= ~(i>>(sizeof(i)*8-1));
	// if i-255 is negative, return i.  Else return 255.  Slight overkill.
	return (u8) (255 + ((i-255) & ((i-255) >> (sizeof(int)*8-1))));
}


// Just like RefreshEnabledDevices(), but takes into account
// mouse and focus state and which devices have bindings for
// enabled pads.  Also releases keyboards if window is not focused.
// And releases games if not focused and config.background is not set.
void UpdateEnabledDevices(int updateList = 0) {
	// Enable all devices I might want.  Can ignore the rest.
	RefreshEnabledDevices(updateList);
	// Figure out which pads I'm getting input for.
	int padsEnabled[2] = {
		pads[0].initialized && !config.disablePad[0],
		pads[1].initialized && !config.disablePad[1]
	};
	for (int i=0; i<dm->numDevices; i++) {
		Device *dev = dm->devices[i];

		if (!dev->enabled) continue;
		if (!dev->attached) {
			dm->DisableDevice(i);
			continue;
		}

		// Disable ignore keyboard if don't have focus or there are no keys to ignore.
		if (dev->api == IGNORE_KEYBOARD) {
			if (config.keyboardApi == NO_API || !activeWindow || !dev->pads[0].numBindings) {
				dm->DisableDevice(i);
			}
			continue;
		}
		// Keep for PCSX2 keyboard shotcuts, unless unfocused.
		if (dev->type == KEYBOARD) {
			if (!activeWindow) dm->DisableDevice(i);
		}
		// Keep for cursor hiding consistency.
		else if (dev->type == MOUSE) {
			if (!miceEnabled || !activeWindow) dm->DisableDevice(i);
		}
		else if (!activeWindow && !config.background) dm->DisableDevice(i);
		else {
			int needDevice = 0;
			for (int pad=0; pad<2; pad++) {
				needDevice |= (padsEnabled[pad] && dev->pads[pad].numBindings+dev->pads[pad].numFFBindings);
			}
			if (!needDevice)
				dm->DisableDevice(i);
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void* lpvReserved) {
	hInst = hInstance;
	if (fdwReason == DLL_PROCESS_ATTACH) {
		InitializeCriticalSection(&readInputCriticalSection);
		//safeShutdown = 0;
		user32 = GetModuleHandle(L"user32.dll");
		if (user32) {
			pRegisterRawInputDevices = (_RegisterRawInputDevices) GetProcAddress(user32, "RegisterRawInputDevices");
			pGetRawInputDeviceInfo = (_GetRawInputDeviceInfo) GetProcAddress(user32, "GetRawInputDeviceInfoW");
			pGetRawInputData = (_GetRawInputData) GetProcAddress(user32, "GetRawInputData");
			pGetRawInputDeviceList = (_GetRawInputDeviceList) GetProcAddress(user32, "GetRawInputDeviceList");
			if (!pRegisterRawInputDevices ||
				!pGetRawInputDeviceInfo ||
				!pGetRawInputData ||
				!pGetRawInputDeviceList) {

				pRegisterRawInputDevices = 0;
				pGetRawInputDeviceInfo = 0;
				pGetRawInputData = 0;
				pGetRawInputDeviceList = 0;
			}
		}
		DisableThreadLibraryCalls(hInstance);
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		DeleteCriticalSection(&readInputCriticalSection);
		//safeShutdown = 1;
		//Sleep(20);
		activeWindow = 0;
		while (openCount)
			PADclose();
		PADshutdown();
		//CleanupInput();
	}
	return 1;
}

BOOL WINAPI MyDllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved) {
	return DllMain((HINSTANCE) hDllHandle, dwReason, lpReserved);
}

void AddForce(ButtonSum *sum, u8 cmd, int delta = 255) {
	if (!delta) return;
	if (cmd<0x14) {
		sum->buttons[cmd-0x10] += delta;
	}
	else if (cmd < 0x18) {
		if (cmd == 0x14) {
			sum->sticks[0].vert -= delta;
		}
		else if (cmd == 0x15) {
			sum->sticks[0].horiz += delta;
		}
		else if (cmd == 0x16) {
			sum->sticks[0].vert += delta;
		}
		else if (cmd == 0x17) {
			sum->sticks[0].horiz -= delta;
		}
	}
	else if (cmd < 0x20) {
		sum->buttons[cmd-0x10-4] += delta;
	}
	else if (cmd < 0x24) {
		if (cmd == 32) {
			sum->sticks[2].vert -= delta;
		}
		else if (cmd == 33) {
			sum->sticks[2].horiz += delta;
		}
		else if (cmd == 34) {
			sum->sticks[2].vert += delta;
		}
		else if (cmd == 35) {
			sum->sticks[2].horiz -= delta;
		}
	}
	else if (cmd < 0x28) {
		if (cmd == 36) {
			sum->sticks[1].vert -= delta;
		}
		else if (cmd == 37) {
			sum->sticks[1].horiz += delta;
		}
		else if (cmd == 38) {
			sum->sticks[1].vert += delta;
		}
		else if (cmd == 39) {
			sum->sticks[1].horiz -= delta;
		}
	}
}

void ProcessButtonBinding(Binding *b, ButtonSum *sum, unsigned int value) {
	int sensitivity = b->sensitivity;
	if (sensitivity < 0) {
		sensitivity = -sensitivity;
		value = (1<<16)-value;
	}
	if (value) {
		AddForce(sum, b->command, (int)((((sensitivity*(255*(__int64)value)) + BASE_SENSITIVITY/2)/BASE_SENSITIVITY + FULLY_DOWN/2)/FULLY_DOWN));
	}
}

void CapSum(ButtonSum *sum) {
	int i;
	for (i=0; i<3; i++) {
		int a1 = abs(sum->sticks[i].horiz);
		int a2 = abs(sum->sticks[i].vert);
		if (a1 < a2) a1 = a2;
		if (a1 > 255) {
			sum->sticks[i].horiz = sum->sticks[i].horiz * 255 / a1;
			sum->sticks[i].vert = sum->sticks[i].vert * 255 / a1;
		}
	}
	for (i=0; i<12; i++) {
		sum->buttons[i] = Cap(sum->buttons[i]);
	}
}

// Counters for when to next update pad state.
// Read all devices at once, so don't need to read them again
// for pad 2 immediately after pad 1.  3rd counter is for
// when neither pad is being read, so still respond to
// key press info requests.
int summed[3] = {0, 0, 0};

int lockStateChanged[2] = {0,0};
#define LOCK_DIRECTION 2
#define LOCK_BUTTONS 4
#define LOCK_BOTH 1

extern HWND hWndStealing;

void Update(int pad);

void CALLBACK PADupdate(int pad) {
	if (config.GSThreadUpdates) Update(pad);
}

void Update(int pad) {
	if ((unsigned int)pad > 2 /* || safeShutdown//*/) return;
	if (summed[pad]) {
		summed[pad]--;
		return;
	}
	int i;
	ButtonSum s[2];
	s[0] = pads[0].lockedSum;
	s[1] = pads[1].lockedSum;
	InitInfo info = {
		0, hWnd, hWnd, 0
	};

	if (!config.GSThreadUpdates) {
		EnterCriticalSection(&readInputCriticalSection);
	}
	dm->Update(&info);
	static int turbo = 0;
	turbo++;
	for (i=0; i<dm->numDevices; i++) {
		Device *dev = dm->devices[i];
		// Skip both disabled devices and inactive enabled devices.
		// Shouldn't be any of the latter, in general, but just in case...
		if (!dev->virtualControlState) continue;
		for (int pad=0; pad<2; pad++) {
			if (config.disablePad[pad]) continue;
			for (int j=0; j<dev->pads[pad].numBindings; j++) {
				Binding *b = dev->pads[pad].bindings+j;
				int cmd = b->command;
				int state = dev->virtualControlState[b->controlIndex];
				if (!(turbo & b->turbo)) {
					if (cmd > 0x0F && cmd != 0x28) {
						ProcessButtonBinding(b, s+pad, state);
					}
					else if ((state>>15) && !(dev->oldVirtualControlState[b->controlIndex]>>15)) {
						if (cmd == 0x0F) {
							miceEnabled = !miceEnabled;
							UpdateEnabledDevices();
						}
						else if (cmd == 0x0C) {
							lockStateChanged[pad] |= LOCK_BUTTONS;
						}
						else if (cmd == 0x0E) {
							lockStateChanged[pad] |= LOCK_DIRECTION;
						}
						else if (cmd == 0x0D) {
							lockStateChanged[pad] |= LOCK_BOTH;
						}
						else if (cmd == 0x28) {
							if (!pads[pad].modeLock) {
								if (pads[pad].mode != MODE_DIGITAL)
									pads[pad].mode = MODE_DIGITAL;
								else
									pads[pad].mode = MODE_ANALOG;
							}
						}
					}
				}
			}
		}
	}
	dm->PostRead();

	if (!config.GSThreadUpdates) {
		LeaveCriticalSection(&readInputCriticalSection);
	}

	for (int currentPad = 0; currentPad<2; currentPad++) {
		if (config.guitar[currentPad]) {
			if (!config.GH2) {
				s[currentPad].sticks[1].vert = -s[currentPad].sticks[1].vert;
			}
			// GH2 hack.
			else if (config.GH2) {
				const unsigned int oldIdList[5] = {ID_R2, ID_CIRCLE, ID_TRIANGLE, ID_CROSS, ID_SQUARE};
				const unsigned int idList[5] = {ID_L2, ID_L1, ID_R1, ID_R2, ID_CROSS};
				int values[5];
				int i;
				for (i=0; i<5; i++) {
					int id = oldIdList[i] - 0x1104;
					values[i] = s[currentPad].buttons[id];
					s[currentPad].buttons[id] = 0;
				}
				s[currentPad].buttons[ID_TRIANGLE-0x1104] = values[1];
				for (i=0; i<5; i++) {
					int id = idList[i] - 0x1104;
					s[currentPad].buttons[id] = values[i];
				}
				if (abs(s[currentPad].sticks[0].vert) <= 48) {
					for (int i=0; i<5; i++) {
						unsigned int id = idList[i] - 0x1104;
						if (pads[currentPad].sum.buttons[id] < s[currentPad].buttons[id]) {
							s[currentPad].buttons[id] = pads[currentPad].sum.buttons[id];
						}
					}
				}
				else if (abs(pads[currentPad].sum.sticks[0].vert) <= 48) {
					for (int i=0; i<5; i++) {
						unsigned int id = idList[i] - 0x1104;
						if (pads[currentPad].sum.buttons[id]) {
							s[currentPad].buttons[id] = 0;
						}
					}
				}
			}
		}

		if (pads[currentPad].mode == 0x41) {
		//if (activeConfigs[currentPad] && pads[currentPad].mode == 0x41 && activeConfigs[currentPad]->analogDigitals) {
			s[currentPad].sticks[0].horiz +=
				s[currentPad].sticks[1].horiz +
				s[currentPad].sticks[2].horiz;
			s[currentPad].sticks[0].vert +=
				s[currentPad].sticks[1].vert +
				s[currentPad].sticks[2].vert;
		}

		CapSum(&s[currentPad]);
		if (lockStateChanged[currentPad]) {
			if (lockStateChanged[currentPad] & LOCK_BOTH) {
				if (pads[currentPad].lockedState != (LOCK_DIRECTION | LOCK_BUTTONS))
					// enable the one that's not enabled.
					lockStateChanged[currentPad] ^= pads[currentPad].lockedState^(LOCK_DIRECTION | LOCK_BUTTONS);
				else
					// Disable both
					lockStateChanged[currentPad] ^= LOCK_DIRECTION | LOCK_BUTTONS;
			}
			if (lockStateChanged[currentPad] & LOCK_DIRECTION) {
				if (pads[currentPad].lockedState & LOCK_DIRECTION) {
					memset(pads[currentPad].lockedSum.sticks, 0, sizeof(pads[currentPad].lockedSum.sticks));
				}
				else {
					memcpy(pads[currentPad].lockedSum.sticks, s[currentPad].sticks, sizeof(pads[currentPad].lockedSum.sticks));
				}
				pads[currentPad].lockedState ^= LOCK_DIRECTION;
			}
			if (lockStateChanged[currentPad] & LOCK_BUTTONS) {
				if (pads[currentPad].lockedState & LOCK_BUTTONS) {
					memset(pads[currentPad].lockedSum.buttons, 0, sizeof(pads[currentPad].lockedSum.buttons));
				}
				else {
					memcpy(pads[currentPad].lockedSum.buttons, s[currentPad].buttons, sizeof(pads[currentPad].lockedSum.buttons));
				}
				pads[currentPad].lockedState ^= LOCK_BUTTONS;
			}
			for (i=0; i<sizeof(pads[0].lockedSum)/4; i++) {
				if (((int*)&pads[0].lockedSum)[i]) break;
			}
			if (i==sizeof(pads[0].lockedSum)/4) {
				pads[currentPad].lockedState = 0;
			}
		}
		lockStateChanged[currentPad] = 0;
	}
	pads[0].sum = s[0];
	pads[1].sum = s[1];
	if (config.disablePad[0]) {
		memset(&pads[0].sum, 0, sizeof(pads[0].sum));
	}
	if (config.disablePad[1]) {
		memset(&pads[1].sum, 0, sizeof(pads[1].sum));
	}
	summed[0] = 1;
	summed[1] = 1;
	summed[2] = 2;
	summed[pad]--;
}

inline void SetVibrate(Pad *pad, int motor, u8 val) {
	if (val | pad->vibrateVal[motor]) {
		dm->SetEffect(pad - pads, motor, val);
		pad->vibrateVal[motor] = val;
	}
}

u32 CALLBACK PS2EgetLibType(void) {
	return PS2E_LT_PAD;
}

#define VERSION ((0<<8) | 9 | (9<<24))

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	if (type == PS2E_LT_PAD)
		return (PS2E_PAD_VERSION<<16) | VERSION;
	return 0;
}

char* CALLBACK PS2EgetLibName(void) {
#ifdef _DEBUG
	return "LilyPad Debug";
#else
	return "LilyPad";
#endif
}

//void CALLBACK PADgsDriverInfo(GSdriverInfo *info) {
//	info=info;
//}

void CALLBACK PADshutdown() {
	pads[0].initialized = 0;
	pads[1].initialized = 0;
	UnloadConfigs();
}

#ifdef _DEBUG
#include "crtdbg.h"
#endif


inline void StopVibrate() {
	for (int i=0; i<4; i++) {
		SetVibrate(&pads[i/2], i&1, 0);
	}
}

inline void ResetVibrate(Pad *pad) {
	SetVibrate(pad, 0, 0);
	SetVibrate(pad, 1, 0);
	((int*)(pad->vibrate))[0] = 0xFFFFFF5A;
	((int*)(pad->vibrate))[1] = 0xFFFFFFFF;
}



s32 CALLBACK PADinit(u32 flags) {
	// Note:  Won't load settings if already loaded.
	if (LoadSettings() < 0) {
		return -1;
	}
	int pad = (flags & 3);
	if (pad == 3) {
		if (PADinit(1)) return -1;
		return PADinit(2);
	}
	#ifdef _DEBUG
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( tmpFlag );
	#endif
	pad --;

	memset(&pads[pad], 0, sizeof(pads[0]));
	pads[pad].mode = MODE_DIGITAL;
	pads[pad].umask[0] = pads[pad].umask[1] = 0xFF;
	ResetVibrate(pads+pad);
	if (config.AutoAnalog[pad]) {
		pads[pad].mode = MODE_ANALOG;
	}

	pads[pad].initialized = 1;

	return 0;
}



// Note to self:  Has to be a define for the sizeof() to work right.
// Note to self 2: All are the same size, anyways, except for full DS2 response and digital mode response.
#define SET_RESULT(a) { \
	memcpy(query.response+2, a, sizeof(a)); \
	query.numBytes = 2+sizeof(a); \
}

#define SET_FINAL_RESULT(a) {			  \
	memcpy(query.response+2, a, sizeof(a));\
	query.numBytes = 2+sizeof(a);		  \
	query.queryDone = 1;			  \
}

static const u8 ConfigExit[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static const u8 ConfigExit[7] = {0x5A, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const u8 noclue[7] =			{0x5A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5A};
static u8 queryMaskMode[7] =    {0x5A, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x5A};
//static const u8 DSNonNativeMode[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 setMode[7] =         {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// DS2
static const u8 queryModelDS2[7] =      {0x5A, 0x03, 0x02, 0x00, 0x02, 0x01, 0x00};
// DS1
static const u8 queryModelDS1[7] =      {0x5A, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};

static const u8 queryAct[2][7] =    {{0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A},
									 {0x5A, 0x00, 0x00, 0x01, 0x01, 0x01, 0x14}};

static const u8 queryComb[7] =       {0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};

static const u8 queryMode[7] =		{0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


static const u8 setNativeMode[7] =  {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A};

struct QueryInfo {
	u8 pad;
	u8 lastByte;
	u8 currentCommand;
	u8 numBytes;
	u8 queryDone;
	u8 response[22];
} query = {0,0,0,0, 0,0xFF, 0xF3};

ExtraWndProcResult HackWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	switch (uMsg) {
		case WM_DEVICECHANGE:
			if (wParam == DBT_DEVNODES_CHANGED) {
				// Need to do this when not reading input from gs thread.
				// Checking for that case not worth the effort.
				EnterCriticalSection(&readInputCriticalSection);
				RefreshEnabledDevices(1);
				LeaveCriticalSection(&readInputCriticalSection);
			}
			break;
		case WM_ACTIVATEAPP:
			// Need to do this when not reading input from gs thread.
			// Checking for that case not worth the effort.
			EnterCriticalSection(&readInputCriticalSection);
			if (!wParam) {
				activeWindow = 0;
				UpdateEnabledDevices();
			}
			else {
				activeWindow = 1;
				UpdateEnabledDevices();
			}
			LeaveCriticalSection(&readInputCriticalSection);
			break;
		case WM_CLOSE:
			if (config.closeHacks & 1) {
				QueueKeyEvent(VK_ESCAPE, KEYPRESS);
				return NO_WND_PROC;
			}
			else if (config.closeHacks & 2) {
				ExitProcess(0);
				return NO_WND_PROC;
			}
			break;
		case WM_SYSCOMMAND:
			if ((wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) && config.disableScreenSaver)
				return NO_WND_PROC;
			break;
		case WM_DESTROY:
			QueueKeyEvent(VK_ESCAPE, KEYPRESS);
			break;
		default:
			break;
	}
	return CONTINUE_BLISSFULLY;
}

// All that's needed to force hiding the cursor in the proper thread.
// Could have a special case elsewhere, but this make sure it's called
// only once, rather than repeatedly.
ExtraWndProcResult HideCursorProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	ShowCursor(0);
	return CONTINUE_BLISSFULLY_AND_RELEASE_PROC;
}

s32 CALLBACK PADopen(void *pDsp) {
	if (openCount++) return 0;

	// Not really needed, shouldn't do anything.
	if (LoadSettings()) return -1;
	miceEnabled = !config.mouseUnfocus;
	if (!hWnd) {
		if (IsWindow((HWND)pDsp)) {
			hWnd = (HWND) pDsp;
		}
		else if (pDsp && !IsBadReadPtr(pDsp, 4) && IsWindow(*(HWND*) pDsp)) {
			hWnd = *(HWND*) pDsp;
		}
		else {
			openCount = 0;
			return -1;
		}
		while (GetWindowLong (hWnd, GWL_STYLE) & WS_CHILD)
			hWnd = GetParent (hWnd);
		// Implements most hacks, as well as enabling/disabling mouse
		// capture when focus changes.
		if (!EatWndProc(hWnd, HackWndProc)) {
			openCount = 0;
			return -1;
		}
		if (config.forceHide) {
			EatWndProc(hWnd, HideCursorProc);
		}
	}

	memset(&pads[0].sum, 0, sizeof(pads[0].sum));
	memset(&pads[0].lockedSum, 0, sizeof(pads[0].lockedSum));
	pads[0].lockedState = 0;
	memset(&pads[1].sum, 0, sizeof(pads[0].sum));
	memset(&pads[1].lockedSum, 0, sizeof(pads[0].lockedSum));
	pads[1].lockedState = 0;

	query.lastByte = 1;
	query.numBytes = 0;

	if (GetAncestor(hWnd, GA_ROOT) == GetAncestor(GetForegroundWindow(), GA_ROOT))
		activeWindow = 1;
	UpdateEnabledDevices();
	return 0;
}

void CALLBACK PADclose() {
	if (openCount && !--openCount) {
		dm->ReleaseInput();
		ReleaseEatenProc();
		hWnd = 0;
		ClearKeyQueue();
	}
}

u8 CALLBACK PADstartPoll(int pad) {
	DEBUG_NEW_SET();
	pad--;
	if (pad == !(!pad)) {
		query.queryDone = 0;
		query.pad = pad;
		query.numBytes = 2;
		query.lastByte = 0;
		DEBUG_IN(pad);
		DEBUG_OUT(0xFF);
		return 0xFF;
	}
	else {
		query.queryDone = 1;
		query.numBytes = 0;
		query.lastByte = 1;
		DEBUG_IN(pad);
		DEBUG_OUT(0);
		return 0;
	}
}

u8 CALLBACK PADpoll(u8 value) {
	DEBUG_IN(value);
	if (query.lastByte+1 >= query.numBytes) {
		DEBUG_OUT(0);
		return 0;
	}
	if (query.lastByte && query.queryDone) {
		DEBUG_OUT(query.response[1+query.lastByte]);
		return query.response[++query.lastByte];
	}
	int i;
	Pad *pad = &pads[query.pad];
	if (query.lastByte == 0) {
		query.lastByte++;
		query.currentCommand = value;
		switch(value) {
		// CONFIG_MODE
		case 0x43:
			if (pad->config) {
				// In config mode.  Might not actually be leaving it.
				SET_RESULT(ConfigExit);
				DEBUG_OUT(0xF3);
				return 0xF3;
			}
		// READ_DATA_AND_VIBRATE
		case 0x42:
			query.response[2] = 0x5A;
			{
				if (!config.GSThreadUpdates) {
					Update(pad != pads);
				}
				ButtonSum *sum = &pad->sum;

				u8 b1 = 0xFF, b2 = 0xFF;
				for (i = 0; i<4; i++) {
					b1 -= (sum->buttons[i]>=128) << i;
				}
				for (i = 0; i<8; i++) {
					b2 -= (sum->buttons[i+4]>=128) << i;
				}
				if (config.guitar[query.pad] && !config.GH2) {
					sum->sticks[0].horiz = -256;
					// Not sure about this.  Forces wammy to be from 0 to 0x7F.
					// if (sum->sticks[2].vert > 0) sum->sticks[2].vert = 0;
				}
				if (sum->sticks[0].vert) {
					sum=sum;
				}
				b1 -= ((sum->sticks[0].vert<=-128) << 4);
				b1 -= ((sum->sticks[0].horiz>=128) << 5);
				b1 -= ((sum->sticks[0].vert>=128) << 6);
				b1 -= ((sum->sticks[0].horiz<=-128) << 7);
				query.response[3] = b1;
				query.response[4] = b2;

				if (pad->mode == MODE_DIGITAL) {
					query.numBytes = 5;
				}
				else {
					query.response[5] = Cap((sum->sticks[1].horiz+255)/2);
					query.response[6] = Cap((sum->sticks[1].vert+255)/2);
					query.response[7] = Cap((sum->sticks[2].horiz+255)/2);
					query.response[8] = Cap((sum->sticks[2].vert+255)/2);
					if (pad->mode == MODE_ANALOG) {
						query.numBytes = 9;
					}
					else {
						// Good idea?  No clue.
						//query.response[3] &= pad->mask[0];
						//query.response[4] &= pad->mask[1];
						query.response[9] = Cap(sum->sticks[0].horiz);
						query.response[10] = Cap(-sum->sticks[0].horiz);
						query.response[11] = Cap(-sum->sticks[0].vert);
						query.response[12] = Cap(sum->sticks[0].vert);

						// No need to cap these.
						query.response[13] = (unsigned char) sum->buttons[8];
						query.response[14] = (unsigned char) sum->buttons[9];
						query.response[15] = (unsigned char) sum->buttons[10];
						query.response[16] = (unsigned char) sum->buttons[11];
						query.response[17] = (unsigned char) sum->buttons[6];
						query.response[18] = (unsigned char) sum->buttons[7];
						query.response[19] = (unsigned char) sum->buttons[4];
						query.response[20] = (unsigned char) sum->buttons[5];
						query.numBytes = 21;
					}
				}
			}

			query.lastByte=1;
			//if (!pad->config) {
				DEBUG_OUT(pad->mode);
				return pad->mode;
			/*}
			else {
				DEBUG_OUT(0xF3);
				return 0xF3;
			}//*/
		// SET_VREF_PARAM
		case 0x40:
			SET_FINAL_RESULT(noclue);
			break;
		// QUERY_DS2_ANALOG_MODE
		case 0x41:
			//if (pad->mode == MODE_FULL_ANALOG) {
			if (pad->mode == MODE_DIGITAL) {
				queryMaskMode[1] = queryMaskMode[2] = queryMaskMode[3] = 0;
				queryMaskMode[6] = 0x00;
			}
			else {
				queryMaskMode[1] = pad->umask[0];
				queryMaskMode[2] = pad->umask[1];
				queryMaskMode[3] = 0x03;
				// Not entirely sure about this.
				//queryMaskMode[3] = 0x01 | (pad->mode == MODE_FULL_ANALOG)*2;
				queryMaskMode[6] = 0x5A;
			}
			SET_FINAL_RESULT(queryMaskMode);
			/*}
			else {
				SET_FINAL_RESULT(DSNonNativeMode);
			}//*/
			break;
		// SET_MODE_AND_LOCK
		case 0x44:
			SET_RESULT(setMode);
			ResetVibrate(pad);
			break;
		// QUERY_MODEL_AND_MODE
		case 0x45:
			//queryModel[1] = 3 - 2*activeConfigs[query.pad]->guitar;
			if (!config.guitar[query.pad] || config.GH2) SET_FINAL_RESULT(queryModelDS2)
			else SET_FINAL_RESULT(queryModelDS1);
			query.response[5] = pad->mode != MODE_DIGITAL;
			break;
		// QUERY_ACT
		case 0x46:
			SET_RESULT(queryAct[0]);
			break;
		// QUERY_COMB
		case 0x47:
			SET_FINAL_RESULT(queryComb);
			break;
		// QUERY_MODE
		case 0x4C:
			SET_RESULT(queryMode);
			break;
		// VIBRATION_TOGGLE
		case 0x4D:
			memcpy(query.response+2, pad->vibrate, 7);
			query.numBytes = 9;
			ResetVibrate(pad);
			break;
		// SET_DS2_NATIVE_MODE
		case 0x4F:
			SET_RESULT(setNativeMode);
			break;
		default:
			query.numBytes = 0;
			query.queryDone = 1;
			break;
		}
		DEBUG_OUT(0xF3);
		return 0xF3;
	}
	else {
		query.lastByte++;
		switch (query.currentCommand) {
			// READ_DATA_AND_VIBRATE
			case 0x42:
				if (query.lastByte == pad->vibrateI[0]) {
					SetVibrate(pad, 1, 255*(0!=value));
				}
				else if (query.lastByte == pad->vibrateI[1]) {
					SetVibrate(pad, 0, value);
				}
				break;
			// CONFIG_MODE
			case 0x43:
				if (query.lastByte == 3) {
					// If leaving config mode, return all 0's.
					/*if (value == 0) {
						SET_RESULT(setMode);
					}//*/
					query.queryDone = 1;
					pad->config = value;
				}
				break;
			// SET_MODE_AND_LOCK
			case 0x44:
				if (query.lastByte == 3 && value < 2) {
					static const u8 modes[2] = {MODE_DIGITAL, MODE_ANALOG};
					pad->mode = modes[value];
				}
				else if (query.lastByte == 4) {
					if (value == 3) {
						pad->modeLock = 3;
					}
					else {
						pad->modeLock = 0;
						if (pad->mode == MODE_DIGITAL && config.AutoAnalog[query.pad]) {
							pad->mode = MODE_ANALOG;
						}
					}
					query.queryDone = 1;
				}
				break;
			// QUERY_ACT
			case 0x46:
				if (query.lastByte == 3) {
					if (value<2) SET_RESULT(queryAct[value])
					// bunch of 0's
					// else SET_RESULT(setMode);
					query.queryDone = 1;
				}
				break;
			// QUERY_MODE
			case 0x4C:
				if (query.lastByte == 3 && value<2) {
					query.response[6] = 4+value*3;
					query.queryDone = 1;
				}
				// bunch of 0's
				//else data = setMode;
				break;
			// VIBRATION_TOGGLE
			case 0x4D:
				{
					if (query.lastByte>=3) {
						if (value == 0) {
							pad->vibrateI[0] = (u8)query.lastByte;
						}
						else if (value == 1) {
							pad->vibrateI[1] = (u8)query.lastByte;
						}
						pad->vibrate[query.lastByte-2] = value;
					}
				}
				break;
			case 0x4F:
				if (query.lastByte == 3 || query.lastByte == 4) {
					pad->umask[query.lastByte-3] = value;
				}
				else if (query.lastByte == 5) {
					if (!(value & 1)) {
						pad->mode = MODE_DIGITAL;
					}
					else if (!(value & 2)) {
						pad->mode = MODE_ANALOG;
					}
					else {
						pad->mode = MODE_FULL_ANALOG;
					}
				}
				break;
			default:
				DEBUG_OUT(0);
				return 0;
		}
		DEBUG_OUT(query.response[query.lastByte]);
		return query.response[query.lastByte];
	}
	DEBUG_OUT(0);
	return 0;
}

// returns: 1 if supports pad1
//			2 if supports pad2
//			3 if both are supported
u32 CALLBACK PADquery() {
	return 3;
}

// extended funcs

//void CALLBACK PADgsDriverInfo(GSdriverInfo *info) {
//}

INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDOK) {
		EndDialog(hwndDlg, 0);
		return 1;
	}
	return 0;
}


void CALLBACK PADabout() {
	DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), 0, AboutDialogProc);
}

s32 CALLBACK PADtest() {
	return 0;
}

DWORD CALLBACK HideWindow(void *) {
	ShowWindow(hWnd, 0);
	return 0;
}

// For escape fillscreen hack.
ExtraWndProcResult KillFullScreenProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	// Double check to prevent infinite recursion.
	if (IsWindowMaximized(hWnd)) {
		ShowWindow(hWnd, 0);
	}
	return CONTINUE_BLISSFULLY_AND_RELEASE_PROC;
}

keyEvent* CALLBACK PADkeyEvent() {
	if (!config.GSThreadUpdates) {
		Update(2);
	}
	static keyEvent ev;
	if (!GetQueuedKeyEvent(&ev)) return 0;
	if (ev.key == VK_ESCAPE && ev.event == KEYPRESS && config.escapeFullscreenHack) {
		if (IsWindowMaximized(hWnd)) {
			EatWndProc(hWnd, KillFullScreenProc);
			return 0;
		}
	}

	if (ev.key == VK_LSHIFT || ev.key == VK_RSHIFT) {
		ev.key = VK_SHIFT;
	}
	else if (ev.key == VK_LCONTROL || ev.key == VK_RCONTROL) {
		ev.key = VK_CONTROL;
	}
	else if (ev.key == VK_LMENU || ev.key == VK_RMENU) {
		ev.key = VK_MENU;
	}
	return &ev;
}

typedef struct {
	unsigned char controllerType;
	unsigned short buttonStatus;
	unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY;
	unsigned char moveX, moveY;
	unsigned char reserved[91];
} PadDataS;

u32 CALLBACK PADreadPort1 (PadDataS* pads) {
	PADstartPoll(1);
	PADpoll(0x42);
	memcpy(pads, query.response+1, 7);
	pads->controllerType = pads[0].controllerType>>4;
	memset (pads+7, 0, sizeof(PadDataS)-7);
	return 0;
}

u32 CALLBACK PADreadPort2 (PadDataS* pads) {
	PADstartPoll(2);
	PADpoll(0x42);
	memcpy(pads, query.response+1, 7);
	pads->controllerType = pads->controllerType>>4;
	memset (pads+7, 0, sizeof(PadDataS)-7);
	return 0;
}

u32 CALLBACK PSEgetLibType() {
	return 8;
}

u32 CALLBACK PSEgetLibVersion() {
	return (VERSION & 0xFFFFFF);
}

char* CALLBACK PSEgetLibName() {
	return PS2EgetLibName();
}

// Littly funkiness to handle rounding floating points to ints.
// Unfortunately, means I can't use /GL optimization option.
#ifdef NO_CRT
extern "C" long _cdecl _ftol();
extern "C" long _cdecl _ftol2_sse() {
	return _ftol();
}
extern "C" long _cdecl _ftol2() {
	return _ftol();
}
#endif
