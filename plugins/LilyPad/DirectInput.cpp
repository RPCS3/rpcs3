#include "global.h"
#include "VKey.h"
#include "DirectInput.h"
#include <dinput.h>
#include "InputManager.h"
#include "DeviceEnumerator.h"


inline static u16 flipShort(u16 s) {
	return (s>>8) | (s<<8);
}

inline static u32 flipLong(u32 l) {
	return (((u32)flipShort((u16)l))<<16) | flipShort((u16)(l>>16));
}

static void GUIDtoString(wchar_t *data, const GUID *pg) {
	wsprintfW(data, L"%08X-%04X-%04X-%04X-%04X%08X",
		pg->Data1, (u32)pg->Data2, (u32)pg->Data3,
		flipShort(((u16*)pg->Data4)[0]), 
		flipShort(((u16*)pg->Data4)[1]),
		flipLong(((u32*)pg->Data4)[1]));
}

static int StringToGUID(GUID *pg, wchar_t *dataw) {
	char data[100];
	if (wcslen(dataw) > 50) return 0;
	int w = 0;
	while (dataw[w]) {
		data[w] = (char) dataw[w];
		w++;
	}
	data[w] = 0;
	u32 temp[5];
	sscanf(data, "%08X-%04X-%04X-%04X-%04X%08X",
		&pg->Data1, temp, temp+1,
		temp+2, temp+3, temp+4);
	pg->Data2 = (u16) temp[0];
	pg->Data3 = (u16) temp[1];
	((u16*)pg->Data4)[0] = flipShort((u16)temp[2]);
	((u16*)pg->Data4)[1] = flipShort((u16)temp[3]);
	((u32*)pg->Data4)[1] = flipLong(temp[4]);
	return 1;
}


struct DI8Effect {
	IDirectInputEffect *die;
	int scale;
};

BOOL CALLBACK EnumDeviceObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOW pdei, LPVOID pvRef);
class DirectInputDevice : public Device {
public:
	DI8Effect *diEffects;

	IDirectInputDevice8 *did;
	DirectInputDevice(DeviceType type, IDirectInputDevice8* did, wchar_t *displayName, wchar_t *instanceID, wchar_t *productID) : Device(DI, type, displayName, instanceID, productID) {
		int i;
		diEffects = 0;
		this->did = did;
		did->EnumEffects(EnumEffectsCallback, this, DIEFT_ALL);
		did->EnumObjects(EnumDeviceObjectsCallback, this, DIDFT_ALL);
		DIOBJECTDATAFORMAT *formats = (DIOBJECTDATAFORMAT*)malloc(sizeof(DIOBJECTDATAFORMAT) * numPhysicalControls);
		for (i=0; i<numPhysicalControls; i++) {
			formats[i].pguid = 0;
			formats[i].dwType = physicalControls[i].type | DIDFT_MAKEINSTANCE(physicalControls[i].id);
			formats[i].dwOfs = 4*i;
			formats[i].dwFlags = 0;
		}
		DIDATAFORMAT format;
		format.dwSize = sizeof(format);
		format.dwDataSize = 4 * numPhysicalControls;
		format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
		format.dwFlags = 0;
		format.dwNumObjs = numPhysicalControls;
		format.rgodf = formats;
		int res = did->SetDataFormat(&format);
		for (i=0; i<numPhysicalControls; i++) {
			if (physicalControls[i].type == ABSAXIS) {
				DIPROPRANGE prop;
				prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
				prop.diph.dwSize = sizeof(DIPROPRANGE);
				prop.diph.dwObj = formats[i].dwType;
				prop.diph.dwHow = DIPH_BYID;
				prop.lMin = -FULLY_DOWN;
				prop.lMax = FULLY_DOWN;
				did->SetProperty(DIPROP_RANGE, &prop.diph);

				// May do something like this again, if there's any need.
				/*
				if (FAILED(DI->did->SetProperty(DIPROP_RANGE, &prop.diph))) {
					if (FAILED(DI->did->GetProperty(DIPROP_RANGE, &prop.diph))) {
						// ????
						DI->objects[i].min = prop.lMin;
						DI->objects[i].max = prop.lMax;
						continue;
					}
				}
				DI->objects[i].min = prop.lMin;
				DI->objects[i].max = prop.lMax;
				//*/
			}
		}
		free(formats);
	}

