#include "Global.h"
#include <xinput.h>
#include "VKey.h"
#include "InputManager.h"

// This way, I don't require that XInput junk be installed.
typedef void (CALLBACK *_XInputEnable)(BOOL enable);
typedef DWORD (CALLBACK *_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD (CALLBACK *_XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);

_XInputEnable pXInputEnable = 0;
_XInputGetState pXInputGetState = 0;
_XInputSetState pXInputSetState = 0;

static int xInputActiveCount = 0;

// Completely unncessary, really.
__forceinline int ShortToAxis(int v) {
	// If positive and at least 1 << 14, increment.
	v += (!((v>>15)&1)) & ((v>>14)&1);
	// Just double.
	return v * 2;
}

class XInputDevice : public Device {
	// Cached last vibration values by pad and motor.
	// Need this, as only one value is changed at a time.
	int ps2Vibration[2][4][2];
	// Minor optimization - cache last set vibration values
	// When there's no change, no need to do anything.
	XINPUT_VIBRATION xInputVibration;
public:
	int index;

	XInputDevice(int index, wchar_t *displayName) : Device(XINPUT, OTHER, displayName) {
		memset(ps2Vibration, 0, sizeof(ps2Vibration));
		memset(&xInputVibration, 0, sizeof(xInputVibration));
		this->index = index;
		int i;
		for (i=0; i<14; i++) {
			// The i > 9 accounts for the 2 bit skip in button flags.
			AddPhysicalControl(PSHBTN, i + 2*(i > 9), 0);
		}
		for (i=14; i<16; i++) {
			// The i > 9 accounts for the 2 bit skip in button flags.
			AddPhysicalControl(PRESSURE_BTN, i + 2*(i > 9), 0);
		}
		for (; i<20; i++) {
			AddPhysicalControl(ABSAXIS, i + 2, 0);
		}
		AddFFAxis(L"Slow Motor", 0);
		AddFFAxis(L"Fast Motor", 1);
		AddFFEffectType(L"Constant Effect", L"Constant", EFFECT_CONSTANT);
	}

	wchar_t *GetPhysicalControlName(PhysicalControl *c) {
		const static wchar_t *names[] = {
			L"D-pad Up",
			L"D-pad Down",
			L"D-pad Left",
			L"D-pad Right",
			L"Start",
			L"Back",
			L"Left Thumb",
			L"Right Thumb",
			L"Left Shoulder",
			L"Right Shoulder",
			L"A",
			L"B",
			L"X",
			L"Y",
			L"Left Trigger",
			L"Right Trigger",
			L"Left Thumb X",
			L"Left Thumb Y",
			L"Right Thumb X",
			L"Right Thumb Y",
		};
		unsigned int i = (unsigned int) (c - physicalControls);
		if (i < 20) {
			return (wchar_t*)names[i];
		}
		return Device::GetPhysicalControlName(c);
	}

	int Activate(InitInfo *initInfo) {
		if (active) Deactivate();
		if (!xInputActiveCount) {
			pXInputEnable(1);
		}
		xInputActiveCount++;
		active = 1;
		AllocState();
		return 1;
	}

	int Update() {
		if (!active) return 0;
		XINPUT_STATE state;
		if (ERROR_SUCCESS != pXInputGetState(index, &state)) {
			Deactivate();
			return 0;
		}
		int buttons = state.Gamepad.wButtons;
		for (int i=0; i<14; i++) {
			physicalControlState[i] = ((buttons >> physicalControls[i].id)&1)<<16;
		}
		physicalControlState[14] = (((int)state.Gamepad.bLeftTrigger) + (state.Gamepad.bLeftTrigger>>7)) << 8;
		physicalControlState[15] = (((int)state.Gamepad.bRightTrigger) + (state.Gamepad.bRightTrigger>>7)) << 8;
		physicalControlState[16] = ShortToAxis(state.Gamepad.sThumbLX);
		physicalControlState[17] = ShortToAxis(state.Gamepad.sThumbLY);
		physicalControlState[18] = ShortToAxis(state.Gamepad.sThumbRX);
		physicalControlState[19] = ShortToAxis(state.Gamepad.sThumbRY);
		return 1;
	}

	void SetEffects(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force) {
		ps2Vibration[port][slot][motor] = force;
		int newVibration[2] = {0,0};
		for (int p=0; p<2; p++) {
			for (int s=0; s<4; s++) {
				for (int i=0; i<pads[p][s].numFFBindings; i++) {
					// Technically should also be a *65535/BASE_SENSITIVITY, but that's close enough to 1 for me.
					ForceFeedbackBinding *ffb = &pads[p][s].ffBindings[i];
					newVibration[0] += (int)((ffb->axes[0].force * (__int64)ps2Vibration[p][s][ffb->motor]) / 255);
					newVibration[1] += (int)((ffb->axes[1].force * (__int64)ps2Vibration[p][s][ffb->motor]) / 255);
				}
			}
		}
		newVibration[0] = abs(newVibration[0]);
		if (newVibration[0] > 65535) {
			newVibration[0] = 65535;
		}
		newVibration[1] = abs(newVibration[1]);
		if (newVibration[1] > 65535) {
			newVibration[1] = 65535;
		}
		if (newVibration[0] || newVibration[1] || newVibration[0] != xInputVibration.wLeftMotorSpeed || newVibration[1] != xInputVibration.wRightMotorSpeed) {
			XINPUT_VIBRATION newv = {newVibration[0], newVibration[1]};
			if (ERROR_SUCCESS == pXInputSetState(index, &newv)) {
				xInputVibration = newv;
			}
		}
	}

	void SetEffect(ForceFeedbackBinding *binding, unsigned char force) {
		PadBindings pBackup = pads[0][0];
		pads[0][0].ffBindings = binding;
		pads[0][0].numFFBindings = 1;
		SetEffects(0, 0, binding->motor, 255);
		pads[0][0] = pBackup;
	}

	void Deactivate() {
		memset(&xInputVibration, 0, sizeof(xInputVibration));
		memset(ps2Vibration, 0, sizeof(ps2Vibration));
		pXInputSetState(index, &xInputVibration);

		FreeState();
		if (active) {
			if (!--xInputActiveCount) {
				pXInputEnable(0);
			}
			active = 0;
		}
	}

	~XInputDevice() {
	}
};

void EnumXInputDevices() {
	int i;
	wchar_t temp[30];
	if (!pXInputSetState) {
		// Also used as flag to indicute XInput not installed, so
		// don't repeatedly try to load it.
		if (pXInputEnable) return;

		HMODULE hMod = 0;
		for (i=3; i>= 0; i--) {
			wsprintfW(temp, L"xinput1_%i.dll", i);
			if (hMod = LoadLibraryW(temp)) break;
		}
		if (hMod) {
			if ((pXInputEnable = (_XInputEnable) GetProcAddress(hMod, "XInputEnable")) &&
				(pXInputGetState = (_XInputGetState) GetProcAddress(hMod, "XInputGetState"))) {
					pXInputSetState = (_XInputSetState) GetProcAddress(hMod, "XInputSetState");
			}
		}
		if (!pXInputSetState) {
			pXInputEnable = (_XInputEnable)-1;
			return;
		}
	}
	pXInputEnable(1);
	for (i=0; i<4; i++) {
		XINPUT_STATE state;
		if (ERROR_SUCCESS == pXInputGetState(i, &state)) {
			wsprintfW(temp, L"XInput Pad %i", i);
			dm->AddDevice(new XInputDevice(i, temp));
		}
	}
	pXInputEnable(0);
}