	void SetEffect(ForceFeedbackBinding *binding, unsigned char force) {
		unsigned int index = binding - pads[0].ffBindings;
		if (index >= (unsigned int)pads[0].numFFBindings) {
			index = pads[0].numFFBindings + (binding - pads[1].ffBindings);
		}
		IDirectInputEffect *die = diEffects[index].die;
		if (die) {
			DIEFFECT dieffect;
			memset(&dieffect, 0, sizeof(dieffect));
			union {
				DIPERIODIC periodic;
				DIRAMPFORCE ramp;
				DICONSTANTFORCE constant;
			};

			//cf.lMagnitude = 0;
			//memset(&dieffect, 0, sizeof(dieffect));
			dieffect.dwSize = sizeof(dieffect);
			//dieffect.lpEnvelope = 0;
			dieffect.dwStartDelay = 0;
			dieffect.lpvTypeSpecificParams = &periodic;
			int magnitude = abs((int)((force*10000*(__int64)diEffects[index].scale)/BASE_SENSITIVITY/255));
			if (magnitude > 10000) magnitude = 10000;
			int type = ffEffectTypes[binding->effectIndex].type;
			if (type == EFFECT_CONSTANT) {
				dieffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
				constant.lMagnitude = magnitude;
			}
			else if (type == EFFECT_PERIODIC) {
				dieffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
				periodic.dwMagnitude = 0;
				periodic.lOffset = magnitude;
				periodic.dwPhase = 0;
				periodic.dwPeriod = 2000000;
			}
			else if (type == EFFECT_RAMP) {
				dieffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
				ramp.lEnd = ramp.lStart = magnitude;
			}
			dieffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
			dieffect.dwDuration = 2000000;
			die->SetParameters(&dieffect, DIEP_TYPESPECIFICPARAMS | DIEP_START);
		}
	}

	int Activate(void *d) {
		int i, j;
		if (active) Deactivate();
		InitInfo *info = (InitInfo*)d;
		// Note:  Have to use hWndTop to properly hide cursor for mouse device.
		if (type == OTHER) {
			did->SetCooperativeLevel(info->hWndTop, DISCL_BACKGROUND | DISCL_EXCLUSIVE);
		}
		else if (type == KEYBOARD) {
			did->SetCooperativeLevel(info->hWndTop, DISCL_FOREGROUND);
		}
		else {
			did->SetCooperativeLevel(info->hWndTop, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
		}
		if (did->Acquire() != DI_OK) {
			return 0;
		}
		AllocState();
		diEffects = (DI8Effect*) calloc(pads[0].numFFBindings + pads[1].numFFBindings, sizeof(DI8Effect));
		for (i=0; i<pads[0].numFFBindings + pads[1].numFFBindings; i++) {
			ForceFeedbackBinding *b = 0;
			if (i >= pads[0].numFFBindings) {
				b = &pads[1].ffBindings[i-pads[0].numFFBindings];
			}
			else
				b = &pads[0].ffBindings[i];
			ForceFeedbackEffectType *eff = ffEffectTypes + b->effectIndex;
			GUID guid;
			if (!StringToGUID(&guid, eff->effectID)) continue;

			DIEFFECT dieffect;
			memset(&dieffect, 0, sizeof(dieffect));
			dieffect.dwSize = sizeof(dieffect);
			dieffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
			dieffect.dwDuration = 2000000;
			dieffect.dwGain = 10000;
			dieffect.dwTriggerButton = DIEB_NOTRIGGER;
			union {
				DIPERIODIC pediodic;
				DIRAMPFORCE ramp;
				DICONSTANTFORCE constant;
			} stuff = {0,0,0,0};

			if (eff->type == EFFECT_CONSTANT) {
				dieffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
			}
			else if (eff->type == EFFECT_PERIODIC) {
				dieffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
			}
			else if (eff->type == EFFECT_RAMP) {
				dieffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
			}
			dieffect.lpvTypeSpecificParams = &stuff;

			int maxForce = 0;
			int numAxes = 0;
			int *axes = (int*) malloc(sizeof(int) * 3 * numFFAxes);
			DWORD *axisIDs = (DWORD*)(axes + numFFAxes);
			LONG *dirList = (LONG*)(axisIDs + numFFAxes);
			dieffect.rgdwAxes = axisIDs;
			dieffect.rglDirection = dirList;
			for (j=0; j<numFFAxes; j++) {
				if (b->axes[j].force) {
					int force = abs(b->axes[j].force);
					if (force > maxForce) {
						maxForce = force;
					}
					axes[numAxes] = j;
					axisIDs[numAxes] = ffAxes[j].id;
					dirList[numAxes] = b->axes[j].force;
					numAxes++;
				}
			}
			if (!numAxes) {
				free(axes);
				continue;
			}
			dieffect.cAxes = numAxes;
			diEffects[i].scale = maxForce;
			if (!SUCCEEDED(did->CreateEffect(guid, &dieffect, &diEffects[i].die, 0))) {
				diEffects[i].die = 0;
				diEffects[i].scale = 0;
			}

			free(axes);
			axes = 0;
		}
		active = 1;
		return 1;
	}

	int Update() {
		if (!active) return 0;
		if (numPhysicalControls) {
			HRESULT res = did->Poll();
			// ??
			if ((res != DI_OK && res != DI_NOEFFECT) ||
				DI_OK != did->GetDeviceState(4*numPhysicalControls, physicalControlState)) {
					Deactivate();
					return 0;
			}
			for (int i=0; i<numPhysicalControls; i++) {
				if (physicalControls[i].type & RELAXIS) {
					physicalControlState[i] *= (FULLY_DOWN/3);
				}
				else if (physicalControls[i].type & BUTTON) {
					physicalControlState[i] = (physicalControlState[i]&0x80) * FULLY_DOWN / 128;
				}
			}
		}
		return 1;
	}

	void Deactivate() {
		FreeState();
		if (diEffects) {
			for (int i=0; i<pads[0].numFFBindings + pads[1].numFFBindings; i++) {
				if (diEffects[i].die) {
					diEffects[i].die->Stop();
					diEffects[i].die->Release();
					diEffects[i].die = 0;
				}
			}
			free(diEffects);
			diEffects = 0;
		}
		if (active) {
			did->Unacquire();
			active = 0;
		}
	}

	~DirectInputDevice() {
		did->Release();
	}
};

BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOW pdei, LPVOID pvRef) {
	DirectInputDevice * did = (DirectInputDevice*) pvRef;
	EffectType type;
	int diType = DIEFT_GETTYPE(pdei->dwEffType);
	if (diType == DIEFT_CONSTANTFORCE) {
		type = EFFECT_CONSTANT;
	}
	else if (diType == DIEFT_RAMPFORCE) {
		type = EFFECT_RAMP;
	}
	else if (diType == DIEFT_PERIODIC) {
		type = EFFECT_PERIODIC;
	}
	else {
		return DIENUM_CONTINUE;
	}
	wchar_t guidString[50];
	GUIDtoString(guidString, &pdei->guid);
	did->AddFFEffectType(pdei->tszName, guidString, type);

	return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumDeviceObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef) {
	DirectInputDevice * did = (DirectInputDevice*) pvRef;
	if (lpddoi->dwType & DIDFT_FFACTUATOR) {
		did->AddFFAxis(lpddoi->tszName, lpddoi->dwType);
		/*
		void * t = realloc(DI->actuators, sizeof(DIActuator) * (DI->numActuators+1));
		if (t) {
			DI->actuators = (DIActuator*) t;
			int i = DI->numActuators;
			while (i > 0 && (lpddoi->dwType & 0xFFFF00) < (DI->actuators[i-1].id & 0xFFFF00)) {
				DI->actuators[i] = DI->actuators[i-1];
				i--;
			}
			DI->actuators[i].maxForce = lpddoi->dwFFMaxForce;
			DI->actuators[i].id = lpddoi->dwType;
			DI->numActuators++;
		}
		//*/
	}

	ControlType type;

	if (lpddoi->dwType & DIDFT_POV)
		type = POV;
	else if (lpddoi->dwType & DIDFT_ABSAXIS)
		type = ABSAXIS;
	else if (lpddoi->dwType & DIDFT_RELAXIS)
		type = RELAXIS;
	else if (lpddoi->dwType & DIDFT_PSHBUTTON)
		type = PSHBTN;
	else if (lpddoi->dwType & DIDFT_TGLBUTTON)
		type = TGLBTN;
	else
		return DIENUM_CONTINUE;
	// If too many objects, ignore extra buttons.
	if ((did->numPhysicalControls > 255 && DIDFT_GETINSTANCE(lpddoi->dwType) > 255) && (type & (DIDFT_PSHBUTTON | DIDFT_TGLBUTTON))) {
		int i;
		for (i = did->numPhysicalControls-1; i>did->numPhysicalControls-4; i--) {
			if (!lpddoi->tszName[0]) break;
			const wchar_t *s1 = lpddoi->tszName;
			const wchar_t *s2 = did->physicalControls[i].name;
			while (*s1 && *s1 == *s2) {
				s1++;
				s2++;
			}
			// If perfect match with one of last 4 names, break.
			if (!*s1 && !*s2) break;

			while (s1 != lpddoi->tszName && (s1[-1] >= '0' && s1[-1] <= '9')) s1--;
			int check = 0;
			while (*s1 >= '0' && *s1 <= '9') {
				check = check*10 + *s1 - '0';
				s1++;
			}
			while (*s2 >= '0' && *s2 <= '9') {
				s2++;
			}
			// If perfect match other than final number > 30, then break.
			// takes care of "button xx" case without causing issues with F keys.
			if (!*s1 && !*s2 && check > 30) break;
		}
		if (i != did->numPhysicalControls-4) {
			return DIENUM_CONTINUE;
		}
	}

	int vkey = 0;
	if (lpddoi->tszName[0] && did->type == KEYBOARD) {
		for (u32 i = 0; i<256; i++) {
			wchar_t *t = GetVKStringW((u8)i);
			if (!wcsicmp(lpddoi->tszName, t)) {
				vkey = i;
				break;
			}
		}
	}
	did->AddPhysicalControl(type, DIDFT_GETINSTANCE(lpddoi->dwType), vkey, lpddoi->tszName);
	return DIENUM_CONTINUE;
}

struct CreationInfo {
	LPDIRECTINPUT8 lpDI8;
	int count;
};


BOOL CALLBACK EnumCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	CreationInfo *ci = (CreationInfo*) pvRef;
	int *count = (int*) pvRef;
	const wchar_t *name;
	wchar_t temp[40];
	if (lpddi->tszInstanceName[0]) {
		name = lpddi->tszInstanceName;
	}
	else if (lpddi->tszProductName[0]) {
		name = lpddi->tszProductName;
	}
	else {
		wsprintfW (temp, L"Device %i", ci->count);
		name = temp;
	}
	ci->count++;
	wchar_t *fullName = (wchar_t *) malloc((wcslen(name) + 4) * sizeof(wchar_t));
	wsprintf(fullName, L"DX %s", name);
	wchar_t instanceID[100];
	wchar_t productID[100];
	GUIDtoString(instanceID, &lpddi->guidInstance);
	GUIDtoString(productID, &lpddi->guidProduct);
	DeviceType type = OTHER;
	if ((lpddi->dwDevType & 0xFF) == DI8DEVTYPE_KEYBOARD) {
		type = KEYBOARD;
	}
	else if ((lpddi->dwDevType & 0xFF) == DI8DEVTYPE_MOUSE) {
		type = MOUSE;
	}
	IDirectInputDevice8 *did;
	if (DI_OK == ci->lpDI8->CreateDevice(lpddi->guidInstance, &did, 0)) {
		dm->AddDevice(new DirectInputDevice(type, did, fullName, instanceID, productID));
	}
	free(fullName);
	return DIENUM_CONTINUE;
}

void EnumDirectInputDevices() {
	CreationInfo ci = {0,0};
	if (FAILED(DirectInput8Create(hInst, 0x800, IID_IDirectInput8, (void**) &ci.lpDI8, 0))) return;
	ci.lpDI8->EnumDevices(DI8DEVCLASS_ALL, EnumCallback, &ci, DIEDFL_ATTACHEDONLY);
	ci.lpDI8->Release();
}

